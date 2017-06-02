#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "min_heap.hpp"

#define LEFT_CHILD(i) ((i << 1) + 1)
#define RIGHT_CHILD(i) ((i << 1) + 2)
#define PARENT_ENTRY(i) ((i - 1) >> 1)

template <class P, class V> MinHeap<P, V>::MinHeap(int size) {
    this->length = size;
    this->occupied = 0;
    this->map = (int *) calloc(size, sizeof(int));
    this->array = (struct MinHeapElement<P, V> *) calloc(size, sizeof(MinHeapElement<P, V>));

    for (int i = 0; i < size; i++) {
        this->map[i] = -1;
    }
}

template <class P, class V> MinHeap<P, V>::~MinHeap() {
    free(this->map);
    free(this->array);
}

template <class P, class V> void MinHeap<P, V>::swap(V ia, V ib) {
    MinHeapElement<P, V> *a, *b, tmp;

    a = this->array + ia;
    b = this->array + ib;

    tmp = *a;
    *a = *b;
    *b = tmp;

    this->map[b->value] = ib;
    this->map[a->value] = ia;
}

template <class P, class V> void MinHeap<P, V>::update(V index) {
    MinHeapElement<P, V> *elem, *parent;
    int parent_index;

    while (index > 0) {
        parent_index = PARENT_ENTRY(index);

        elem = this->array + index;
        parent = this->array + parent_index;

        if (elem->priority < parent->priority) {
            this->swap(index, parent_index);
            index = parent_index;
        } else {
            break;
        }
    }
}

template <class P, class V> void MinHeap<P, V>::decrease_raw(P priority, V value) {
    int index = this->map[value];

    if (index != -1) {
        #pragma omp critical(min_heap_lock)
        {
            this->array[index].priority = priority;
            this->update(index);
        }
    }
}

template <class P, class V> bool MinHeap<P, V>::decrease(P priority, V value) {
    int index = this->map[value];

    if (index != -1 && priority < this->array[index].priority) {
        #pragma omp critical(min_heap_lock)
        {
            this->array[index].priority = priority;
            this->update(index);
        }

        return true;
    } else {
        return false;
    }
}

template <class P, class V> void MinHeap<P, V>::insert(P priority, V value) {
    MinHeapElement<P, V> *elem;
    int index;

    index = this->occupied++;
    elem = this->array + index;

    elem->priority = priority;
    elem->value = value;

    this->map[value] = index;

    this->update(index);
}

template <class P, class V> V MinHeap<P, V>::extract() {
    MinHeapElement<P, V> *elem, *child_left, *child_right, *child_smallest;
    V rv = this->array[0].value, index, child_index;

    this->map[this->array[0].value] = -1;
    elem = this->array + --this->occupied;
    this->array[0].priority = elem->priority;
    this->array[0].value = elem->value;
    this->map[this->array[0].value] = 0;

    index = 0;

    while (index < this->occupied) {
        elem = this->array + index;

        if (LEFT_CHILD(index) < this->occupied) {
            child_left = this->array + LEFT_CHILD(index);
        } else {
            child_left = NULL;
        }

        if (RIGHT_CHILD(index) < this->occupied) {
            child_right = this->array + RIGHT_CHILD(index);
        } else {
            child_right = NULL;
        }

        if (child_right && child_right->priority < child_left->priority) {
            child_smallest = child_right;
            child_index = RIGHT_CHILD(index);
        } else if (child_right && child_left->priority <= child_right->priority) {
            child_smallest = child_left;
            child_index = LEFT_CHILD(index);
        } else if (child_left) {
            child_smallest = child_left;
            child_index = LEFT_CHILD(index);
        } else {
            break;
        }

        if (elem->priority > child_smallest->priority) {
            this->swap(index, child_index);
            index = child_index;
        } else {
            break;
        }
    }

    return rv;
}

template class MinHeap<float, int>;
