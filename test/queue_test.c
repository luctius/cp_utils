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

#include "queue.h"


bool test_simple_push(void *mem_start, ssize_t size)
{
    int counter = 0;
    int max = 0;
    struct queue *q;

    q = queue_init(0, size);
    assert(q == NULL);
    q = queue_init(mem_start, 0);
    assert(q == NULL);
    q = queue_init(mem_start, size);
    assert(q != NULL);

    max = queue_free(q);
    assert(queue_size(q) == 0);

    /*queue is now empty*/

    counter = 0;
    while (queue_free(q) > 0 )
    {
        assert(queue_push_tail(q, counter) == QUEUE_SUCCESS);
        assert(queue_peek_tail(q) == counter);
        counter++;
    }

    /*queue is now full*/

    assert(queue_peek_head(q) == 0);
    assert(queue_push_tail(q, 0xDEADBEEF) != QUEUE_SUCCESS);
    assert(queue_free(q) == 0);
    assert(queue_size(q) == max);
    assert(queue_size(q) == counter);

    counter = 0;
    while (queue_size(q) > 0 )
    {
        assert(queue_peek_head(q) == counter);
        assert(queue_pop_head(q) == counter);
        counter++;
    }

    /*queue is now empty*/

    printf("queue_free(q) == %d\n", queue_free(q) );
    assert(queue_free(q) == max);
    assert(queue_free(q) == counter);
    assert(queue_size(q) == 0);


    counter = 0;
    while (queue_free(q) > 0 )
    {
        assert(queue_push_tail(q, counter) == QUEUE_SUCCESS);
        assert(queue_peek_tail(q) == counter);
        counter++;
    }

    /*queue is now full*/

    assert(queue_peek_head(q) == 0);
    assert(queue_push_tail(q, 0xDEADBEEF) != QUEUE_SUCCESS);
    assert(queue_free(q) == 0);
    assert(queue_size(q) == max);
    assert(queue_size(q) == counter);

    counter = 0;
    while (queue_size(q) >  (max / 2) )
    {
        assert(queue_peek_head(q) == counter);
        assert(queue_pop_head(q) == counter);
        counter++;
    }

    /*queue is now half full*/

    while (queue_free(q) > 0 )
    {
        assert(queue_push_tail(q, counter) == QUEUE_SUCCESS);
        assert(queue_peek_tail(q) == counter);
        counter++;
    }

    /*queue is now full*/

    counter = 0;
    while (queue_size(q) > 0)
    {
        //assert(queue_peek_head(q) == counter);
        //assert(queue_pop_head(q) == counter);
        printf("ctr: %d\n", (int) queue_pop_head(q) );
        counter++;
    }

    /*queue is now empty*/

    printf("queue_free(q) == %d\n", queue_free(q) );
    assert(queue_free(q) == max);
    assert(queue_free(q) == counter);
    assert(queue_size(q) == 0);

    queue_exit(q);
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

int main(int argc, char* argv[])
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

