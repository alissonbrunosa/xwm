#include <stdio.h>
#include <assert.h>

#include "xalloc.h"
#include "desktop.h"

wm_desktop_t* allocate_desktop(xcb_connection_t* conn) {
    wm_desktop_t* desktop = (wm_desktop_t*) xcalloc(1, sizeof(wm_desktop_t));
    if (desktop == NULL) {
        fprintf(stderr, "Failed to allocate memory for split layout\n");
        return NULL;
    }

    wm_layout_t* layout = xcalloc(1, sizeof(wm_layout_t));
    if (layout == NULL) {
        fprintf(stderr, "Failed to allocate memory for split layout\n");
        free(desktop);
        return NULL;
    }

    wm_split_layout_t* split = xcalloc(1, sizeof(wm_split_layout_t));
    if (split == NULL) {
        fprintf(stderr, "Failed to allocate memory for split layout\n");
        free(desktop);
        free(layout);
        return NULL;
    }

    split->max_width = desktop->width;
    split->max_height = desktop->height;
    layout->type = WM_LAYOUT_TILING;
    layout->split_layout = split;
    fprintf(stderr, "######################### Layout = %p\n", (void*)layout);
    desktop->conn = conn;
    desktop->layout = layout;
    fprintf(stderr, "[ALLOC] layout=%p, desktop->layout=%p\n", layout, desktop->layout);
    return desktop;
}


// TODO: Remove this function later
uint32_t get_color_pixel(wm_desktop_t* desktop, const char* color) {
    unsigned int red, green, blue;
    if (sscanf(color + 1, "%02x%02x%02x", &red, &green, &blue) == 3) {
        return (0xFF << 24) | (red << 16 | green << 8 | blue);
    } else {
        return 0;
    }
}

void center_cursor(wm_desktop_t* desktop, wm_client_t* client) {
    xcb_get_geometry_cookie_t cookie = xcb_get_geometry(desktop->conn, client->window);
    xcb_get_geometry_reply_t* geometry = xcb_get_geometry_reply(desktop->conn, cookie, NULL);

    if (geometry == NULL) {
        return;
    }

    uint16_t win_w = geometry->width;
    uint16_t win_h = geometry->height;

    xcb_translate_coordinates_cookie_t coord_cookie = xcb_translate_coordinates(desktop->conn, client->window, geometry->root, 0, 0);
    xcb_translate_coordinates_reply_t* reply = xcb_translate_coordinates_reply(desktop->conn, coord_cookie, NULL);

    if (reply == NULL) {
        free(geometry);
        return;
    }

    int x = reply->dst_x + win_w / 2;
    int y = reply->dst_y + win_h / 2;

    xcb_warp_pointer(desktop->conn, XCB_NONE, geometry->root, 0, 0, 0, 0, x, y);
    xcb_flush(desktop->conn);

    free(reply);
    free(geometry);
}

//TODO: Implement this function
void desktop_move_client_right(wm_desktop_t* desktop) {
    xcb_flush(desktop->conn);
}

// TOOD: Implement this function
void desktop_move_client_left(wm_desktop_t* desktop) {
    xcb_flush(desktop->conn);
}

//TODO: Implement this function
void desktop_swap_client_left(wm_desktop_t* desktop) {
    xcb_flush(desktop->conn);
}

// TODO: Implement this function
void desktop_swap_client_right(wm_desktop_t* desktop) {
    xcb_flush(desktop->conn);
}

void desktop_intern_atom(wm_desktop_t* desktop, wm_atom_t index, const char* name) {
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(desktop->conn, 0, strlen(name), name);
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(desktop->conn, cookie, NULL);
    if (reply) {
        desktop->atoms[index] = reply->atom;
        free(reply);
    } else {
        fprintf(stderr, "Failed to intern atom: %s\n", name);
        desktop->atoms[index] = XCB_ATOM_NONE; // Set to invalid atom on failure
    }
}

