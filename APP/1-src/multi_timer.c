#include "multi_timer.h"
#include <signal.h>
#include "pthread.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <unistd.h>
#include <time.h>
#include "sys/time.h"
#include "time.h"
#include <signal.h>
#include "assert.h"
#include "user_data.h"



#define CDebugAssert assert

/**
 *定义一个数组可以指针，每个g_aSPPMultiTimer[i]，都指向一个tMultiTimer指针
 */
tMultiTimer *g_aSPPMultiTimer[MAX_TIMER_UPPER_LIMIT];
unsigned int g_aTimerID[MAX_TIMER_UPPER_LIMIT] = {TIMER_0, TIMER_1, TIMER_2, TIMER_3, TIMER_4, TIMER_5, TIMER_6, TIMER_7, TIMER_8, TIMER_9, TIMER_10, TIMER_NO_USE};

static void base_timer_handler(int signo);
/**
 * @function    把一个定时任务添加到定时检测链表中
 * @parameter   一个定时器对象，可以由全局变量 g_aSPPMultiTimer 通过 TIMER_ID 映射得到
*/
static void AddTimerToCheckList(tMultiTimer *pTimer)
{
    tMultiTimer *pEarliestTimer = NULL;
    tMultiTimer *pEarliestTimer_pre = NULL;

    CDebugAssert(pTimer->nInterval != 0);

    pTimer->nTimeStamp = g_nAbsoluteTime + pTimer->nInterval;
    if (pTimer->nTimeStamp < g_nAbsoluteTime)
        pTimer->bIsOverflow = !(pTimer->bIsOverflow);
    if (g_pTimeoutCheckListHead == NULL)
    {
        g_pTimeoutCheckListHead = pTimer;
        g_pTimeoutCheckListHead->pNextTimer = NULL;
        g_pTimeoutCheckListHead->pPreTimer = NULL;
        g_pTimeoutCheckListHead->pNextHandle = NULL;
        return;
    }
    else
    {
        pEarliestTimer = g_pTimeoutCheckListHead;
        while (pEarliestTimer != NULL)
        {
            //如果超时时间小于新加的timer则直接跳过；
            if ((pEarliestTimer->bIsOverflow != pTimer->bIsOverflow) || (pEarliestTimer->nTimeStamp < pTimer->nTimeStamp))
            {
                pEarliestTimer_pre = pEarliestTimer;
                pEarliestTimer = pEarliestTimer->pNextTimer;
            }
            else
            {
                if (pEarliestTimer->nTimeStamp == pTimer->nTimeStamp) //超时时刻相等，直接添加到相同时刻处理列表的列表头
                {
                    pTimer->pNextHandle = pEarliestTimer->pNextHandle;
                    pEarliestTimer->pNextHandle = pTimer;
                    return;
                }
                else //找到了超时时刻大于新加入timer的第一个节点
                {
                    if (pEarliestTimer->pPreTimer == NULL) //新加入的是最早到达超时时刻的，添加到链表头
                    {
                        pEarliestTimer->pPreTimer = pTimer;
                        pTimer->pNextTimer = pEarliestTimer;
                        pTimer->pPreTimer = NULL;
                        pTimer->pNextHandle = NULL;
                        g_pTimeoutCheckListHead = pTimer;
                        return;
                    }
                    else //中间节点
                    {
                        pEarliestTimer->pPreTimer->pNextTimer = pTimer;
                        pTimer->pNextTimer = pEarliestTimer;
                        pTimer->pPreTimer = pEarliestTimer->pPreTimer;
                        pEarliestTimer->pPreTimer = pTimer;
                        pTimer->pNextHandle = NULL;
                        return;
                    }
                }
            }
        }
        if (pEarliestTimer == NULL) //新加入的timer超时时间是最晚的那个
        {
            pEarliestTimer_pre->pNextTimer = pTimer;
            pTimer->pPreTimer = pEarliestTimer_pre;
            pTimer->pNextTimer = NULL;
            pTimer->pNextHandle = NULL;
        }
        return;
    }
}

