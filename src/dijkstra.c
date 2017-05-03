#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "universe.h"

#define AU_TO_M 149597870700.0

struct route *dijkstra(struct universe *u, struct entity *src, struct entity *dst, struct trip *parameters) {
    int *prev = calloc(u->entity_count, sizeof(int));
    int *step = calloc(u->entity_count, sizeof(int));
    double *dist = calloc(u->entity_count, sizeof(double));
    char *visited = calloc(u->entity_count, sizeof(char));

    int jump_drive = 0;
    double jump_range = 5.0, warp_speed = 3.0, gate_cost = 12.0;

    if (parameters != NULL) {
        jump_drive = parameters->jump_drive;
        jump_range = parameters->jump_range;
        warp_speed = parameters->warp_speed;
    }

    for (int i = 0; i < u->entity_count; i++) {
        dist[i] = INFINITY;
    }

    inline void update_arrays(int u, int v, double cost) {
        double alt = dist[u] + cost;

        if (alt < dist[v]) {
            dist[v] = alt;
            prev[v] = u;
            step[v] = step[u] + 1;
        }
    }

    int count = u->entity_count;
    int remaining = count;

    int tmp;

    struct system *sys;

    dist[src->seq_id] = 0;
    step[src->seq_id] = 0;

    while (remaining > 0) {
        tmp = -1;

        for (int i = 0; i < count; i++) {
            if (!visited[i]) {
                if (tmp == -1 || dist[i] < dist[tmp]) {
                    tmp = i;
                }
            }
        }

        if (tmp == -1 || tmp == dst->seq_id || dist[tmp] == INFINITY) {
            break;
        }

        visited[tmp] = 1;
        remaining--;

        // System set
        sys = u->entities[tmp]->system;

        for (int i = 0; i < sys->entity_count; i++) {
            if (sys->entities[i].type == CELESTIAL && sys != dst->system) continue;
            if (tmp == sys->entities[i].seq_id) continue;

            update_arrays(tmp, sys->entities[i].seq_id, entity_distance(u->entities[tmp], &sys->entities[i]) / AU_TO_M);
        }

        // Gate set
        if (u->entities[tmp]->type == STARGATE) {
            update_arrays(tmp, u->entities[tmp]->destination->seq_id, 1);
        }

        // Jump set


    }

    struct route *route = malloc(sizeof(struct route));

    route->length = step[dst->seq_id] + 1;
    route->points = malloc(route->length * sizeof(struct entity *));

    for (int c = dst->seq_id, i = route->length - 1; c == src->seq_id || prev[c]; c = prev[c], i--) {
        route->points[i] = u->entities[c];
    }

    printf("Distance: %lf (%d steps)\n", dist[dst->seq_id], step[dst->seq_id]);
    printf("Route: ");

    for (int i = 0; i < route->length; i++) {
        if (i > 0 && route->points[i]->system != route->points[i - 1]->system) continue;
        printf("%s%s", route->points[i]->name, (i == route->length -1 ? "" : ", "));
    }

    printf(".\n");

    return route;
}
