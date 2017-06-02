#pragma once
#include <stdbool.h>

template <class P, class V> class MinHeapElement {
public:
    P priority;
    V value;
};

template <class P, class V> class MinHeap {
public:
    MinHeap(int);
    ~MinHeap();
    void insert(P, V);
    V extract();
    bool decrease(P, V);
    void decrease_raw(P, V);

private:
    void swap(V, V);
    void update(V);

    int length, occupied;
    int *map;
    MinHeapElement<P, V> *array;
};
