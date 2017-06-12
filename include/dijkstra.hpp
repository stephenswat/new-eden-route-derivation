#pragma once

#include "universe.hpp"
#include "min_heap.hpp"

class Dijkstra {
public:
    Dijkstra(Universe &, Celestial *, Celestial *, Parameters *);
    ~Dijkstra();

    Route *get_route();

private:
    void solve_w_set(Celestial *);
    void solve_g_set(Celestial *);
    void solve_j_set(Celestial *);
    void solve_r_set(Celestial *);
    void solve_internal();

    void update_administration(Celestial *, Celestial *, float, enum movement_type);

    Universe& universe;
    Celestial *src, *dst;
    Parameters *parameters;

    float *sys_c, *sys_x, *sys_y, *sys_z;
    float *cost, *fatigue, *reactivation;
    enum movement_type *type;
    int *prev, *vist;

    int loops = 0;

    MinHeap<float, int> *queue;
};
