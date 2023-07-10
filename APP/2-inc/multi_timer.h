#ifndef __MULTI_TIMER_H__
#define __MULTI_TIMER_H__
#include <stdint.h>
#include <stddef.h>
#include "sys/time.h"
#include <pthread.h>
#include <semaphore.h>
#define MAX_TIMER_UPPER_LIMIT 12
#define MULTI_BASE_TIMER_MS 10
//timer ID
#define TIMER_0 0
#define TIMER_1 1
#define TIMER_2 2 //udp超时重发调用
#define TIMER_3 3
#define TIMER_4 4
#define TIMER_5 5
#define TIMER_6 6
#define TIMER_7 7
#define TIMER_8 8
#define TIMER_9 9
#define TIMER_10 10
#define TIMER_NO_USE 11 //空闲任务调用

#define CANCEL_MODE_IMMEDIATELY 1
#define CANCEL_MODE_AFTER_NEXT_TIMEOUT 0

typedef void TimeoutCallBack(void *);

typedef int bool;
#define false 0
#define true 1

//#define DEBUG_TIMER
//========================================================
//                      timer结构定义
//========================================================
typedef struct tMultiTimer
{
    uint8_t nTimerID;    //
    uint32_t nInterval;  //定时时长
    uint32_t nTimeStamp; //时间戳
    bool bIsSingleUse;   //是否单次使用
    bool bIsOverflow;    //用于解决计数溢出问题
    TimeoutCallBack *pTimeoutCallbackfunction;
    void *pTimeoutCallbackParameter;

    //双向链表指针
    struct tMultiTimer *pNextTimer;
    struct tMultiTimer *pPreTimer;
    //相同时间戳的下一个处理函数    这里可能会有隐藏的 bug，如果基础时间中断比较快，那么可能在处理多个同一时间节点的
    //回调函数的时候被下一次的中断打断，这里会引起时序错误，
    //解决方案有三种，
    //一是可以人为避免，不设置有公约数的定时时间，这样的话同一个时刻有多个定时任务的情况就小很多；
    //二是回调函数尽量少做事，快速退出定时处理函数；
    //三是另开一个线程，这个线程仅把回调函数放到一个队列中，另一个线程持续从队列中取回调函数执行，这个是没有问题的方案，但是需要支持多线程或者多任务，并且需要注意加锁
    struct tMultiTimer *pNextHandle;

} tMultiTimer;

//========================================================
//               实现多定时任务的相关变量
//========================================================

tMultiTimer *g_pTimeoutCheckListHead;
bool g_bIs_g_nAbsoluteTimeOverFlow;
uint32_t g_nAbsoluteTime;

//========================================================
//                      外部接口
//========================================================
int set_base_timer(int ms);
void multi_timer_init(void);
uint8_t set_timer_task(uint8_t nTimerID, uint32_t nInterval, bool bIsSingleUse, TimeoutCallBack *pCallBackFunction, void *pCallBackParameter);
uint8_t cancel_timer_task(uint8_t nTimerID, uint8_t nCancelMode);
void cancel_all_mutil_timer_task();
void cancle_base_timer(void);
void select_sleep_us(unsigned long us);
void select_sleep_ms(unsigned long ms);
void select_sleep_s(unsigned long s);
int cond_timedwait_millsecs(pthread_mutex_t *mutex, pthread_cond_t *cond,long msecs);
int lock_timedwait_millsecs(pthread_mutex_t *mutex, long msecs);
int sem_timedwait_millsecs(sem_t *sem, long msecs);
#ifdef DEBUG_TIMER
void test_multi_timer(void);
#endif
#endif
