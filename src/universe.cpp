#include <string>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "universe.hpp"
#include "dijkstra.hpp"

using namespace std;

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
        sys = this->get_system(id);

        for (int i = 0; i < sys->entity_count; i++) {
            if (sys->entities[i].type == STATION) {
                ent = &sys->entities[i];
                break;
            } else if (sys->entities[i].group_id == 6) {
                ent = &sys->entities[i];
            }
        }
    } else {
        ent = this->get_entity(id);
    }

    return ent;
}

Route *Universe::get_route(int src_id, int dst_id, Parameters *param) {
    return this->get_route(
        *this->get_entity_or_default(src_id),
        *this->get_entity_or_default(dst_id),
        param
    );
}

Route *Universe::get_route(Celestial &src, Celestial &dst, Parameters *param) {
    return Dijkstra(*this, &src, &dst, param).get_route();
}

std::map<Celestial *, float> *Universe::get_all_distances(int src_id, Parameters *param) {
    return this->get_all_distances(
        *this->get_entity_or_default(src_id),
        param
    );
}

std::map<Celestial *, float> *Universe::get_all_distances(Celestial &src, Parameters *param) {
    return Dijkstra(*this, &src, NULL, param).get_all_distances();
}

void Universe::add_dynamic_bridge(int src, float range) {
    add_dynamic_bridge(this->get_entity(src), range);
}

void Universe::add_dynamic_bridge(Celestial *src, float range) {
    src->jump_range = range;

    if (src->system->gates > src) src->system->gates = src;
}

void Universe::add_static_bridge(int src, int dst) {
    add_static_bridge(this->get_entity(src), this->get_entity(dst));
}

void Universe::add_static_bridge(Celestial *src, Celestial *dst) {
    if (src->bridge || dst->bridge) {
        throw 20;
    }

    src->bridge = dst;
    if (src->system->gates > src) src->system->gates = src;

    dst->bridge = src;
    if (dst->system->gates > dst) dst->system->gates = dst;
}


void Universe::add_system(int id, char *name, double x, double y, double z, unsigned int entities, float security) {
    int seq_id = this->system_count++;
    System *s = &(this->systems[seq_id]);
    this->system_map[id] = seq_id;

    s->name = std::string(name);
    s->id = id;
    s->seq_id = seq_id;
    s->entity_count = 0;

    s->x = x;
    s->y = y;
    s->z = z;

    s->security = security;

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

    e->name = std::string(name);
    e->id = id;
    e->seq_id = seq_id;

    e->x = x;
    e->y = y;
    e->z = z;

    e->system = s;
    e->jump_range = NAN;
    e->bridge = NULL;

    e->type = type;
    e->destination = destination;

    return e;
}

void Universe::load_systems_and_entities(FILE *f) {
    int res;
    Celestial *ent;

    unsigned int id, type_id, group_id, system_id, region_id;
    char solar_system_str[128], region_str[128], name[128], security[128], buffer[512];
    double x, y, z;

    std::map<int, int> per_system_entities;

    for (int i = 0; i < 2; i++) {
        fseek(f, 0, SEEK_SET);
        fgets(buffer, 512, f);

        do {
            res = fscanf(f, "%d,%d,%d,%[^,],%*[^,],%[^,],%*[^,],%lf,%lf,%lf,%*[^,],%[^,],%[^,],%*[^,],%*[^,\n]\n",
                &id, &type_id, &group_id, solar_system_str, region_str, &x, &y, &z, name, security
            );

            if (strcmp("None", solar_system_str) == 0) {
                system_id = 0;
            } else {
                system_id = atoi(solar_system_str);
            }

            if (id >= 30000000 && id < 70000000) {
                region_id = atoi(region_str);

                if (region_id == 10000019 || region_id == 10000017 || region_id == 10000004) {
                    continue;
                }
            }

            if (i == 0) {
                if (id >= 40000000 && id < 70000000) {
                    if (per_system_entities.find(system_id) == per_system_entities.end()) {
                        per_system_entities[system_id] = 1;
                    } else {
                        per_system_entities[system_id]++;
                    }
                }
            } else {
                if (id >= 30000000 && id < 40000000) {
                    this->add_system(id, name, x, y, z, per_system_entities[id], atof(security));
                } else if (id >= 40000000 && id < 50000000) {
                    ent = this->add_entity(system_id, id, CELESTIAL, name, x, y, z, NULL);
                    ent->group_id = group_id;
                } else if (id >= 50000000 && id < 60000000) {
                    ent = this->add_entity(system_id, id, STARGATE, name, x, y, z, NULL);
                } else if (id >= 60000000 && id < 70000000) {
                    ent = this->add_entity(system_id, id, STATION, name, x, y, z, NULL);
                }
            }
        } while (res != EOF);
    }
}

void Universe::load_stargates(FILE *f) {
    int src, dst, res;
    char buffer[512];
    Celestial *src_e, *dst_e;

    fgets(buffer, 512, f);

    do {
        res = fscanf(f, "%d,%d\n", &src, &dst);

        src_e = this->get_entity(src);
        dst_e = this->get_entity(dst);

        if (src_e == NULL || dst_e == NULL) {
            continue;
        }

        src_e->destination=dst_e;

        src_e->name = std::string(src_e->system->name + " - " + dst_e->system->name + " gate");
    } while (res != EOF);
}

void Universe::initialise(FILE *entities, FILE *gates) {
    this->entities = new Celestial[500000];
    this->systems = new System[9000];

    this->last_entity = this->entities;

    this->load_systems_and_entities(entities);
    this->load_stargates(gates);
}

Universe::Universe(std::string entities, std::string gates) {
    FILE *e = fopen(entities.c_str(), "r");
    FILE *g = fopen(gates.c_str(), "r");

    this->initialise(e, g);

    fclose(e);
    fclose(g);
}

Universe::Universe(FILE *entities, FILE *gates) {
    this->initialise(entities, gates);
}

Universe::~Universe() {
    delete[] this->entities;
    delete[] this->systems;
}
