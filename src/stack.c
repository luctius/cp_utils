#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <libcp_utils-1.0/stack.h>

#include "lro2_os.h"

#define STACK_BAIL_PRINTF(fmt, ...) {fprintf(stderr, "\033[0;31mSTACK-> Error: [%s:%s] " fmt "\033[0m\n", __FILE__, __func__,  ##__VA_ARGS__);}
#define STACK_BAIL(error, fmt, ...) \
{ \
    STACK_BAIL_PRINTF(fmt, ##__VA_ARGS__) \
    return  error; \
}

struct stack
{
    void *mem_ptr;
    size_t mem_sz;

    lro2_sem_t semaphore;

    int nr_elements;
    int last_element;
    intptr_t stack_start[];
};

int stack_free_unsafe(struct stack *st);
int stack_size_unsafe(struct stack *st);

int stack_minimum_size()
{
    return sizeof(struct stack) +(2 * sizeof(intptr_t) );
}

struct stack *stack_init(void *mem, size_t sz)
{
    struct stack *st = mem;

    if (mem == NULL) STACK_BAIL(NULL, "Parameter 'mem' invalid.");
    if (sz < sizeof(struct stack) + (2 * sizeof(intptr_t) ) ) STACK_BAIL(NULL, "Parameter 'sz' invalid.");
    if (lro2_sem_init(&st->semaphore, 0, 1) != 0) STACK_BAIL(NULL, "Requesting semaphore failed.");

    st->mem_ptr = mem;
    st->mem_sz = sz;

    stack_reset(st);
    return st;
}

int stack_reset(struct stack *st)
{
    if (st == NULL) STACK_BAIL(-EINVAL, "Invalid stack context.");
    lro2_sem_wait(&st->semaphore); /*sem down*/

    st->nr_elements = 0;
    st->last_element = 0;
    st->nr_elements = ( st->mem_sz - sizeof(struct stack) ) / sizeof(intptr_t);

    lro2_sem_post(&st->semaphore); /*sem up*/
    return STACK_SUCCESS;
}

int stack_push(struct stack *st, intptr_t element)
{
    if (st == NULL) STACK_BAIL(-EINVAL, "Invalid stack context.");
    if (stack_free_unsafe(st) == 0) STACK_BAIL(-ENOMEM, "Out of memory.");
    lro2_sem_wait(&st->semaphore); /*sem down*/

    st->last_element++;
    st->stack_start[st->last_element] = element;

    lro2_sem_post(&st->semaphore); /*sem up*/
    return STACK_SUCCESS;
}

intptr_t stack_pop(struct stack *st)
{
    intptr_t retval = -ENODATA;
    if (st == NULL) STACK_BAIL(-EINVAL, "Invalid stack context.");
    lro2_sem_wait(&st->semaphore); /*sem down*/

    if (stack_size_unsafe(st) != 0)
    {
        retval = st->stack_start[st->last_element];
        st->last_element--;
    }

    lro2_sem_post(&st->semaphore); /*sem up*/
    return retval;
}

intptr_t stack_peek(struct stack *st)
{
    intptr_t retval = -ENODATA;
    if (st == NULL) STACK_BAIL(-EINVAL, "Invalid stack context.");
    lro2_sem_wait(&st->semaphore); /*sem down*/

    if (stack_size_unsafe(st) != 0)
    {
        retval = st->stack_start[st->last_element];
    }

    lro2_sem_post(&st->semaphore); /*sem up*/
    return retval;
}

int stack_size_unsafe(struct stack *st)
{
    int last_element = 0;
    if (st == NULL) STACK_BAIL(-EINVAL, "Invalid stack context.");
    
    last_element = st->last_element;

    return last_element;
}

int stack_size(struct stack *st)
{
    int last_element = 0;
    lro2_sem_wait(&st->semaphore); /*sem down*/
    last_element = stack_size_unsafe(st);
    lro2_sem_post(&st->semaphore); /*sem up*/
    return last_element;
}

int stack_free_unsafe(struct stack *st)
{
    int nr_elements = 0;
    if (st == NULL) STACK_BAIL(-EINVAL, "Invalid stack context.");
    nr_elements = st->nr_elements - st->last_element;
    return nr_elements;
}

int stack_free(struct stack *st)
{
    int nr_elements = 0;
    lro2_sem_wait(&st->semaphore); /*sem down*/
    nr_elements = stack_free_unsafe(st);

    lro2_sem_post(&st->semaphore); /*sem up*/
    return nr_elements;
}

