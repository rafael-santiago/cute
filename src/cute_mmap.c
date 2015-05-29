/*
 *                                Copyright (C) 2015 by Rafael Santiago
 *
 * This is a free software. You can redistribute it and/or modify under
 * the terms of the GNU General Public License version 2.
 *
 */
#include "cute_mmap.h"
#include "cute.h"
#include <string.h>
#include <pthread.h>

#define new_cute_mmap_ctx(m) ( g_cute_leak_check = 0,\
                               (m) = malloc(sizeof(struct cute_mmap_ctx)),\
                               (m)->next = NULL,\
                               g_cute_leak_check = 1 )

pthread_mutex_t mmap_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct cute_mmap_ctx *get_cute_mmap_ctx_tail(struct cute_mmap_ctx *mmap) {
    struct cute_mmap_ctx *p;
    if (mmap == NULL) {
        return NULL;
    }
    for (p = mmap; p->next != NULL; p = p->next)
         ;
    return p;
}

struct cute_mmap_ctx *add_allocation_to_cute_mmap_ctx(struct cute_mmap_ctx *mmap,
                                                      size_t size, void *addr) {
    struct cute_mmap_ctx *head = NULL;
    struct cute_mmap_ctx *p = NULL;
    pthread_mutex_lock(&mmap_mutex);
    head = mmap;
    if (head == NULL) {
        new_cute_mmap_ctx(head);
        p = head;
    } else {
        p = get_cute_mmap_ctx_tail(mmap);
        new_cute_mmap_ctx(p->next);
        p = p->next;
    }
    p->id = ++g_cute_mmap_id;
    p->size = size;
    p->addr = addr;
    pthread_mutex_unlock(&mmap_mutex);
    return head;
}

struct cute_mmap_ctx *rm_allocation_from_cute_mmap_ctx(struct cute_mmap_ctx *mmap,
                                                       void *addr) {
    struct cute_mmap_ctx *head = NULL;
    struct cute_mmap_ctx *burn = NULL;
    struct cute_mmap_ctx *last = NULL;
    pthread_mutex_lock(&mmap_mutex);
    head = mmap;
    if (mmap == NULL) {
        pthread_mutex_unlock(&mmap_mutex);
        return NULL;
    }
    for (burn = mmap; burn != NULL; last = burn, burn = burn->next) {
        if (burn->addr == addr) {
            break;
        }
    }
    if (burn != NULL) {
        if (last == NULL) {
            head = burn->next;
            burn->next = NULL;
        } else {
            last->next = burn->next;
            burn->next = NULL;
        }
        del_cute_mmap_ctx(burn);
    }
    pthread_mutex_unlock(&mmap_mutex);
    return head;
}

void del_cute_mmap_ctx(struct cute_mmap_ctx *mmap) {
    struct cute_mmap_ctx *p = NULL, *t = NULL;
    pthread_mutex_lock(&mmap_mutex);
    g_cute_leak_check = 0;
    for (t = p = mmap; t != NULL; p = t) {
        t = p->next;
        free(p);
    }
    g_cute_leak_check = 1;
    pthread_mutex_unlock(&mmap_mutex);
}