#ifndef WM_DESKTOP_H
#define WM_DESKTOP_H

#include <X11/Xlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>

#include "wm_client.h"
#include "wm_layout.h"

#define XCB_CONFIG_WINDOW_POSITION   (XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y)
#define XCB_CONFIG_WINDOW_DIMENSIONS (XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT)
#define XCB_CONFIG_WINDOW            (XCB_CONFIG_WINDOW_POSITION | XCB_CONFIG_WINDOW_DIMENSIONS | XCB_CONFIG_WINDOW_BORDER_WIDTH)

#define ROOT_EVENT_MASK (XCB_EVENT_MASK_FOCUS_CHANGE          | \
                         XCB_EVENT_MASK_BUTTON_PRESS          | \
                         XCB_EVENT_MASK_STRUCTURE_NOTIFY      | \
                         XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY   | \
                         XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT)

#define CLIENT_FOCUS_EVENT_MASK (XCB_EVENT_MASK_FOCUS_CHANGE | \
                                 XCB_EVENT_MASK_ENTER_WINDOW)

#define CLIENT_EVENT_MASK (XCB_EVENT_MASK_FOCUS_CHANGE     | \
                           XCB_EVENT_MASK_ENTER_WINDOW     | \
                           XCB_EVENT_MASK_PROPERTY_CHANGE  | \
                           XCB_EVENT_MASK_STRUCTURE_NOTIFY)

#define WM_PROTOCOLS_ATOM (desktop->atoms[WM_PROTOCOLS])
#define WM_DELETE_WINDOW_ATOM (desktop->atoms[WM_DELETE_WINDOW])
#define WM_TAKE_FOCUS_ATOM (desktop->atoms[WM_TAKE_FOCUS])

#define NET_WM_STATE_ATOM (desktop->atoms[NET_WM_STATE])
#define NET_ACTIVE_WINDOW_ATOM (desktop->atoms[NET_ACTIVE_WINDOW])
#define NET_WM_WINDOW_TYPE_NORMAL_ATOM (desktop->atoms[NET_WM_WINDOW_TYPE_NORMAL])
#define NET_WM_WINDOW_TYPE_DIALOG_ATOM (desktop->atoms[NET_WM_WINDOW_TYPE_DIALOG])
#define NET_WM_WINDOW_TYPE_UTILITY_ATOM (desktop->atoms[NET_WM_WINDOW_TYPE_UTILITY])
#define NET_WM_WINDOW_TYPE_TOOLBAR_ATOM (desktop->atoms[NET_WM_WINDOW_TYPE_TOOLBAR])
#define NET_WM_WINDOW_TYPE_SPLASH_ATOM (desktop->atoms[NET_WM_WINDOW_TYPE_SPLASH])
#define NET_WM_WINDOW_TYPE_NOTIFICATION_ATOM (desktop->atoms[NET_WM_WINDOW_TYPE_NOTIFICATION])


typedef enum wm_atom wm_atom_t;
typedef struct wm_desktop wm_desktop_t;

wm_client_t* desktop_find_client_by_window(wm_desktop_t* desktop, xcb_window_t window);

wm_desktop_t* allocate_desktop(xcb_connection_t* connection);
void desktop_cleanup(wm_desktop_t* desktop);
void desktop_setup_atoms(wm_desktop_t* desktop);
void desktop_send_take_focus(wm_desktop_t* desktop, xcb_window_t window);

void center_cursor(wm_desktop_t* desktop, wm_client_t* client);
void desktop_update_wm_protocols(wm_desktop_t* desktop, wm_client_t* client);
void desktop_update_wm_normal_hints(wm_desktop_t* desktop, wm_client_t* client);
void desktop_update_wm_window_type(wm_desktop_t* desktop, wm_client_t* client);

void desktop_move_client_right(wm_desktop_t* desktop);
void desktop_move_client_left(wm_desktop_t* desktop);
void desktop_swap_client_left(wm_desktop_t* desktop);
void desktop_swap_client_right(wm_desktop_t* desktop);
void desktop_close_focused_client(wm_desktop_t* desktop);
void desktop_arrange(wm_desktop_t* desktop);
void desktop_destroy_window(wm_desktop_t* desktop, xcb_window_t window);
void desktop_configure_window(wm_desktop_t* desktop, wm_client_t* client);
int should_manage_window(wm_desktop_t* desktop, xcb_window_t window, int8_t adopted);
void desktop_manage_window(wm_desktop_t* desktop, xcb_window_t window, int8_t adopted);

enum wm_atom {
    WM_PROTOCOLS,
    WM_DELETE_WINDOW,
    WM_TAKE_FOCUS,

    NET_WM_STATE,
    NET_ACTIVE_WINDOW,
    NET_WM_WINDOW_TYPE_NORMAL,
    NET_WM_WINDOW_TYPE_DIALOG,
    NET_WM_WINDOW_TYPE_UTILITY,
    NET_WM_WINDOW_TYPE_TOOLBAR,
    NET_WM_WINDOW_TYPE_SPLASH,
    NET_WM_WINDOW_TYPE_NOTIFICATION,

    ATOM_COUNT // This should always be the last item
};

// Desktop struct definition
struct wm_desktop {
    xcb_window_t root;
    xcb_connection_t* conn;

    // TODO: Implementr an array for wm_client_t -> wm_client_array_t* floating_clients;
    wm_layout_t* layout;

    int32_t width; // screen width
    int32_t height; // screen height
    xcb_atom_t atoms[ATOM_COUNT];
};

#endif // WM_DESKTOP_H
