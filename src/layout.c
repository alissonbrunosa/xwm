#include <assert.h>
#include <stdio.h>

#include "layout.h"

void wm_layout_apply(wm_layout_t* layout) {
    assert(layout != NULL);

    switch (layout->type) {
        case WM_LAYOUT_TILING:
            wm_split_layout_apply(layout->split_layout);
            break;

        case WM_LAYOUT_MONOCLE:
            // TODO: Implement monocle layout logic
            break;

        case WM_LAYOUT_MASTER:
            // TODO: Implement master layout logic
            break;

        default:
            fprintf(stderr, "Unknown layout type: %d\n", layout->type);
            break;
    }
}

wm_client_t* wm_layout_find_client_by_window(wm_layout_t* layout, xcb_window_t window) {
    assert(layout != NULL);

    if (layout == NULL) {
        fprintf(stderr, "NULL pointer passed to wm_layout_find_client_by_window\n");
        return NULL;
    }

    switch (layout->type) {
        case WM_LAYOUT_TILING:
            return wm_split_layout_find_client(layout->split_layout, window);

        default:
            fprintf(stderr, "Unknown layout type: %d\n", layout->type);
            return NULL;
    }
}

void wm_layout_render(xcb_connection_t* conn, wm_layout_t* layout) {
    assert(layout != NULL);

    if (layout == NULL || conn == NULL) {
        fprintf(stderr, "NULL pointer passed to wm_layout_add_client\n");
        return;
    }

    switch (layout->type) {
        case WM_LAYOUT_TILING:
            wm_split_layout_render(conn, layout->split_layout);
            break;
    }
}

void wm_layout_add_client(wm_layout_t* layout, wm_client_t* client) {
    assert(layout != NULL && client != NULL);

    if (layout == NULL || client == NULL) {
        fprintf(stderr, "NULL pointer passed to wm_layout_add_client\n");
        return;
    }

    switch (layout->type) {
        case WM_LAYOUT_TILING: {
            wm_split_layout_add_client(layout->split_layout, client);
            break;
        }
    }
}

