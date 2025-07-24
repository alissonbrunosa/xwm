#include <stdio.h>
#include <assert.h>
#include "wm_desktop.h"


wm_desktop_t* allocate_desktop(xcb_connection_t* conn) {
    wm_desktop_t* desktop = (wm_desktop_t*) calloc(1, sizeof(wm_desktop_t));
    if (desktop == NULL) {
        return NULL;
    }

    desktop->conn = conn;
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

void update_focus_state(wm_desktop_t* desktop, int8_t flush) {
    if (desktop == NULL) {
        return;
    }

    wm_client_t* old_focused_client = desktop_fetch_client_at(desktop, desktop->last_focus);
    if (old_focused_client != NULL) {
        fprintf(stderr, "Removing decoration from old focused client: %u\n", old_focused_client->window);
        uint32_t border_pixel = get_color_pixel(desktop, "#DA2C43");
        xcb_change_window_attributes(desktop->conn, old_focused_client->window, XCB_CW_BORDER_PIXEL, &border_pixel);
    }

    wm_client_t* focused_client = desktop_fetch_focused_client(desktop);
    if (focused_client != NULL) {
        fprintf(stderr, "Decorating focused client: %u\n", focused_client->window);
        uint32_t border_pixel = get_color_pixel(desktop, "#06FA36");
        xcb_change_window_attributes(desktop->conn, focused_client->window, XCB_CW_BORDER_PIXEL, &border_pixel);

        if (focused_client->flags & WM_CLIENT_TAKE_FOCUS) {
            fprintf(stderr, "Sending take focus message to client: %u\n", focused_client->window);
            desktop_send_take_focus(desktop, focused_client->window);
        } else {
            fprintf(stderr, "Setting input focus to client: %u\n", focused_client->window);
            uint32_t values = CLIENT_EVENT_MASK & ~CLIENT_FOCUS_EVENT_MASK;
            xcb_change_window_attributes(desktop->conn, focused_client->window, XCB_CW_EVENT_MASK, &values);
            xcb_set_input_focus(desktop->conn, XCB_INPUT_FOCUS_POINTER_ROOT, focused_client->window, XCB_CURRENT_TIME);
            values = CLIENT_EVENT_MASK;
            xcb_change_window_attributes(desktop->conn, focused_client->window, XCB_CW_EVENT_MASK, &values);
        }

        /* TODO: Repalce XCB_NONE for _NET_ACTIVE_WINDOW
         * Also, perhaps I should load the atoms myself instead of using ewmh?
         * Reason is that I don't want to keep to "conn" around xcb_connection_t* and ewmh_connection_t*;
         * Just making the code compile :) 
         */
        xcb_change_property(
            desktop->conn,
            XCB_PROP_MODE_REPLACE,
            desktop->root,
            NET_ACTIVE_WINDOW_ATOM,
            XCB_ATOM_WINDOW,
            32,
            1,
            &focused_client->window
        );
    }

    if (flush) {
        xcb_flush(desktop->conn);
    }
}

void desktop_move_client_right(wm_desktop_t* desktop) {
    desktop->last_focus = desktop->focus;

    if (desktop->focus.x + 1 >= desktop->size) {
        fprintf(stderr, "No container to move right\n");
        return;
    }

    desktop->focus.x++;
    desktop_arrange(desktop);
    center_cursor(desktop, desktop_fetch_focused_client(desktop));
    xcb_flush(desktop->conn);
}

void desktop_move_client_left(wm_desktop_t* desktop) {
    desktop->last_focus = desktop->focus;

    if (desktop->focus.x > 0) {
        desktop->focus.x--;
    }

    desktop_arrange(desktop);
    center_cursor(desktop, desktop_fetch_focused_client(desktop));
    xcb_flush(desktop->conn);
}

void desktop_swap_client_left(wm_desktop_t* desktop) {
    if (desktop->focus.x <= 0) {
        fprintf(stderr, "No container to swap left\n");
        return;
    }

    wm_container_t temp = desktop->containers[desktop->focus.x - 1];
    desktop->containers[desktop->focus.x - 1] = desktop->containers[desktop->focus.x];
    desktop->containers[desktop->focus.x] = temp;

    desktop->focus.x--;
    desktop_arrange(desktop);
    center_cursor(desktop, desktop_fetch_focused_client(desktop));
    xcb_flush(desktop->conn);
}

void desktop_swap_client_right(wm_desktop_t* desktop) {
    if (desktop->focus.x >= desktop->size - 1) {
        fprintf(stderr, "No container to swap right\n");
        return;
    }

    wm_container_t temp = desktop->containers[desktop->focus.x + 1];
    desktop->containers[desktop->focus.x + 1] = desktop->containers[desktop->focus.x];
    desktop->containers[desktop->focus.x] = temp;

    desktop->focus.x++;
    desktop_arrange(desktop);
    center_cursor(desktop, desktop_fetch_focused_client(desktop));
    xcb_flush(desktop->conn);
}

int wm_desktop_next_container_offset(wm_desktop_t* desktop) {
    if (desktop->containers == NULL) {
        desktop->size = 0;
        desktop->capacity = 1;
        desktop->containers = (wm_container_t*) calloc(desktop->capacity, sizeof(wm_container_t));
        if (desktop->containers == NULL) {
            fprintf(stderr, "failed to allocate memory for wm_desktop containers\n");
            return -1;
        }
    } else if (desktop->size == desktop->capacity) {
        wm_container_t* new_containers;
        int allocated_memory = desktop->capacity * sizeof(wm_container_t);
        int required_memory = allocated_memory << 1;

        desktop->capacity <<= 1;
        new_containers = (wm_container_t*) realloc(desktop->containers, required_memory);
        if (new_containers == NULL) {
            fprintf(stderr, "failed to reallocate memory for wm_desktop containers\n");
            return -1;
        }

        desktop->containers = new_containers;
        memset((char*) desktop->containers + allocated_memory, 0, required_memory - allocated_memory);
    }

    fprintf(stderr, "Allocating new container at index %d\n", desktop->size);
    return desktop->size++;
}

wm_desktop_t* allocate_wm_desktop(xcb_connection_t* conn) {
    wm_desktop_t* desktop = (wm_desktop_t*) calloc(1, sizeof(wm_desktop_t));
    if (desktop == NULL) {
        return NULL;
    }

    desktop->conn = conn;
    return desktop;
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

void desktop_cleanup(wm_desktop_t* desktop) {
    for (int i = 0; i < desktop->size; i++) {
        wm_container_t container = desktop->containers[i];
        container_free(&container);
    }

    if (desktop->containers != NULL) {
        free(desktop->containers);
    }

    if (desktop->conn != NULL) {
        xcb_flush(desktop->conn);
        xcb_disconnect(desktop->conn);
    }

    free(desktop);
}

wm_client_t* desktop_fetch_client_at(wm_desktop_t* desktop, wm_coordinates_t coordinates) {
    if (desktop == NULL || desktop->containers == NULL) {
        fprintf(stderr, "No wm_desktop or containers to fetch client\n");
        return NULL;
    }

    if (coordinates.x < 0 || coordinates.x >= desktop->size) {
        fprintf(stderr, "Invalid x-coordinate: %d\n", coordinates.x);
        return NULL;
    }

    wm_container_t container = desktop->containers[coordinates.x];
    if (container.size == 0 || coordinates.y < 0 || coordinates.y >= container.size) {
        fprintf(stderr, "No clients in the specified container or invalid y-coordinate: %d\n", coordinates.y);
        return NULL;
    }

    return container_get_client(&container, coordinates.y);
}

wm_client_t* desktop_fetch_focused_client(wm_desktop_t* desktop) {
    if (desktop == NULL || desktop->containers == NULL) {
        fprintf(stderr, "No wm_desktop or containers to fetch focused client\n");
        return NULL;
    }

    wm_coordinates_t focus = desktop->focus;
    if (focus.x < 0 || focus.x >= desktop->size) {
        fprintf(stderr, "Invalid focus coordinates: (%d, %d)\n", focus.x, focus.y);
        return NULL;
    }

    wm_container_t container = desktop->containers[focus.x];
    if (container.size == 0 || focus.y < 0 || focus.y >= container.size) {
        fprintf(stderr, "No clients in the focused container or invalid focus y-coordinate\n");
        return NULL;
    }

    return container_get_client(&container, focus.y);
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
    wm_client_t* client = desktop_fetch_focused_client(desktop);

    if (client == NULL) {
        return;
    }

    if (client->flags & WM_CLIENT_WINDOW_DELETE) {
        fprintf(stderr, "Closing focused client: %u\n", client->window);

        xcb_client_message_event_t event = {0};
        event.type = WM_PROTOCOLS_ATOM;
        event.format = 32;
        event.window = client->window;
        event.response_type = XCB_CLIENT_MESSAGE;
        event.data.data32[0] = WM_DELETE_WINDOW_ATOM;
        event.data.data32[1] = XCB_CURRENT_TIME;
        xcb_send_event(desktop->conn, 0, client->window, XCB_EVENT_MASK_NO_EVENT, (const char*) &event);
    } else {
        fprintf(stderr, "Killing focused client: %u\n", client->window);
        xcb_kill_client(desktop->conn, client->window);
    }

    xcb_flush(desktop->conn);
}

typedef struct floating_client {
    wm_client_t* client;
    struct floating_client* previous;
} wm_floating_client_t;

void desktop_arrange(wm_desktop_t* desktop) {
    if (desktop == NULL || desktop->containers == NULL) {
        fprintf(stderr, "No desktop or containers to desktop_arrange\n");
        xcb_flush(desktop->conn);
        return;
    }

    int gap = 10;
    int border_width = 3;

    int base_width = desktop->width;
    if (desktop->size > 1) {
        base_width /= 2;
    }

    wm_floating_client_t* floating_clients = NULL;

    int start_point = desktop->size > 2 ? desktop->focus.x : 0;

    int x = gap;
    for (int i = start_point; i < desktop->size; i++) {
        int y = gap;
        int horizontal_inner_gap = desktop->size > 1 ? gap / 2 : gap;

        wm_container_t* container = &desktop->containers[i];
        if (container == NULL || container->size == 0) {
            continue;
        }

        int base_height = desktop->height / container->size;

        int seen_dialog = 0;
        for (int j = 0; j < container->size; j++) {
            int vertical_inner_gap = container->size > 1 ? gap / 2 : gap;

            wm_client_t* client = container_get_client(container, j);
            if (client == NULL) {
                continue;
            }

            if (client->flags & WM_CLIENT_FLOATING) {
                seen_dialog = 1;
                if (floating_clients == NULL) {
                    floating_clients = calloc(1, sizeof(wm_floating_client_t));
                    floating_clients->client = client;
                } else {
                    wm_floating_client_t* fc = calloc(1, sizeof(wm_floating_client_t));
                    fc->client = client;
                    fc->previous = floating_clients;
                    floating_clients = fc;
                }

                continue;
            }

            uint32_t values[5];
            if (client->flags & WM_CLIENT_FULLSCREEN) {
                values[0] = 0;
                values[1] = 0;
                values[2] = desktop->width;
                values[3] = desktop->height;
                values[4] = 0;
            } else  {
                values[0] = x;
                values[1] = y;
                values[2] = base_width - gap - horizontal_inner_gap - (2 * border_width);
                values[3] = base_height - gap - vertical_inner_gap - (2 * border_width);
                values[4] = border_width;
            }

            if (!client_update_values(client, values)) {
                fprintf(stderr, "No configuration change for client %u\n", client->window);
                continue;
            }
            desktop_configure_window(desktop, client);
            y += base_height;
        }
        if (seen_dialog) {
            continue;
        }
        x += base_width - horizontal_inner_gap;
    }

    x = -base_width;
    fprintf(stderr, "arranging containers before focused container\n");
    for (int i = 0; i < start_point; i++) {
        int y = gap;
        int horizontal_inner_gap = desktop->size > 1 ? gap / 2 : gap;

        wm_container_t* container = &desktop->containers[i];
        int base_height = desktop->height / container->size;

        int seen_dialog = 0;
        for (int j = 0; j < container->size; j++) {
            int vertical_inner_gap = container->size > 1 ? gap / 2 : gap;

            wm_client_t* client = container_get_client(container, j);
            if (client == NULL) {
                continue;
            }

            if (client->flags & WM_CLIENT_FLOATING) {
                seen_dialog = 1;
                if (floating_clients == NULL) {
                    floating_clients = calloc(1, sizeof(wm_floating_client_t));
                    floating_clients->client = client;
                } else {
                    wm_floating_client_t* fc = calloc(1, sizeof(wm_floating_client_t));
                    fc->client = client;
                    fc->previous = floating_clients;
                    floating_clients = fc;
                }

                continue;
            }

            uint32_t values[5];
            if (client->flags & WM_CLIENT_FULLSCREEN) {
                values[0] = 0;
                values[1] = 0;
                values[2] = desktop->width;
                values[3] = desktop->height;
                values[4] = 0;
            } else  {
                values[0] = x;
                values[1] = y;
                values[2] = base_width - gap - horizontal_inner_gap - (2 * border_width);
                values[3] = base_height - gap - vertical_inner_gap - (2 * border_width);
                values[4] = border_width;
            }

            if (!client_update_values(client, values)) {
                fprintf(stderr, "No configuration change for client %u\n", client->window);
                continue;
            }

            xcb_configure_window(desktop->conn, client->window, XCB_CONFIG_WINDOW, values);
            y += base_height;
        }

        if (seen_dialog) {
            continue;
        }

        x -= (base_width + horizontal_inner_gap);
    }

    while (floating_clients != NULL) {
        wm_client_t* client = floating_clients->client;

        uint32_t x = client->x == 0 ? desktop->width / 2 - client->width : client->x;
        uint32_t y = client->y == 0 ? desktop->height / 2 - client->height : client->y;

        uint32_t values[5];
        values[0] = x;
        values[1] = y;
        values[2] = client->width;
        values[3] = client->height;
        values[4] = border_width;

        xcb_configure_window(desktop->conn, client->window, XCB_CONFIG_WINDOW, values);
        floating_clients = floating_clients->previous;
        free(floating_clients);
    }

    update_focus_state(desktop, 0);
}

wm_coordinates_t desktop_fetch_coordinates_for_client(wm_desktop_t* desktop, wm_client_t* client) {
    wm_coordinates_t coordinates = {-1, -1};

    if (desktop == NULL || client == NULL) {
        fprintf(stderr, "No wm_desktop or client to fetch coordinates\n");
        return coordinates;
    }

    for (int i = 0; i < desktop->size; i++) {
        wm_container_t container = desktop->containers[i];
        for (int j = 0; j < container.size; j++) {
            wm_client_t* c = container_get_client(&container, j);
            if (c == client) {
                coordinates.x = i;
                coordinates.y = j;
                return coordinates;
            }
        }
    }

    return coordinates;
}

void desktop_destroy_window(wm_desktop_t* desktop, xcb_window_t window) {
    wm_client_t* client = desktop_find_client_by_window(desktop, window);
    if (client == NULL) {
        fprintf(stderr, "Could not find a client for the window %u\n", window);
        fprintf(stderr, "Destroying unmanaged window: %u\n", window);
        xcb_destroy_window(desktop->conn, window);
        return;
    }

    wm_coordinates_t coordinates = desktop_fetch_coordinates_for_client(desktop, client);
    if (coordinates.x < 0 || coordinates.y < 0) {
        fprintf(stderr, "Could not find coordinates for the client of window %u\n", window);
        fprintf(stderr, "Destroying unmanaged window: %u\n", window);
        xcb_destroy_window(desktop->conn, window);
        return;
    }

    wm_container_t* container = &desktop->containers[coordinates.x];
    container_remove_client(container, coordinates.y);
    if (container->size == 0) {
        for (int i = coordinates.x; i < desktop->size - 1; i++) {
            desktop->containers[i] = desktop->containers[i + 1];
        }

        desktop->size--;
    }

    if (desktop->size == 0) {
        desktop->focus.x = -1;
        desktop->focus.y = -1;
    } else if (desktop->focus.x == coordinates.x && desktop->focus.y == coordinates.y) {
        if (container->size == 0) {
            desktop->focus.x = desktop->size - 1;
            desktop->focus.y = 0;
        } else {
            desktop->focus.y = container->size - 1;
        }
    }

    fprintf(stderr, "Destroying window: %u, coordinates (%d, %d)\n", window, coordinates.x, coordinates.y);
    fprintf(stderr, "new focus coordinates: (%d, %d)\n", desktop->focus.x, desktop->focus.y);
    xcb_destroy_window(desktop->conn, window);
    desktop_arrange(desktop);
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
    } else {
        fprintf(stderr, "Failed to get WM hints for window %u\n", client->window);
    }

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
        fprintf(stderr, "Failed to get normal hints for window %u\n", client->window);
    }
}

