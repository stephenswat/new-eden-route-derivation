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

#if defined(__AVX__)
#include <immintrin.h>
#define VECTOR_WIDTH 8
typedef __m256 vector_type;
#elif defined(__SSE__)
#define VECTOR_WIDTH 4
typedef __m128 vector_type;
#else
#error "Here's a nickel kid, buy yourself a real computer."
#endif

enum measurement_point {
    MB_TOTAL_START, MB_TOTAL_END,
    MB_INIT_START, MB_INIT_END,
    MB_SYSTEM_START, MB_SYSTEM_END,
    MB_GATE_START, MB_GATE_END,
    MB_JUMP_START, MB_JUMP_END,
    MB_SELECT_START, MB_SELECT_END,
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

Route *dijkstra(Universe &u, Celestial *src, Celestial *dst, struct trip *parameters) {
    update_timers(MB_TOTAL_START);
    update_timers(MB_INIT_START);

    int *prev = (int *) malloc(u.entity_count * sizeof(int));
    int *vist = (int *) malloc(u.entity_count * sizeof(int));
    float *cost = (float *) malloc(u.entity_count * sizeof(float));
    enum movement_type *type = (enum movement_type *) malloc(u.entity_count * sizeof(enum movement_type));

    float *sys_c = (float *) calloc(4 * (u.system_count + VECTOR_WIDTH), sizeof(float));
    float *sys_x = (float *) calloc((u.system_count + VECTOR_WIDTH), sizeof(float));
    float *sys_y = (float *) calloc((u.system_count + VECTOR_WIDTH), sizeof(float));
    float *sys_z = (float *) calloc((u.system_count + VECTOR_WIDTH), sizeof(float));

    MinHeap<float, int> queue(u.entity_count);

    float distance, cur_cost;

    for (int i = 0; i < u.system_count; i++) {
        _mm_store_ps(&sys_c[i * 4], u.systems[i].pos);
        sys_x[i] = u.systems[i].pos[0];
        sys_y[i] = u.systems[i].pos[1];
        sys_z[i] = u.systems[i].pos[2];
    }

    for (int i = 0; i < u.entity_count; i++) {
        if (!u.entities[i].destination && u.entities[i].seq_id != src->seq_id && u.entities[i].seq_id != dst->seq_id) continue;

        vist[i] = i == src->seq_id ? 1 : 0;
        prev[i] = i == src->seq_id ? -2 : -1;
        type[i] = STRT;
        cost[i] = i == src->seq_id ? 0.0 : INFINITY;

        queue.insert(cost[i], i);
    }

    int tmp, v;
    int loops = 0;
    float sqjr = pow(parameters->jump_range * LY_TO_M, 2.0);

    vector_type x_vec, y_vec, z_vec;
    vector_type x_src_vec, y_src_vec, z_src_vec;

    System *sys, *jsys;
    Celestial *ent;

    tmp = queue.extract();

    int remaining = u.entity_count;

    update_timers(MB_INIT_END);

    #pragma omp parallel private(v, cur_cost, jsys, distance, x_vec, y_vec, z_vec) num_threads(3)
    while (remaining > 0 && tmp != dst->seq_id && !isinf(cost[tmp])) {
        #pragma omp master
        {
            update_timers(MB_SYSTEM_START);
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
                    type[v] = WARP;
                }
            }
            update_timers(MB_SYSTEM_END);

            update_timers(MB_GATE_START);
            // Gate set
            if (ent->destination && parameters->gate_cost >= 0.0) {
                v = ent->destination->seq_id;
                cur_cost = cost[tmp] + parameters->gate_cost;

                if (cur_cost <= cost[v] && !vist[v]) {
                    queue.decrease_raw(cur_cost, v);
                    prev[v] = tmp;
                    cost[v] = cur_cost;
                    type[v] = GATE;
                }
            }
            update_timers(MB_GATE_END);

            update_timers(MB_JUMP_START);

            #if VECTOR_WIDTH == 4
            x_src_vec = _mm_set1_ps(sys->pos[0]);
            y_src_vec = _mm_set1_ps(sys->pos[1]);
            z_src_vec = _mm_set1_ps(sys->pos[2]);
            #elif VECTOR_WIDTH == 8
            x_src_vec = _mm256_set1_ps(sys->pos[0]);
            y_src_vec = _mm256_set1_ps(sys->pos[1]);
            z_src_vec = _mm256_set1_ps(sys->pos[2]);
            #endif
        }

        // Jump set
        if (!isnan(parameters->jump_range)) {
            #pragma omp for schedule(guided)
            for (int k = 0; k < u.system_count; k += VECTOR_WIDTH) {
                #if VECTOR_WIDTH == 4
                x_vec = _mm_load_ps(sys_x + k);
                y_vec = _mm_load_ps(sys_y + k);
                z_vec = _mm_load_ps(sys_z + k);
                #elif VECTOR_WIDTH == 8
                x_vec = _mm256_load_ps(sys_x + k);
                y_vec = _mm256_load_ps(sys_y + k);
                z_vec = _mm256_load_ps(sys_z + k);
                #endif

                x_vec -= x_src_vec;
                y_vec -= y_src_vec;
                z_vec -= z_src_vec;

                x_vec = (x_vec * x_vec) + (y_vec * y_vec) + (z_vec * z_vec);

                for (int i = 0; i < VECTOR_WIDTH; i++) {
                    if (x_vec[i] > sqjr || sys->id == u.systems[i].id) continue;

                    jsys = u.systems + k + i;
                    distance = sqrt(x_vec[i]) / LY_TO_M;

                    for (int j = jsys->gates - jsys->entities; j < jsys->entity_count; j++) {
                        if (!jsys->entities[j].destination && ((jsys->entities[j].seq_id != src->seq_id && jsys->entities[j].seq_id != dst->seq_id))) continue;

                        v = jsys->entities[j].seq_id;
                        cur_cost = cost[tmp] + 60 * (distance + 1);

                        if (cur_cost <= cost[v] && !vist[v]) {
                            queue.decrease_raw(cur_cost, v);
                            prev[v] = tmp;
                            cost[v] = cur_cost;
                            type[v] = JUMP;
                        }
                    }
                }
            }
        }

        #pragma omp master
        {
            update_timers(MB_JUMP_END);

            update_timers(MB_SELECT_START);
            remaining--;
            loops++;
            tmp = queue.extract();
            update_timers(MB_SELECT_END);
        }

        #pragma omp barrier
    }

    update_timers(MB_TOTAL_END);

    Route *route = new Route();

    if (verbose >= 1) {
        unsigned long mba_total = 0;

        for (int i = 3; i < MB_MAX; i += 2) {
            mba_total += mba[i];
        }

        for (int i = 1; i < MB_MAX; i += 2) {
            fprintf(stderr, "%0.3f ms (%.1f%%), ", mba[i] / 1000000.0, (mba[i] / (double) mba_total) * 100);
        }

        fprintf(stderr, "\n");
    }

    route->loops = loops;
    route->cost = cost[dst->seq_id];

    for (int c = dst->seq_id; c != -2; c = prev[c]) {
        route->points.push_front((struct waypoint) {&u.entities[c], type[c]});
    }

    free(prev);
    free(cost);
    free(vist);
    free(type);
    free(sys_c);

    return route;
}
