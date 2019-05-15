/*
 *线程池执行任务
*/
#if !defined(TASK_H_)
#define TASK_H_

struct Task
{
    char *category;             //同类任务互斥
    void *(*run)(void *args);   //函数指针，需要执行的任务
    void *arg;                  //参数
    struct Task *next;          //任务队列中下一个任务
};


#endif // TASK_H_
