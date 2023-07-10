#ifndef _SEC_APP_H_
#define _SEC_APP_H_

struct CAN_CON_ST
{
    unsigned int proc_connect_err_cnt;
    unsigned char bear_switch_err_cnt;
    unsigned char ploy_switch_err_cnt;
    unsigned char proc_can_err_flag;
};

void proc_can_st_check(void);
/*************************************************
Function:    sec_thread_entry
Description: 自检和秒线程
Input:
Output:
Return:
Others:
*************************************************/
void sec_thread_entry(void);


/*************************************************
Function:    init_sec_thread
Description: 初始化自检和秒线程
Input:
Output:
Return:成功：0
	　　失败:非0
Others:
*************************************************/
int init_sec_thread(void);



/*************************************************
Function:    init_timer
Description: 初始化定时器
Input:
Output:
Return:
Others:
*************************************************/
void init_timer();



/*************************************************
Function:    timer_handler
Description: 定时器回调函数，用于需要定时的任务
Input:
Output:
Return:
Others:
*************************************************/
void timer_handler(void);


#endif
