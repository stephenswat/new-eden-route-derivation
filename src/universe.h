#pragma once

#define LIMIT_ENTITIES 500000
#define LIMIT_ENTITIES_PER_SYSTEM 256
#define LIMIT_SYSTEMS 9000

enum entity_type {
    CELESTIAL, STATION, STARGATE
};

enum movement_type {
    JUMP, GATE, WARP, STRT
};

struct trip {
    double jump_range;
    double warp_speed;
    double align_time;
    double gate_cost;
};

struct waypoint {
    struct entity *entity;
    enum movement_type type;
};

struct route {
    int length, loops;
    double cost;
    struct waypoint points[];
};

struct entity {
    float x, y, z;
    int id, seq_id;
    char *name;

    enum entity_type type;
    int group_id;
    struct system *system;
    struct entity *destination;
} __attribute__ ((aligned(64)));

struct system {
    float x, y, z;
    int id, seq_id;
    char *name;

    int entity_count;
    struct entity entities[LIMIT_ENTITIES_PER_SYSTEM];
};

struct universe {
    int system_count, entity_count, stargate_count;
    struct system systems[LIMIT_SYSTEMS];
    struct entity **stargate_start;
    struct entity *entities[LIMIT_SYSTEMS * LIMIT_ENTITIES_PER_SYSTEM];
};

struct universe *universe_init();
void universe_free(struct universe *);
void universe_add_system(struct universe *, int, char *, double, double, double);
struct entity *universe_add_entity(struct universe *, int, int, enum entity_type, char *, double, double, double, struct entity *);
void universe_route(struct universe *, int, int, struct trip *);
struct entity *universe_get_entity(struct universe *, int);
struct entity *universe_get_typed_entity(struct universe *, int, enum entity_type);
struct entity *universe_get_entity_or_default(struct universe *, int);
