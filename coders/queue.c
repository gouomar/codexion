/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   queue.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gomar <gomar@student.1337.ma>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/15 14:32:13 by gomar             #+#    #+#             */
/*   Updated: 2026/05/15 14:32:15 by gomar            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	heap_swap(t_waiter *a, t_waiter *b)
{
	t_waiter	tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}

static void	heap_push_up(t_waiter *heap, int i)
{
	int	parent;

	while (i > 0)
	{
		parent = (i - 1) / 2;
		if (heap[parent].priority <= heap[i].priority)
			break ;
		heap_swap(&heap[parent], &heap[i]);
		i = parent;
	}
}

static void	heap_push_down(t_waiter *heap, int size, int i)
{
	int	left;
	int	right;
	int	smallest;

	while (1)
	{
		left = 2 * i + 1;
		right = 2 * i + 2;
		smallest = i;
		if (left < size && heap[left].priority < heap[smallest].priority)
			smallest = left;
		if (right < size && heap[right].priority < heap[smallest].priority)
			smallest = right;
		if (smallest == i)
			break ;
		heap_swap(&heap[i], &heap[smallest]);
		i = smallest;
	}
}

void	heap_insert(t_dongle *d, int coder_id, t_ll priority)
{
	int	i;

	i = d->heap_size;
	d->heap[i].coder_id = coder_id;
	d->heap[i].priority = priority;
	d->heap_size++;
	heap_push_up(d->heap, i);
}

void	heap_pop(t_dongle *d)
{
	d->heap[0] = d->heap[d->heap_size - 1];
	d->heap_size--;
	if (d->heap_size > 0)
		heap_push_down(d->heap, d->heap_size, 0);
}
