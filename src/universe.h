#pragma once

#define LIMIT_ENTITIES_PER_SYSTEM 256
#define LIMIT_SYSTEMS 9000

enum entity_type {
    CELESTIAL, STATION, STARGATE
};

struct trip {
    int jump_drive;
    double jump_range;
    double warp_speed;
};

struct route {
    int length;
    struct entity **points;
};

struct entity {
    int id, seq_id;
    double x, y, z;
    char name[128];
    enum entity_type type;
    int group_id;
    struct system *system;
    struct entity *destination;
};

struct system {
    int id, seq_id;
    double x, y, z;
    char name[128];
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
void universe_add_system(struct universe *, int, char *, double, double, double);
struct entity *universe_add_entity(struct universe *, int, int, enum entity_type, char *, double, double, double, struct entity *);
void universe_route(struct universe *, int, int);
struct entity *universe_get_entity(struct universe *, int);
struct entity *universe_get_typed_entity(struct universe *, int, enum entity_type);
double entity_distance(struct entity *, struct entity *);
