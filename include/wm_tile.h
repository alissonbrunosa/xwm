#ifndef TILE_H
#define TILE_H

#include <xcb/xcb.h>

#define WM_CLIENT_MAPPED        (1 << 0)
#define WM_CLIENT_FLOATING      (1 << 1)
#define WM_CLIENT_FULLSCREEN    (1 << 2)
#define WM_CLIENT_INITIALIZED   (1 << 3)
#define WM_CLIENT_INPUT_HINT    (1 << 4)
#define WM_CLIENT_CHANGED       (1 << 5)
#define WM_CLIENT_TAKE_FOCUS    (1 << 6)
#define WM_CLIENT_WINDOW_DELETE (1 << 7)

typedef struct wm_tile wm_tile_t;
typedef struct wm_client wm_client_t;
typedef struct wm_container wm_container_t;
typedef enum wm_tile_type wm_tile_type_t;

int client_update_values(wm_client_t* client, uint32_t values[5]);
wm_tile_t* allocate_tile(wm_tile_type_t type, size_t size);


int container_next_client_offset(wm_container_t* container);
int container_remove_client(wm_container_t* container, int index);
wm_client_t* container_get_client(wm_container_t* container, int index);
int container_init(wm_container_t* container);
int container_next_client_offset(wm_container_t* container);
wm_client_t* container_get_client(wm_container_t* container, int index);
int container_remove_client(wm_container_t* container, int index);
void container_free(wm_container_t* container);

enum wm_tile_type {
    CLIENT,
    CONTAINER,
};

struct wm_tile {
    wm_tile_type_t type;
};

struct wm_client {
    wm_tile_t base; /* Base so we can have an array of wm_tile_t */
    wm_client_t* parent; /*Parent client for floating windows*/

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

struct wm_container {
    wm_tile_t base; /* Base so we can have an array of wm_tile_t */

    int size;
    int capacity;
    wm_client_t* clients;
    uint32_t width;
};

#endif // TILE_H
