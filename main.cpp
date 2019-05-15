#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "thread_pool.h"
#include "task_pool.h"

char Arr[12][10] = {"save", "load", "update", "delete", "zf", "nudt", "using", "sdfd", "erewa", "2323", "qwecvcx", "32rwr"};

void* mytask(void *arg)
{
    printf("thread %d is working on task %d\n", (int)pthread_self(), (*(int*)arg) % 12);
    // sleep(1);
    free(arg);
    return NULL;
}

int main(int argc, char const *argv[])
{
    ThreadPool pool;
    //初始化线程池，最多三个线程
    ThreadPoolInit(&pool, 4);
    int i;
    //创建十个任务
    for(i=0; i < 100; i++)
    {
        int *arg = (int*) malloc(sizeof(int));
        *arg = i;
        ThreadPoolAddTask(&pool, Arr[i%12], mytask, arg);
    }
    sleep(3);
    ThreadPoolDestroy(&pool);
    return 0;
}
