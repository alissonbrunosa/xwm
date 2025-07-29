#include <X11/keysym.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>

#include "wm_desktop.h"

wm_desktop_t* desktop;
xcb_connection_t* conn;
xcb_ewmh_connection_t* ewmh;
xcb_screen_t* screen;
xcb_key_symbols_t* keysyms;

void handle_key_press(xcb_key_press_event_t* event) {
    xcb_keysym_t keysym = xcb_key_symbols_get_keysym(keysyms, event->detail, 0);
    uint16_t state = event->state;

    if (state & XCB_MOD_MASK_4) {
        fprintf(stderr, "Super was held down\n");
        if (state & XCB_MOD_MASK_SHIFT) {
            switch (keysym) {
                case XK_c: {
                    desktop_close_focused_client(desktop);
                    return;
                }
                case XK_r: {
                    execvp("/home/carcara/Code/C/xwm-endless/xwm", NULL);
                    return;
                }

                case XK_h: {
                    desktop_swap_client_left(desktop);
                    return;
                }

                case XK_l: {
                    desktop_swap_client_left(desktop);
                    return;
                }
            }
        }

        switch (keysym) {
            case XK_l: {
                desktop_move_client_right(desktop);
                break;
            }

            case XK_h: {
                desktop_move_client_left(desktop);
                break;
            }

            case XK_d: {
                if (fork() == 0) {
                    execlp("appsmenu", "appsmenu", NULL);
                    _exit(1);
                }
                break;
            }

            case XK_q: {
                desktop_cleanup(desktop);
                exit(0);
                break;
            }

            default: {
                fprintf(stderr, "Unhandled key press: %u\n", keysym);
                break;
            }
        }
    }
}

void handle_client_message(wm_desktop_t* desktop, xcb_client_message_event_t* event) {
    if (event->type == WM_PROTOCOLS_ATOM) {
        xcb_atom_t protocol = event->data.data32[0];
        if (protocol == WM_DELETE_WINDOW_ATOM) {
            fprintf(stderr, "Destroying window: %u\n", event->window);
            desktop_destroy_window(desktop, event->window);
            return;
        }

        if (protocol == WM_TAKE_FOCUS_ATOM) {
            fprintf(stderr, "Taking focus for window: %u\n", event->window);
            xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, event->window, XCB_CURRENT_TIME);
            xcb_flush(conn);
            return;
        }

        fprintf(stderr, "Unhandled client message protocol: %u\n", protocol);
        return;
    }

    if (event->type == ewmh->_NET_WM_STATE) {
        wm_client_t* client = desktop_find_client_by_window(desktop, event->window);
        if (client == NULL) {
            fprintf(stderr, "Client not found for window: %u\n", event->window);
            return;
        }

        uint32_t action = event->data.data32[0];
        xcb_atom_t state = event->data.data32[1];
        if (state == ewmh->_NET_WM_STATE_FULLSCREEN) {
            switch (action) {
                case XCB_EWMH_WM_STATE_ADD:
                    client->flags |= WM_CLIENT_FULLSCREEN;
                    fprintf(stderr, "Enabling fullscreen for window: %u\n", event->window);
                    break;
                case XCB_EWMH_WM_STATE_REMOVE:
                    fprintf(stderr, "Disabling fullscreen for window: %u\n", event->window);
                    client->flags &= ~WM_CLIENT_FULLSCREEN;
                    break;
                case XCB_EWMH_WM_STATE_TOGGLE:
                    client->flags ^= WM_CLIENT_FULLSCREEN;
                    fprintf(stderr, "Toggling fullscreen for window: %u\n", event->window);
                    break;
            }

            desktop_arrange(desktop);
        }
        return;
    }
}

void property_notify(xcb_property_notify_event_t* event) {
    fprintf(stderr, "Property notify for window: %u\n", event->window);
    wm_client_t* client = desktop_find_client_by_window(desktop, event->window);
    if (client == NULL) {
        fprintf(stderr, "Property notify for unmanaged window: %u\n", event->window);
        return;
    }


   // fprintf(stderr, "Property notify for managed window: %u, atoms: %u\n", event->window, event->atom);
   // xcb_configure_window(conn, client->window, XCB_CONFIG_WINDOW, client->values);
   // fprintf(stderr, "Reconfigured window: %u with values: x=%d, y=%d, width=%d, height=%d, border_width=%d\n",
   //        client->window, client->values[0], client->values[1], client->values[2], client->values[3], client->values[4]);
    xcb_flush(conn);
}

