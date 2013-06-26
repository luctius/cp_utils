#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>
#include <stdint.h>

#include "cmalloc.h"

#define NVALGRIND
#ifndef NVALGRIND
#include <valgrind/memcheck.h>
#endif


float cmalloc_time_total = 0;
float malloc_time_total = 0;

float cmalloc_alloc_totaltime = 0;
float malloc_alloc_totaltime = 0;
size_t cmalloc_alloc_ctr = 0;
size_t malloc_alloc_ctr = 0;
float cmalloc_free_totaltime = 0;
float malloc_free_totaltime = 0;
size_t cmalloc_free_ctr = 0;
size_t malloc_free_ctr = 0;

bool simple_tests(void *ptr, size_t size, size_t chunksz)
{
    int ctr = 0;
    int max;
    struct cmalloc *cm;

    printf("--> Ugly Part\n");
    /* invalid inits of pool */
    assert(cmalloc_init(NULL, size, chunksz) == NULL);
    assert(cmalloc_init(ptr, 0, chunksz) == NULL);
    assert(cmalloc_init(ptr, size, 0) == NULL);
    assert(cmalloc_exit(cmalloc_init(ptr, size, 0) ) != 0);

    /* init pool */
    cm = cmalloc_init(ptr, size, chunksz);
    assert(cm == ptr);

    cm = cmalloc_init(ptr, size, chunksz);
    assert(cm == ptr);
    void *test_ptr = cmalloc_alloc(cm, 1000);
    assert(cmalloc_exit(cm) != 0);

    printf("--> Testing Part\n");
    cm = cmalloc_init(ptr, size, chunksz);
    assert(cm == ptr);

    /* pre check sanity */
    max = cmalloc_unallocated(cm);
    assert(cmalloc_allocated(cm) == 0);
    assert(cmalloc_unallocated(cm) == max);

    cmalloc_free(cm, NULL); /*Valgrind does not like frreeing NULL*/
    assert(cmalloc_alloc(cm, size) == NULL);

    /* A simple allocate all, free all test */
    {
        void *array[cmalloc_unallocated(cm) / chunksz];
        ctr = 0;

        /* Allocate array */
        while (cmalloc_unallocated(cm) > 0)
        {
            array[ctr] = cmalloc_alloc(cm, chunksz);
            assert(array[ctr] != NULL);
            assert(array[ctr] > ptr);
            memset(array[ctr], 0xFF, chunksz);
            ctr++;
        }

        /* mid check sanity */
        assert(cmalloc_alloc(cm, chunksz) == NULL);
        assert(cmalloc_allocated(cm) >= (max - chunksz) );
        assert(cmalloc_unallocated(cm) <= chunksz);

        /* free array from top */
        ctr-=1;
        while (cmalloc_allocated(cm) > 0)
        {
            cmalloc_free(cm, array[ctr]);
            ctr--;
        }

        cmalloc_free(cm, array[0]); /* should not case an assert */
        cmalloc_free(cm, NULL);

        /* post check sanity */
        assert(cmalloc_allocated(cm) == 0);
        assert(cmalloc_unallocated(cm) == max);
    }

    /* A simple allocate all, free all test */
    {
        void *array[cmalloc_unallocated(cm) / chunksz];
        ctr = 0;
        int membsz = sizeof(void*);
        int nmemb = chunksz / membsz;

        memset(array, 0, cmalloc_unallocated(cm) / chunksz );

        /* Allocate array */
        while (cmalloc_unallocated(cm) >= chunksz)
        {
            array[ctr] = cmalloc_calloc(cm, nmemb, membsz);
            assert(array[ctr] != NULL);
            assert(array[ctr] > ptr);

            ctr++;
        }

        /* mid check sanity */
        assert(cmalloc_alloc(cm, chunksz) == NULL);
        assert(cmalloc_allocated(cm) >= (max - chunksz) );
        assert(cmalloc_unallocated(cm) <= chunksz);

        /* free array from top */
        ctr-=1;
        while (ctr > 0)
        {
            assert(array[ctr] != NULL);
            assert(array[ctr] > ptr);
            cmalloc_free(cm, array[ctr]);
            ctr--;
        }

        cmalloc_free(cm, array[0]); /* should not case an assert */
        cmalloc_free(cm, NULL);

        /* post check sanity */
        assert(cmalloc_allocated(cm) == 0);
        assert(cmalloc_unallocated(cm) == max);
    }

    /* some manual tests */
    {
        void *array[10];

        /* pre check sanity */
        assert(cmalloc_allocated(cm) == 0);
        assert(cmalloc_unallocated(cm) == max);

        array[0] = cmalloc_alloc(cm, 1);
        array[1] = cmalloc_alloc(cm, 1);
        array[2] = cmalloc_alloc(cm, 1);
        cmalloc_free(cm, array[1]);
        array[3] = cmalloc_alloc(cm, 1);
        cmalloc_free(cm, array[0]);
        cmalloc_free(cm, array[2]);
        array[0] = cmalloc_alloc(cm, chunksz *20);
        cmalloc_free(cm, array[3]);
        cmalloc_free(cm, array[0]);

        /* post check sanity */
        assert(cmalloc_allocated(cm) == 0);
        assert(cmalloc_unallocated(cm) == max);
    }

    assert(cmalloc_exit(cm) == 0);
    return true;
}

