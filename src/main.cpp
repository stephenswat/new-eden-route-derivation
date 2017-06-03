#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <argp.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "main.hpp"
#include "universe.hpp"
#include "dijkstra.hpp"

int verbose = 0;

struct arguments {
    char *args[2];
    char *batch;
    int silent;
    int quit;
    int gen_count;
    int src;
    int dst;
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
    struct arguments *arguments = (struct arguments *) state->input;

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

void load_systems_and_entities(Universe &u, FILE *f) {
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
                    u.add_system(id, name, x, y, z, per_system_entities[id]);
                } else if (id >= 40000000 && id < 50000000) {
                    ent = u.add_entity(system_id, id, CELESTIAL, name, x, y, z, NULL);
                    ent->group_id = group_id;
                } else if (id >= 50000000 && id < 60000000) {
                    ent = u.add_entity(system_id, id, STARGATE, name, x, y, z, NULL);
                } else if (id >= 60000000 && id < 70000000) {
                    ent = u.add_entity(system_id, id, STATION, name, x, y, z, NULL);
                }
            }
        } while (res != EOF);
    }
}

void load_stargates(Universe &u, FILE *f) {
    int src, dst, res;
    char buffer[512];
    Celestial *src_e, *dst_e;

    fgets(buffer, 512, f);

    do {
        res = fscanf(f, "%d,%d\n", &src, &dst);

        src_e = u.get_entity(src);
        dst_e = u.get_entity(dst);

        if (src_e == NULL || dst_e == NULL) {
            continue;
        }

        src_e->destination=dst_e;

        if (src_e->name) {
            delete src_e->name;
        }

        src_e->name = new std::string(*src_e->system->name + " - " + *dst_e->system->name + " gate");
    } while (res != EOF);
}

void run_batch_experiment(Universe &u, FILE *f) {
    int res;
    int src, dst;
    Celestial *src_e, *dst_e;
    Route *route;

    do {
        res = fscanf(f, "%d %d\n", &src, &dst);

        src_e = u.get_entity(src);
        dst_e = u.get_entity(dst);

        clock_gettime(CLOCK_MONOTONIC, &timer_start);
        route = dijkstra(u, src_e, dst_e, &parameters);
        clock_gettime(CLOCK_MONOTONIC, &timer_end);

        printf("%d %d %ld %d %d\n", src, dst, time_diff(&timer_start, &timer_end), route->points.size(), route->loops);

        free(route);
    } while (res != EOF);
}

void run_user_interface(Universe &universe) {
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
            universe.route(s, d, &parameters);
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

void run_generate_batch(Universe &u, char type, int count) {
    System *src_s, *dst_s;
    Celestial *src_e, *dst_e;

    for (int i = 0; i < count; i++) {
        do {
            src_s = &u.systems[rand() % u.system_count];
        } while (src_s->id >= 31000000);

        do {
            dst_s = &u.systems[rand() % u.system_count];
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
    fprintf(stderr, "%12s: %lu bytes\n", "universe", sizeof(Universe));
    fprintf(stderr, "%12s: %lu bytes\n", "trip", sizeof(struct trip));
    fprintf(stderr, "%12s: %lu bytes\n", "route", sizeof(Route));
    fprintf(stderr, "%12s: %lu bytes\n", "entity", sizeof(Celestial));
    fprintf(stderr, "%12s: %lu bytes\n", "system", sizeof(System));
}

int main(int argc, char **argv) {
    struct arguments arguments;
    Universe universe(9000, 500000);

    arguments.batch = NULL;
    arguments.src = 0;
    arguments.dst = 0;
    arguments.gen_type = 0;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if (verbose >= 1) {
        print_additional_information();
    }

    clock_gettime(CLOCK_MONOTONIC, &timer_start);
    load_systems_and_entities(universe, fopen(arguments.args[0], "r"));
    load_stargates(universe, fopen(arguments.args[1], "r"));
    clock_gettime(CLOCK_MONOTONIC, &timer_end);

    fprintf(stderr, "Loaded New Eden (%d systems, %d entities) in %.3f seconds...\n",
        universe.system_count, universe.entity_count, time_diff(&timer_start, &timer_end) / 1E9
    );

    if (arguments.src != 0 && arguments.dst != 0) {
        universe.route(arguments.src, arguments.dst, &parameters);
    } else if (arguments.batch != NULL) {
        run_batch_experiment(universe, fopen(arguments.batch, "r"));
    } else if (arguments.gen_type != 0) {
        run_generate_batch(universe, arguments.gen_type, arguments.gen_count);
    } else if (!arguments.quit) {
        run_user_interface(universe);
    }

    return 0;
}