void desktop_update_wm_window_type(wm_desktop_t* desktop, wm_client_t* client) {
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

void desktop_update_focus_state(wm_desktop_t* desktop, int8_t flush) {
    if (desktop == NULL) {
        return;
    }

    xcb_ewmh_connection_t* ewmh;

    wm_client_t* old_focused_client = desktop_fetch_client_at(desktop, desktop->last_focus);
    if (old_focused_client != NULL) {
        fprintf(stderr, "Removing decoration from old focused client: %u\n", old_focused_client->window);
        uint32_t border_pixel = get_color_pixel(desktop, "#DA2C43");
        xcb_change_window_attributes(desktop->conn, old_focused_client->window, XCB_CW_BORDER_PIXEL, &border_pixel);
    }

    wm_client_t* focused_client = desktop_fetch_focused_client(desktop);
    if (focused_client != NULL) {
        fprintf(stderr, "Decorating focused client: %u\n", focused_client->window);
        uint32_t border_pixel = get_color_pixel(desktop, "#06FA36");
        xcb_change_window_attributes(desktop->conn, focused_client->window, XCB_CW_BORDER_PIXEL, &border_pixel);

        if (focused_client->flags & WM_CLIENT_TAKE_FOCUS) {
            fprintf(stderr, "Sending take focus message to client: %u\n", focused_client->window);
            xcb_client_message_event_t event = {0};
            event.response_type = XCB_CLIENT_MESSAGE;
            event.format = 32;
            event.sequence = 0;
            event.window = focused_client->window;
            event.type = WM_PROTOCOLS_ATOM;
            event.data.data32[0] = WM_TAKE_FOCUS_ATOM;
            event.data.data32[1] = XCB_CURRENT_TIME;

            xcb_send_event(desktop->conn, 0, focused_client->window, XCB_EVENT_MASK_NO_EVENT, (const char*) &event);
            // xcb_add_property_atom(desktop->conn, focused_client->window, ewmh->_NET_WM_STATE,
            // desktop->atoms.NET_STATE_FOCUSED);
        } else {
            fprintf(stderr, "Setting input focus to client: %u\n", focused_client->window);
            uint32_t values = CLIENT_EVENT_MASK & ~CLIENT_FOCUS_EVENT_MASK;
            xcb_change_window_attributes(desktop->conn, focused_client->window, XCB_CW_EVENT_MASK, &values);
            xcb_set_input_focus(desktop->conn, XCB_INPUT_FOCUS_POINTER_ROOT, focused_client->window,
                                XCB_CURRENT_TIME);
            values = CLIENT_EVENT_MASK;
            xcb_change_window_attributes(desktop->conn, focused_client->window, XCB_CW_EVENT_MASK, &values);
        }

        xcb_change_property(desktop->conn, XCB_PROP_MODE_REPLACE, desktop->root, NET_ACTIVE_WINDOW_ATOM,
                            XCB_ATOM_WINDOW, 32, 1, &focused_client->window);
    }

    if (flush) {
        xcb_flush(desktop->conn);
    }
}

void desktop_manage_window(wm_desktop_t* desktop, xcb_window_t window, int8_t adopted) {
    wm_client_t* client = NULL;

    if (!should_manage_window(desktop, window, adopted)) {
        fprintf(stderr, "Skipping management for window %u\n", window);
        return;
    }

    client = desktop_find_client_by_window(desktop, window);
    if (client != NULL) {
        fprintf(stderr, "Window %u is already managed\n", window);
        return;
    }

    int container_offset = wm_desktop_next_container_offset(desktop);
    if (container_offset < 0) {
        return;
    }

    wm_container_t* container = &desktop->containers[container_offset];
    int client_offset = container_next_client_offset(container);
    if (client_offset < 0) {
        fprintf(stderr, "Failed to allocate space for new client\n");
        return;
    }

    client = container_get_client(container, client_offset);
    client->window = window;
    desktop_update_wm_protocols(desktop, client);
    desktop_update_wm_normal_hints(desktop, client);
    desktop_update_wm_window_type(desktop, client);

    uint32_t values = CLIENT_EVENT_MASK;
    xcb_change_window_attributes(desktop->conn, client->window, XCB_CW_EVENT_MASK, &values);

    // TODO: Re-implement this floating focus thing
    desktop->focus.x = container_offset;
    desktop->focus.y = client_offset;
    fprintf(stderr, "Managing new window: %u, focus at (%d, %d)\n", window, container_offset, client_offset);
    desktop_arrange(desktop);

    if (!client->flags & WM_CLIENT_MAPPED) {
        client->flags |= WM_CLIENT_MAPPED;
        xcb_map_window(desktop->conn, client->window);
    }

    xcb_flush(desktop->conn);
}

wm_client_t* desktop_find_client_by_window(wm_desktop_t* desktop, xcb_window_t window) {
    if (desktop == NULL || desktop->containers == NULL) {
        fprintf(stderr, "desktop_find_client_by_window | No desktop or containers to search for client\n");
        return NULL;
    }

    for (int i = 0; i < desktop->size; i++) {
        wm_container_t container = desktop->containers[i];
        for (int j = 0; j < container.size; j++) {
            wm_client_t* client = container_get_client(&container, j);
            if (client != NULL && client->window == window) {
                return client;
            }
        }
    }

    return NULL;
}