/**
 * @function    设置一个定时任务，指定超时间隔与回调函数，当超时到来，自动执行回调
 * @parameter1  TIMER_ID
 * @parameter2  超时间隔时间
 * @parameter3  是否是一次性定时任务
 * @parameter4  回调函数，注意，回调函数的函数形式  void function(void*);
 * @parameter5  void* 回调函数的参数，建议用结构体强转成 void*，在回调函数中再强转回来
 * @return      错误码
*/
uint8_t set_timer_task(uint8_t nTimerID, uint32_t nInterval, bool bIsSingleUse, TimeoutCallBack *pCallBackFunction, void *pCallBackParameter)
{
//    printf("\nset timer %d\n", nTimerID);
    tMultiTimer *pChoosedTimer = NULL;
    pChoosedTimer = g_aSPPMultiTimer[nTimerID];
    pChoosedTimer->nInterval = nInterval;
    pChoosedTimer->bIsSingleUse = bIsSingleUse;
    pChoosedTimer->pTimeoutCallbackfunction = pCallBackFunction;
    pChoosedTimer->pTimeoutCallbackParameter = pCallBackParameter;

    //如果超时任务链表中已经有这个任务了，先取消，然后再设置，即重置超时任务
    if (pChoosedTimer->pNextTimer != NULL || pChoosedTimer->pPreTimer != NULL)
        cancel_timer_task(nTimerID, CANCEL_MODE_IMMEDIATELY);

    AddTimerToCheckList(pChoosedTimer);
    return 0;
}

/**
 *空函数
 */
static void idle_timer_handle(void)
{
}

/**
 *  增加默认的空定时器函数，多定时器启动之后必须至少包含一个定时器任务，此任务不可删除
 */
static void set_idle_timer_task(void)
{
    int ret;
    set_timer_task(TIMER_NO_USE, 1000000, 0, (TimeoutCallBack *)idle_timer_handle, &ret);
}

