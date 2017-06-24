#pragma once

#include <string>
#include <vector>
#include <list>
#include <map>
#include <xmmintrin.h>

#include "parameters.hpp"

class Celestial;
class System;

enum entity_type {
    CELESTIAL, STATION, STARGATE
};

enum movement_type {
    JUMP, GATE, WARP, STRT
};

static std::string movement_type_str[4] = { [JUMP] = "JUMP", [GATE] = "GATE", [WARP] = "WARP", [STRT] = "STRT" };

struct waypoint {
    Celestial *entity;
    enum movement_type type;
    float time, fatigue, reactivation, wait, distance;
};

class Route {
public:
    int loops;
    double cost;
    std::list<struct waypoint> points;

    void concatenate(const Route& that) {
        cost += that.cost;
        points.insert(points.end(), that.points.begin(), that.points.end());
    }
};

class Entity {
public:
    float x, y, z;
    int id, seq_id;
    std::string name;
};

class Celestial: public Entity {
public:
    enum entity_type type;
    int group_id;
    System *system;
    Celestial *destination;
    Celestial *bridge;
    float jump_range;

    bool is_relevant() {
        return this->destination || this->bridge || !isnan(this->jump_range);
    }
};

class System: public Entity {
public:
    int entity_count;
    Celestial *entities, *gates;
    float security;
};

class Universe {
public:
    Universe(FILE *, FILE *);
    Universe(std::string, std::string);
    ~Universe();

    void add_system(int, char *, double, double, double, unsigned int, float);
    Celestial *add_entity(int, int, enum entity_type, char *, double, double, double, Celestial *);

    void add_dynamic_bridge(Celestial *, float);
    void add_dynamic_bridge(int, float);

    void add_static_bridge(Celestial *, Celestial *);
    void add_static_bridge(int, int);

    #ifndef SWIG
    Route *get_route(int, int, Parameters *);
    Route *get_route(Celestial &, Celestial &, Parameters *);
    Route *get_route(std::vector<Celestial *>, Parameters *);
    #endif

    Route *get_route(std::vector<int>, Parameters *);

    std::map<Celestial *, float> *get_all_distances(int, Parameters *);
    std::map<Celestial *, float> *get_all_distances(Celestial &, Parameters *);

    Celestial *get_entity(int);
    System *get_system(int);

    Celestial *get_entity_by_seq_id(int);
    System *get_system_by_seq_id(int);

    Celestial *get_entity_or_default(int);

    int system_count = 0, entity_count = 0, stargate_count = 0;
    System *systems;
    Celestial *entities;

private:
    void initialise(FILE *, FILE *);
    void load_stargates(FILE *);
    void load_systems_and_entities(FILE *);
    Celestial *last_entity;
    std::map<int, int> entity_map, system_map;
};