void desktop_setup_atoms(wm_desktop_t* desktop) {
    desktop_intern_atom(desktop, WM_PROTOCOLS, "WM_PROTOCOLS");
    desktop_intern_atom(desktop, WM_DELETE_WINDOW, "WM_DELETE_WINDOW");
    desktop_intern_atom(desktop, WM_TAKE_FOCUS, "WM_TAKE_FOCUS");
    desktop_intern_atom(desktop, NET_WM_STATE, "_NET_WM_STATE");
    desktop_intern_atom(desktop, NET_ACTIVE_WINDOW, "_NET_ACTIVE_WINDOW");
    desktop_intern_atom(desktop, NET_WM_WINDOW_TYPE_NORMAL, "_NET_WM_WINDOW_TYPE_NORMAL");
    desktop_intern_atom(desktop, NET_WM_WINDOW_TYPE_DIALOG, "_NET_WM_WINDOW_TYPE_DIALOG");
    desktop_intern_atom(desktop, NET_WM_WINDOW_TYPE_UTILITY, "_NET_WM_WINDOW_TYPE_UTILITY");
    desktop_intern_atom(desktop, NET_WM_WINDOW_TYPE_TOOLBAR, "_NET_WM_WINDOW_TYPE_TOOLBAR");
    desktop_intern_atom(desktop, NET_WM_WINDOW_TYPE_SPLASH, "_NET_WM_WINDOW_TYPE_SPLASH");
    desktop_intern_atom(desktop, NET_WM_WINDOW_TYPE_NOTIFICATION, "_NET_WM_WINDOW_TYPE_NOTIFICATION");
}

// TODO: Implement this function
void desktop_cleanup(wm_desktop_t* desktop) {
    fprintf(stderr, "Cleaning up desktop resources\n");
    assert(desktop != NULL);
    if (desktop->conn != NULL) {
        xcb_flush(desktop->conn);
        xcb_disconnect(desktop->conn);
    }
}

void desktop_send_take_focus(wm_desktop_t* desktop, xcb_window_t window) {
    assert(desktop != NULL);

    xcb_client_message_event_t event = {0};
    event.type = WM_PROTOCOLS_ATOM;
    event.format = 32;
    event.window = window;
    event.response_type = XCB_CLIENT_MESSAGE;
    event.data.data32[0] = WM_TAKE_FOCUS_ATOM;
    event.data.data32[1] = XCB_CURRENT_TIME;

    xcb_send_event(desktop->conn, 0, window, XCB_EVENT_MASK_NO_EVENT, (const char*) &event);
}

void desktop_close_focused_client(wm_desktop_t* desktop) {
    assert(desktop != NULL);

   // if (client->flags & WM_CLIENT_WINDOW_DELETE) {
   //     fprintf(stderr, "Closing focused client: %u\n", client->window);

   //     xcb_client_message_event_t event = {0};
   //     event.type = WM_PROTOCOLS_ATOM;
   //     event.format = 32;
   //     event.window = client->window;
   //     event.response_type = XCB_CLIENT_MESSAGE;
   //     event.data.data32[0] = WM_DELETE_WINDOW_ATOM;
   //     event.data.data32[1] = XCB_CURRENT_TIME;
   //     xcb_send_event(desktop->conn, 0, client->window, XCB_EVENT_MASK_NO_EVENT, (const char*) &event);
   // } else {
   //     fprintf(stderr, "Killing focused client: %u\n", client->window);
   //     xcb_kill_client(desktop->conn, client->window);
   // }

   // free(client);
   // xcb_flush(desktop->conn);
}

void desktop_arrange(wm_desktop_t* desktop) {
    assert(desktop != NULL);

    wm_layout_apply(desktop->layout);
}


// TODO: Implement this
void desktop_destroy_window(wm_desktop_t* desktop, xcb_window_t window) {
    assert(desktop != NULL);

    xcb_destroy_window(desktop->conn, window);

    //desktop_arrange(desktop);
    xcb_flush(desktop->conn);
}

void desktop_configure_window(wm_desktop_t* desktop, wm_client_t* client) {
    if (desktop == NULL || client == NULL) {
        fprintf(stderr, "No desktop or client to configure window\n");
        return;
    }

    /* TODO: Does this trigger an event?
    * If so, we need to ignore the events caused by this configuration change
    */
    xcb_configure_window(desktop->conn, client->window, XCB_CONFIG_WINDOW, client->values);
}

