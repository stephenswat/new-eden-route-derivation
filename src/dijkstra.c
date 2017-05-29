#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "main.h"
#include "universe.h"

#define FINE_BENCHMARKING 1

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define AU_TO_M 149597870700.0
#define LY_TO_M 9460730472580800.0

static struct timespec fgt1, fgt2, fgt3, fgt4, fgt5;
static long fga1, fga2, fga3, fga4;

double entity_distance(struct entity *a, struct entity *b) {
    if (a->system != b->system) return INFINITY;

    double dx = a->x - b->x;
    double dy = a->y - b->y;
    double dz = a->z - b->z;

    return sqrt(dx * dx + dy * dy + dz * dz);
}

double system_distance(struct system *a, struct system *b) {
    double dx = a->x - b->x;
    double dy = a->y - b->y;
    double dz = a->z - b->z;

    return sqrt(dx * dx + dy * dy + dz * dz);
}

double get_time(double distance, double v_wrp) {
    double k_accel = v_wrp;
    double k_decel = (v_wrp / 3) < 2 ? (v_wrp / 3) : 2;

    double v_max_wrp = v_wrp * AU_TO_M;

    double d_accel = AU_TO_M;
    double d_decel = v_max_wrp / k_decel;

    double d_min = d_accel + d_decel;

    double cruise_time = 0;

    if (d_min > distance) {
        v_max_wrp = distance * k_accel * k_decel / (k_accel + k_decel);
    } else {
        cruise_time = (distance - d_min) / v_max_wrp;
    }

    double t_accel = log(v_max_wrp / k_accel) / k_accel;
    double t_decel = log(v_max_wrp / 100) / k_decel;

    return cruise_time + t_accel + t_decel;
}

inline void update_arrays(double *dist, int *prev, int *step, int u, int v, double cost) {
    double alt = dist[u] + cost;

    if (alt < dist[v]) {
        dist[v] = alt;
        prev[v] = u;
        step[v] = step[u] + 1;
    }
}

struct route *dijkstra(struct universe *u, struct entity *src, struct entity *dst, struct trip *parameters) {
    static int prev[LIMIT_ENTITIES];
    static int step[LIMIT_ENTITIES];
    static double dist[LIMIT_ENTITIES];
    static char visited[LIMIT_ENTITIES];

    static double gate_cost = 10.0;

    double distance;

    double jump_range = parameters->jump_range;
    double warp_speed = parameters->warp_speed;
    double align_time = parameters->align_time;

    for (int i = 0; i < u->entity_count; i++) {
        prev[i] = -1;
        dist[i] = INFINITY;
        step[i] = 0;
        visited[i] = 0;
    }

    int count = u->entity_count;

    int tmp;
    int loops = 0;

    struct system *sys;
    struct entity *ent;

    dist[src->seq_id] = 0;

    for (int remaining = count; remaining > 0; remaining--, loops++) {
        #if FINE_BENCHMARKING == 1
        clock_gettime(CLOCK_MONOTONIC, &fgt1);
        #endif

        tmp = 0;

        for (int i = 0; i < count; i++) {
            if (!visited[i] && (dist[i] < dist[tmp] || visited[tmp])) {
                tmp = i;
            }
        }

        if (visited[tmp] || tmp == dst->seq_id || dist[tmp] == INFINITY) {
            break;
        }

        visited[tmp] = 1;

        #if FINE_BENCHMARKING == 1
        clock_gettime(CLOCK_MONOTONIC, &fgt2);
        fga1 += time_diff(&fgt1, &fgt2);
        #endif

        // System set
        ent = u->entities[tmp];
        sys = ent->system;

        for (int i = 0; i < sys->entity_count; i++) {
            // if (!sys->entities[i].destination && sys != dst->system) continue;
            if (tmp == sys->entities[i].seq_id) continue;

            update_arrays(dist, prev, step, tmp, sys->entities[i].seq_id, align_time + get_time(entity_distance(ent, &sys->entities[i]), warp_speed));
        }

        #if FINE_BENCHMARKING == 1
        clock_gettime(CLOCK_MONOTONIC, &fgt3);
        fga2 += time_diff(&fgt2, &fgt3);
        #endif

        // Gate set
        if (ent->destination) {
            update_arrays(dist, prev, step, tmp, ent->destination->seq_id, gate_cost);
        }

        #if FINE_BENCHMARKING == 1
        clock_gettime(CLOCK_MONOTONIC, &fgt4);
        fga3 += time_diff(&fgt3, &fgt4);
        #endif

        // Jump set
        if (!isnan(jump_range)) {
            for (int i = 0; i < u->system_count; i++) {
                if (sys->id == u->systems[i].id) continue;

                distance = system_distance(sys, &u->systems[i]) / LY_TO_M;

                if (distance < jump_range) {
                    for (int j = 0; j < u->systems[i].entity_count; j++) {
                        update_arrays(dist, prev, step, tmp, u->systems[i].entities[j].seq_id, 60 * (distance + 1));
                    }
                }
            }
        }

        #if FINE_BENCHMARKING == 1
        clock_gettime(CLOCK_MONOTONIC, &fgt5);
        fga4 += time_diff(&fgt4, &fgt5);
        #endif
    }

    struct route *route = malloc(sizeof(struct route));

    #if FINE_BENCHMARKING == 1
    fprintf(stderr, "%lu %lu %lu %lu\n", fga1, fga2, fga3, fga4);
    #endif

    route->loops = loops;
    route->length = step[dst->seq_id] + 1;
    route->cost = dist[dst->seq_id];
    route->points = malloc(route->length * sizeof(struct entity *));

    for (int c = dst->seq_id, i = route->length - 1; c >= 0; c = prev[c], i--) {
        route->points[i] = u->entities[c];
    }

    return route;
}
