#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <argp.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "main.h"
#include "universe.h"
#include "dijkstra.h"

int verbose = 0;

struct universe *universe;

struct arguments {
    char *args[2];
    char *batch;
    int silent;
    int quit;
    int gen_count;
    int src, dst;
    char gen_type;
};

static struct timespec timer_start, timer_end;

static char args_doc[] = "mapDenormalized mapJumps";

static struct trip parameters = { .jump_range = NAN, .warp_speed = 3, .align_time = 5.0};

static struct argp_option options[] = {
    {"quiet", 'q', 0, 0, "Don't produce any output", 0 },
    {"silent", 's', 0, OPTION_ALIAS },
    {"verbose", 'v', "level", OPTION_ARG_OPTIONAL , "Produce some extra output", 0 },

    {"route", 'R', "origin:destination", 0, "Route between two entity IDs", 1},
    {"nothing", 'N', 0, 0, "Uninteractively exit without doing routing", 1},
    {"experiment", 'E', "file", 0, "Read experiment parameters from file", 1},
    {"generate", 'G', "count:l|r|w", 0, "Write experimental parameters to stdout", 1},

    {"jump", 2000, "value", 0, "Set jump drive range in lightyears", 2},
    {"align", 2001, "value", 0, "Set align time in seconds", 2},
    {"warp", 2002, "value", 0, "Set warp speed in astronomical units per second", 2},
    {"gate", 2003, "value", 0, "Set the per-gate time in seconds (negative to disable)", 2},

    {"version", 3000, 0, 0, "Print the version of the program", 3},
    { 0 }
};

long time_diff(struct timespec *start, struct timespec *end) {
    return ((end->tv_sec * 1000000000 + end->tv_nsec) -
            (start->tv_sec * 1000000000 + start->tv_nsec));
}

static error_t parse_opt (int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;

    switch (key) {
        case 'v':
            verbose = arg ? atoi(arg) : 1;
            break;
        case 'q':
        case 's':
            // arguments->silent = 1;
            break;
        case 'E':
            arguments->batch = arg;
            break;
        case 'N':
            arguments->quit = 1;
            break;
        case 'G':
            sscanf(arg, "%d:%c", &arguments->gen_count, &arguments->gen_type);
            break;
        case 'R':
            sscanf(arg, "%d:%d", &arguments->src, &arguments->dst);
            break;
        case 2000:
            parameters.jump_range = atof(arg);
            break;
        case 2001:
            parameters.align_time = atof(arg);
            break;
        case 2002:
            parameters.warp_speed = atof(arg);
            break;
        case 2003:
            parameters.gate_cost = atof(arg);
            break;
        case 3000:
            fprintf(stderr, "NERD-" VERSION "\n");
            exit(0);
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num >= 2)
            argp_usage(state);
            arguments->args[state->arg_num] = arg;
            break;
        case ARGP_KEY_END:
            if (state->arg_num < 2)
            argp_usage(state);
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, NULL };

struct universe *load_systems_and_entities(FILE *f) {
    int res;
    struct entity *ent;
    struct universe *u;

    unsigned int id, type_id, group_id, system_id, region_id;
    char solar_system_str[128], constellation_id[128], region_str[128];
    double x, y, z;
    char name[128], security[128], buffer[512];

    fgets(buffer, 512, f);

    unsigned int system_count = 0, entity_count = 0, system_max = 0;

    fgets(buffer, 512, f);

    do {
        res = fscanf(f, "%d,%*d,%*d,%*[^,],%*[^,],%[^,],%*[^,],%*f,%*f,%*f,%*[^,],%*[^,],%*[^,],%*[^,],%*[^,\n]\n", &id, region_str);

        if (id >= 30000000 && id < 40000000) {
            region_id = atoi(region_str);

            if (region_id == 10000019 || region_id == 10000017 || region_id == 10000004) {
                continue;
            }

            system_count++;

            if (id > system_max) system_max = id;
        } else if (id >= 40000000) {
            break;
        }
    } while (res != EOF);

    int system_map[(system_max - 30000000) + 1];
    unsigned int per_system_entities[system_count + 1];
    system_count = 0;

    for (unsigned int i = 0; i < ((system_max - 30000000) + 1); i++) {
        system_map[i] = -1;
    }

