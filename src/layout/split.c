#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "xcb/xcb.h"

#include "xalloc.h"
#include "wm_logger.h"
#include "layout/split.h"

#define XCB_CONFIG_WINDOW_POSITION   (XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y)
#define XCB_CONFIG_WINDOW_DIMENSIONS (XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT)
#define XCB_CONFIG_WINDOW            (XCB_CONFIG_WINDOW_POSITION | XCB_CONFIG_WINDOW_DIMENSIONS | XCB_CONFIG_WINDOW_BORDER_WIDTH)

wm_node_t* wm_node_find_by_window(wm_node_t* node, xcb_window_t window);


wm_split_layout_t* wm_layout_create(void) {
    wm_split_layout_t* layout = xcalloc(1, sizeof(wm_split_layout_t));
    if (!layout) {
        return NULL;
    }

    layout->root = NULL;
    return layout;
}

void render_node(xcb_connection_t* conn, wm_node_t* node) {
    if (node == NULL) {
        return;
    }

    switch (node->type) {
        case WINDOW_NODE: {
            uint32_t values[5];
            values[0] = node->geometry.x;
            values[1] = node->geometry.y;
            values[2] = node->geometry.width - 6;
            values[3] = node->geometry.height - 6;
            values[4] = 3;

            wm_client_t* client = node->client;
            xcb_void_cookie_t cookie = xcb_configure_window(conn, client->window, XCB_CONFIG_WINDOW, values);
            DEBUG("Window configured! Cookie sequence: %d\n", cookie.sequence);

            if (client->flags & WM_CLIENT_MAPPED) {
                return;
            }

            client->flags |= WM_CLIENT_MAPPED;
            xcb_map_window(conn, client->window);
            break;
        }

        case SPLIT_NODE: {
            render_node(conn, node->children.left);
            render_node(conn, node->children.right);
            break;
        }
    }
}

// TODO: Implement this function
void wm_split_layout_render(xcb_connection_t* conn, wm_split_layout_t* layout) { render_node(conn, layout->root); }

void wm_split_layout_apply(wm_split_layout_t* layout) {
    assert(layout != NULL);

    wm_layout_calculate_geometry(layout->root, 0, 0, layout->max_width, layout->max_height);
}

void print_tree(wm_node_t* node, int depth) {
    if (node == NULL) {
        return;
    }

    for (int i = 0; i < depth; i++) {
        fprintf(stderr, "  ");
    }

    if (node->type == WINDOW_NODE) {
        xcb_window_t window = 0;
        if (node->client) window = node->client->window;

        fprintf(stderr, "Window Node: %p, Window: %u, Geometry: (%d, %d, %d, %d)\n",
               (void*)node, window,
               node->geometry.x, node->geometry.y,
               node->geometry.width, node->geometry.height);
        return;
    } else {
        fprintf(stderr, "Split Node: %p, Split: %s\n", (void*)node,
               node->split == HORIZONTAL_SPLIT ? "HORIZONTAL" : "VERTICAL");
    }

    print_tree(node->children.left, depth + 1);
    print_tree(node->children.right, depth + 1);
}

wm_client_t* wm_split_layout_find_client(wm_split_layout_t* layout, xcb_window_t window) {
    assert(layout != NULL);

    if (layout->root == NULL) {
        fprintf(stderr, "Layout root is NULL, cannot find client\n");
        return NULL;
    }

    wm_node_t* node = wm_node_find_by_window(layout->root, window);
    if (node == NULL || node->type != WINDOW_NODE) {
        fprintf(stderr, "Client with window %u not found in layout\n", window);
        return NULL;
    }

    return node->client;
}

void wm_split_layout_add_client(wm_split_layout_t* layout, wm_client_t* client) {
    assert(layout != NULL && client != NULL);

    if (layout->root == NULL) {
        fprintf(stderr, "######### Creating root node for layout\n");
        fprintf(stderr, "######### Client %u is the first client in the layout\n", client->window);
        layout->root = wm_node_create_leaf(client);
        fprintf(stderr, "######### Root node created with client %u\n", client->window);
        fprintf(stderr, "######### Layout root: %p\n", (void*)layout->root);
        fprintf(stderr, "######### Layout root->left: %p - %d\n", (void*)layout->root->children.left, layout->root->children.left == NULL);
        fprintf(stderr, "######### Layout root->right: %p - %d\n", (void*)layout->root->children.right, layout->root->children.right == NULL);
        return;
    }
    fprintf(stderr, "######### Layout root type = %d\n", layout->root->type);
    switch (layout->root->type) {
        case WINDOW_NODE:
            fprintf(stderr, "######### Splitting root node to add new client\n");
            fprintf(stderr, "######### Client %u is the second client in the layout\n", client->window);
            wm_node_t* right = wm_node_create_leaf(client);
            if (right == NULL) {
                return;
            }

            wm_node_t* new_root = wm_node_split_vertical(layout->root, right);
            if (new_root == NULL) {
                free(right);
                return;
            }

            layout->root = new_root;
            break;

        case SPLIT_NODE:
            fprintf(stderr, "######### Adding deeper node\n");
            fprintf(stderr, "######### Client %u is being added to the existing layout\n", client->window);
            wm_node_t* new_node = wm_node_create_leaf(client);
            if (new_node == NULL) {
                return;
            }

            wm_node_insert(layout->root, new_node);
            break;
    }

    print_tree(layout->root, 0);
}


