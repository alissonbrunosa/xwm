#ifndef WM_CLIENT_H
#define WM_CLIENT_H

#include <xcb/xcb.h>

#define WM_CLIENT_MAPPED        (1 << 0)
#define WM_CLIENT_FLOATING      (1 << 1)
#define WM_CLIENT_FULLSCREEN    (1 << 2)
#define WM_CLIENT_INITIALIZED   (1 << 3)
#define WM_CLIENT_INPUT_HINT    (1 << 4)
#define WM_CLIENT_CHANGED       (1 << 5)
#define WM_CLIENT_TAKE_FOCUS    (1 << 6)
#define WM_CLIENT_WINDOW_DELETE (1 << 7)

#define CLIENT_UPDATE(client, field, new_value) \
    do {                                        \
        if ((client)->field != new_value) {     \
            (client)->field = new_value;        \
            (client)->updated = 1;              \
        }                                       \
    } while (0)

typedef struct wm_client wm_client_t;

struct wm_client {
    wm_client_t* parent;       /*Parent client for floating windows*/

    xcb_window_t window;

    char name[256];
    char class[256];
    uint32_t values[5];

    int32_t x;
    int32_t y;

    int32_t width;
    int32_t height;

    int32_t min_width;
    int32_t max_width;

    int32_t min_height;
    int32_t max_height;

    int32_t base_width;
    int32_t base_height;

    int8_t updated;
    int8_t flags; // Bitmask for various states
};

#endif // WM_CLIENT_H
