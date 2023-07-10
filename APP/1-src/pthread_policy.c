#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <assert.h>
#include "pthread_policy.h"
#include "user_data.h"

//获取当前系统线程调度策略
int api_get_thread_policy (pthread_attr_t *attr)
{
    int policy;
    int rs = pthread_attr_getschedpolicy (attr, &policy);
    assert (rs == 0);

    switch (policy)
    {
        case SCHED_FIFO:
        	DEBUG ("policy = SCHED_FIFO\r\n");
            break;
        case SCHED_RR:
        	DEBUG ("policy = SCHED_RR\r\n");
            break;
        case SCHED_OTHER:
        	DEBUG ("policy = SCHED_OTHER\r\n");
            break;
        default:
        	DEBUG ("policy = UNKNOWN\r\n");
            break;
    }
    return policy;
}



void api_show_thread_priority (pthread_attr_t *attr,int policy)
{
    int priority = sched_get_priority_max (policy);
    assert (priority != -1);
    DEBUG ("max_priority = %d\n", priority);
    priority = sched_get_priority_min (policy);
    assert (priority != -1);
    DEBUG ("min_priority = %d\n", priority);
}




int api_get_thread_priority (pthread_attr_t *attr)
{
    struct sched_param param;
    int rs = pthread_attr_getschedparam (attr, &param);
    assert (rs == 0);
    DEBUG ("priority = %d\n", param.__sched_priority);
    return param.__sched_priority;
}



void api_set_thread_policy (pthread_attr_t *attr,int policy)
{
    int rs = pthread_attr_setschedpolicy (attr, policy);
    assert (rs == 0);
    api_get_thread_policy (attr);
}


int function(void)
{
    pthread_attr_t attr;       // 线程属性
//    struct sched_param sched;  // 调度策略
    int rs;

    /*
     * 对线程属性初始化
     * 初始化完成以后，pthread_attr_t 结构所包含的结构体
     * 就是操作系统实现支持的所有线程属性的默认值
     */
    rs = pthread_attr_init (&attr);
    assert (rs == 0);     // 如果 rs 不等于 0，程序 abort() 退出

    /* 获得当前调度策略 */
    int policy = api_get_thread_policy (&attr);

    /* 显示当前调度策略的线程优先级范围 */
    DEBUG ("Show current configuration of priority\n");
    api_show_thread_priority(&attr, policy);

    /* 获取 SCHED_FIFO 策略下的线程优先级范围 */
    DEBUG ("show SCHED_FIFO of priority\n");
    api_show_thread_priority(&attr, SCHED_FIFO);

    /* 获取 SCHED_RR 策略下的线程优先级范围 */
    DEBUG ("show SCHED_RR of priority\n");
    api_show_thread_priority(&attr, SCHED_RR);

    /* 显示当前线程的优先级 */
    DEBUG ("show priority of current thread\n");
//    int priority =
    api_get_thread_priority (&attr);

    /* 手动设置调度策略 */
    DEBUG ("Set thread policy\n");

//    printf ("set SCHED_FIFO policy\n");
//    api_set_thread_policy(&attr, SCHED_FIFO);

    DEBUG ("set SCHED_RR policy\n");
    api_set_thread_policy(&attr, SCHED_RR);

    DEBUG ("Show 222222222222222 current configuration of priority\n");
    api_show_thread_priority(&attr, policy);
//
//    /* 还原之前的策略 */
//    printf ("Restore current policy\n");
//    api_set_thread_policy (&attr, policy);

    /*
     * 反初始化 pthread_attr_t 结构
     * 如果 pthread_attr_init 的实现对属性对象的内存空间是动态分配的，
     * phread_attr_destory 就会释放该内存空间
     */
    rs = pthread_attr_destroy (&attr);
    assert (rs == 0);

    return 0;
}
