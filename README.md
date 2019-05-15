# CL-threadpool

## 写在前面 ##

该线程池适用于**某些特定任务**需要**顺序执行**的特殊场景，即并发执行大量的具有相关性的任务的场景

例如：程序维持了一个**数据缓存池**（注意不是缓冲区，数据缓存池可以理解为多个缓冲区的合体），程序会对数据缓存池内的数据进行读写；有多个任务都是对池内的同一块缓存进行读写操作，那么任务就必须要顺序执行，否则就可能产生数据错误

---

## 设计思路 ##

1. 对任务进行分类，给定一个分类标识，同类任务不可已并发执行
2. 线程池内维持一个任务池，所有添加的任务都链到任务池
3. 每次工作线程取任务的时候，任务池会弹出**未被锁定的类别的排在最前面**的任务，之后将该类别锁定
4. 被锁定的类别的任务正在执行时，同类别的其他任务不会被任务池弹出

## 代码解释 ##

**任务池关键代码**

```C++
//任务池数据结构
struct TaskPool
{
    Task *head_node;            //头节点
    Task *end_node;             //尾节点
    int parallel_size;          //并行数量
    char **lock_categoties;     //已锁定的类别
};


//取出未被锁定类别的任务
Task* PopTask(TaskPool *pool)
{
    if (pool == nullptr)
    {
        return nullptr;
    }
    Task *task = pool->head_node;
    Task *before = nullptr;
    while (task != nullptr)
    {
        bool isNext = false;
        //检查是否被锁定
        for (int i = 0; i < pool->parallel_size; i++)
        {
            if (pool->lock_categoties[i]) {
                //已被锁定，直接退出，取下一个节点检查
                if (strcmp(pool->lock_categoties[i], task->category) == 0) {
                    isNext = true;
                    break;
                }
            }
        }
        if (isNext) {
            before = task;
            task = task->next;
            continue;
        } else {
            //未被锁定，取出该节点
            if (before) {
                before->next = task->next;
                if (task == pool->end_node) {
                    pool->end_node = before;
                }
            } else {
                pool->head_node = task->next;
            }
            //锁定类别
            for (size_t i = 0; i < pool->parallel_size; i++)
            {
                if (pool->lock_categoties[i] == nullptr)
                {
                    pool->lock_categoties[i] = task->category;
                    break;
                }
            }
            return task;
        }
    }
    return task;
}
```

**线程池关键代码**

```C++
//工作线程执行的函数
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
                //被主线程唤醒进行退出操作
                if (pool->quit) {
                    break;
                }
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
```

**总结**

该轮子其实和其他的线程池轮子没有太大的区别，只不过是在取任务的步骤上做了一些改动，使其能适用于一些特定的场景了。如有疑问和指教，欢迎issue