bool extended_tests(void *ptr, size_t size, size_t chunksz)
{
    int ctr = 0;
    int max;
    struct cmalloc *cm;

    /* init pool */
    cm = cmalloc_init(ptr, size, chunksz);
    assert(cm == ptr);

    /* Pre Check sanity */
    max = cmalloc_unallocated(cm);
    assert(cmalloc_allocated(cm) == 0);
    assert(cmalloc_unallocated(cm) == max);

    {
        void *array[cmalloc_unallocated(cm) / chunksz];

        /* allocate array */
        while (cmalloc_unallocated(cm) > (chunksz) )
        {
            int rand = random() % chunksz;
            size_t alloc_sz = rand * chunksz;

            array[ctr] = NULL;
            if (alloc_sz > cmalloc_unallocated(cm) ) alloc_sz = chunksz;
            array[ctr] = cmalloc_alloc(cm, alloc_sz);
            assert(array[ctr] != NULL);
            assert(array[ctr] > ptr);
            memset(array[ctr], 0xFF, alloc_sz);
            ctr++;
        }

        /* mid check sanity */
        assert(cmalloc_alloc(cm, chunksz) == NULL);
        assert(cmalloc_allocated(cm) >= (max - chunksz) );
        assert(cmalloc_unallocated(cm) <= chunksz);

        /* free array from bottom */
        ctr=0;
        while (cmalloc_allocated(cm) > 0)
        {
            cmalloc_free(cm, array[ctr]);
            ctr++;
        }
    }

    /* post check sanity */
    assert(cmalloc_allocated(cm) == 0);
    assert(cmalloc_unallocated(cm) == max);

    assert(cmalloc_exit(cm) == 0);
    return true;
}

bool rand_alloc_free_test(void *ptr, size_t size, size_t chunksz)
{
    int ctr = 0;
    int max;
    struct cmalloc *cm;

    /* init pool */
    cm = cmalloc_init(ptr, size, chunksz);
    assert(cm == ptr);

    /* pre check sanity */
    max = cmalloc_unallocated(cm);
    assert(cmalloc_allocated(cm) == 0);
    assert(cmalloc_unallocated(cm) == max);

    {
        ssize_t array_max = 1000000;
        ssize_t loop_counter = 10000000;
        void *array[array_max];

        /* 
            allocate and deallocate randomly for a fixed amount of times.
            when out of counts, free remaining allocated slots.
         */
        for (; loop_counter >= 0; loop_counter--)
        {
            int rand = random() % 10;
            if (rand > 4 && (ctr < (array_max) ) )
            {
                long int r = random();
                size_t alloc_sz = (random() % (chunksz * 100) );

                if (alloc_sz >= (cmalloc_unallocated(cm) ) )
                {
                    array[ctr] = cmalloc_alloc(cm, alloc_sz);
                    assert(array[ctr] == NULL);
                }
                else if ( (alloc_sz == 0) && (cmalloc_unallocated(cm) != 0) )
                {
                    array[ctr] = cmalloc_alloc(cm, alloc_sz);
                    assert(array[ctr] != NULL);
                }
                else if (cmalloc_unallocated(cm) == 0)
                {
                    array[ctr] = cmalloc_alloc(cm, alloc_sz);
                    assert(array[ctr] == NULL);
                }
                else
                {
                    //printf("\t--> alloc_sz %ld vs unalloc_sz = %ld\n", alloc_sz, cmalloc_unallocated(cm) );
                    if (alloc_sz <= (cmalloc_unallocated(cm) - chunksz) )
                    {
                        array[ctr] = cmalloc_alloc(cm, alloc_sz);
                        assert(array[ctr] != NULL);
                        memset(array[ctr], 0xFF, alloc_sz);
                    }
                    else
                    {
                        array[ctr] = cmalloc_alloc(cm, alloc_sz);
                    }
                }

                ctr++;
            }
            else if (ctr > 0)
            {
                ctr--;
                cmalloc_free(cm, array[ctr]);
            }

            /* mid sanity check */
            assert(cmalloc_allocated(cm) + cmalloc_unallocated(cm) == max);
        }

        /* free remaining slots */
        while (ctr > 0)
        {
            ctr--;
            cmalloc_free(cm, array[ctr]);
        }
    }

    /* post sanity check */
    assert(cmalloc_allocated(cm) == 0);
    assert(cmalloc_unallocated(cm) == max);

    assert(cmalloc_exit(cm) == 0);
    return true;
}

