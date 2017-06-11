#pragma once

#include <string>
#include <list>
#include <map>
#include <xmmintrin.h>

class Celestial;
class System;

enum entity_type {
    CELESTIAL, STATION, STARGATE
};

enum movement_type {
    JUMP, GATE, WARP, STRT
};

static std::string movement_type_str[4] = { [JUMP] = "JUMP", [GATE] = "GATE", [WARP] = "WARP", [STRT] = "STRT" };

struct trip {
    double jump_range;
    double warp_speed;
    double align_time;
    double gate_cost;
};

struct waypoint {
    Celestial *entity;
    enum movement_type type;
};

class Route {
public:
    int loops;
    double cost;
    std::list<struct waypoint> points;
};

class Entity {
public:
    __m128 pos;
    int id, seq_id;
    std::string *name;
};

class Celestial: public Entity {
public:
    enum entity_type type;
    int group_id;
    System *system;
    Celestial *destination;
};

class System: public Entity {
public:
    int entity_count;
    Celestial *entities, *gates;
};

class Universe {
public:
    Universe(FILE *, FILE *);
    Universe(std::string, std::string);
    ~Universe();
    void add_system(int, char *, double, double, double, unsigned int);
    Celestial *add_entity(int, int, enum entity_type, char *, double, double, double, Celestial *);
    Route *route(int, int, struct trip *);
    Route *route(Celestial &, Celestial &, struct trip *);
    Celestial *get_entity(int);
    System *get_system(int);
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
