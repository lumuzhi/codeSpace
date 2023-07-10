#ifndef _PTHREAD_POLICY_H
#define _PTHREAD_POLICY_H
#include <pthread.h>
#include <sched.h>

int api_get_thread_policy (pthread_attr_t *attr);
void api_show_thread_priority (pthread_attr_t *attr,int policy);
int api_get_thread_priority (pthread_attr_t *attr);
void api_set_thread_policy (pthread_attr_t *attr,int policy);
int function(void);


#endif
