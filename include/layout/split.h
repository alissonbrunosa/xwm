#ifndef NODE_H
#define NODE_H

#include <xcb/xcb.h>

#include "client.h"

typedef struct geometry geometry_t;
typedef struct wm_node wm_node_t;
typedef enum wm_split wm_split_t;
typedef struct wm_split_layout wm_split_layout_t;


enum wm_split {
    HORIZONTAL_SPLIT,
    VERTICAL_SPLIT
};

struct wm_split_layout {
    wm_node_t* root;

    uint32_t max_width;
    uint32_t max_height;
};


struct geometry {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

struct wm_node {
    wm_node_t* parent;
    wm_split_t split;

    enum {
        WINDOW_NODE,
        SPLIT_NODE
    } type;

    union {
        wm_client_t* client;
        struct {
            wm_node_t* left;
            wm_node_t* right;
        } children;
    };

    geometry_t geometry;
};

wm_split_layout_t* wm_layout_create(void);

void wm_node_insert(wm_node_t* root, wm_node_t* new_node);
void wm_layout_calculate_geometry(wm_node_t* node, int x, int y, int width, int height);
void wm_split_layout_add_client(wm_split_layout_t* layout, wm_client_t* client);
void wm_split_layout_apply(wm_split_layout_t* layout);
//void wm_split_layout_render(wm_split_layout_t* layout);
void wm_split_layout_render(xcb_connection_t* coon, wm_split_layout_t* layout);
wm_client_t* wm_split_layout_find_client(wm_split_layout_t* layout, xcb_window_t window);

wm_node_t* wm_node_create_parent(wm_split_t);
wm_node_t* wm_node_create_leaf(wm_client_t*);
wm_node_t* wm_node_split_horizontal(wm_node_t*, wm_node_t*);
wm_node_t* wm_node_split_vertical(wm_node_t*, wm_node_t*);

#endif // WM_NODE_H

