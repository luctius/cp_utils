#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>
#include <sys/resource.h>

#include <libcp_utils-1.0/stack.h>

bool test_simple_push(void *mem_start, ssize_t size)
{
    size_t counter = 0;
    int max = 0;
    struct stack *st;

    st = stack_init(0, size);
    assert(st == NULL);
    st = stack_init(mem_start, 0);
    assert(st == NULL);
    st = stack_init(mem_start, size);
    assert(st != NULL);

    max = stack_free(st);
    assert(stack_size(st) == 0);

    while (stack_free(st) > 0 )
    {
        assert(stack_push(st, counter) == STACK_SUCCESS );
        assert(stack_peek(st) == (intptr_t) counter);
        counter++;
    }

    assert(stack_push(st, 0xDEADBEEF) != STACK_SUCCESS);
    assert(stack_free(st) == 0);
    assert(stack_size(st) == max);

    while (stack_size(st) > 0 )
    {
        assert(stack_pop(st) == (intptr_t) --counter);
    }

    printf("stack_free(st) == %d\n", stack_free(st) );
    assert(stack_free(st) == max);
    assert(stack_size(st) == 0);

    return true;
}

void test_sequence(int seq, ssize_t size)
{
    void *mem_start = malloc(size);
    printf("starting test sequence[%d] with size %ld KB\n", seq, (size / 1024) );

    printf("starting test_simple_push\n");
    if (!test_simple_push(mem_start, size) )
    {
        abort();
    }

    free(mem_start);
}

int main(void)
{
    {
        ssize_t size = 1024;
        test_sequence(0, size);
    }

    {
        ssize_t size = 1024 * 10;
        test_sequence(1, size);
    }

    {
        ssize_t size = 100 * 1024;
        test_sequence(2, size);
    }

    return 0;
}

