#include <array>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <omp.h>

#include <xmmintrin.h>
#include <pmmintrin.h>

#include "dijkstra.hpp"
#include "min_heap.hpp"
#include "universe.hpp"

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

#define DEBUG_PRINTS 0

enum measurement_point {
    MB_TOTAL_START, MB_TOTAL_END,
    MB_INIT_START, MB_INIT_END,
    MB_SYSTEM_START, MB_SYSTEM_END,
    MB_GATE_START, MB_GATE_END,
    MB_JUMP_START, MB_JUMP_END,
    MB_SELECT_START, MB_SELECT_END,
    MB_MAX
};

#if DEBUG_PRINTS == 1
static struct timespec mbt[MB_MAX];
static long mba[MB_MAX] = {0};
#endif

static void update_timers(enum measurement_point p) {
    #if DEBUG_PRINTS == 1
    clock_gettime(CLOCK_MONOTONIC, &mbt[p]);
    if (p % 2 == 1) {
        mba[p] += time_diff(&mbt[p - 1], &mbt[p]);
    }
    #endif
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

Dijkstra::Dijkstra(Universe &u, Celestial *src, Celestial *dst, Parameters *parameters) : universe(u) {
    this->src = src;
    this->dst = dst;
    this->parameters = parameters;

    this->prev = (int *) malloc(this->universe.entity_count * sizeof(int));
    this->vist = (int *) malloc(this->universe.entity_count * sizeof(int));
    this->cost = (float *) malloc(this->universe.entity_count * sizeof(float));
    this->type = (enum movement_type *) malloc(this->universe.entity_count * sizeof(enum movement_type));

    this->sys_c = (float *) calloc(4 * (this->universe.system_count + VECTOR_WIDTH), sizeof(float));
    this->sys_x = (float *) calloc((this->universe.system_count + VECTOR_WIDTH), sizeof(float));
    this->sys_y = (float *) calloc((this->universe.system_count + VECTOR_WIDTH), sizeof(float));
    this->sys_z = (float *) calloc((this->universe.system_count + VECTOR_WIDTH), sizeof(float));

    this->fatigue = (float *) calloc(this->universe.entity_count, sizeof(float));
    this->reactivation = (float *) calloc(this->universe.entity_count, sizeof(float));

    this->queue = new MinHeap<float, int>(u.entity_count);

    for (int i = 0; i < this->universe.system_count; i++) {
        _mm_store_ps(&sys_c[i * 4], this->universe.systems[i].pos);
        sys_x[i] = this->universe.systems[i].pos[0];
        sys_y[i] = this->universe.systems[i].pos[1];
        sys_z[i] = this->universe.systems[i].pos[2];
    }

    for (int i = 0; i < this->universe.entity_count; i++) {
        if (!this->universe.entities[i].is_relevant() && this->universe.entities[i].seq_id != src->seq_id && this->universe.entities[i].seq_id != dst->seq_id) continue;

        vist[i] = i == src->seq_id ? 1 : 0;
        prev[i] = i == src->seq_id ? -2 : -1;
        type[i] = STRT;
        cost[i] = i == src->seq_id ? 0.0 : INFINITY;

        fatigue[i] = 0.0;
        reactivation[i] = 0.0;

        queue->insert(cost[i], i);
    }
}

Dijkstra::~Dijkstra() {
    free(prev);
    free(cost);
    free(vist);
    free(type);

    free(sys_c);
    free(sys_x);
    free(sys_y);
    free(sys_z);
}

void Dijkstra::solve_w_set(Celestial *ent) {
    System *sys = ent->system;

    for (int i = 0; i < sys->entity_count; i++) {
        if (!sys->entities[i].is_relevant() && sys->entities[i].seq_id != src->seq_id && sys->entities[i].seq_id != dst->seq_id) {
            continue;
        }

        if (ent == &sys->entities[i]) {
            continue;
        }

        update_administration(ent, &sys->entities[i], parameters->align_time + get_time(entity_distance(ent, &sys->entities[i]), parameters->warp_speed), WARP);
    }
}

void Dijkstra::solve_g_set(Celestial *ent) {
    // This should account for fatigue when a bridge is taken
    if (ent->destination && parameters->gate_cost >= 0.0) {
        update_administration(ent, ent->destination, parameters->gate_cost, GATE);
    }
}

void Dijkstra::solve_j_set(Celestial *ent) {
    vector_type x_vec, y_vec, z_vec;
    vector_type x_src_vec, y_src_vec, z_src_vec;

    System *jsys, *sys = ent->system;

    float distance;
    float range, range_sq;

    #if VECTOR_WIDTH == 4
    x_src_vec = _mm_set1_ps(sys->pos[0]);
    y_src_vec = _mm_set1_ps(sys->pos[1]);
    z_src_vec = _mm_set1_ps(sys->pos[2]);
    #elif VECTOR_WIDTH == 8
    x_src_vec = _mm256_set1_ps(sys->pos[0]);
    y_src_vec = _mm256_set1_ps(sys->pos[1]);
    z_src_vec = _mm256_set1_ps(sys->pos[2]);
    #endif

    if (!isnan((range = parameters->jump_range)) || !isnan((range = ent->jump_range))) {
        range_sq = pow(range * LY_TO_M, 2.0);

        #pragma omp for schedule(guided)
        for (int k = 0; k < this->universe.system_count; k += VECTOR_WIDTH) {
            #if VECTOR_WIDTH == 4
            x_vec = _mm_load_ps(sys_x + k);
            y_vec = _mm_load_ps(sys_y + k);
            z_vec = _mm_load_ps(sys_z + k);
            #elif VECTOR_WIDTH == 8
            x_vec = _mm256_loadu_ps(sys_x + k);
            y_vec = _mm256_loadu_ps(sys_y + k);
            z_vec = _mm256_loadu_ps(sys_z + k);
            #endif

            x_vec -= x_src_vec;
            y_vec -= y_src_vec;
            z_vec -= z_src_vec;

            x_vec = (x_vec * x_vec) + (y_vec * y_vec) + (z_vec * z_vec);

            for (int i = 0; i < VECTOR_WIDTH; i++) {
                if (x_vec[i] > range_sq ||
                    k + i >= this->universe.system_count ||
                    sys == (jsys = this->universe.systems + i + k) ||
                    jsys->security >= 0.5) continue;

                distance = sqrt(x_vec[i]) / LY_TO_M;

                for (int j = (jsys == dst->system ? 0 : jsys->gates - jsys->entities); j < jsys->entity_count; j++) {
                    update_administration(ent, &jsys->entities[j], distance * (1 - parameters->jump_range_reduction), JUMP);
                }
            }
        }
    }
}

void Dijkstra::update_administration(Celestial *src, Celestial *dst, float ccost, enum movement_type ctype) {
    float dcost, cur_cost;

    if (ctype == JUMP) {
        if (parameters->fatigue_model == FATIGUE_IGNORE) {
            dcost = 10.0;
        } else if (parameters->fatigue_model == FATIGUE_REACTIVATION_COST) {
            dcost = 60 * (ccost + 1);
        } else if (parameters->fatigue_model == FATIGUE_FATIGUE_COST) {
            dcost = 600 * ccost;
        } else if (parameters->fatigue_model == FATIGUE_REACTIVATION_COUNTDOWN) {
            dcost = reactivation[src->seq_id] + 10.0;
        } else if (parameters->fatigue_model == FATIGUE_FATIGUE_COUNTDOWN) {
            dcost = fatigue[src->seq_id] + 10.0;
        } else {
            dcost = INFINITY;
        }
    } else {
        dcost = ccost;
    }

    cur_cost = cost[src->seq_id] + dcost;

    if (cur_cost <= cost[dst->seq_id] && !vist[dst->seq_id]) {
        #pragma omp critical
        {
            queue->decrease_raw(cur_cost, dst->seq_id);
            prev[dst->seq_id] = src->seq_id;
            cost[dst->seq_id] = cur_cost;
            type[dst->seq_id] = ctype;

            fatigue[dst->seq_id] = std::max(fatigue[src->seq_id] - dcost, 0.f);
            reactivation[dst->seq_id] = std::max(reactivation[src->seq_id] - dcost, 0.f);

            if (ctype == JUMP) {
                fatigue[dst->seq_id] = std::min(60*60*24*7.f, std::max(fatigue[src->seq_id], 600.f) * (ccost + 1));
                reactivation[dst->seq_id] = std::max(fatigue[src->seq_id] / 600, 60 * (ccost + 1));
            }
        }
    }
}

Route *Dijkstra::get_route() {
    Route *route = new Route();

    route->loops = loops;
    route->cost = cost[dst->seq_id];

    for (int c = dst->seq_id; c != -2; c = prev[c]) {
        route->points.push_front((struct waypoint) {&this->universe.entities[c], type[c]});
    }

    return route;
}

void Dijkstra::solve_internal() {
    update_timers(MB_TOTAL_START);
    update_timers(MB_INIT_START);

    int tmp = -1;
    int remaining = this->universe.entity_count;

    Celestial *ent;

    update_timers(MB_INIT_END);

    #pragma omp parallel num_threads(3)
    while (remaining > 0 && tmp != dst->seq_id && (tmp == -1 || !isinf(cost[tmp]))) {
        #pragma omp master
        {
            update_timers(MB_SELECT_START);
            tmp = queue->extract();
            ent = &this->universe.entities[tmp];
            vist[tmp] = 1;
            update_timers(MB_SELECT_END);

            update_timers(MB_SYSTEM_START);
            solve_w_set(ent);
            update_timers(MB_SYSTEM_END);

            update_timers(MB_GATE_START);
            solve_g_set(ent);
            update_timers(MB_GATE_END);

            update_timers(MB_JUMP_START);
        }

        #pragma omp barrier

        solve_j_set(ent);

        #pragma omp master
        {
            update_timers(MB_JUMP_END);
            remaining--;
            loops++;
        }

        #pragma omp barrier
    }

    update_timers(MB_TOTAL_END);
}

Route *Dijkstra::solve() {
    solve_internal();
    return get_route();
}