int should_manage_window(wm_desktop_t* desktop, xcb_window_t window, int8_t adopted) {
    wm_client_t* client = desktop_find_client_by_window(desktop, window);
    if (client != NULL) {
        fprintf(stderr, "Window %u is already managed by client %p\n", window, (void*)client);
        return 0;
    }

    xcb_get_window_attributes_cookie_t attr_cookie = xcb_get_window_attributes(desktop->conn, window);
    xcb_get_window_attributes_reply_t* reply = xcb_get_window_attributes_reply(desktop->conn, attr_cookie, NULL);

    if (reply == NULL) {
        fprintf(stderr, "Failed to get attributes for window %u\n", window);
        return 0;
    }

    if (reply->override_redirect) {
        fprintf(stderr, "Window %u is override-redirect, skipping management\n", window);
        free(reply);
        return 0;
    }

    if (adopted && reply->map_state != XCB_MAP_STATE_VIEWABLE) {
        fprintf(stderr, "Window %u is not viewable, skipping management: %d\n", window, reply->map_state);
        free(reply);
        return 0;
    }

    return 1;
}

void desktop_update_wm_protocols(wm_desktop_t* desktop, wm_client_t* client) {
    xcb_icccm_get_wm_protocols_reply_t protocols;
    xcb_get_property_cookie_t cookie = xcb_icccm_get_wm_protocols(desktop->conn, client->window, WM_PROTOCOLS_ATOM);
    if (xcb_icccm_get_wm_protocols_reply(desktop->conn, cookie, &protocols, NULL)) {
        for (uint i = 0; i < protocols.atoms_len; ++i) {
            if (protocols.atoms[i] == WM_DELETE_WINDOW_ATOM) {
                client->flags |= WM_CLIENT_WINDOW_DELETE;
            } else if (protocols.atoms[i] == WM_TAKE_FOCUS_ATOM) {
                client->flags |= WM_CLIENT_TAKE_FOCUS;
            }
        }

        xcb_icccm_get_wm_protocols_reply_wipe(&protocols);
    }

    xcb_icccm_wm_hints_t hints;
    xcb_get_property_cookie_t hints_cookie = xcb_icccm_get_wm_hints(desktop->conn, client->window);
    if (xcb_icccm_get_wm_hints_reply(desktop->conn, hints_cookie, &hints, NULL)) {
        if (hints.flags & XCB_ICCCM_WM_HINT_INPUT && hints.input) {
            client->flags |= WM_CLIENT_INPUT_HINT;
        }
    }

    fprintf(stderr, "window %u protocols: %s%s%s\n", 
            client->window,
            (client->flags & WM_CLIENT_WINDOW_DELETE) ? "WM_DELETE_WINDOW " : "",
            (client->flags & WM_CLIENT_TAKE_FOCUS) ? "WM_TAKE_FOCUS " : "",
            (client->flags & WM_CLIENT_INPUT_HINT) ? "INPUT_HINT " : ""
    );
}

void desktop_update_wm_normal_hints(wm_desktop_t* desktop, wm_client_t* client) {
    xcb_get_property_cookie_t cookie = xcb_icccm_get_wm_normal_hints(desktop->conn, client->window);

    xcb_size_hints_t size_hints;
    int ok = xcb_icccm_get_wm_normal_hints_reply(desktop->conn, cookie, &size_hints, NULL);
    if (ok) {
        if (size_hints.flags & (XCB_ICCCM_SIZE_HINT_US_POSITION | XCB_ICCCM_SIZE_HINT_P_POSITION)) {
            CLIENT_UPDATE(client, x, size_hints.x);
            CLIENT_UPDATE(client, y, size_hints.y);
        }

        if (size_hints.flags & (XCB_ICCCM_SIZE_HINT_US_SIZE | XCB_ICCCM_SIZE_HINT_P_SIZE)) {
            CLIENT_UPDATE(client, width, size_hints.width);
            CLIENT_UPDATE(client, height, size_hints.height);
        }

        if (size_hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE) {
            CLIENT_UPDATE(client, min_width, size_hints.min_width);
            CLIENT_UPDATE(client, min_height, size_hints.min_height);
        }

        if (size_hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE) {
            CLIENT_UPDATE(client, max_width, size_hints.max_width);
            CLIENT_UPDATE(client, max_height, size_hints.max_height);
        }

        if (size_hints.flags & XCB_ICCCM_SIZE_HINT_BASE_SIZE) {
            CLIENT_UPDATE(client, base_width, size_hints.base_width);
            CLIENT_UPDATE(client, base_height, size_hints.base_height);
        }
    } else {
        fprintf(stderr, "Window %u height: %d, width: %d\n", 
                client->window, client->height, client->width);
    }
}

