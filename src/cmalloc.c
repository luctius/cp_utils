//#define DEBUG_CMALLOC
//#define CMALLOC_BACKUP_PLAN
#define NVALGRIND

#ifdef __GNUC__
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

//#include <execinfo.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
/*#if !(defined __UCLIBC__)
#include <execinfo.h>
#endif*/

#ifndef NVALGRIND
#include <valgrind/memcheck.h>
#endif

#include <libcp_utils-1.0/cmalloc.h>

#include "lro2_os.h"

#define CMALLOC_BAIL_PRINTF(fmt, ...) {fprintf(stderr, "\033[0;31m[CMALLOC:Error] [%s:%s:%d] " fmt "\033[0m\n", __FILE__, __func__,__LINE__,  ##__VA_ARGS__);}
#define CMALLOC_BAIL(error, fmt, ...) \
{ \
    CMALLOC_BAIL_PRINTF(fmt, ##__VA_ARGS__) \
    return  error; \
}

#define CMALLOC_BAIL_VOID(fmt, ...) \
{ \
    CMALLOC_BAIL_PRINTF(fmt, ##__VA_ARGS__) \
    return; \
}

#define CMALLOC_BAIL_ABORT(fmt, ...) \
{ \
    CMALLOC_BAIL_PRINTF(fmt, ##__VA_ARGS__) \
    abort(); \
}

#ifdef DEBUG_CMALLOC
#define CMALLOC_DEBUG_PRINTF(fmt, ...) fprintf(stdout, "[CMALLOC:debug] [%s:%s:%d] " fmt "\033[0m\n", __FILE__, __func__,__LINE__,  ##__VA_ARGS__);
#else
#define CMALLOC_DEBUG_PRINTF(fmt, ...) {}
#endif /*DEBUG_CMALLOC*/

#define CMALLOC_CHECK_MAGIC_NUMBER (0xDEADBABEBEEFBACE)

#define sem_post(a)
#define sem_wait(a)
#define sem_init(a,b,c) (0)
#define sem_destroy(a)

struct cmalloc
{
    uint64_t check1;
    void *base;
    size_t size;
    size_t chunk_sz;

    size_t total_sz;
    size_t free_sz;

    lro2_sem_t semaphore;

    uint64_t alloc_counter;
    uint64_t free_counter;
    struct chunk *chunk_max;

    uint64_t check2;

    size_t small_sz_max;
    struct chunk *first_chunk_small;
    struct chunk *first_chunk_big;
};

/*
   Struct chunk is used as the header of a free block.
   Whenever there is an alloc, the size of the chunk is split (if it is big enough),
   the size of the old chunk is recalulated, and a new chunk is made at the end of the
   to-be-allocated chunk.
   Then the addres of 'this' is returned, thus leaving sz and check before the returned
   pointer.
 */
struct chunk
{
    size_t sz; /**< The size of the chunk in bytes. >**/
    size_t check; /**< A simple parity check over sz to check the validity of the sz data and to check if this really has been returned by alloc. >**/
    uint8_t *this; /**< The address returned by alloc. >**/
    struct chunk *next; /**< Links to the next free chunk, or to the special end chunk >**/
    struct chunk *prev; /**< Links to the previous free chunk, or to the special begin chunk >**/
    bool special; /**< indicates if it is the start or end of a list. >**/
    void *padding;
};

int cmalloc_minimum_size(void)
{
    return (sizeof(struct cmalloc) + (5 * 64) );
}

static size_t calc_parity(size_t val)
{
    int parity = 0;
    if ( sizeof(size_t) == sizeof(int) ) parity = __builtin_parity(val);
    if ( sizeof(size_t) == sizeof(long) ) parity=  __builtin_parityl(val);
    if ( sizeof(size_t) == sizeof(long long) ) parity = __builtin_parityll(val);

    return (val << 1) | parity;
}

/* 
   Search smallest block in this list which is still equal or larger than size.
   Does not return special chunks.
 */
static struct chunk *search_best_fit(struct chunk *list, size_t sz)
{
    struct chunk *ptr = list;
    struct chunk *best_ptr = NULL;
    size_t best_sz = ~(0);

    assert(ptr != NULL);
    assert(sz > 0);

    if (ptr->special == true) ptr = ptr->next;

    while (ptr->special == false)
    {
        if (ptr->sz < best_sz)
        {
            if ( (sz) <= ptr->sz)
            {
                best_ptr = ptr;
                best_sz  = ptr->sz;
            }
        }

        if (ptr->check != calc_parity(ptr->sz) ) CMALLOC_BAIL_ABORT("memory corruption detected!");
        ptr = ptr->next;
    }

    return best_ptr;
}

static void sanity_check(struct cmalloc *c, struct chunk *list)
{
    struct chunk *list_ptr = list;

    assert(c != NULL);
    assert(list != NULL);
    assert( c->base < (void *) list);
    assert(list != list->next);

    if (list_ptr->special == true) 
    {
        list_ptr = list_ptr->next;
    }

    CMALLOC_DEBUG_PRINTF("Sanity Check");
    while (list_ptr->special == false)
    {
        assert(list_ptr != list_ptr->next);
        assert(list_ptr->this < ( ( (uint8_t *) c->base) + c->size) );
        if (list == c->first_chunk_small) assert(list_ptr->sz <= c->small_sz_max);
        assert(list_ptr->check == calc_parity(list_ptr->sz) );
        assert( ( (struct chunk *) (list_ptr->this + list_ptr->sz) ) <= list_ptr->next);
        assert( ( (struct chunk *) (list_ptr->this + list_ptr->sz) ) <= (c->chunk_max));
        list_ptr = list_ptr->next;
    }
}

/* Search within list if ptr is within a free block. */
static bool search_chunk_is_free(struct cmalloc *c, struct chunk *list, struct chunk *ptr)
{
    struct chunk *list_ptr = list;

    assert(c != NULL);
    assert(ptr != NULL);
    assert(list != NULL);
    assert( c->base < (void *) list);
    assert(list < ptr);
    assert(list != list->next);

    if (list_ptr->special == true) 
    {
        list_ptr = list_ptr->next;
        assert(list_ptr != list_ptr->next);
        assert(list_ptr->this < ( ( (uint8_t *) c->base) + c->size) );
    }


    while (list_ptr->special == false)
    {
        if (list_ptr->this <= ptr->this)
        {
            if ( (list_ptr->this + list_ptr->sz) >= (ptr->this + ptr->sz) )
            {
                CMALLOC_DEBUG_PRINTF("Chunk allready free!");
                return true;
            }
        }

        list_ptr = list_ptr->next;
        assert(list_ptr != list_ptr->next);
        assert(list_ptr->prev->special != true);
        assert(list_ptr->this < ( ( (uint8_t *) c->base) + c->size) );
        if (list == c->first_chunk_small) assert(list_ptr->sz <= c->small_sz_max);
        if (list_ptr->check != calc_parity(list_ptr->sz) ) CMALLOC_BAIL_ABORT("memory corruption detected!");
    }

    return false;
}

/* get the right neighbour of pointer. */
static struct chunk *search_next_chunk(struct chunk *list, struct chunk *ptr)
{
    struct chunk *cptr = list->next;

    assert(list != NULL);
    assert(list->special == true);
    assert(ptr != NULL);
    assert(ptr->special == false);
    assert(cptr != NULL);
    
    if (cptr > ptr) return cptr;

    do
    {
        cptr = cptr->next;
        if (cptr > ptr) return cptr;

        assert(cptr != cptr->next);
        assert(cptr->special != true);
    }
    while (cptr->special == false);

    return NULL;
}

/* check if both chunks are bordering each other, IN THAT ORDER */
static bool is_neighbour(struct chunk *ptr1, struct chunk *ptr2)
{
    if (ptr1 == NULL) return false;
    if (ptr2 == NULL) return false;
    if (ptr1->special == true) return false;
    if (ptr2->special == true) return false;
    if (ptr1 == ptr2) return false;
    if (ptr1 > ptr2) return false;

    assert( (ptr1->this +ptr1->sz) <= (ptr2->this) );
    if ( (ptr1->this +ptr1->sz +sizeof(void *)) >= ptr2->this )
    {
        CMALLOC_DEBUG_PRINTF("-- chunks [%p < %p < %p] are neighbours", ptr1->this, ptr2->this, ptr1->this + ptr1->sz);
        return true;
    }
    
    CMALLOC_DEBUG_PRINTF("-- chunks [%p < %p < %p] are not neighbours", ptr1->this, ptr2->this, ptr1->this + ptr1->sz);
    return false;
}

static struct chunk *split_and_delink_chunk(struct cmalloc *c, struct chunk *ptr, size_t sz)
{
    struct chunk *next_ptr = ptr->next;
    struct chunk *prev_ptr = ptr->prev;

    assert(c != NULL);
    assert(ptr != NULL);
    assert(ptr->special == false);
    assert(sz > 0);
    assert(sz <= c->free_sz);
    assert( (sz % c->chunk_sz) == 0);
    assert( (ptr->sz % c->chunk_sz) == 0);
    assert( ( (struct chunk *) (ptr->this + sz) ) < c->chunk_max);
    assert ( ( (struct chunk *) (ptr->this + ptr->sz) ) < (c->chunk_max));

    /* split chunk */
    if ( (ptr->sz > (sz + c->chunk_sz) ) )
    {
        struct chunk *new_ptr = (struct chunk *) ( ( (uint8_t *) ptr) + sz);

        new_ptr->this = (uint8_t *) &new_ptr->this;
        new_ptr->sz = ptr->sz - (new_ptr->this - ptr->this);
        new_ptr->check = calc_parity(new_ptr->sz);
        new_ptr->next = next_ptr;
        new_ptr->prev = prev_ptr;
        new_ptr->special = false;

        if ( ( (struct chunk *) (new_ptr->this + new_ptr->sz) ) >= (c->chunk_max)) CMALLOC_BAIL_ABORT("memory corruption detected!");

        next_ptr = new_ptr;
        prev_ptr = new_ptr;

        ptr->sz = new_ptr->this - ptr->this;
        ptr->check = calc_parity(ptr->sz);
        
        assert( ( ( (uint8_t *) ptr) +ptr->sz) <= (uint8_t *) new_ptr);
        assert(new_ptr->sz % c->chunk_sz == 0);
        assert( (new_ptr->this - ptr->this) % sizeof(void*) == 0);
        assert( (size_t) (new_ptr->this - ptr->this) == sz);
        assert( (new_ptr->this - ptr->this) % c->chunk_sz == 0);
    }

    /* delink chunk */
    ptr->prev->next = next_ptr;
    ptr->next->prev = prev_ptr;
    
    assert(next_ptr != NULL);
    if (next_ptr->special == false) assert(next_ptr->sz % c->chunk_sz == 0);
    assert(ptr->sz % c->chunk_sz == 0);
    assert(ptr->sz >= sz);
    assert((((void*)next_ptr) > c->base) && (((void*)next_ptr) < (c->base + c->size) ) );

    return next_ptr;
}

static void check_for_demote(struct cmalloc *c, struct chunk *ptr)
{
    assert(c != NULL);
    assert(ptr != NULL);
    assert(ptr->special == false);

    if (ptr->sz <= c->small_sz_max)
    {
        struct chunk *new_next_ptr = search_next_chunk(c->first_chunk_small, ptr);

        /* delink in promoted list */
        ptr->prev->next = ptr->next;
        ptr->next->prev = ptr->prev;

        /* link in demoted list */
        ptr->next = new_next_ptr;
        ptr->prev = new_next_ptr->prev;

        {
            //struct chunk *new_prev_ptr = new_next_ptr->prev;
            new_next_ptr->prev->next = ptr;
            new_next_ptr->prev = ptr;
        }
    }
}

struct cmalloc *cmalloc_init_debug(void *base_ptr, size_t sz, size_t chunk_sz, bool backtrace, int backtrace_sz)
{
    struct cmalloc *c = cmalloc_init(base_ptr, sz, chunk_sz);
    /*c->backtrace = backtrace;
    c->backtrace_sz = backtrace_sz;*/
    return c;
}

struct cmalloc *cmalloc_init(void *base_ptr, size_t sz, size_t chunk_sz)
{
    int offset = sizeof(struct cmalloc) +sizeof(void *);
    struct cmalloc *cm;
    //size_t orig_chunk_sz = chunk_sz;

    if (base_ptr == NULL) CMALLOC_BAIL(NULL, "Parameter 'base_ptr' invalid.");
    if (chunk_sz < (8 * sizeof(void *) ) )    CMALLOC_BAIL(NULL, "Parameter 'chunk_sz' invalid.");

    chunk_sz += offsetof(struct chunk, this);
    if (sz < (sizeof(struct cmalloc) + (6 * sizeof(struct chunk) ) +(2 * chunk_sz) ) ) CMALLOC_BAIL(NULL, "Parameter 'sz' invalid.");
    if ( (chunk_sz % sizeof(void *) ) != 0) chunk_sz = ( (chunk_sz / sizeof(void *) ) + 1) * sizeof(void *);   /* Align to machine size */

    CMALLOC_DEBUG_PRINTF("Init @ %p for %ld sz in %ld chunk_sz -> max: %p", base_ptr, sz, chunk_sz, ( (uint8_t *) base_ptr) + sz);

    cm = base_ptr;
    cm->base = base_ptr;
    /*cm->backtrace = false;
    cm->backtrace_sz = 0;*/
    cm->size = sz;
    cm->chunk_sz = chunk_sz;
    cm->alloc_counter = 0;
    cm->free_counter = 0;
    cm->small_sz_max = chunk_sz * 10;
    cm->first_chunk_small = base_ptr + offset;
    cm->first_chunk_big   = (struct chunk *) ( (uint8_t *) cm->first_chunk_small) + sizeof(struct chunk);
    cm->check1 = cm->check2 = CMALLOC_CHECK_MAGIC_NUMBER;

    if (lro2_sem_init(&cm->semaphore, 0, 1) != 0) CMALLOC_BAIL(NULL, "Requesting semaphore failed.");
    /* 
       Initialise both linked lists.
       There is one block; which will be put into biglist
     */
    {
        struct chunk *ch_sm_begin = cm->first_chunk_small;
        struct chunk *ch_bg_begin = cm->first_chunk_big;
        struct chunk *ch_real = (struct chunk *) ( (uint8_t *) cm->first_chunk_big) +sizeof(struct chunk);
        struct chunk *ch_sm_end = (base_ptr + sz) - (2 * sizeof(struct chunk) );
        struct chunk *ch_bg_end = (struct chunk *) ( ( (uint8_t *) ch_sm_end ) + sizeof(struct chunk) );

        ch_sm_begin->sz = sizeof(struct chunk);
        ch_sm_begin->check = calc_parity(ch_sm_begin->sz);
        ch_sm_begin->this = (uint8_t *) &ch_sm_begin->this;
        ch_sm_begin->next = ch_sm_end;
        ch_sm_begin->prev = NULL;
        ch_sm_begin->special = true;

        ch_bg_begin->sz = sizeof(struct chunk);
        ch_bg_begin->check = calc_parity(ch_bg_begin->sz);
        ch_bg_begin->this = (uint8_t *) &ch_bg_begin->this;
        ch_bg_begin->next = ch_real;
        ch_bg_begin->prev = NULL;
        ch_bg_begin->special = true;

        ch_real->sz = sizeof(struct chunk);
        ch_real->check = calc_parity(ch_real->sz);
        ch_real->this = (uint8_t *) &ch_real->this;
        ch_real->next = ch_bg_end;
        ch_real->prev = ch_bg_begin;
        ch_real->special = false;

        ch_sm_end->sz = sizeof(struct chunk);
        ch_sm_end->check = calc_parity(ch_sm_end->sz);
        ch_sm_end->this = (uint8_t *) &ch_sm_end->this;
        ch_sm_end->next = NULL;
        ch_sm_end->prev = ch_sm_begin;
        ch_sm_end->special = true;

        ch_bg_end->sz = sizeof(struct chunk);
        ch_bg_end->check = calc_parity(ch_bg_end->sz);
        ch_bg_end->this = (uint8_t *) &ch_bg_end->this;
        ch_bg_end->next = NULL;
        ch_bg_end->prev = ch_real;
        ch_bg_end->special = true;

        cm->chunk_max = ch_sm_end;

        cm->free_sz = ( ( ( (ch_sm_end->this - ch_real->this) / chunk_sz ) -1 ) * chunk_sz);
        cm->total_sz = cm->free_sz;

        /* recalulate block size */
        ch_real->sz = cm->free_sz;
        ch_real->check = calc_parity(ch_real->sz);
/*
#ifndef NVALGRIND
        if (RUNNING_ON_VALGRIND != 0)
        {
            VALGRIND_CREATE_MEMPOOL(cm, 0, 0);
            int header_sz = ( ( (uint8_t *) &ch_real ) - ( (uint8_t *) cm->base )- 1);
            //VALGRIND_MAKE_MEM_NOACCESS(cm->base, offsetof(struct chunk, check) );
        }
#endif
*/
    }

    return cm;
}

int cmalloc_exit(struct cmalloc *c)
{
    if ( (c == NULL) || 
        ( (c->check1 != CMALLOC_CHECK_MAGIC_NUMBER) || (c->check2 != CMALLOC_CHECK_MAGIC_NUMBER) ) )
    {
        CMALLOC_BAIL(-EINVAL, "CMalloc context invalid.");
    }

    lro2_sem_wait(&c->semaphore); /*sem down*/
    lro2_sem_post(&c->semaphore); /*sem up*/
    lro2_sem_destroy(&c->semaphore);
/*
#ifndef NVALGRIND
        if (RUNNING_ON_VALGRIND != 0)
        {
            VALGRIND_DESTROY_MEMPOOL(c);
        }
#endif
*/
    cmalloc_sanity_check(c);
    if (cmalloc_allocated(c) > 0) CMALLOC_BAIL(-EFAULT, "Internal memory still in use: %zd", cmalloc_allocated(c) );

    return 0;
}

void* cmalloc_alloc(struct cmalloc *c, size_t sz)
{
    void *retval = NULL;
    struct chunk *chunk_ptr = NULL;
    size_t orig_sz = sz;

#ifdef CMALLOC_BACKUP_PLAN
    return malloc(sz);
#endif /*CMALLOC_BACKUP_PLAN*/

    if (c == NULL) CMALLOC_BAIL(NULL, "CMalloc context invalid.");
    if ( ( (c->check1 != CMALLOC_CHECK_MAGIC_NUMBER) || (c->check2 != CMALLOC_CHECK_MAGIC_NUMBER) ) ) CMALLOC_BAIL(NULL, "CMalloc context invalid.");

/*
#if !(defined __UCLIBC__)
    if (unlikely(c->backtrace) )
    {
        void *array[c->backtrace_sz];
        size_t array_sz;
        char **strings;
        array_sz = backtrace (array, c->backtrace_sz);
        strings = backtrace_symbols (array, array_sz);
        for (int i = 0; i < array_sz; i++) printf ("cmalloc_alloc caller[%d]: %s\n", i, strings[i]);
    }
#endif
*/

    lro2_sem_wait(&c->semaphore); /*sem down*/

    /* Align sz to chunk_sz; bail if there is not enough memory left */
    {
        sz = (sz / c->chunk_sz) * c->chunk_sz;
        if (orig_sz > sz) sz += c->chunk_sz;
        if (orig_sz + offsetof(struct chunk, this) >= sz) sz += c->chunk_sz;
        if (sz > c->free_sz) return NULL; 
    }

    //cmalloc_sanity_check(c);

    if (sz <= c->small_sz_max)
    {
        /* Search small chunk list */
        chunk_ptr = search_best_fit(c->first_chunk_small, sz);

        /* delink chunk */
        if (chunk_ptr != NULL)
        {
            split_and_delink_chunk(c, chunk_ptr, sz);

            if ((((void*)chunk_ptr) <= c->base) || 
                (((void*)chunk_ptr) >= (c->base + c->size) ) ||
                ( ( (struct chunk *) (chunk_ptr->this + chunk_ptr->sz) ) >= (c->chunk_max) ) ) 
            {
                    CMALLOC_BAIL_ABORT("memory corruption detected!");
            }
        }
    }

    if (chunk_ptr == NULL)
    {
        /* Search big chunk list */
        chunk_ptr = search_best_fit(c->first_chunk_big, sz);

        /* delink chunk */
        if (chunk_ptr != NULL)
        {
            struct chunk *new_ptr = split_and_delink_chunk(c, chunk_ptr, sz);
            if (new_ptr->special == false)
            {
                check_for_demote(c, new_ptr);
            }

            if ((((void*)chunk_ptr) <= c->base) || 
                (((void*)chunk_ptr) >= (c->base + c->size) ) ||
                ( ( (struct chunk *) (chunk_ptr->this + chunk_ptr->sz) ) >= (c->chunk_max) ) )
            {
                    CMALLOC_BAIL_ABORT("memory corruption detected!");
            }
        }
    }

    /* Do bookkeeping with the chunk we found */
    if (chunk_ptr != NULL) 
    {
        c->free_sz -= chunk_ptr->sz;
        retval = chunk_ptr->this;
        c->alloc_counter++;

#ifdef DEBUG_CMALLOC
        CMALLOC_DEBUG_PRINTF("-- malloc[%p] [%ld /%ld] free [%ld / %ld]", chunk_ptr, chunk_ptr->sz, orig_sz, c->free_sz, c->total_sz);
        memset(&chunk_ptr->padding, 0xAA, sz - sizeof(struct chunk));

        struct chunk *tmp_s = search_next_chunk(c->first_chunk_small, chunk_ptr);
        struct chunk *tmp_b = search_next_chunk(c->first_chunk_big, chunk_ptr);
        if (tmp_s != NULL) assert(tmp_s->check == calc_parity(tmp_s->sz) );
        else if (tmp_b != NULL) assert(tmp_b->check == calc_parity(tmp_b->sz) );
        else CMALLOC_BAIL_ABORT("memory corruption detected!");
#endif

        assert( (sz + sizeof(void *) + sizeof(void *) ) > orig_sz);
        assert((((void*)chunk_ptr) > c->base) );
        assert(((void*)chunk_ptr->this + chunk_ptr->sz) < (c->base + c->size) );
        assert(c->free_sz >= 0);
        assert(chunk_ptr->sz >= sz);
        assert(c->free_sz <= c->total_sz);
        assert( ( (struct chunk *) (chunk_ptr->this + chunk_ptr->sz) ) < c->chunk_max);
        assert(chunk_ptr->special == false);
        assert(chunk_ptr->check == calc_parity(chunk_ptr->sz) );

#ifndef NVALGRIND
        /*
        if (RUNNING_ON_VALGRIND != 0)
        {
            //memset(chunk_ptr->this, 0xFF, chunk_ptr->sz);
            VALGRIND_MEMPOOL_ALLOC(c, chunk_ptr->this, orig_sz);
            //VALGRIND_MAKE_MEM_NOACCESS(chunk_ptr, offsetof(struct chunk, check) );
        }
        */
        VALGRIND_MALLOCLIKE_BLOCK(chunk_ptr->this, orig_sz, 0, 0);
#endif
    }
    /*this should not happen; we'll invistigate later... */
    else CMALLOC_DEBUG_PRINTF("-- malloc failed: [%ld / %ld] free [%ld / %ld]", sz, orig_sz, c->free_sz, c->total_sz);

    cmalloc_sanity_check(c);
    CMALLOC_DEBUG_PRINTF("-- malloc alloc end\n");
    lro2_sem_post(&c->semaphore); /*sem up*/

    return retval;
}

void* cmalloc_calloc(struct cmalloc *c, size_t nmemb, size_t sz)
{
    void *ptr;

    if (c == NULL) CMALLOC_BAIL(NULL, "CMalloc context invalid.");

    ptr = cmalloc_alloc(c, nmemb * sz);
    if (ptr != NULL) memset(ptr, 0x0, nmemb * sz);

    assert(ptr > c->base);
    return ptr;
}

void cmalloc_free(struct cmalloc *c, void *ptr)
{
    //int orig_sz;
    size_t freed_sz = 0;
    bool in_biglist = false;
    bool in_smalllist = false;

#ifdef CMALLOC_BACKUP_PLAN
    return free(ptr);
#endif /*CMALLOC_BACKUP_PLAN*/

    struct chunk *chunk_ptr = (struct chunk *) (ptr - offsetof(struct chunk, this) );

    if (c == NULL) CMALLOC_BAIL_VOID("CMalloc context invalid.");
    if ( ( (c->check1 != CMALLOC_CHECK_MAGIC_NUMBER) || (c->check2 != CMALLOC_CHECK_MAGIC_NUMBER) ) ) CMALLOC_BAIL_VOID("CMalloc context invalid.");
    if (ptr == NULL) return;
    if (ptr <= c->base) CMALLOC_BAIL_VOID("Parameter 'ptr' invalid.");
    if (chunk_ptr >= (c->chunk_max) ) CMALLOC_BAIL_VOID("Parameter 'ptr' invalid.");

/*
#if !(defined __UCLIBC__)
    if (unlikely(c->backtrace) )
    {
        void *array[c->backtrace_sz];
        size_t array_sz;
        char **strings;
        array_sz = backtrace (array, c->backtrace_sz);
        strings = backtrace_symbols (array, array_sz);
        for (int i = 0; i < array_sz; i++) printf ("cmalloc_free caller[%d]: %s\n", i, strings[i]);
    }
#endif
*/
    lro2_sem_wait(&c->semaphore); /*sem down*/

    /* restore chunk settings */
    chunk_ptr->this = (uint8_t *) ptr;
    chunk_ptr->special = false;
    //orig_sz = chunk_ptr->sz;

    cmalloc_sanity_check(c);

    CMALLOC_DEBUG_PRINTF("-- freeing [%p, %ld] --> [%p] {%ld} ", chunk_ptr, chunk_ptr->sz, c->chunk_max, c->free_sz);

    if (unlikely(chunk_ptr->sz == 0) ) CMALLOC_BAIL_ABORT("memory corruption detected!");
    if (unlikely(chunk_ptr->sz % c->chunk_sz != 0) ) CMALLOC_BAIL_ABORT("memory corruption detected!");
    if (unlikely(chunk_ptr->check != calc_parity(chunk_ptr->sz) ) ) CMALLOC_BAIL_ABORT("memory corruption detected!");
    if (unlikely( ( (struct chunk *) (chunk_ptr->this + chunk_ptr->sz) ) >= c->chunk_max) ) CMALLOC_BAIL_ABORT("memory corruption detected!");

    CMALLOC_DEBUG_PRINTF("Checking if chunk is free: big list");
    if (search_chunk_is_free(c, c->first_chunk_big,   chunk_ptr) == true) goto free_clean;
    CMALLOC_DEBUG_PRINTF("Checking if chunk is free: small list");
    if (search_chunk_is_free(c, c->first_chunk_small, chunk_ptr) == true) goto free_clean;

    freed_sz = chunk_ptr->sz;

    /* Search for big neighbours */
    {
        struct chunk *nb_2_ptr = search_next_chunk(c->first_chunk_big, chunk_ptr);
        struct chunk *nb_1_ptr = nb_2_ptr->prev;

        assert(chunk_ptr != nb_1_ptr);
        assert(chunk_ptr != nb_2_ptr);
        assert(nb_1_ptr->check == calc_parity(nb_1_ptr->sz) );
        assert(nb_2_ptr->check == calc_parity(nb_2_ptr->sz) );
        assert((((void*) nb_1_ptr) > c->base) && (((void*)nb_1_ptr) < (c->base + c->size) ) );
        assert((((void*) nb_2_ptr) > c->base) && (((void*)nb_2_ptr) < (c->base + c->size) ) );
        assert( ( (struct chunk *) (chunk_ptr->this + chunk_ptr->sz) ) < (c->chunk_max));

        if (is_neighbour(chunk_ptr, nb_2_ptr) )
        {
            CMALLOC_DEBUG_PRINTF("Has Big Neighbour 2");
            assert( ( (struct chunk *) (nb_2_ptr->this + nb_2_ptr->sz) ) < (c->chunk_max));

            chunk_ptr->next = nb_2_ptr->next;
            chunk_ptr->prev = nb_2_ptr->prev;
            nb_2_ptr->next->prev = chunk_ptr;
            nb_2_ptr->prev->next = chunk_ptr;

            /* chunk_ptr eats neighbour */
            chunk_ptr->sz += nb_2_ptr->sz;
            chunk_ptr->check = calc_parity(chunk_ptr->sz);

            in_biglist = true;
        }

        if (is_neighbour(nb_1_ptr, chunk_ptr) )
        {
            CMALLOC_DEBUG_PRINTF("Has Big Neighbour 1");
            assert( ( (struct chunk *) (nb_1_ptr->this + nb_1_ptr->sz) ) < (c->chunk_max));

            /* neighbour eats chunk_ptr */
            nb_1_ptr->sz += chunk_ptr->sz;
            nb_1_ptr->check = calc_parity(nb_1_ptr->sz);

            if (in_biglist)
            {
                CMALLOC_DEBUG_PRINTF("Has ALSO Big Neighbour 2");
                nb_1_ptr->next = chunk_ptr->next;
                chunk_ptr->next->prev = nb_1_ptr;
            }

            /* alias nb_1_ptr as chunk_ptr*/
            chunk_ptr = nb_1_ptr;
            in_biglist = true;
        }

        assert(chunk_ptr != chunk_ptr->next);
        assert(chunk_ptr != chunk_ptr->prev);
    }

    /* Search for small neighbours */
    {
        struct chunk *nb_2_ptr = search_next_chunk(c->first_chunk_small, chunk_ptr);
        struct chunk *nb_1_ptr = nb_2_ptr->prev;

        assert(chunk_ptr != nb_1_ptr);
        assert(chunk_ptr != nb_2_ptr);
        assert(nb_1_ptr->check == calc_parity(nb_1_ptr->sz) );
        assert(nb_2_ptr->check == calc_parity(nb_2_ptr->sz) );
        assert((((void*) nb_1_ptr) > c->base) && (((void*)nb_1_ptr) < (c->base + c->size) ) );
        assert((((void*) nb_2_ptr) > c->base) && (((void*)nb_2_ptr) < (c->base + c->size) ) );
        assert ( ( (struct chunk *) (chunk_ptr->this + chunk_ptr->sz) ) < (c->chunk_max));

        if (is_neighbour(chunk_ptr, nb_2_ptr) )
        {
            CMALLOC_DEBUG_PRINTF("Has Little Neighbour 2");

            /* chunk_ptr eats neighbour */
            chunk_ptr->sz += nb_2_ptr->sz;
            chunk_ptr->check = calc_parity(chunk_ptr->sz);

            if (in_biglist == true)
            {
                CMALLOC_DEBUG_PRINTF("Has a Big Neighbour");

                nb_1_ptr->next = chunk_ptr->next;
                /* delink neighbour in small */
                nb_2_ptr->prev->next = nb_2_ptr->next;
                nb_2_ptr->next->prev = nb_2_ptr->prev;
            }
            else 
            {
                CMALLOC_DEBUG_PRINTF("Has no Big Neighbour");

                /*link chunk_ptr in small*/
                chunk_ptr->next = nb_2_ptr->next;
                chunk_ptr->prev = nb_2_ptr->prev;
                nb_2_ptr->prev->next = chunk_ptr;
                nb_2_ptr->next->prev = chunk_ptr;
                in_smalllist = true;
            }
        }

        if (is_neighbour(nb_1_ptr, chunk_ptr) )
        {
            CMALLOC_DEBUG_PRINTF("Has Little Neighbour 1");

            /* neightbour eats chunk_ptr */
            nb_1_ptr->sz += chunk_ptr->sz;
            nb_1_ptr->check = calc_parity(nb_1_ptr->sz);

            /* promote nb_1_ptr */
            if (in_biglist == true)
            {
                CMALLOC_DEBUG_PRINTF("Has a Big Neighbour");

                /* delink neighbour in small */
                nb_1_ptr->prev->next = nb_1_ptr->next;
                nb_1_ptr->next->prev = nb_1_ptr->prev;

                /* link neighbour in big */
                nb_1_ptr->prev = chunk_ptr->prev;
                nb_1_ptr->next = chunk_ptr->next;

                /* delink chunk_ptr in big */
                chunk_ptr->prev->next = nb_1_ptr;
                chunk_ptr->next->prev = nb_1_ptr;
            }
            else if (in_smalllist == true)
            {
                CMALLOC_DEBUG_PRINTF("Has another little Neighbour");

                /* link neighbour in small*/
                nb_1_ptr->next = chunk_ptr->next;

                /* delink chunk_ptr */
                chunk_ptr->next->prev = nb_1_ptr;
            }
            else 
            {
                CMALLOC_DEBUG_PRINTF("Has no little Neighbour");
                in_smalllist = true; /* if it isn't in small or big list yet, it will go into small list */
            }

            /* alias nb_1_ptr as chunk_ptr*/
            chunk_ptr = nb_1_ptr;
        }

        assert(chunk_ptr != chunk_ptr->next);
        assert(chunk_ptr != chunk_ptr->prev);
    }

    assert( (in_biglist == false) || (in_smalllist == false) );

    /* promote if possible */
    if (in_biglist == false)
    {
        if (chunk_ptr->sz > c->small_sz_max)
        {
            CMALLOC_DEBUG_PRINTF("promoting");
            struct chunk *nb_ptr = search_next_chunk(c->first_chunk_big, chunk_ptr);
            assert((((void*)nb_ptr) > c->base) && (((void*)nb_ptr) < (c->base + c->size) ) );

            if (in_smalllist == true)
            {
                CMALLOC_DEBUG_PRINTF("promoting from little list");
                /* delink from small list */
                chunk_ptr->prev->next = chunk_ptr->next;
                chunk_ptr->next->prev = chunk_ptr->prev;

                in_smalllist = false;
            }

            /* link in big list */
            chunk_ptr->next = nb_ptr;
            chunk_ptr->prev = nb_ptr->prev;
            nb_ptr->prev->next = chunk_ptr;
            nb_ptr->prev = chunk_ptr;

            in_biglist = true;
        }
        assert(chunk_ptr != chunk_ptr->next);
        assert(chunk_ptr != chunk_ptr->prev);
    }

    /* No direct neighbours, and too small for biglist; put into small list */
    if (in_biglist == false && in_smalllist == false)
    {
        struct chunk *nb_ptr = search_next_chunk(c->first_chunk_small, chunk_ptr);
        assert((((void*)nb_ptr) > c->base) && (((void*)nb_ptr) < (c->base + c->size) ) );
        CMALLOC_DEBUG_PRINTF("homeless, putting in little list");

        /* link in small list */
        chunk_ptr->next = nb_ptr;
        chunk_ptr->prev = nb_ptr->prev;
        nb_ptr->prev->next = chunk_ptr;
        nb_ptr->prev = chunk_ptr;

        in_smalllist = true;

        assert ( ( (struct chunk *) (chunk_ptr->this + chunk_ptr->sz) ) < (c->chunk_max));
        assert(chunk_ptr != chunk_ptr->next);
        assert(chunk_ptr != chunk_ptr->prev);
    }

    assert( (in_biglist == false) || (in_smalllist == false) );
    assert( (in_biglist == true ) || (in_smalllist == true) );

    c->free_sz += freed_sz;
    c->free_counter++;

    CMALLOC_DEBUG_PRINTF("-- freed [%ld @ %ld / %ld]", freed_sz, c->free_sz, c->total_sz);

    assert(chunk_ptr != chunk_ptr->next);
    assert(chunk_ptr != chunk_ptr->prev);
    assert(c->free_sz > 0);
    assert(c->free_sz <= c->total_sz);
    cmalloc_sanity_check(c);
    CMALLOC_DEBUG_PRINTF("-- malloc free end\n");

#ifndef NVALGRIND
        /*
        if (RUNNING_ON_VALGRIND != 0)
        {
            VALGRIND_MEMPOOL_FREE(c, ptr);
        }
        */
        VALGRIND_FREELIKE_BLOCK(ptr, 0);
#endif

free_clean:
    lro2_sem_post(&c->semaphore); /*sem up*/
}

int cmalloc_sanity_check(struct cmalloc *c)
{
    if (c == NULL) CMALLOC_BAIL(-EINVAL, "CMalloc context invalid.");
    if ( ( (c->check1 != CMALLOC_CHECK_MAGIC_NUMBER) || (c->check2 != CMALLOC_CHECK_MAGIC_NUMBER) ) ) CMALLOC_BAIL(-EINVAL, "CMalloc context invalid.");

    sanity_check(c, c->first_chunk_big);
    sanity_check(c, c->first_chunk_small);

    if ((c->first_chunk_big->special             != true) ||
        (c->first_chunk_big->check               != calc_parity(c->first_chunk_big->sz) ) ||
        (c->first_chunk_small->special           != true) ||
        (c->first_chunk_small->check             != calc_parity(c->first_chunk_small->sz) ) ||
        (c->first_chunk_big->next->special       != false) ||
        (c->first_chunk_big->next->check         != calc_parity(c->first_chunk_big->next->sz) ) )
    {
        CMALLOC_BAIL(-EFAULT, "memory corruption detected!");
    }
    return 0;
}

size_t cmalloc_allocated(struct cmalloc *c)
{
    if (c == NULL) CMALLOC_BAIL(-EINVAL, "CMalloc context invalid.");
    if ( ( (c->check1 != CMALLOC_CHECK_MAGIC_NUMBER) || (c->check2 != CMALLOC_CHECK_MAGIC_NUMBER) ) ) CMALLOC_BAIL(-EINVAL, "CMalloc context invalid.");
    return c->total_sz- cmalloc_unallocated(c);
}

size_t cmalloc_unallocated(struct cmalloc *c)
{
    if (c == NULL) CMALLOC_BAIL(-EINVAL, "CMalloc context invalid.");
    if ( ( (c->check1 != CMALLOC_CHECK_MAGIC_NUMBER) || (c->check2 != CMALLOC_CHECK_MAGIC_NUMBER) ) ) CMALLOC_BAIL(-EINVAL, "CMalloc context invalid.");
    return c->free_sz;
}

