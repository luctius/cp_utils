#pragma once
#ifndef LRO2_MUTEX_H_
#define LRO2_MUTEX_H_
#include "def.h"

#ifdef __WIN32__
    #define sleep Sleep

    #define __ORDER_LITTLE_ENDIAN__ 1234
    #define __FLOAT_WORD_ORDER__ __ORDER_LITTLE_ENDIAN__
    #define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
#endif

#ifdef __linux__
    //#warning Compiling for Linux
    #include <semaphore.h>
    #define lro2_sem_t       sem_t

    #define lro2_sem_init    sem_init
    #define lro2_sem_destroy sem_destroy
    #define lro2_sem_wait    sem_wait
    #define lro2_sem_post    sem_post
#elif __APPLE__
    #warning Compiling for OSX
    #include "darwin_thread.h"
    #define lro2_sem_t       usbi_mutex_t

    #define lro2_sem_init    usbi_mutex_init
    #define lro2_sem_destroy usbi_mutex_destroy
    #define lro2_sem_wait    usbi_mutex_lock
    #define lro2_sem_post    usbi_mutex_unlock
#elif __WIN32__
    #warning Compiling for win32
    #include "threads_windows.h"
    #define lro2_sem_t       usbi_mutex_t

    #define lro2_sem_init    usbi_mutex_init
    #define lro2_sem_destroy usbi_mutex_destroy
    #define lro2_sem_wait    usbi_mutex_lock
    #define lro2_sem_post    usbi_mutex_unlock
#endif

#endif /*LRO2_MUTEX_H_*/