/**
 * @function    取消超时检测链表中的指定超时任务
 * @parameter1  要取消的超时任务的ID
 * @parameter2  模式选择，是立即取消，还是下次执行后取消
 * @return      错误码
*/
uint8_t cancel_timer_task(uint8_t nTimerID, uint8_t nCancelMode)
{
//    printf("\ncancle timer %d\n", nTimerID);
    tMultiTimer *pEarliestTimer = NULL;
    tMultiTimer *pHandleTimer = NULL;
    tMultiTimer *pHandleTimer_pre = NULL;
    tMultiTimer *pChoosedTimer = NULL;

    pEarliestTimer = g_pTimeoutCheckListHead;
    pChoosedTimer = g_aSPPMultiTimer[nTimerID];

    if (nCancelMode == CANCEL_MODE_IMMEDIATELY)
    {
        while (pEarliestTimer != NULL)
        {
            pHandleTimer = pEarliestTimer;
            pHandleTimer_pre = NULL;
            while (pHandleTimer != NULL)
            {
                if (pHandleTimer->nTimerID == nTimerID)
                {
                    if (pHandleTimer_pre == NULL)
                    {
                        if (pHandleTimer->pNextHandle != NULL)
                        {
                            pEarliestTimer = pHandleTimer->pNextHandle;
                            pEarliestTimer->pPreTimer = pHandleTimer->pPreTimer;
                            if (pHandleTimer->pPreTimer != NULL)
                                pHandleTimer->pPreTimer->pNextTimer = pEarliestTimer;
                            pEarliestTimer->pNextTimer = pHandleTimer->pNextTimer;
                            if (pHandleTimer->pNextTimer != NULL)
                                pHandleTimer->pNextTimer->pPreTimer = pEarliestTimer;
                            pHandleTimer->pNextTimer = NULL;
                            pHandleTimer->pPreTimer = NULL;
                            pHandleTimer->pNextHandle = NULL;
                        }
                        else
                        {
                            if (pEarliestTimer->pPreTimer == NULL)
                            {
                                g_pTimeoutCheckListHead = pEarliestTimer->pNextTimer;
                                g_pTimeoutCheckListHead->pPreTimer = NULL;
                                pEarliestTimer->pNextTimer = NULL;
                            }
                            else if (pEarliestTimer->pNextTimer == NULL)
                            {
                                pEarliestTimer->pPreTimer->pNextTimer = NULL;
                                pEarliestTimer->pPreTimer = NULL;
                            }
                            else
                            {
                                pEarliestTimer->pPreTimer->pNextTimer = pEarliestTimer->pNextTimer;
                                pEarliestTimer->pNextTimer->pPreTimer = pEarliestTimer->pPreTimer;
                                pEarliestTimer->pPreTimer = NULL;
                                pEarliestTimer->pNextTimer = NULL;
                            }
                        }
                    }
                    else
                    {
                        pHandleTimer_pre->pNextHandle = pHandleTimer->pNextHandle;
                        pHandleTimer->pNextHandle = NULL;
                    }
                    return 0;
                }
                else
                {
                    pHandleTimer_pre = pHandleTimer;
                    pHandleTimer = pHandleTimer_pre->pNextHandle;
                }
            }
            pEarliestTimer = pEarliestTimer->pNextTimer;
        }
#ifdef DEBUG_PRINTF
        DEBUG("\nThere is no this timer task!\n");
#endif
        return 2; //出错，超时检测链表中没有这个超时任务
    }
    else if (nCancelMode == CANCEL_MODE_AFTER_NEXT_TIMEOUT)
    {
        pChoosedTimer->bIsSingleUse = true;
        return 0;
    }
    else
    {
        return 1; //出错，模式错误，不认识该模式
    }
}
/**
 * @function    定时器处理函数，用于检测是否有定时任务超时，如果有则调用该定时任务的回调函数，并更新超时检测链表
 *              更新动作：如果超时的那个定时任务不是一次性的，则将新的节点加入到检测超时链表中，否则直接删掉该节点；
 * @parameter
 * @return
*/
void base_timer_handler(int signo)
{
    // printf("%d:enter base_timer_handler\n",g_nAbsoluteTime);
    if (signo != SIGALRM)
        return;
    tMultiTimer *pEarliestTimer = NULL;
    tMultiTimer *pWaitingToHandle = NULL;
    tMultiTimer *pEarliestTimerPreHandle = NULL;

    if (g_pTimeoutCheckListHead != NULL)
    {
        if ((g_pTimeoutCheckListHead->nTimeStamp <= g_nAbsoluteTime) && (g_pTimeoutCheckListHead->bIsOverflow == g_bIs_g_nAbsoluteTimeOverFlow))
        {
            pWaitingToHandle = g_pTimeoutCheckListHead;
            g_pTimeoutCheckListHead = g_pTimeoutCheckListHead->pNextTimer;
            if (g_pTimeoutCheckListHead != NULL)
                g_pTimeoutCheckListHead->pPreTimer = NULL;
            pWaitingToHandle->pNextTimer = NULL;

            pEarliestTimer = pWaitingToHandle;
            while (pEarliestTimer != NULL)
            {
                pEarliestTimerPreHandle = pEarliestTimer;
                pEarliestTimer = pEarliestTimer->pNextHandle;
                pEarliestTimerPreHandle->pNextHandle = NULL;
                pEarliestTimerPreHandle->pNextTimer = NULL;
                pEarliestTimerPreHandle->pPreTimer = NULL;
                pEarliestTimerPreHandle->pTimeoutCallbackfunction(pEarliestTimerPreHandle->pTimeoutCallbackParameter);
                if (!(pEarliestTimerPreHandle->bIsSingleUse))
                    AddTimerToCheckList(pEarliestTimerPreHandle);
            }
        }
    }

    g_nAbsoluteTime++;
    if (g_nAbsoluteTime == 0)
        g_bIs_g_nAbsoluteTimeOverFlow = !g_bIs_g_nAbsoluteTimeOverFlow;

    return;
}

void cancel_all_mutil_timer_task()
{
    tMultiTimer *pEarliestTimer = NULL;
    tMultiTimer *pHandleTimer = NULL;

    while (g_pTimeoutCheckListHead != NULL)
    {
        pEarliestTimer = g_pTimeoutCheckListHead;
        g_pTimeoutCheckListHead = g_pTimeoutCheckListHead->pNextTimer;

        while (pEarliestTimer != NULL)
        {
            pHandleTimer = pEarliestTimer;
            pEarliestTimer = pEarliestTimer->pNextHandle;

            pHandleTimer->pNextHandle = NULL;
            pHandleTimer->pNextTimer = NULL;
            pHandleTimer->pPreTimer = NULL;
            pHandleTimer->bIsOverflow = false;
        }
    }
    g_bIs_g_nAbsoluteTimeOverFlow = false;
    g_nAbsoluteTime = 0;
    return;
}

