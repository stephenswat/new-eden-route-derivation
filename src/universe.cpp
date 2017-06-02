#include <string>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "universe.hpp"
#include "dijkstra.hpp"

using namespace std;

static string movement_type_str[4] = { [JUMP] = "JUMP", [GATE] = "GATE", [WARP] = "WARP", [STRT] = "STRT" };

struct system *universe_get_system(struct universe *u, int id) {
    int index = u->system_map[id % 10000000];

    if (index == -1) return NULL;
    return u->systems + index;
}

struct entity *universe_get_entity(struct universe *u, int id) {
    int index = -1;

    if (id >= 40000000 && id < 50000000) {
        index = u->celestial_map[id % 10000000];
    } else if (id >= 50000000 && id < 60000000) {
        index = u->stargate_map[id % 10000000];
    } else if (id >= 60000000 && id < 70000000) {
        index = u->station_map[id % 10000000];
    }

    if (index == -1) return NULL;
    return u->entities + index;
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
        cerr << "    " << movement_type_str[route->points[i].type] << ": " << route->points[i].entity->name;
    }

    free(route);
}

void universe_add_system(struct universe *u, int id, char *name, double x, double y, double z, unsigned int entities) {
    int seq_id = u->system_count++;
    struct system *s = &(u->systems[seq_id]);
    u->system_map[id % 10000000] = seq_id;

    s->name = strdup(name);
    s->id = id;
    s->seq_id = seq_id;
    s->entity_count = 0;

    s->pos[0] = x;
    s->pos[1] = y;
    s->pos[2] = z;
    s->pos[3] = 0.0;

    s->gates = NULL;
    s->entities = u->last_entity;
    u->last_entity += entities;
}

struct entity *universe_add_entity(struct universe *u, int system, int id, enum entity_type type, char *name, double x, double y, double z, struct entity *destination) {
    struct system *s = universe_get_system(u, system);
    struct entity *e = &s->entities[s->entity_count++];
    int seq_id = e - u->entities;
    u->entity_count++;
    int important = 0;

    if (id >= 40000000 && id < 50000000) {
        u->celestial_map[id % 10000000] = seq_id;
    } else if (id >= 50000000 && id < 60000000) {
        important = 1;
        u->stargate_map[id % 10000000] = seq_id;
    } else if (id >= 60000000 && id < 70000000) {
        important = 1;
        u->station_map[id % 10000000] = seq_id;
    }

    if (important && s->gates == NULL) {
        s->gates = e;
    }

    e->name = strdup(name);
    e->id = id;
    e->seq_id = seq_id;

    e->pos[0] = x;
    e->pos[1] = y;
    e->pos[2] = z;
    e->pos[3] = 0.0;

    e->system = s;

    e->type = type;
    e->destination = destination;

    return e;
}

struct universe *universe_init(unsigned int systems, unsigned int entities) {
    struct universe *u = (struct universe *) calloc(1, sizeof(struct universe));

    u->entities = (struct entity *) calloc(entities, sizeof(struct entity));
    u->systems = (struct system *) calloc(systems, sizeof(struct system));
    u->last_entity = u->entities;

    return u;
}

void universe_free(struct universe *u) {
    for (int i = 0; i < u->entity_count; i++) {
        if (u->entities[i].name) free(u->entities[i].name);
    }

    for (int i = 0; i < u->system_count; i++) {
        if (u->systems[i].name) free(u->systems[i].name);
    }

    free(u);
}