bool simple_speed_tests(void *ptr, size_t size, size_t chunksz)
{
    int ctr = 0;
    int ctr_max = (size / chunksz) - 2;
    int max;
    struct cmalloc *cm;
    float cmalloc_time;
    float malloc_time;

    /* init pool */
    cm = cmalloc_init(ptr, size, chunksz);
    assert(cm == ptr);

    /* Pre Check sanity */
    max = cmalloc_unallocated(cm);

    {
        void *array[ctr_max +1];
        memset(array, 0, (ctr_max +1) * sizeof(void*) );

        clock_t t = clock();

        /* allocate array */
        for (ctr = 0; ctr < ctr_max; ctr++)
        {
            clock_t temp = clock();
            array[ctr] = cmalloc_alloc(cm, chunksz);
            cmalloc_alloc_totaltime += (double)(clock() - temp) / CLOCKS_PER_SEC;
            cmalloc_alloc_ctr++;

            ctr++;
        }

        /* free array from bottom */
        for (; ctr >= 0; ctr--)
        {
            clock_t temp = clock();
            cmalloc_free(cm, array[ctr]);
            cmalloc_free_totaltime += (double)(clock() - temp) / CLOCKS_PER_SEC;
            cmalloc_free_ctr++;
        }
        cmalloc_time = (double)(clock() - t) / CLOCKS_PER_SEC;
    }
    assert(cmalloc_allocated(cm) == 0);
    assert(cmalloc_unallocated(cm) == max);

    {
        void *array[ctr_max +1];
        memset(array, 0, (ctr_max +1) * sizeof(void*) );

        clock_t t = clock();

        /* allocate array */
        for (ctr = 0; ctr < ctr_max; ctr++)
        {
            clock_t temp = clock();
            array[ctr] = malloc(chunksz);
            malloc_alloc_totaltime += (double)(clock() - temp) / CLOCKS_PER_SEC;
            cmalloc_alloc_ctr++;

            ctr++;
        }

        /* free array from bottom */
        for (ctr = ctr_max; ctr > 0; ctr--)
        {
            clock_t temp = clock();
            free(array[ctr]);
            malloc_free_totaltime += (double)(clock() - temp) / CLOCKS_PER_SEC;
            cmalloc_free_ctr++;
        }
        malloc_time = (double)(clock() - t) / CLOCKS_PER_SEC;
    }

    cmalloc_time_total += cmalloc_time;
    malloc_time_total += malloc_time;
    printf("\t--> cmalloc_time %lf vs malloc time = %lf\n", cmalloc_time, malloc_time);

    assert(cmalloc_exit(cm) == 0);
    return true;
}

