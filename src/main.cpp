#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <argp.h>

#include <readline/readline.h>
#include <readline/history.h>

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

static Parameters parameters(3.0, 5.0);

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
            fprintf(stderr, "NERD-0.0.15\n");
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

void run_batch_experiment(Universe &u, FILE *f) {
    int res;
    int src, dst;
    Celestial *src_e, *dst_e;
    Route *route;

    do {
        res = fscanf(f, "%d %d\n", &src, &dst);

        src_e = u.get_entity(src);
        dst_e = u.get_entity(dst);

        route = Dijkstra(u, src_e, dst_e, &parameters).get_route();

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
            universe.get_route(s, d, &parameters);
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

void run_route(Universe &u, int src_id, int dst_id, Parameters *param) {
    Celestial *src = u.get_entity_or_default(src_id);
    Celestial *dst = u.get_entity_or_default(dst_id);

    std::cout << "Routing from " << src->name << " to " << dst->name << "...\n";

    Route *route = Dijkstra(u, src, dst, param).get_route();

    fprintf(stderr, "Travel time: %u minutes, %02u seconds (%lu steps)\n", ((int) route->cost) / 60, ((int) route->cost) % 60, route->points.size());
    fprintf(stderr, "Route: \n");

    for (auto i = route->points.begin(); i != route->points.end(); i++) {
        std::cout << "    " << movement_type_str[i->type] << ": " << i->entity->name << "\n";
    }

    delete route;
}

void print_additional_information(void) {
    fprintf(stderr, "%12s: %lu bytes\n", "universe", sizeof(Universe));
    fprintf(stderr, "%12s: %lu bytes\n", "trip", sizeof(Parameters));
    fprintf(stderr, "%12s: %lu bytes\n", "route", sizeof(Route));
    fprintf(stderr, "%12s: %lu bytes\n", "entity", sizeof(Celestial));
    fprintf(stderr, "%12s: %lu bytes\n", "system", sizeof(System));
}

int main(int argc, char **argv) {
    struct arguments arguments;

    arguments.batch = NULL;
    arguments.src = 0;
    arguments.dst = 0;
    arguments.gen_type = 0;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if (verbose >= 1) {
        print_additional_information();
    }

    clock_gettime(CLOCK_MONOTONIC, &timer_start);
    Universe universe(arguments.args[0], arguments.args[1]);
    clock_gettime(CLOCK_MONOTONIC, &timer_end);

    fprintf(stderr, "Loaded New Eden (%d systems, %d entities) in %.3f seconds...\n",
        universe.system_count, universe.entity_count, time_diff(&timer_start, &timer_end) / 1E9
    );

    if (arguments.src != 0 && arguments.dst != 0) {
        run_route(universe, arguments.src, arguments.dst, &parameters);
    } else if (arguments.batch != NULL) {
        run_batch_experiment(universe, fopen(arguments.batch, "r"));
    } else if (arguments.gen_type != 0) {
        run_generate_batch(universe, arguments.gen_type, arguments.gen_count);
    } else if (!arguments.quit) {
        run_user_interface(universe);
    }

    return 0;
}
