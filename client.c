#include <stdlib.h>
#include "client.h"

XContext window_context;

client_t *client_create(Window w)
{
    client_t *client;

    client = malloc(sizeof(client_t));
    if (client == NULL) return NULL;
    memset(client, 0, sizeof(client_t));
    
    client->window = w;
    client->transient_for = None;
    client->managed = 1;
    client->name = "???";
    client->state = WithdrawnState;

    if (XSaveContext(dpy, w, window_context, client) != 0) {
        free(client);
        return NULL;
    }
    printf("Created an entry for window 0x%08X at 0x%08X\n", w, client);
    return client;
}

client_t *client_find(Window w)
{
    client_t *client;

    if (XFindContext(dpy, w, window_context, &client) != 0) return NULL;
    return client;
}