    fseek(f, 0, SEEK_SET);
    fgets(buffer, 512, f);

    unsigned int entity_type_max[10] = {0};
    unsigned int sid, cat;

    do {
        res = fscanf(f, "%d,%*d,%*d,%[^,],%*[^,],%[^,],%*[^,],%*f,%*f,%*f,%*[^,],%*[^,],%*[^,],%*[^,],%*[^,\n]\n",
            &id, solar_system_str, region_str
        );

        if (id >= 30000000 && id < 70000000) {
            region_id = atoi(region_str);

            if (region_id == 10000019 || region_id == 10000017 || region_id == 10000004) {
                continue;
            }
        }

        if (strcmp("None", solar_system_str) == 0) {
            system_id = 0;
        } else {
            system_id = atoi(solar_system_str);
        }

        if (id >= 30000000 && id < 40000000) {
            if (system_map[id - 30000000] == -1) {
                system_map[id - 30000000] = system_count++;
            }
        } else if (id >= 40000000 && id < 70000000) {
            entity_count++;
            assert(system_map[system_id - 30000000] != -1);
            per_system_entities[system_map[system_id - 30000000]]++;
        }

        sid = id % 10000000;
        cat = id / 10000000;

        if (sid > entity_type_max[cat]) entity_type_max[cat] = sid;
    } while (res != EOF);

    u = universe_init(system_count, entity_count);

    u->system_map = calloc(entity_type_max[3], sizeof(unsigned int));
    u->celestial_map = calloc(entity_type_max[4], sizeof(unsigned int));
    u->stargate_map = calloc(entity_type_max[5], sizeof(unsigned int));
    u->station_map = calloc(entity_type_max[6], sizeof(unsigned int));

    for (unsigned int i = 0; i < entity_type_max[3]; i++) u->system_map[i] = -1;
    for (unsigned int i = 0; i < entity_type_max[4]; i++) u->celestial_map[i] = -1;
    for (unsigned int i = 0; i < entity_type_max[5]; i++) u->stargate_map[i] = -1;
    for (unsigned int i = 0; i < entity_type_max[6]; i++) u->station_map[i] = -1;

    fseek(f, 0, SEEK_SET);
    fgets(buffer, 512, f);