void multi_timer_init(void)
{
    g_pTimeoutCheckListHead = NULL;
    g_bIs_g_nAbsoluteTimeOverFlow = false;
    g_nAbsoluteTime = 0;
    for (uint8_t index = 0; index < MAX_TIMER_UPPER_LIMIT; index++)
    {
        g_aSPPMultiTimer[index] = (tMultiTimer *)malloc(sizeof(tMultiTimer));
        if (g_aSPPMultiTimer[index])
        {
            g_aSPPMultiTimer[index]->nTimerID = g_aTimerID[index];
            g_aSPPMultiTimer[index]->nInterval = 0;
            g_aSPPMultiTimer[index]->nTimeStamp = 0;
            g_aSPPMultiTimer[index]->bIsSingleUse = true;
            g_aSPPMultiTimer[index]->bIsOverflow = false;
            g_aSPPMultiTimer[index]->pTimeoutCallbackfunction = NULL;
            g_aSPPMultiTimer[index]->pTimeoutCallbackParameter = NULL;
            g_aSPPMultiTimer[index]->pNextTimer = NULL;
            g_aSPPMultiTimer[index]->pPreTimer = NULL;
            g_aSPPMultiTimer[index]->pNextHandle = NULL;
        }
        else
        {
            perror("muti_timer malloc err");
            return;
        }
    }
    set_idle_timer_task();
    signal(SIGALRM, base_timer_handler);

#ifdef DEBUG_TIMER
    test_multi_timer();
#endif
}

/**
 * 取消定时器的时间基
 */
void cancle_base_timer(void)
{
    struct itimerval itv, oldtv;
    itv.it_interval.tv_sec = 0;
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec = 0;
    itv.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, &oldtv);
}

/**
 * 设置定时器的时间基，单位ms
 * 返回值：0成功，1失败
 */
int set_base_timer(int ms)
{
    if (ms <= 0)
    {
        return -1;
    }

    struct itimerval itv, oldtv;
    itv.it_interval.tv_sec = 0;
    itv.it_interval.tv_usec = 1000 * ms;
    itv.it_value.tv_sec = 0;
    itv.it_value.tv_usec = 1000 * ms;
    if (setitimer(ITIMER_REAL, &itv, &oldtv) == 0)
    {
//        printf("ok\n");
        return 0;
    }
    else
    {
//        printf("err\n");
        return -2;
    }
}

/*
 *线程安全的延时方式：select实现sleep_us
 **/
void select_sleep_us(unsigned long us)
{
	struct timeval t;
	t.tv_sec=us/1000000;
	t.tv_usec=us%1000000;
	select(0,NULL,NULL,NULL,&t);
}

/*
 *线程安全的延时方式：select实现sleep_ms
 **/
void select_sleep_ms(unsigned long ms)
{
	struct timeval t;
	t.tv_sec=ms/1000;
	t.tv_usec=(ms%1000)*1000;
	select(0,NULL,NULL,NULL,&t);
}

/*
 *线程安全的延时方式：select实现sleep_ms
 **/
void select_sleep_s(unsigned long s)
{
	struct timeval t;
	t.tv_sec=s;
	t.tv_usec=0;
	select(0,NULL,NULL,NULL,&t);
}

/**
 * 信号量超时等待:输入为信号量、毫秒
 */
int sem_timedwait_millsecs(sem_t *sem, long msecs)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long secs = msecs / 1000;
    msecs = msecs % 1000;
    long add = 0;
    msecs = msecs * 1000 * 1000 + ts.tv_nsec;
    add = msecs / (1000 * 1000 * 1000);
    ts.tv_sec += (add + secs);
    ts.tv_nsec = msecs % (1000 * 1000 * 1000);
    return sem_timedwait(sem, &ts);
}

/**
 * 锁超时等待:输入为锁、毫秒
 */
int lock_timedwait_millsecs(pthread_mutex_t *mutex, long msecs)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long secs = msecs / 1000;
    msecs = msecs % 1000;
    long add = 0;
    msecs = msecs * 1000 * 1000 + ts.tv_nsec;
    add = msecs / (1000 * 1000 * 1000);
    ts.tv_sec += (add + secs);
    ts.tv_nsec = msecs % (1000 * 1000 * 1000);
    return pthread_mutex_timedlock(mutex,&ts);
}

