/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libdlmod.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-09 18:52
 * updated: 2015-11-09 18:52
 ******************************************************************************/
#include <unistd.h>
#include <libgzf.h>
#include "libdlmod.h"
#include <libthread.h>

unsigned int sleep(unsigned int __seconds)
{
    printf("sleep prev\n");
    unsigned int ret = CALL(sleep, __seconds);
    printf("sleep post\n");
    return ret;
}

int pthread_create (pthread_t *__restrict __newthread,
			   const pthread_attr_t *__restrict __attr,
			   void *(*__start_routine) (void *),
			   void *__restrict __arg)
{
    printf("thread prev\n");
    unsigned int ret = CALL(pthread_create, __newthread,
__attr,__start_routine,__arg);
    printf("thread post\n");
    return ret;
}

void *malloc(size_t size)
{
    static int malloc_index = 0;
    malloc_index++;
    printf("malloc prev, malloc_index = %d\n", ++malloc_index);
    void *ptr = CALL(malloc, size);
    printf("malloc post\n");
    return ptr;
}

static void *thread(struct thread *t, void *arg)
{
    struct libthread *thread_if = (struct libthread *)arg;
    thread_if->mutex_lock(t);
    thread_if->cond_wait(t);
    thread_if->mutex_unlock(t);
    return NULL;
}

void test_thread()
{
    int i;
    struct capability_desc desc;
    struct libthread *thread_if = CALLOC(1, struct libthread);
    struct dl_handle *dl = dl_load("/usr/local/lib/libthread.so");
    int ret = dl_capability(dl, "thread", &desc);
    if (ret != 0) {
        printf("dl_capability failed!\n");
        return;
    }
    for (i = 0; i < desc.entry; ++i) {
        printf("cap[%d] = %s\n", i, desc.cap[i]);
    }
    thread_if->create = dl_get_func(dl, desc.cap[THREAD_CREATE]);
    thread_if->print_info = dl_get_func(dl, desc.cap[THREAD_PRINT_INFO]);
    thread_if->mutex_lock = dl_get_func(dl, desc.cap[THREAD_MUTEX_LOCK]);
    thread_if->mutex_unlock = dl_get_func(dl, desc.cap[THREAD_MUTEX_UNLOCK]);
    thread_if->cond_wait = dl_get_func(dl, desc.cap[THREAD_COND_WAIT]);
    if (!(thread_if->create && thread_if->print_info && thread_if->mutex_lock
       && thread_if->mutex_unlock && thread_if->cond_wait)) {
        printf("load libthread failed!\n");
        return;
    }

    struct thread *t1 = thread_if->create("t1", thread, thread_if);
    struct thread *t2 = thread_if->create("t2", thread, thread_if);
    struct thread *t3 = thread_if->create(NULL, thread, thread_if);
    printf("%s: t1->tid = %ld\n", __func__, t1->tid);
    printf("%s: t2->tid = %ld\n", __func__, t2->tid);
    thread_if->print_info(t1);
    thread_if->print_info(t2);
    thread_if->print_info(t3);
    while (1) {
        sleep(1);
    }

    dl_unload(dl);
}

void *my_thread(void *arg)
{
    while (1) {
        sleep(1);
        printf("%s:%d xxx\n", __func__, __LINE__);
    }

}

int main(int argc, char **argv)
{
    pthread_t pid;
    pthread_create(&pid, NULL, my_thread, NULL);
    test_thread();
    return 0;
}