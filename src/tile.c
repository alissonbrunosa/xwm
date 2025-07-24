#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <xcb/xcb.h>

#include "tile.h"

int client_update_values(wm_client_t* client, uint32_t values[5]) {
    if (client == NULL) {
        fprintf(stderr, "No client to check configuration change\n");
        return 0;
    }

    for (int i = 0; i < 5; i++) {
        if (client->values[i] != values[i]) {
            memcpy(client->values, values, sizeof(uint32_t) * 5);
            return 1;
        }
    }

    return 0;
}

int container_init(wm_container_t* container) {
    container->size = 0;
    container->capacity = 1;
    container->clients = (wm_client_t*) calloc(container->capacity, sizeof(wm_client_t));

    if (container->clients == NULL) {
        fprintf(stderr, "Failed to allocate memory for container clients\n");
        return 0;
    }

    return 1;
}

int container_next_client_offset(wm_container_t* container) {
    if (container->clients == NULL && !container_init(container)) {
        return -1;
    } else if (container->size == container->capacity) {
        wm_client_t* new_clients;
        int allocated_memory = container->capacity * sizeof(wm_client_t);
        int required_memory = allocated_memory << 1;

        container->capacity <<= 1;
        new_clients = (wm_client_t*) realloc(container->clients, required_memory);
        if (new_clients == NULL) {
            fprintf(stderr, "Failed to reallocate memory for container clients\n");
            return -1;
        }
        container->clients = new_clients;
        memset((char*) container->clients + allocated_memory, 0, required_memory - allocated_memory);
    }

    fprintf(stderr, "Allocating new client at index %d\n", container->size);
    return container->size++;
}

wm_client_t* container_get_client(wm_container_t* container, int index) {
    if (index < 0 || index >= container->size) {
        return NULL;
    }

    return &container->clients[index];
}

int container_remove_client(wm_container_t* container, int index) {
    if (container == NULL) {
        return 0;
    }

    for (int i = index; i < container->size - 1; ++i) {
        container->clients[i] = container->clients[i + 1];
    }
    container->size--;

    memset(&container->clients[container->size], 0, sizeof(wm_client_t));
    return 1;
}

void container_free(wm_container_t* container) {
    if (container == NULL) {
        return;
    }

    if (container->clients != NULL) {
        free(container->clients);
    }
}
