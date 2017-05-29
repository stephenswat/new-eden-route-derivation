#pragma once
#include <stdbool.h>

struct min_heap_element {
    double priority;
    int value;
};

struct min_heap {
    int length, occupied;
    int *map;
    struct min_heap_element *array;
};

extern void min_heap_init(struct min_heap *, int);
extern void min_heap_destroy(struct min_heap *);
extern void min_heap_insert(struct min_heap *, double, int);
extern int min_heap_extract(struct min_heap *);
extern bool min_heap_decrease(struct min_heap *, double, int);
