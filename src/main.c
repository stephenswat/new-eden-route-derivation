#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "universe.h"

int print_timer = 0;

void load_systems_and_entities(FILE *f, struct universe *u) {
    int res;
    struct entity *ent;

    int id, type_id, group_id, system_id;
    char solar_system_str[128], constellation_id[128], region_id[128];
    double x, y, z;
    char name[128], security[128], buffer[512];

    fgets(buffer, 512, f);

    do {
        res = fscanf(f, "%d,%d,%d,%[^,],%[^,],%[^,],%*[^,],%lf,%lf,%lf,%*[^,],%[^,],%[^,],%*[^,],%*[^,\n]\n",
            &id, &type_id, &group_id, solar_system_str, constellation_id, region_id, &x, &y, &z, name, security
        );

        if (strcmp("None", solar_system_str) == 0) {
            system_id = 0;
        } else {
            system_id = atoi(solar_system_str);
        }

        if (id >= 30000000 && id < 40000000) {
            universe_add_system(u, id, name, x, y, z);
        } else if (id >= 40000000 && id < 50000000) {
            ent = universe_add_entity(u, system_id, id, CELESTIAL, name, x, y, z, NULL);
            ent->group_id = group_id;
        } else if (id >= 50000000 && id < 60000000) {
            ent = universe_add_entity(u, system_id, id, STARGATE, name, x, y, z, NULL);
        } else if (id >= 60000000 && id < 70000000) {
            ent = universe_add_entity(u, system_id, id, STATION, name, x, y, z, NULL);
        }
    } while (res != EOF);
}

void load_stargates(FILE *f, struct universe *u) {
    int src, dst, res;
    char buffer[512];
    struct entity *src_e, *dst_e;

    fgets(buffer, 512, f);

    do {
        res = fscanf(f, "%d,%d\n", &src, &dst);

        src_e = universe_get_typed_entity(u, src, STARGATE);
        dst_e = universe_get_typed_entity(u, dst, STARGATE);

        src_e->destination=dst_e;
        sprintf(src_e->name, "%s - %s gate", src_e->system->name, dst_e->system->name);
    } while (res != EOF);
}

int main(int argc, char **argv) {
    struct universe *universe = universe_init();

    printf("Size: %lu!\n", sizeof(struct universe));

    struct timespec timer_start;
    clock_gettime(CLOCK_MONOTONIC, &timer_start);

    load_systems_and_entities(fopen(argv[1], "r"), universe);
    load_stargates(fopen(argv[2], "r"), universe);

    struct timespec timer_end;
    clock_gettime(CLOCK_MONOTONIC, &timer_end);

    printf("Loaded New Eden (%d systems, %d entities) in %.3f seconds...\n",
        universe->system_count, universe->entity_count,
        ((timer_end.tv_sec * 1000000000 + timer_end.tv_nsec) -
         (timer_start.tv_sec * 1000000000 + timer_start.tv_nsec)) / 1E9
    );

    char *pch, *input, *param;
    int quit = 0;

    struct trip parameters = { .jump_drive = 0, .warp_speed = 3};

    while (!quit) {
        input = readline("NERD> ");
        add_history(input);

        pch = strtok(input, " ");

        if (strcmp(pch, "quit") == 0) {
            quit = 1;
        } else if (strcmp(pch, "route") == 0) {
            int s = atoi(strtok(NULL, " "));
            int d = atoi(strtok(NULL, " "));
            universe_route(universe, s, d);
        } else if (strcmp(pch, "parameters") == 0) {
            if (parameters.jump_drive) {
                printf("Jump drive: %.1f LY\n", parameters.jump_range);
            } else {
                printf("Jump drive: None\n");
            }

            printf("Warp speed: %.1f AU/s\n", parameters.warp_speed);
        } else if (strcmp(pch, "set") == 0) {
            // pch = strtok(NULL, " ");
            //
            // if (strcmp(pch, "jump") == 0) {
            //     pch = strtok(NULL, " ");
            //
            // }
        } else if (strcmp(pch, "warp") == 0) {
            int s = atoi(strtok(NULL, " "));
            int d = atoi(strtok(NULL, " "));

            struct entity *a = universe_get_entity(universe, s);
            struct entity *b = universe_get_entity(universe, d);

            printf("Distance: %f!\n", entity_distance(a, b));
            // double res = universe_warp_time(universe, s, d);
        }
    }

    return 0;
}
