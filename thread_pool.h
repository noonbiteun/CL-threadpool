/*
 * 线程池
*/
#if !defined(THREAD_POOL_H_)
#define THREAD_POOL_H_

#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "condition.h"
#include "task_pool.h"

struct ThreadPool
{
    Condition ready;            //条件锁
    TaskPool *task_pool;        //任务池

    int max_threads;             //线程池最大线程数
    int counter;                //线程池中已有线程数
    int idle;                   //线程池中空闲线程数
    int quit;                   //是否退出标志
};

void *thread_routine(void *arg)
{
    struct timespec abstime;
    int timeout;
    printf("thread %d is starting\n", (int)pthread_self());
    ThreadPool *pool = (ThreadPool *)arg;
    
    while(true)
    {
        timeout = 0;
        //访问线程池之前需要加锁
        ConditionLock(&pool->ready);
        //空闲
        pool->idle++;
        //取出第一个未被锁定的类别的任务
        Task *task = PopTask(pool->task_pool);
        //未取到有效任务
        while (task == nullptr)
        {
            if (pool->quit)
            {
                break;
            }
            //获取从当前时间，并加上等待时间， 设置进程的超时睡眠时间
            clock_gettime(CLOCK_REALTIME, &abstime);
            //等待10秒钟
            abstime.tv_sec += 10;
            int status;
            status = ConditionTimedwait(&pool->ready, &abstime);  //该函数会解锁，允许其他线程访问，当被唤醒时，加锁
            if(status == ETIMEDOUT) {
                //等待超时，回收线程
                printf("thread %d wait timed out\n", (int)pthread_self());
                timeout = 1;
                break;
            } else {
                //被唤醒，重新获取一次任务
                task = PopTask(pool->task_pool);
            }
        }
        
        //开始工作
        pool->idle--;
        if (task) {
            //由于任务执行需要消耗时间，先解锁让其他线程访问线程池
            ConditionUnlock(&pool->ready);
            //执行任务
            task->run(task->arg);
            //留存类别
            char *task_category = task->category;
            //执行完任务释放内存
            delete task;
            //重新加锁
            ConditionLock(&pool->ready);
            //移除锁定类别
            RemoveLockCategory(task_category, pool->task_pool);
        }
        
        //退出线程池（前提：有效任务已取完或者所有任务都取完）
        if(pool->quit && (!task || !pool->task_pool->head_node)) {
            pool->counter--;//当前工作的线程数-1
            //若线程池中没有线程，通知等待线程（主线程）全部任务已经完成
            if(pool->counter == 0)
            {
                ConditionSignal(&pool->ready);
            }
            ConditionUnlock(&pool->ready);
            break;
        }

        //超时，跳出销毁线程
        if(timeout == 1) {
            pool->counter--;//当前工作的线程数-1
            ConditionUnlock(&pool->ready);
            break;
        }

        ConditionUnlock(&pool->ready);
    }
    printf("thread %d is exiting\n", (int)pthread_self());
    return nullptr;
}

//线程池初始化
void ThreadPoolInit(ThreadPool *pool, int threads)
{
    if (threads < 1) {
        printf("Error : threads must greater than 0");
    }
    pool->task_pool = new TaskPool();
    pool->counter =0;
    pool->idle =0;
    pool->max_threads = threads;
    pool->quit =0;
    ConditionInit(&pool->ready);
    TaskPoolInit(pool->task_pool, threads);
}

//增加一个任务到线程池
bool ThreadPoolAddTask(ThreadPool *pool, char *category, void *(*run)(void *arg), void *arg)
{
    //已调用销毁，不可再添加
    if (pool->quit)
    {
        return false;
    }
    //产生一个新的任务
    Task *newtask = new Task();
    newtask->category = category;
    newtask->run = run;
    newtask->arg = arg;
    newtask->next = nullptr;

    //线程池的状态被多个线程共享，操作前需要加锁
    ConditionLock(&pool->ready);

    AddTask(newtask, pool->task_pool);//添加到任务池

    //线程池中有线程空闲，唤醒
    if(pool->idle > 0) {
        ConditionSignal(&pool->ready);
    } else if (pool->counter < pool->max_threads)  //当前线程池中线程个数没有达到设定的最大值，创建新线程
    {
        pthread_t tid;
        pthread_create(&tid, NULL, thread_routine, pool);
        pthread_detach(tid);
        pool->counter++;
    }

    //解锁
    ConditionUnlock(&pool->ready);
    return true;
}

//清空所有任务
void ThreadPoolCleanTask(ThreadPool *pool)
{
    //线程池的状态被多个线程共享，操作前需要加锁
    ConditionLock(&pool->ready);

    if (pool && pool->task_pool)
    {
        CleanTaskPool(pool->task_pool);
    }

    //解锁
    ConditionUnlock(&pool->ready);
}

//线程池销毁
void ThreadPoolDestroy(ThreadPool *pool)
{
    //如果已经调用销毁，直接返回
    if(pool->quit) {
    return;
    }
    //加锁
    ConditionLock(&pool->ready);
    //设置销毁标记为1
    pool->quit = 1;
    //线程池中线程个数大于0
    if(pool->counter > 0) {
        //唤醒所有的空余等待线程
        if(pool->idle > 0) {
            ConditionBroadcast(&pool->ready);
        }
        //正在执行任务的线程，等待他们结束任务
        while(pool->counter)
        {
            ConditionWait(&pool->ready);
        }
    }
    ConditionUnlock(&pool->ready);
    ConditionDestroy(&pool->ready);
}

#endif // THREAD_POOL_H_
