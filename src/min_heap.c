#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "min_heap.h"

#define LEFT_CHILD(i) ((i << 1) + 1)
#define RIGHT_CHILD(i) ((i << 1) + 2)
#define PARENT_ENTRY(i) ((i - 1) >> 1)

void min_heap_init(struct min_heap *heap, int size) {
    heap->length = size;
    heap->occupied = 0;
    heap->map = calloc(size, sizeof(int));
    heap->array = calloc(size, sizeof(struct min_heap_element));

    for (int i = 0; i < size; i++) {
        heap->map[i] = -1;
    }
}

void min_heap_destroy(struct min_heap *heap) {
    free(heap->map);
    free(heap->array);
}

static void min_heap_swap(struct min_heap *heap, int ia, int ib) {
    struct min_heap_element *a, *b, tmp;

    a = heap->array + ia;
    b = heap->array + ib;

    tmp = *a;
    *a = *b;
    *b = tmp;

    heap->map[b->value] = ib;
    heap->map[a->value] = ia;
}

static void min_heap_ify(struct min_heap *heap, int index) {
    struct min_heap_element *elem, *parent;
    int parent_index;

    while (index > 0) {
        parent_index = PARENT_ENTRY(index);

        elem = heap->array + index;
        parent = heap->array + parent_index;

        if (elem->priority < parent->priority) {
            min_heap_swap(heap, index, parent_index);
            index = parent_index;
        } else {
            break;
        }
    }
}

void min_heap_decrease_raw(struct min_heap *heap, double priority, int value) {
    int index = heap->map[value];

    if (index != -1) {
        #pragma omp critical(min_heap_lock)
        {
            heap->array[index].priority = priority;
            min_heap_ify(heap, index);
        }
    }
}

bool min_heap_decrease(struct min_heap *heap, double priority, int value) {
    int index = heap->map[value];

    if (index != -1 && priority < heap->array[index].priority) {
        #pragma omp critical(min_heap_lock)
        {
            heap->array[index].priority = priority;
            min_heap_ify(heap, index);
        }

        return true;
    } else {
        return false;
    }
}

void min_heap_insert(struct min_heap *heap, double priority, int value) {
    struct min_heap_element *elem;
    int index;

    index = heap->occupied++;
    elem = heap->array + index;

    elem->priority = priority;
    elem->value = value;

    heap->map[value] = index;

    min_heap_ify(heap, index);
}

int min_heap_extract(struct min_heap *heap) {
    struct min_heap_element *elem, *child_left, *child_right, *child_smallest;
    int rv = heap->array[0].value, index, child_index;

    heap->map[heap->array[0].value] = -1;
    elem = heap->array + --heap->occupied;
    heap->array[0].priority = elem->priority;
    heap->array[0].value = elem->value;
    heap->map[heap->array[0].value] = 0;

    index = 0;

    while (index < heap->occupied) {
        elem = heap->array + index;

        if (LEFT_CHILD(index) < heap->occupied) {
            child_left = heap->array + LEFT_CHILD(index);
        } else {
            child_left = NULL;
        }

        if (RIGHT_CHILD(index) < heap->occupied) {
            child_right = heap->array + RIGHT_CHILD(index);
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
            min_heap_swap(heap, index, child_index);
            index = child_index;
        } else {
            break;
        }
    }

    return rv;
}

// void main(void) {
//     struct min_heap heap;
//
//     // Expected: 299, 7, 11, 199, 5, 8, 3, 10, 2
//
//     min_heap_init(&heap, 10);
//
//     min_heap_insert(&heap, 5.0, 10);
//     min_heap_insert(&heap, 3.0, 3);
//     min_heap_insert(&heap, 10.0, 2);
//     min_heap_insert(&heap, 200.0, 7);
//     min_heap_insert(&heap, INFINITY, 5);
//     min_heap_insert(&heap, 1.0, 8);
//     min_heap_insert(&heap, 6.0, 11);
//     min_heap_insert(&heap, 0.5, 199);
//     min_heap_insert(&heap, 0.25, 299);
//
//     min_heap_decrease(&heap, 50, 299);
//
//     printf("%d\n", min_heap_extract(&heap));
//
//     min_heap_decrease(&heap, 0.125, 7);
//
//     printf("%d\n", min_heap_extract(&heap));
//
//     min_heap_decrease(&heap, 0.2, 11);
//
//     printf("%d\n", min_heap_extract(&heap));
//     printf("%d\n", min_heap_extract(&heap));
//
//     min_heap_decrease(&heap, 0.01, 5);
//
//     printf("%d\n", min_heap_extract(&heap));
//     printf("%d\n", min_heap_extract(&heap));
//     printf("%d\n", min_heap_extract(&heap));
//     printf("%d\n", min_heap_extract(&heap));
//     printf("%d\n", min_heap_extract(&heap));
// }