/*
   This is the same as the random test above,
   except it is done twice, once with cmalloc and once with the system malloc,
   and it does not include error checking. We allready did that above.

   Thus this is purely a speed test.
*/
bool rand_alloc_free_speed_test(void *ptr, size_t size, size_t chunksz)
{
    int ctr = 0;
    struct cmalloc *cm;
    float cmalloc_time;
    float malloc_time;
    ssize_t array_max = 1000000;
    ssize_t loop_counter_max = 10000;

    /* init pool */
    cm = cmalloc_init(ptr, size, chunksz);
    assert(cm == ptr);

    {
        ssize_t loop_counter = loop_counter_max;
        void *array[array_max];

        clock_t t = clock();

        /* 
            allocate and deallocate randomly for a fixed amount of times.
            when out of counts, free remaining allocated slots.
         */
        for (; loop_counter >= 0; loop_counter--)
        {
            int rand = random() % 10;
            if (rand > 4 && (ctr < (array_max) ) )
            {
                long int r = random();
                size_t alloc_sz = random() % (chunksz * chunksz);

                clock_t temp = clock();
                array[ctr] = cmalloc_alloc(cm, alloc_sz);
                cmalloc_alloc_totaltime += (double)(clock() - temp) / CLOCKS_PER_SEC;
                cmalloc_alloc_ctr++;

                ctr++;
            }
            else if (ctr > 0)
            {
                ctr--;

                clock_t temp = clock();
                cmalloc_free(cm, array[ctr]);
                cmalloc_free_totaltime += (double)(clock() - temp) / CLOCKS_PER_SEC;
                cmalloc_free_ctr++;

                array[ctr] = 0x0;
            }
        }

        /* free remaining slots */
        for (; ctr >= 0; ctr--)
        {
            clock_t temp = clock();
            cmalloc_free(cm, array[ctr]);
            cmalloc_free_totaltime += (double)(clock() - temp) / CLOCKS_PER_SEC;
            cmalloc_free_ctr++;
        }

        cmalloc_time = (double)(clock() - t) / CLOCKS_PER_SEC;
    }
    assert(cmalloc_allocated(cm) == 0);

    {
        ssize_t loop_counter = loop_counter_max;
        void *array[array_max];
        size_t size_max = size;

        memset(array, 0, array_max * sizeof(void*) );

        ctr = 0;
        clock_t t = clock();
        /* 
            allocate and deallocate randomly for a fixed amount of times.
            when out of counts, free remaining allocated slots.
         */
        for (; loop_counter >= 0; loop_counter--)
        {
            int rand = random() % 10;
            if (rand > 4 && (ctr < (array_max) ) )
            {
                long int r = random();
                size_t alloc_sz = random() % (chunksz * chunksz);

                if (alloc_sz >= size_max)
                {
                    clock_t temp = clock();
                    array[ctr] = malloc(alloc_sz);
                    malloc_alloc_totaltime += (double)(clock() - temp) / CLOCKS_PER_SEC;
                    malloc_alloc_ctr++;

                    size_max -= alloc_sz;
                    ctr++;
                }
                else 
                {
                    clock_t temp = clock();
                    array[ctr] = malloc(alloc_sz);
                    malloc_alloc_totaltime += (double)(clock() - temp) / CLOCKS_PER_SEC;
                    malloc_alloc_ctr++;

                    temp = clock();
                    free(array[ctr]);
                    malloc_free_totaltime += (double)(clock() - temp) / CLOCKS_PER_SEC;
                    malloc_free_ctr++;
                }
            }
            else if (ctr > 0)
            {
                ctr--;

                clock_t temp = clock();
                free(array[ctr]);
                malloc_free_totaltime += (double)(clock() - temp) / CLOCKS_PER_SEC;

                malloc_free_ctr++;

                array[ctr] = 0x0;
            }
        }

        /* free remaining slots */
        for (; ctr > 0; ctr--)
        {
            clock_t temp = clock();
            free(array[ctr]);
            malloc_free_totaltime += (double)(clock() - temp) / CLOCKS_PER_SEC;
            malloc_free_ctr++;

            array[ctr] = 0x0;
        }

        malloc_time = (double)(clock() - t) / CLOCKS_PER_SEC;
    }

    cmalloc_time_total += cmalloc_time;
    malloc_time_total += malloc_time;
    printf("\t--> cmalloc_time %lf vs malloc time = %lf with %ld iterations\n", cmalloc_time, malloc_time, loop_counter_max);

    assert(cmalloc_exit(cm) == 0);
    return true;
}

