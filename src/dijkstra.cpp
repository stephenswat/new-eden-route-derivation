#include <array>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <omp.h>

#include <xmmintrin.h>
#include <pmmintrin.h>

#include "main.hpp"
#include "min_heap.hpp"
#include "universe.hpp"

#define IACA_SSC_MARK( MARK_ID )						\
__asm__ __volatile__ (								\
					  "\n\t  movl $"#MARK_ID", %%ebx"	\
					  "\n\t  .byte 0x64, 0x67, 0x90"	\
					  : : : "memory" );

#define IACA_START {IACA_SSC_MARK(111)}
#define IACA_END {IACA_SSC_MARK(222)}

#define AU_TO_M 149597870700.0
#define LY_TO_M 9460730472580800.0

enum measurement_point {
    MB_INIT_START, MB_INIT_END,
    MB_SELECT_START, MB_SELECT_END,
    MB_SYSTEM_SET_START, MB_SYSTEM_SET_END,
    MB_GATE_SET_START, MB_GATE_SET_END,
    MB_JUMP_SET_START, MB_JUMP_SET_END,
    MB_CLEANUP_START, MB_CLEANUP_END,
    MB_MAX
};

static struct timespec mbt[MB_MAX];
static long mba[MB_MAX] = {0};

static void update_timers(enum measurement_point p) {
    if (verbose >= 1) {
        clock_gettime(CLOCK_MONOTONIC, &mbt[p]);
        mba[p] += time_diff(&mbt[p - 1], &mbt[p]);
    }
}

inline float __attribute__((always_inline)) entity_distance(Celestial *a, Celestial *b) {
    if (a->system != b->system) return INFINITY;

    float dx = a->pos[0] - b->pos[0];
    float dy = a->pos[1] - b->pos[1];
    float dz = a->pos[2] - b->pos[2];

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

struct route *dijkstra(Universe &u, Celestial *src, Celestial *dst, struct trip *parameters) {
    int *prev = (int *) malloc(u.entity_count * sizeof(int));
    int *step = (int *) malloc(u.entity_count * sizeof(int));
    int *vist = (int *) malloc(u.entity_count * sizeof(int));
    double *cost = (double *) malloc(u.entity_count * sizeof(double));
    enum movement_type *type = (enum movement_type *) malloc(u.entity_count * sizeof(enum movement_type));

    float *sys_c = (float *) malloc(4 * u.system_count * sizeof(float));

    MinHeap<float, int> queue(u.entity_count);

    double distance, cur_cost;
    double sqjr = pow(parameters->jump_range * LY_TO_M, 2.0);

    for (int i = 0; i < u.system_count; i++) {
        _mm_store_ps(&sys_c[i * 4], u.systems[i].pos);
    }

    for (int i = 0; i < u.entity_count; i++) {
        if (!u.entities[i].destination && u.entities[i].seq_id != src->seq_id && u.entities[i].seq_id != dst->seq_id) continue;

        vist[i] = i == src->seq_id ? 1 : 0;
        prev[i] = -1;
        type[i] = STRT;
        step[i] = i == src->seq_id ? 0 : -1;
        cost[i] = i == src->seq_id ? 0.0 : INFINITY;

        queue.insert(cost[i], i);
    }

    int tmp, v;
    int loops = 0;
    __m128 tmp_coord;

    System *sys, *jsys;
    Celestial *ent;

    tmp = queue.extract();

    int remaining = u.entity_count;

    #pragma omp parallel private(v, cur_cost, tmp_coord, jsys, distance) num_threads(1)
    while (remaining > 0 && tmp != dst->seq_id && !isinf(cost[tmp])) {
        #pragma omp single
        {
            ent = &u.entities[tmp];
            sys = ent->system;
            vist[tmp] = 1;

            // System set
            for (int i = 0; i < sys->entity_count; i++) {
                if (!sys->entities[i].destination && sys->entities[i].seq_id != src->seq_id && sys->entities[i].seq_id != dst->seq_id) {
                    continue;
                }

                if (tmp == sys->entities[i].seq_id) {
                    continue;
                }

                v = sys->entities[i].seq_id;
                cur_cost = cost[tmp] + parameters->align_time + get_time(entity_distance(ent, &sys->entities[i]), parameters->warp_speed);

                if (cur_cost <= cost[v] && !vist[v]) {
                    queue.decrease_raw(cur_cost, v);
                    prev[v] = tmp;
                    cost[v] = cur_cost;
                    step[v] = step[tmp] + 1;
                    type[v] = WARP;
                }
            }

            // Gate set
            if (ent->destination && parameters->gate_cost >= 0.0) {
                v = ent->destination->seq_id;
                cur_cost = cost[tmp] + parameters->gate_cost;

                if (cur_cost <= cost[v] && !vist[v]) {
                    queue.decrease_raw(cur_cost, v);
                    prev[v] = tmp;
                    cost[v] = cur_cost;
                    step[v] = step[tmp] + 1;
                    type[v] = GATE;
                }
            }
        }

        #pragma omp barrier

        // Jump set
        if (!isnan(parameters->jump_range)) {
            #pragma omp for schedule(guided)
            for (int i = 0; i < u.system_count; i++) {
                tmp_coord = _mm_sub_ps(sys->pos, _mm_load_ps(&sys_c[i * 4]));
                tmp_coord = _mm_mul_ps(tmp_coord, tmp_coord);
                tmp_coord = _mm_hadd_ps(tmp_coord, tmp_coord);
                tmp_coord = _mm_hadd_ps(tmp_coord, tmp_coord);

                if (tmp_coord[0] > sqjr || sys->id == u.systems[i].id) continue;

                jsys = u.systems + i;
                distance = sqrt(tmp_coord[0]) / LY_TO_M;

                for (int j = jsys->gates - jsys->entities; j < jsys->entity_count; j++) {
                    if (!jsys->entities[j].destination && ((jsys->entities[j].seq_id != src->seq_id && jsys->entities[j].seq_id != dst->seq_id))) continue;

                    v = jsys->entities[j].seq_id;
                    cur_cost = cost[tmp] + 60 * (distance + 1);

                    if (cur_cost <= cost[v] && !vist[v]) {
                        queue.decrease_raw(cur_cost, v);
                        prev[v] = tmp;
                        cost[v] = cur_cost;
                        step[v] = step[tmp] + 1;
                        type[v] = JUMP;
                    }
                }
            }
        }

        #pragma omp single
        {
            remaining--;
            loops++;
            tmp = queue.extract();
        }

        #pragma omp barrier
    }

    struct route *route = (struct route *) malloc(sizeof(struct route) + (step[dst->seq_id] + 1) * sizeof(struct waypoint));

    if (verbose >= 1) {
        for (int i = 1; i < MB_MAX; i += 2) {
            fprintf(stderr, "%0.3f ", mba[i] / 10E9);
        }
        fprintf(stderr, "\n");
    }

    route->loops = loops;
    route->length = step[dst->seq_id] + 1;
    route->cost = cost[dst->seq_id];

    for (int c = dst->seq_id, i = route->length - 1; i >= 0; c = prev[c], i--) {
        route->points[i].type = type[c];
        route->points[i].entity = &u.entities[c];
    }

    free(prev);
    free(step);
    free(cost);
    free(vist);
    free(type);
    free(sys_c);

    return route;
}