void desktop_update_wm_window_type(wm_desktop_t* desktop, wm_client_t* client) {
    return; // TODO: Implement this function 
    if (client == NULL) {
        return;
    }

    xcb_ewmh_connection_t* ewmh;
    xcb_ewmh_get_atoms_reply_t reply;
    xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_window_type(ewmh, client->window);
    if (xcb_ewmh_get_wm_window_type_reply(ewmh, cookie, &reply, NULL)) {
        if (xcb_ewmh_get_wm_window_type_reply(ewmh, cookie, &reply, NULL)) {
            for (unsigned int i = 0; i < reply.atoms_len; ++i) {
                xcb_atom_t atom = reply.atoms[i];

                if (atom == NET_WM_WINDOW_TYPE_NORMAL_ATOM) {
                    client->flags &= ~WM_CLIENT_FLOATING;
                    return;
                }

                int floating = (
                    atom == NET_WM_WINDOW_TYPE_DIALOG_ATOM ||
                    atom == NET_WM_WINDOW_TYPE_UTILITY_ATOM ||
                    atom == NET_WM_WINDOW_TYPE_TOOLBAR_ATOM ||
                    atom == NET_WM_WINDOW_TYPE_NOTIFICATION_ATOM ||
                    atom == NET_WM_WINDOW_TYPE_SPLASH
                );

                if (floating) {
                    client->flags |= WM_CLIENT_FLOATING;
                    return;
                }
            }
            xcb_ewmh_get_atoms_reply_wipe(&reply);
        }
    }
}

// TODO: Implement focus state update logic
void desktop_update_focus_state(wm_desktop_t* desktop) {
    assert(desktop != NULL);
}

void desktop_manage_window(wm_desktop_t* desktop, xcb_window_t window, int8_t adopted) {
    wm_client_t* client;

    if (!should_manage_window(desktop, window, adopted)) {
        fprintf(stderr, "Skipping management for window %u\n", window);
        return;
    }

    // TODO: Implement logic to check if the window is already managed
    // client = desktop_find_client_by_window(desktop, window);
    //if (client != NULL) {
    //    fprintf(stderr, "Window %u is already managed\n", window);
    //    return;
    //}

    client = (wm_client_t*) xcalloc(1, sizeof(wm_client_t));
    if (client == NULL) {
        fprintf(stderr, "Failed to allocate memory for client\n");
        return;
    }

    client->window = window;
    desktop_update_wm_protocols(desktop, client);
    desktop_update_wm_normal_hints(desktop, client);
    desktop_update_wm_window_type(desktop, client);
    // desktop_update_wm_state(desktop, client);
    // desktop_update_transient_for(desktop, client);

    if (client->flags & WM_CLIENT_FLOATING) {
        // TODO: Push to floating clients list
    } else {
        fprintf(stderr, "[USE] desktop=%p, desktop->layout=%p\n", desktop, desktop->layout);
        wm_layout_add_client(desktop->layout, client);
    }

    uint32_t values = CLIENT_EVENT_MASK;
    xcb_change_window_attributes(desktop->conn, client->window, XCB_CW_EVENT_MASK, &values);

    desktop_arrange(desktop);
    wm_layout_render(desktop->conn, desktop->layout);

    xcb_flush(desktop->conn);
}

// TODO: Implement this function
wm_client_t* desktop_find_client_by_window(wm_desktop_t* desktop, xcb_window_t window) {
    assert(desktop != NULL);

    return wm_layout_find_client_by_window(desktop->layout, window);
}