int main(void) {
    conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn)) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    desktop = allocate_desktop(conn);
    if (desktop == NULL) {
        fprintf(stderr, "Failed to allocate desktop\n");
        xcb_disconnect(conn);
        return 1;
    }

    ewmh = calloc(1, sizeof(xcb_ewmh_connection_t));
    xcb_intern_atom_cookie_t* cookies = xcb_ewmh_init_atoms(conn, ewmh);
    xcb_ewmh_init_atoms_replies(ewmh, cookies, NULL);

    desktop_setup_atoms(desktop);

    // Get the first screen
    const xcb_setup_t* setup = xcb_get_setup(conn);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    screen = iter.data;
    desktop->root = screen->root;
    desktop->width = screen->width_in_pixels;
    desktop->height = screen->height_in_pixels;

    desktop->layout->split_layout->max_width = desktop->width;
    desktop->layout->split_layout->max_height = desktop->height;
    uint32_t white = screen->white_pixel;
    xcb_change_window_attributes(conn, desktop->root, XCB_CW_BACK_PIXEL, &white);
    xcb_clear_area(conn, 0, desktop->root, 0, 0, screen->width_in_pixels, screen->height_in_pixels);

    keysyms = xcb_key_symbols_alloc(conn);
    if (!keysyms) {
        fprintf(stderr, "Failed to allocate key symbols\n");
        xcb_disconnect(conn);
        return 1;
    }

    xcb_keycode_t* keycodes_h = xcb_key_symbols_get_keycode(keysyms, XK_h);
    xcb_keycode_t* keycodes_l = xcb_key_symbols_get_keycode(keysyms, XK_l);
    xcb_keycode_t* keycodes_q = xcb_key_symbols_get_keycode(keysyms, XK_q);
    xcb_keycode_t* keycodes_d = xcb_key_symbols_get_keycode(keysyms, XK_d);
    xcb_keycode_t* keycodes_c = xcb_key_symbols_get_keycode(keysyms, XK_c);
    xcb_keycode_t* keycodes_r = xcb_key_symbols_get_keycode(keysyms, XK_r);

    if (keycodes_h && keycodes_l && keycodes_q && keycodes_d) {
        for (xcb_keycode_t* kc = keycodes_h; *kc; ++kc) {
            xcb_grab_key(conn, 1, screen->root, XCB_MOD_MASK_4, *kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        }

        for (xcb_keycode_t* kc = keycodes_l; *kc; ++kc) {
            xcb_grab_key(conn, 1, screen->root, XCB_MOD_MASK_4, *kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        }

        for (xcb_keycode_t* kc = keycodes_q; *kc; ++kc) {
            xcb_grab_key(conn, 1, screen->root, XCB_MOD_MASK_4, *kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        }

        for (xcb_keycode_t* kc = keycodes_d; *kc; ++kc) {
            xcb_grab_key(conn, 1, screen->root, XCB_MOD_MASK_4, *kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        }

        xcb_mod_mask_t mod_mask = XCB_MOD_MASK_4 | XCB_MOD_MASK_SHIFT;
        for (xcb_keycode_t* kc = keycodes_h; *kc; ++kc) {
            xcb_grab_key(conn, 1, screen->root, mod_mask, *kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        }

        for (xcb_keycode_t* kc = keycodes_l; *kc; ++kc) {
            xcb_grab_key(conn, 1, screen->root, mod_mask, *kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        }

        for (xcb_keycode_t* kc = keycodes_c; *kc; ++kc) {
            xcb_grab_key(conn, 1, screen->root, mod_mask, *kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        }

        for (xcb_keycode_t* kc = keycodes_r; *kc; ++kc) {
            xcb_grab_key(conn, 1, screen->root, mod_mask, *kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        }
    }

    free(keycodes_h);
    free(keycodes_l);
    free(keycodes_q);
    free(keycodes_d);
    free(keycodes_r);
    free(keycodes_c);

    uint32_t values[] = {ROOT_EVENT_MASK};
    xcb_change_window_attributes(conn, screen->root, XCB_CW_EVENT_MASK, values);
    xcb_flush(conn);

    xcb_query_tree_reply_t* tree = xcb_query_tree_reply(conn, xcb_query_tree(conn, screen->root), NULL);
    if (tree) {
        xcb_window_t* children = xcb_query_tree_children(tree);
        int len = xcb_query_tree_children_length(tree);
        for (int i = 0; i < len; i++) {
            desktop_manage_window(desktop, children[i], 1);
        }
        free(tree);
    }

    while (1) {
        xcb_generic_event_t* event = xcb_wait_for_event(conn);
        if (event == NULL) {
            break;
        }

        switch (event->response_type & ~0x80) {
            case XCB_MAP_REQUEST: {
                xcb_map_request_event_t* map_event = (xcb_map_request_event_t*) event;
                fprintf(stderr, "Map request for window: %u\n", map_event->window);
                desktop_manage_window(desktop, map_event->window, 0);

                break;
            }

            case XCB_UNMAP_NOTIFY: {
                xcb_unmap_notify_event_t* unmap_event = (xcb_unmap_notify_event_t*) event;
                fprintf(stderr, "Unmap notify for window: %u\n", unmap_event->window);
                xcb_unmap_window(conn, unmap_event->window);
                break;
            }

            case XCB_KEY_PRESS: {
                xcb_key_press_event_t* key_event = (xcb_key_press_event_t*) event;
                handle_key_press(key_event);
                break;
            }

            case XCB_DESTROY_NOTIFY: {
                xcb_destroy_notify_event_t* destroy_event = (xcb_destroy_notify_event_t*) event;
                fprintf(stderr, "Destroy notify for window: %u\n", destroy_event->window);
                desktop_destroy_window(desktop, destroy_event->window);

                break;
            }

            case XCB_CLIENT_MESSAGE: {
                xcb_client_message_event_t* client_event = (xcb_client_message_event_t*) event;
                fprintf(stderr, "Client message for window: %u\n", client_event->window);
                handle_client_message(desktop, client_event);
                break;
            }

            case XCB_PROPERTY_NOTIFY: {
                xcb_property_notify_event_t* e = (xcb_property_notify_event_t*) event;
                property_notify(e);
                break;
            }

            case XCB_FOCUS_IN: {
                xcb_focus_in_event_t* ev = (xcb_focus_in_event_t*) event;
                fprintf(stderr, "Focus in event received for window: %u\n", ev->event);
                break;
            }

            case XCB_FOCUS_OUT: {
                xcb_focus_out_event_t* ev = (xcb_focus_out_event_t*) event;
                fprintf(stderr, "Focus out event received for window: %u\n", ev->event);
                break;
            }

            // TODO: Implement this handling
            case XCB_ENTER_NOTIFY: {
                xcb_enter_notify_event_t* ev = (xcb_enter_notify_event_t*) event;
                break;
            }
        }

        free(event);
    }

    desktop_cleanup(desktop);
    free(desktop);

    if (keysyms != NULL) {
        xcb_key_symbols_free(keysyms);
    }

    return 0;
}
