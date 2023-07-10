#ifndef _WATCHDOG_H_
#define _WATCHDOG_H_

/*************************************************
Function:  init_watchdog
Description:  初始化看门狗配置
Input: 　看门狗溢出时间
Output: 成功０，失败１
Return: 成功０，失败１
Others:
*************************************************/
int init_watchdog(int over_time);
void init_hw_watchdog(void);

/*************************************************
Function:  feed_dog
Description:  看门狗喂狗程序
Input: 　无
Output: 成功０，失败１
Return: 成功０，失败１
Others:
*************************************************/
int feed_dog(void);

#endif