bool sequence(int seq, size_t sz, size_t chunksz)
{
    bool retval = false;
    void *ptr = malloc(sz);
    assert(ptr != NULL);

    printf("sequence[%d] with size[%ld MB] and chunk size[%ld] means ratio: %ld\n", seq, sz / (1024*1024), chunksz, sz / (chunksz + sizeof(void*)));
    printf("--> simple tests\n");
    retval = simple_tests(ptr, sz, chunksz);
    if (retval == false) { printf("simple test failed!\n"); goto bail; }

    printf("--> extended tests\n");
    retval = extended_tests(ptr, sz, chunksz);
    if (retval == false) { printf("extended test failed!\n"); goto bail; }

    printf("--> random allocation and free tests\n");
    retval = rand_alloc_free_test(ptr, sz, chunksz);
    if (retval == false) { printf("rand_alloc_free test failed!\n"); goto bail; }

    printf("--> simple speed tests\n");
    retval = simple_speed_tests(ptr, sz, chunksz);
    if (retval == false) { printf("simple speed tests failed!\n"); goto bail; }

    printf("--> rand alloc free speed tests\n");
    retval = rand_alloc_free_speed_test(ptr, sz, chunksz);
    if (retval == false) { printf("rand alloc free speed tests failed!\n"); goto bail; }
bail:
    free(ptr);

    return retval;
}

int main(int argc, char *argv[])
{
    const rlim_t kStackSize = 128L * 1024L * 1024L;   // min stack size = 64 Mb
    struct rlimit rl;
    int result;
    int seq = 0;

    result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0)
    {
        if (rl.rlim_cur < kStackSize)
        {
            rl.rlim_cur = kStackSize;
            result = setrlimit(RLIMIT_STACK, &rl);
            if (result != 0)
            {
                fprintf(stderr, "setrlimit returned result = %d\n", result);
            }
        }
    }

    clock_t clock_total = clock();
    {
        size_t sz = 1 * 1024 * 1024;
        size_t chunksz = 8192;
        sequence(seq++, sz, chunksz);
    }

    {
        size_t sz = 1 * 1024 * 1024;
        size_t chunksz = 4096;
        sequence(seq++, sz, chunksz);
    }
    {
        size_t sz = 1 * 1024 * 1024;
        size_t chunksz = 1024;
        sequence(seq++, sz, chunksz);
    }

    {
        size_t sz = 1 * 1024 * 1024;
        size_t chunksz = 512;
        sequence(seq++, sz, chunksz);
    }

    {
        size_t sz = 1 * 1024 * 1024;
        size_t chunksz = 64;
        sequence(seq++, sz, chunksz);
    }
/*
    {
        size_t sz = 100 * 1024 * 1024;
        size_t chunksz = 8192;
        sequence(seq++, sz, chunksz);
    }

    {
        size_t sz = 100 * 1024 * 1024;
        size_t chunksz = 4096;
        sequence(seq++, sz, chunksz);
    }
    {
        size_t sz = 100 * 1024 * 1024;
        size_t chunksz = 1024;
        sequence(seq++, sz, chunksz);
    }

    {
        size_t sz = 100 * 1024 * 1024;
        size_t chunksz = 512;
        sequence(seq++, sz, chunksz);
    }

    {
        size_t sz = 100 * 1024 * 1024;
        size_t chunksz = 64;
        sequence(seq++, sz, chunksz);
    }

    {
        size_t sz = 1000 * 1024 * 1024;
        size_t chunksz = 8192;
        sequence(seq++, sz, chunksz);
    }

    {
        size_t sz = 1000 * 1024 * 1024;
        size_t chunksz = 4096;
        sequence(seq++, sz, chunksz);
    }
*/  
    printf("-->total time: %lf\n", (float) (clock() - clock_total) / CLOCKS_PER_SEC);
    printf("-->total: cmalloc_time %lf vs malloc time = %lf\n", cmalloc_time_total, malloc_time_total);
    printf("-->total: cmalloc alloc time %lf vs malloc alloc time = %lf with [%ld/%ld] allocations\n", cmalloc_alloc_totaltime, malloc_alloc_totaltime, cmalloc_alloc_ctr, malloc_alloc_ctr);
    printf("-->total: cmalloc free time %lf vs malloc free time = %lf with [%ld/%ld] frees\n", cmalloc_free_totaltime, malloc_free_totaltime, cmalloc_free_ctr, malloc_free_ctr);
    {
        int mult = 1000;
        printf("\t-->total: cmalloc alloc reltime(*%d) %lf vs malloc alloc reltime(*%d) = %lf\n", mult, (cmalloc_alloc_totaltime * mult) / cmalloc_alloc_ctr, mult, (malloc_alloc_totaltime * mult) / malloc_alloc_ctr);
        printf("\t-->total: cmalloc free reltime(*%d) %lf vs malloc free reltime(*%d) = %lf\n", mult, (cmalloc_free_totaltime * mult) / cmalloc_free_ctr, mult, (malloc_free_totaltime * mult) / malloc_free_ctr);

    }
    return 0;
}
