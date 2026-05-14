#include "codexion.h"
#include "stdio.h"

static void heap_swap(t_waiter *a, t_waiter *b)
{
    t_waiter tmp = *a;
    *a = *b;
    *b = tmp;
}

/* Moves the element at index 'i' upward until the heap property is restored.
   Called only by heap_insert — never call this directly. */
static void heap_push_up(t_waiter *heap, int i)
{
    int parent;

    while (i > 0)
    {
        parent = (i - 1) / 2;  /* formula: parent of node i */
        if (heap[parent].priority <= heap[i].priority)
            break ;             /* parent is already smaller — stop */
		heap_swap(&heap[parent], &heap[i]);
        i = parent;             /* continue checking from the new position */
    }
}

/* Moves the element at index 'i' downward until the heap property is restored.
   Called only by heap_pop — never call this directly. */
static void heap_push_down(t_waiter *heap, int size, int i)
{
    int left;
    int right;
    int smallest;

    while (1)
    {
        left     = 2 * i + 1;  /* left child index  */
        right    = 2 * i + 2;  /* right child index */
        smallest = i;

        /* check if left child exists and is smaller */
        if (left < size && heap[left].priority < heap[smallest].priority)
            smallest = left;

        /* check if right child exists and is even smaller */
        if (right < size && heap[right].priority < heap[smallest].priority)
            smallest = right;

        if (smallest == i)
            break ;             /* already smallest among children — stop */

        /* swap with the smallest child and continue from there */
		heap_swap(&heap[i], &heap[smallest]);
        i	= smallest;
    }
}

/* Adds a new coder to the waiting list with the given priority.
   After adding at the end, push_up restores the heap order.
   The mutex on the dongle must be held when calling this. */
void heap_insert(t_dongle *d, int coder_id, t_ll priority)
{
    int i;

    i = d->heap_size;               /* insert at the end of the array  */
    d->heap[i].coder_id = coder_id;
    d->heap[i].priority = priority;
    d->heap_size++;
    heap_push_up(d->heap, i);       /* bubble up to the correct position */
}

/* Removes the root (the winner — heap[0]) from the waiting list.
   Replaces root with the last element, shrinks the array, then
   push_down restores the heap order.
   The mutex on the dongle must be held when calling this. */
void heap_pop(t_dongle *d)
{
    /* move the last element to the root position */
    d->heap[0] = d->heap[d->heap_size - 1];
    d->heap_size--;
    if (d->heap_size > 0)
        heap_push_down(d->heap, d->heap_size, 0); /* push new root down */
}