/**
 * 条件变量超时等待:输入为锁、条件变量，毫秒
 */
int cond_timedwait_millsecs(pthread_mutex_t *mutex, pthread_cond_t *cond,long msecs)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long secs = msecs / 1000;
    msecs = msecs % 1000;
    long add = 0;
    msecs = msecs * 1000 * 1000 + ts.tv_nsec;
    add = msecs / (1000 * 1000 * 1000);
    ts.tv_sec += (add + secs);
    ts.tv_nsec = msecs % (1000 * 1000 * 1000);
    return pthread_cond_timedwait(cond,mutex, &ts);
}


// //  sigset_t g_sigset_mask;
// void *MultiTimer_thread(void *parameter)
// {
//     int err, signo;
//     // struct itimerval new_time_value, old_time_value;

//     // new_time_value.it_interval.tv_sec = 0;
//     // new_time_value.it_interval.tv_usec = 1000;
//     // new_time_value.it_value.tv_sec = 0;
//     // new_time_value.it_value.tv_usec = 1;
//     // setitimer(ITIMER_REAL, &new_time_value, NULL);

//     set_base_timer(10);
//     multi_timer_init();
//     // for (;;)
//     // {
//     //     err = sigwait(&g_sigset_mask, &signo); //信号捕捉
//     //     if (err != 0)
//     //     {
//     //         return;
//     //     }

//     //     if (signo == SIGALRM)
//     //     {
//     //         printf("com in\n");
//     //         base_timer_handler(signo); //SYSTimeoutHandler(signo);
//     //     }
//     // }
//     return ((void *)0);
// }

 /**
  *
  */
// void testTimerSign()
// {
//     struct sigevent evp;
//     struct itimerspec ts;
//     timer_t timer;
//     int ret;
//     evp.sigev_value.sival_ptr = &timer;
//     evp.sigev_notify = SIGEV_SIGNAL;
//     evp.sigev_signo = SIGUSR1;
//     // signal(evp.sigev_signo, SignHandler);
//     //ret = timer_create(ITIMER_REAL, &evp, &timer);
//     if (ret)
//     {
//         perror("timer_create");
//     }
//     ts.it_interval.tv_sec = 1;
//     ts.it_interval.tv_nsec = 0;
//     ts.it_value.tv_sec = 1;
//     ts.it_value.tv_nsec = 0;
//     // printTime();
//     DEBUG("start\n");
//     //ret = timer_settime(timer, 0, &ts, NULL);
//     if (ret)
//     {
//         perror("timer_settime");
//     }
// }

#ifdef DEBUG_TIMER
 void sigalrm_handler0(int *sig)
 {
     static int i = 0;
     DEBUG("    %d we0\n ", ++i);
 }

 void sigalrm_handler1(int *sig)
 {
     static int s = 0;
//     printf("        %d he1\n ", ++s);
 }

void thread1_entry(void)
{
    int a;
    static int cnt=0;
    set_timer_task(TIMER_0, 30, 0, sigalrm_handler0, &a);
	 while (1)
	 {
		 select_sleep_ms(1000);
//		 printf("                			%d t1\n", ++cnt);
	 }
}

 void thread2_entry(void)
 {
     int b;
     set_timer_task(TIMER_1, 5, 0, sigalrm_handler1, &b);
     static int cnt = 0;
     while (1)
     {
         cnt++;
         select_sleep_ms(1000);
         if (cnt == 5)
         {
//             cancel_timer_task(TIMER_1, CANCEL_MODE_IMMEDIATELY);
         }
         else if (cnt == 10)
         {
//             cancel_timer_task(TIMER_0, CANCEL_MODE_IMMEDIATELY);
         }

//         printf("                %d t2\n", cnt);
     }
 }
void test_multi_timer(void)
{
	pthread_t thread1, thread2, thread3;
	int ret1, ret2, ret3;
    ret1 = pthread_create(&thread1, NULL, (void *)&thread1_entry, NULL);
    ret2 = pthread_create(&thread2, NULL, (void *)&thread2_entry, NULL);
}
#endif
