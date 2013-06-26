#pragma once
#ifndef CMALLOC_H
#define CMALLOC_H

#include <errno.h>
#include <unistd.h>
#include <stddef.h>
#include <stdbool.h>

/*
   This is a fixed memory pool implementation of malloc.
   When it is initialised with a buffer of a cetain size (given by malloc)
   it will give out and free chunk, but never more than the given buffer.

   After exit is called one can free the buffer again.
*/

struct cmalloc;
int cmalloc_minimum_size();

struct cmalloc *cmalloc_init(void *base_ptr, size_t sz, size_t chunk_sz)  __attribute__ ((warn_unused_result));
struct cmalloc *cmalloc_init_debug(void *base_ptr, size_t sz, size_t chunk_sz, bool backtrace, int backtrace_sz)  __attribute__ ((warn_unused_result));
int cmalloc_exit(struct cmalloc *c);
void* cmalloc_alloc(struct cmalloc *c, size_t sz)  __attribute__ ((warn_unused_result));
void* cmalloc_calloc(struct cmalloc *c, size_t nmemb, size_t sz)  __attribute__ ((warn_unused_result));
void cmalloc_free(struct cmalloc *c, void *ptr);

int cmalloc_sanity_check(struct cmalloc *c);
ssize_t cmalloc_allocated(struct cmalloc *c);
ssize_t cmalloc_unallocated(struct cmalloc *c);

#endif /*CMALLOC_H*/

