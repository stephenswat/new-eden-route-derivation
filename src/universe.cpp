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

System *Universe::get_system(int id) {
    return this->systems + this->system_map[id];
}

Celestial *Universe::get_entity(int id) {
    return this->entities + this->entity_map[id];
}

Celestial *Universe::get_entity_or_default(int id) {
    System *sys;
    Celestial *ent = NULL;

    if (id >= 30000000 && id < 40000000) {
        cerr << "Warning: ID " << id << " is a system. ";
        sys = this->get_system(id);

        for (int i = 0; i < sys->entity_count; i++) {
            if (sys->entities[i].type == STATION) {
                ent = &sys->entities[i];
                break;
            } else if (sys->entities[i].group_id == 6) {
                ent = &sys->entities[i];
            }
        }

        cerr << "Assuming " << (ent->type == STATION ? "station" : "celestial") << " " << *ent->name << ".\n";
    } else {
        ent = this->get_entity(id);
    }

    return ent;
}

void Universe::route(int src_id, int dst_id, struct trip *param) {
    Celestial *src = this->get_entity_or_default(src_id);
    Celestial *dst = this->get_entity_or_default(dst_id);

    cerr << "Routing from " << *src->name << " to " << *dst->name << "...\n";

    struct route *route = dijkstra(*this, src, dst, param);

    fprintf(stderr, "Travel time: %u minutes, %02u seconds (%d steps)\n", ((int) route->cost) / 60, ((int) route->cost) % 60, route->length);
    fprintf(stderr, "Route: \n");

    for (int i = 0; i < route->length; i++) {
        cerr << "    " << movement_type_str[route->points[i].type] << ": " << *route->points[i].entity->name << "\n";
    }

    free(route);
}

void Universe::add_system(int id, char *name, double x, double y, double z, unsigned int entities) {
    int seq_id = this->system_count++;
    System *s = &(this->systems[seq_id]);
    this->system_map[id] = seq_id;

    s->name = new std::string(name);
    s->id = id;
    s->seq_id = seq_id;
    s->entity_count = 0;

    s->pos[0] = x;
    s->pos[1] = y;
    s->pos[2] = z;
    s->pos[3] = 0.0;

    s->gates = NULL;
    s->entities = this->last_entity;
    this->last_entity += entities;
}

Celestial *Universe::add_entity(int system, int id, enum entity_type type, char *name, double x, double y, double z, Celestial *destination) {
    System *s = this->get_system(system);
    Celestial *e = &s->entities[s->entity_count++];
    int seq_id = e - this->entities;
    this->entity_count++;
    int important = 0;

    if (id >= 40000000 && id < 50000000) {
        this->entity_map[id] = seq_id;
    } else if (id >= 50000000 && id < 60000000) {
        important = 1;
        this->entity_map[id] = seq_id;
    } else if (id >= 60000000 && id < 70000000) {
        important = 1;
        this->entity_map[id] = seq_id;
    }

    if (important && s->gates == NULL) {
        s->gates = e;
    }

    e->name = new std::string(name);
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

Universe::Universe(unsigned int systems, unsigned int entities) {
    this->entities = (Celestial *) calloc(entities, sizeof(Celestial));
    this->systems = (System *) calloc(systems, sizeof(System));

    this->last_entity = this->entities;
}

Universe::~Universe() {
    for (int i = 0; i < this->entity_count; i++) {
        if (this->entities[i].name) delete this->entities[i].name;
    }

    for (int i = 0; i < this->system_count; i++) {
        if (this->systems[i].name) delete this->systems[i].name;
    }

    free(this->entities);
    free(this->systems);
}