    do {
        res = fscanf(f, "%d,%d,%d,%[^,],%[^,],%[^,],%*[^,],%lf,%lf,%lf,%*[^,],%[^,],%[^,],%*[^,],%*[^,\n]\n",
            &id, &type_id, &group_id, solar_system_str, constellation_id, region_str, &x, &y, &z, name, security
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

        if (id >= 30000000 && id < 40000000) {
            universe_add_system(u, id, name, x, y, z, per_system_entities[system_map[id - 30000000]]);
        } else if (id >= 40000000 && id < 50000000) {
            ent = universe_add_entity(u, system_id, id, CELESTIAL, name, x, y, z, NULL);
            ent->group_id = group_id;
        } else if (id >= 50000000 && id < 60000000) {
            ent = universe_add_entity(u, system_id, id, STARGATE, name, x, y, z, NULL);
        } else if (id >= 60000000 && id < 70000000) {
            ent = universe_add_entity(u, system_id, id, STATION, name, x, y, z, NULL);
        }
    } while (res != EOF);

    return u;
}

void load_stargates(FILE *f, struct universe *u) {
    int src, dst, res;
    char buffer[512];
    struct entity *src_e, *dst_e;

    fgets(buffer, 512, f);

    do {
        res = fscanf(f, "%d,%d\n", &src, &dst);

        src_e = universe_get_entity(u, src);
        dst_e = universe_get_entity(u, dst);

        if (src_e == NULL || dst_e == NULL) {
            continue;
        }

        src_e->destination=dst_e;

        if (src_e->name) {
            free(src_e->name);
        }

        asprintf(&src_e->name, "%s - %s gate", src_e->system->name, dst_e->system->name);
    } while (res != EOF);
}

void run_batch_experiment(FILE *f) {
    int res;
    int src, dst;
    struct entity *src_e, *dst_e;
    struct route *route;

    do {
        res = fscanf(f, "%d %d\n", &src, &dst);

        src_e = universe_get_entity(universe, src);
        dst_e = universe_get_entity(universe, dst);

        clock_gettime(CLOCK_MONOTONIC, &timer_start);
        route = dijkstra(universe, src_e, dst_e, &parameters);
        clock_gettime(CLOCK_MONOTONIC, &timer_end);

        printf("%d %d %011ld %d %d\n", src, dst, time_diff(&timer_start, &timer_end), route->length, route->loops);

        free(route);
    } while (res != EOF);
}

void run_user_interface(void) {
    char *pch, *input, *param, *value;

    while (1) {
        input = readline("NERD> ");
        add_history(input);

        pch = strtok(input, " ");

        if (strcmp(pch, "quit") == 0) {
            break;
        } else if (strcmp(pch, "route") == 0) {
            int s = atoi(strtok(NULL, " "));
            int d = atoi(strtok(NULL, " "));
            universe_route(universe, s, d, &parameters);
        } else if (strcmp(pch, "parameters") == 0) {
            fprintf(stderr, "Jump drive: %.1f LY\n", parameters.jump_range);
            fprintf(stderr, "Warp speed: %.1f AU/s\n", parameters.warp_speed);
            fprintf(stderr, "Align time: %.2f s\n", parameters.align_time);
        } else if (strcmp(pch, "set") == 0) {
            param = strtok(NULL, " ");
            value = strtok(NULL, " ");

            if (strcmp(param, "warp") == 0) {
                parameters.warp_speed = atof(value);
            } else if (strcmp(param, "jump") == 0) {
                parameters.jump_range = atof(value);
            } else if (strcmp(param, "align") == 0) {
                parameters.align_time = atof(value);
            }
        }
    }
}

void run_generate_batch(struct universe *u, char type, int count) {
    struct system *src_s, *dst_s;
    struct entity *src_e, *dst_e;

    for (int i = 0; i < count; i++) {
        do {
            src_s = &u->systems[rand() % u->system_count];
        } while (src_s->id >= 31000000);

        do {
            dst_s = &u->systems[rand() % u->system_count];
        } while ((dst_s->id >= 31000000 && type == 'r') || (dst_s->id < 31000000 && type == 'w'));

        if (type == 'l') {
            dst_s = src_s;
        }

        src_e = &src_s->entities[rand() % src_s->entity_count];
        dst_e = &dst_s->entities[rand() % dst_s->entity_count];

        printf("%d %d\n", src_e->id, dst_e->id);
    }
}

void print_additional_information(void) {
    fprintf(stderr, "%12s: %lu bytes\n", "universe", sizeof(struct universe));
    fprintf(stderr, "%12s: %lu bytes\n", "trip", sizeof(struct trip));
    fprintf(stderr, "%12s: %lu bytes\n", "route", sizeof(struct route));
    fprintf(stderr, "%12s: %lu bytes\n", "entity", sizeof(struct entity));
    fprintf(stderr, "%12s: %lu bytes\n", "system", sizeof(struct system));
}

int main(int argc, char **argv) {
    struct arguments arguments = { .batch = NULL, .src = 0, .dst = 0, .gen_type = 0 };

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if (verbose >= 1) {
        print_additional_information();
    }

    clock_gettime(CLOCK_MONOTONIC, &timer_start);
    universe = load_systems_and_entities(fopen(arguments.args[0], "r"));
    load_stargates(fopen(arguments.args[1], "r"), universe);
    clock_gettime(CLOCK_MONOTONIC, &timer_end);

    fprintf(stderr, "Loaded New Eden (%d systems, %d entities) in %.3f seconds...\n",
        universe->system_count, universe->entity_count, time_diff(&timer_start, &timer_end) / 1E9
    );

    if (arguments.src != 0 && arguments.dst != 0) {
        universe_route(universe, arguments.src, arguments.dst, &parameters);
    } else if (arguments.batch != NULL) {
        run_batch_experiment(fopen(arguments.batch, "r"));
    } else if (arguments.gen_type != 0) {
        run_generate_batch(universe, arguments.gen_type, arguments.gen_count);
    } else if (!arguments.quit) {
        run_user_interface();
    }

    universe_free(universe);

    return 0;
}
