#if !defined(TASK_POOL_H_)
#define TASK_POOL_H_

#include <string.h>

#include "task.h"

struct TaskPool
{
    Task *head_node;
    Task *end_node;
    int parallel_size;
    char **lock_categoties;
};

//初始化任务池
void TaskPoolInit(TaskPool *pool, int size)
{   
    pool->head_node = nullptr;
    pool->end_node = nullptr;
    pool->parallel_size = size;
    pool->lock_categoties = new char*[size];
    for (int i = 0; i < size; i++)
    {
        pool->lock_categoties[i] = nullptr;
    }
}

//添加任务到任务池
void AddTask(Task *task, TaskPool *pool)
{
    if (pool) {
        if (pool->head_node == nullptr) {
            pool->head_node = task;
        } else {
            pool->end_node->next = task;
        }
        pool->end_node = task;
    }
}

//移除被锁定的类别
void RemoveLockCategory(const char *category, TaskPool *pool)
{
    if (pool) {
        for (size_t i = 0; i < pool->parallel_size; i++)
        {
            if (pool->lock_categoties[i] && strcmp(pool->lock_categoties[i], category) == 0) {
                pool->lock_categoties[i] = nullptr;
                break;
            }
        }
    }
}

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
        for (int i = 0; i < pool->parallel_size; i++)
        {
            if (pool->lock_categoties[i]) {
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

//清空任务池
void CleanTaskPool(TaskPool *pool)
{
    if (pool)
    {
        Task *task = pool->head_node;
        while (task)
        {
            Task *tmp = task->next;
            delete task;
            task = tmp;
        }
        pool->head_node = nullptr;
        pool->end_node = nullptr;
    }
}


#endif // TASK_POOL_H_
