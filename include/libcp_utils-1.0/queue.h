#pragma once
#ifndef QUEUE_H
#define QUEUE_H

#include <errno.h>
#include <stdint.h>

#define QUEUE_SUCCESS (0)

struct queue;
int queue_minimum_size(void);
int queue_element_size(void);

struct queue *queue_init_simple(int nr_elements) __attribute__ ((warn_unused_result));
struct queue *queue_init(void *mem, size_t sz) __attribute__ ((warn_unused_result));

int queue_exit(struct queue *q);
int queue_push_tail(struct queue *q, intptr_t element);
intptr_t queue_pop_head(struct queue *q) __attribute__ ((warn_unused_result));
intptr_t queue_peek_head(struct queue *q) __attribute__ ((warn_unused_result));
intptr_t queue_peek_nr(struct queue *q, int nr) __attribute__ ((warn_unused_result));
intptr_t queue_peek_tail(struct queue *q) __attribute__ ((warn_unused_result));

int queue_size(struct queue *q);
int queue_free(struct queue *q);

#endif /*QUEUE_H*/
