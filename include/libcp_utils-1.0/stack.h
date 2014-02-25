#pragma once
#ifndef STACK_H
#define STACK_H

#include <errno.h>
#include <stdint.h>

#define STACK_SUCCESS (0)

struct stack;
int stack_minimum_size();

struct stack *stack_init(void *mem, size_t sz) __attribute__ ((warn_unused_result));
int stack_push(struct stack *st, intptr_t element);
intptr_t stack_pop(struct stack *st)  __attribute__ ((warn_unused_result));
intptr_t stack_peek(struct stack *st)  __attribute__ ((warn_unused_result));

int stack_size(struct stack *st);
int stack_free(struct stack *st);

int stack_reset(struct stack *st);

#endif /*STACK_H*/
