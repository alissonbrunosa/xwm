#ifndef WM_LAYOUT_H
#define WM_LAYOUT_H

#include "layout/split.h"

typedef struct wm_layout wm_layout_t;
// TODO: Consider using a callback implemented in Dekstop.
//typedef int (*render_function_callback_t)(wm_layout_t* layout, wm_client_t* client);

struct wm_layout {
    enum {
        WM_LAYOUT_TILING,
        WM_LAYOUT_MONOCLE,
        WM_LAYOUT_MASTER
    } type;

    union {
        wm_split_layout_t* split_layout;
    };
};

// void wm_layout_render(wm_layout_t* layout);
void wm_layout_apply(wm_layout_t* layout);
void wm_layout_render(xcb_connection_t* conn, wm_layout_t* layout);
void wm_layout_add_client(wm_layout_t* layout, wm_client_t* client);
wm_client_t* wm_layout_find_client_by_window(wm_layout_t* layout, xcb_window_t window);

#endif



