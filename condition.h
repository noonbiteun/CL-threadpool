/*
 *封装一个互斥量和条件变量作为状态
*/
#if !defined(CONDITION_H_)
#define CONDITION_H_

#include <pthread.h>

struct Condition
{
    pthread_mutex_t pmutex;
    pthread_cond_t pcond;
};

//初始化
int ConditionInit(Condition *cond)
{
    int status;
    if((status = pthread_mutex_init(&cond->pmutex, NULL)))
        return status;
    
    if((status = pthread_cond_init(&cond->pcond, NULL)))
        return status;
    
    return 0;
}

//加锁
int ConditionLock(Condition *cond)
{
    return pthread_mutex_lock(&cond->pmutex);
}

//解锁
int ConditionUnlock(Condition *cond)
{
    return pthread_mutex_unlock(&cond->pmutex);
}

//等待
int ConditionWait(Condition *cond)
{
    return pthread_cond_wait(&cond->pcond, &cond->pmutex);
}

//固定时间等待
int ConditionTimedwait(Condition *cond, const struct timespec *abstime)
{
    return pthread_cond_timedwait(&cond->pcond, &cond->pmutex, abstime);
}

//唤醒一个睡眠线程
int ConditionSignal(Condition* cond)
{
    return pthread_cond_signal(&cond->pcond);
}

//唤醒所有睡眠线程
int ConditionBroadcast(Condition *cond)
{
    return pthread_cond_broadcast(&cond->pcond);
}

//释放
int ConditionDestroy(Condition *cond)
{
    int status;
    if((status = pthread_mutex_destroy(&cond->pmutex)))
        return status;
    
    if((status = pthread_cond_destroy(&cond->pcond)))
        return status;
        
    return 0;
}

#endif // CONDITION_H_
