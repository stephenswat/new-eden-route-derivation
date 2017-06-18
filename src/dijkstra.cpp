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

inline float __attribute__((always_inline)) entity_distance(Celestial *a, Celestial *b) {
    if (a->system != b->system) return INFINITY;

    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float dz = a->z - b->z;

    return sqrt(dx * dx + dy * dy + dz * dz);
}

inline float __attribute__((always_inline)) system_distance(System *a, System *b) {
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float dz = a->z - b->z;

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

    this->prev = new int[this->universe.entity_count];
    this->vist = new int[this->universe.entity_count];
    this->cost = new float[this->universe.entity_count];
    this->type = new enum movement_type[this->universe.entity_count];

    this->sys_x = new float[this->universe.system_count + VECTOR_WIDTH];
    this->sys_y = new float[this->universe.system_count + VECTOR_WIDTH];
    this->sys_z = new float[this->universe.system_count + VECTOR_WIDTH];

    this->fatigue = new float[this->universe.entity_count];
    this->reactivation = new float[this->universe.entity_count];

    this->queue = new MinHeap<float, int>(u.entity_count);

    for (int i = 0; i < this->universe.system_count; i++) {
        sys_x[i] = this->universe.systems[i].x;
        sys_y[i] = this->universe.systems[i].y;
        sys_z[i] = this->universe.systems[i].z;
    }

    for (int i = 0; i < this->universe.entity_count; i++) {

        vist[i] = i == src->seq_id ? 1 : 0;
        prev[i] = i == src->seq_id ? -2 : -1;
        type[i] = STRT;
        cost[i] = i == src->seq_id ? 0.0 : INFINITY;

        fatigue[i] = 0.0;
        reactivation[i] = 0.0;

        if (celestial_is_relevant(this->universe.entities[i])) {
            queue->insert(cost[i], i);
        }
    }
}

Dijkstra::~Dijkstra() {
    delete[] prev;
    delete[] cost;
    delete[] vist;
    delete[] type;

    delete[] sys_x;
    delete[] sys_y;
    delete[] sys_z;
}

bool Dijkstra::celestial_is_relevant(Celestial &c) {
    return c.is_relevant() || c.id == src->id || (dst && c.id == dst->id);
}

void Dijkstra::solve_w_set(Celestial *ent) {
    System *sys = ent->system;

    for (int i = 0; i < sys->entity_count; i++) {
        if (!celestial_is_relevant(sys->entities[i]) || ent == &sys->entities[i]) {
            continue;
        }

        update_administration(ent, &sys->entities[i], parameters->align_time + get_time(entity_distance(ent, &sys->entities[i]), parameters->warp_speed), WARP);
    }
}

void Dijkstra::solve_g_set(Celestial *ent) {
    if (ent->destination && !isnan(parameters->gate_cost)) {
        update_administration(ent, ent->destination, parameters->gate_cost, GATE);
    }
}

void Dijkstra::solve_r_set(Celestial *ent) {
    if (ent->bridge && isnan(parameters->jump_range)) {
        update_administration(ent, ent->bridge, system_distance(ent->system, ent->bridge->system) * (1 - parameters->jump_range_reduction), JUMP);
    }
}

void Dijkstra::solve_j_set(Celestial *ent) {
    vector_type x_vec, y_vec, z_vec;
    vector_type x_src_vec, y_src_vec, z_src_vec;

    System *jsys, *sys = ent->system;

    float distance;
    float range, range_sq;

    #if VECTOR_WIDTH == 4
    x_src_vec = _mm_set1_ps(sys->x);
    y_src_vec = _mm_set1_ps(sys->y);
    z_src_vec = _mm_set1_ps(sys->z);
    #elif VECTOR_WIDTH == 8
    x_src_vec = _mm256_set1_ps(sys->x);
    y_src_vec = _mm256_set1_ps(sys->y);
    z_src_vec = _mm256_set1_ps(sys->z);
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

                for (int j = ((dst && jsys != dst->system) ? jsys->gates - jsys->entities : 0); j < jsys->entity_count; j++) {
                    update_administration(ent, &jsys->entities[j], distance * (1 - parameters->jump_range_reduction), JUMP);
                }
            }
        }
    }
}

void Dijkstra::update_administration(Celestial *a, Celestial *b, float ccost, enum movement_type ctype) {
    float dcost, cur_cost;

    if (ctype == JUMP) {
        if (parameters->fatigue_model == FATIGUE_IGNORE) {
            dcost = 10.0;
        } else if (parameters->fatigue_model == FATIGUE_REACTIVATION_COST) {
            dcost = 60 * (ccost + 1);
        } else if (parameters->fatigue_model == FATIGUE_FATIGUE_COST) {
            dcost = 600 * ccost;
        } else if (parameters->fatigue_model == FATIGUE_REACTIVATION_COUNTDOWN) {
            dcost = reactivation[a->seq_id] + 10.0;
        } else if (parameters->fatigue_model == FATIGUE_FATIGUE_COUNTDOWN) {
            dcost = fatigue[a->seq_id] + 10.0;
        } else {
            dcost = INFINITY;
        }
    } else {
        dcost = ccost;
    }

    cur_cost = cost[a->seq_id] + dcost;

    if (cur_cost <= cost[b->seq_id] && !vist[b->seq_id]) {
        #pragma omp critical
        {
            queue->decrease_raw(cur_cost, b->seq_id);
            prev[b->seq_id] = a->seq_id;
            cost[b->seq_id] = cur_cost;
            type[b->seq_id] = ctype;

            fatigue[b->seq_id] = std::max(fatigue[a->seq_id] - dcost, 0.f);
            reactivation[b->seq_id] = std::max(reactivation[a->seq_id] - dcost, 0.f);

            if (ctype == JUMP) {
                fatigue[b->seq_id] = std::min(60*60*24*7.f, std::max(fatigue[a->seq_id], 600.f) * (ccost + 1));
                reactivation[b->seq_id] = std::max(fatigue[a->seq_id] / 600, 60 * (ccost + 1));
            }
        }
    }
}

Route *Dijkstra::get_route() {
    if (!dst) throw 20;
    return get_route(dst);
}

Route *Dijkstra::get_route(Celestial *dst) {
    solve_internal();

    if (!vist[dst->seq_id]) throw 21;

    Route *route = new Route();

    route->loops = loops;
    route->cost = cost[dst->seq_id];

    for (int c = dst->seq_id; c != -2; c = prev[c]) {
        route->points.push_front((struct waypoint) {&this->universe.entities[c], type[c]});
    }

    return route;
}



std::map<Celestial *, float> *Dijkstra::get_all_distances() {
    solve_internal();

    std::map<Celestial *, float> *res = new std::map<Celestial *, float>();

    if (dst != NULL) throw 10;

    for (int i = 0; i < universe.entity_count; i++) {
        if (isfinite(cost[i])) {
            res->emplace(&universe.entities[i], cost[i]);
        }
    }

    return res;
}

void Dijkstra::solve_internal() {
    int tmp = -1;
    Celestial *ent;

    #pragma omp parallel num_threads(1)
    while (!queue->is_empty() && (!dst || !vist[dst->seq_id]) && (tmp == -1 || !isinf(cost[tmp]))) {
        #pragma omp master
        {
            tmp = queue->extract();
            ent = &this->universe.entities[tmp];
            vist[tmp] = 1;

            solve_w_set(ent);
            solve_g_set(ent);
            solve_r_set(ent);

            loops++;
        }

        #pragma omp barrier

        solve_j_set(ent);

        #pragma omp barrier
    }
}