void wm_layout_calculate_geometry(wm_node_t* node, int x, int y, int width, int height) {
    if (node == NULL) {
        return;
    }

    node->geometry.x = x;
    node->geometry.y = y;
    node->geometry.width = width;
    node->geometry.height = height;

    if (node->type != SPLIT_NODE) {
        return;
    }

    switch (node->split) {
        case HORIZONTAL_SPLIT: {
            int half = height / 2;
            wm_layout_calculate_geometry(node->children.left, x, y, width, half);
            wm_layout_calculate_geometry(node->children.right, x, y + half, width, height - half);
            break;
        }

        case VERTICAL_SPLIT: {
            int half = width / 2;
            wm_layout_calculate_geometry(node->children.left, x, y, half, height);
            wm_layout_calculate_geometry(node->children.right, x + half, y, width - half, height);
            break;
        }
    }
}

wm_node_t* wm_layout_find_largest_leaf(wm_node_t* node) {
    if (node == NULL) {
        return NULL;
    }

    if (node->type == WINDOW_NODE) {
        return node;
    }

    wm_node_t* left = wm_layout_find_largest_leaf(node->children.left);
    wm_node_t* right = wm_layout_find_largest_leaf(node->children.right);

    if (left != NULL && right != NULL) {
        int left_area = left->geometry.width * left->geometry.height;
        int right_area = right->geometry.width * right->geometry.height;

        if (left_area > right_area) {
            return left;
        }

        return right;
    }

    if (left != NULL) {
        return left;
    }

    return right;
}

void wm_node_replace_child(wm_node_t* parent, wm_node_t* old_child, wm_node_t* new_child) {
    if (parent == NULL || old_child == NULL || new_child == NULL) {
        return;
    }

    if (parent->children.left == old_child) {
        fprintf(stderr, "Replacing left child of parent %p with new child %p\n", (void*)parent, (void*)new_child);
        parent->children.left = new_child;
    } else if (parent->children.right == old_child) {
        fprintf(stderr, "Replacing right child of parent %p with new child %p\n", (void*)parent, (void*)new_child);
        parent->children.right = new_child;
    }else {
        fprintf(stderr, "Old child %p not found in parent %p\n", (void*)old_child, (void*)parent);
        return;
    }

    new_child->parent = parent;
}

int wm_node_area(wm_node_t* node) {
    if (node == NULL) {
        return 0;
    }

    return node->geometry.width * node->geometry.height;
}

void wm_node_insert(wm_node_t* root, wm_node_t* new_node) {
    wm_node_t* largest_leaf = wm_layout_find_largest_leaf(root);

    wm_node_t* parent = largest_leaf->parent;
    wm_node_t* internal_parent = NULL;
    if (parent->split == HORIZONTAL_SPLIT) {
        internal_parent = wm_node_split_vertical(largest_leaf, new_node);
    } else {
        internal_parent = wm_node_split_horizontal(largest_leaf, new_node);
    }

    if (internal_parent == NULL) {
        return;
    }

    wm_node_replace_child(parent, largest_leaf, internal_parent);
}

wm_node_t* wm_node_create_leaf(wm_client_t* client) {
    wm_node_t* node = xcalloc(1, sizeof(wm_node_t));
    if (node == NULL) {
        return NULL;
    }

    node->parent = NULL;
    node->type = WINDOW_NODE;
    node->client = client;
    node->geometry.x = 0;
    node->geometry.y = 0;
    node->geometry.width = 0;
    node->geometry.height = 0;

    fprintf(stderr, "wm_node_create_leaf: Created node %p for client %u\n", (void*)node, client->window);
    fprintf(stderr, "wm_node_create_leaf: Client: %p, Node->left = %p, Node->right = %p\n", (void*)node->client, (void*)node->children.left, (void*)node->children.right);

    return node;
}

wm_node_t* wm_node_create_parent(wm_split_t split) {
    wm_node_t* node = xcalloc(1, sizeof(wm_node_t));
    if (node == NULL) {
        return NULL;
    }

    node->parent = NULL;
    node->type = SPLIT_NODE;
    node->split = split;
    node->children.left = NULL;
    node->children.right = NULL;

    node->geometry.x = 0;
    node->geometry.y = 0;
    node->geometry.width = 0;
    node->geometry.height = 0;

    return node;
}

wm_node_t* wm_node_split_horizontal(wm_node_t* left, wm_node_t* right) {
    wm_node_t* parent = wm_node_create_parent(HORIZONTAL_SPLIT);
    if (parent == NULL) {
        fprintf(stderr, "Failed to create parent node for vertical split\n");
        return NULL;
    }

    left->parent = parent;
    parent->children.left = left;

    right->parent = parent;
    parent->children.right = right;

    return parent;
}

wm_node_t* wm_node_split_vertical(wm_node_t* left, wm_node_t* right) {
    wm_node_t* parent = wm_node_create_parent(VERTICAL_SPLIT);
    if (parent == NULL) {
        fprintf(stderr, "Failed to create parent node for vertical split\n");
        return NULL;
    }

    left->parent = parent;
    parent->children.left = left;

    right->parent = parent;
    parent->children.right = right;

    return parent;
}

wm_node_t* wm_node_find_by_window(wm_node_t* node, xcb_window_t window) {
    if (node == NULL) {
        return NULL;
    }

    if (node->type == WINDOW_NODE && node->client && node->client->window == window) {
        return node;
    }

    if (node->type == SPLIT_NODE) {
        wm_node_t* left = wm_node_find_by_window(node->children.left, window);
        if (left != NULL) {
            return left;
        }

        return wm_node_find_by_window(node->children.right, window);
    }

    return NULL;
}
