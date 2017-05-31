#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "universe.h"
#include "dijkstra.h"

static char *movement_type_str[4] = { [JUMP] = "JUMP", [GATE] = "GATE", [WARP] = "WARP", [STRT] = "STRT" };

struct system *universe_get_system(struct universe *u, int id) {
    int low = 0, mid, high = u->system_count - 1;

    while (low <= high) {
        mid = (low + high) / 2;

        if (u->systems[mid].id == id) {
            return &u->systems[mid];
        } else if (u->systems[mid].id < id) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    return NULL;
}

struct entity *universe_get_entity(struct universe *u, int id) {
    for (int i = 0; i < u->entity_count; i++) {
        if (u->entities[i]->id == id) {
            return u->entities[i];
        }
    }

    return NULL;
}

struct entity *universe_get_typed_entity(struct universe *u, int id, enum entity_type type) {
    int count = u->entity_count;
    struct entity **start = u->entities;

    switch (type) {
        case STARGATE:
            count = u->stargate_count;
            start = u->stargate_start;
        default:
            break;
    }

    int low = 0, mid, high = count - 1;

    while (low <= high) {
        mid = (low + high) / 2;

        if (start[mid]->id == id) {
            return start[mid];
        } else if (start[mid]->id < id) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    return NULL;
}

struct entity *universe_get_entity_or_default(struct universe *u, int id) {
    struct system *sys;
    struct entity *ent = NULL;

    if (id >= 30000000 && id < 40000000) {
        fprintf(stderr, "Warning: ID %d is a system. ", id);
        sys = universe_get_system(u, id);

        for (int i = 0; i < sys->entity_count; i++) {
            if (sys->entities[i].type == STATION) {
                ent = &sys->entities[i];
                break;
            } else if (sys->entities[i].group_id == 6) {
                ent = &sys->entities[i];
            }
        }

        fprintf(stderr, "Assuming %s %s.\n", (ent->type == STATION ? "station" : "celestial"), ent->name);
    } else {
        ent = universe_get_entity(u, id);
    }

    return ent;
}

void universe_route(struct universe *u, int src_id, int dst_id, struct trip *param) {
    struct entity *src = universe_get_entity_or_default(u, src_id);
    struct entity *dst = universe_get_entity_or_default(u, dst_id);
    struct route *route;

    fprintf(stderr, "Routing from %s to %s...\n", src->name, dst->name);

    route = dijkstra(u, src, dst, param);

    fprintf(stderr, "Travel time: %u minutes, %02u seconds (%d steps)\n", ((int) route->cost) / 60, ((int) route->cost) % 60, route->length);
    fprintf(stderr, "Route: \n");

    for (int i = 0; i < route->length; i++) {
        fprintf(stderr, "    %s: %s\n", movement_type_str[route->points[i].type], route->points[i].entity->name);
    }

    free(route);
}

void universe_add_system(struct universe *u, int id, char *name, double x, double y, double z) {
    int seq_id = u->system_count++;
    struct system *s = &(u->systems[seq_id]);

    s->name = strdup(name);
    s->id = id;
    s->seq_id = seq_id;

    s->x = x;
    s->y = y;
    s->z = z;
}

struct entity *universe_add_entity(struct universe *u, int system, int id, enum entity_type type, char *name, double x, double y, double z, struct entity *destination) {
    struct system *s = universe_get_system(u, system);
    struct entity *e = &(s->entities[s->entity_count++]);

    e->name = strdup(name);
    e->id = id;
    e->seq_id = u->entity_count++;
    u->entities[e->seq_id] = e;

    if (type == STARGATE) {
        if (u->stargate_count == 0) u->stargate_start = &u->entities[e->seq_id];
        u->stargate_count++;
    }

    e->x = x;
    e->y = y;
    e->z = z;

    e->system = s;

    e->type = type;
    e->destination = destination;

    return e;
}

struct universe *universe_init() {
    return calloc(1, sizeof(struct universe));
}

void universe_free(struct universe *u) {
    for (int i = 0; i < u->entity_count; i++) {
        if (u->entities[i]->name) free(u->entities[i]->name);
    }

    for (int i = 0; i < u->system_count; i++) {
        if (u->systems[i].name) free(u->systems[i].name);
    }

    free(u);
}
