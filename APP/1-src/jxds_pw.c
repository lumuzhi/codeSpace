/*
 ============================================================================
 Name        : LH-TFDSE-CLB
 Author      : liushuang
 Version     : v1.0.0.0
 Copyright   : Your copyright notice
 Description : lh tfdse in C, Ansi-style
 ============================================================================
 PIS----ETH0---外部 ETH1
 NET----ETH1---外部 ETH3
 PTU----ETH2---外部 ETH2
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <poll.h>
#include <fcntl.h>
#include "sec_app.h"
#include "pw_diagnos.h"
#include "user_data.h"
#include "watchdog.h"
#include "led.h"
#include "hdd_save.h"
#include "ptu_app.h"
#include "wtd_app.h"
#include "sw_diagnos.h"
#ifdef DATA_CONVERT_TXT_TO_BIN
#include "data_test.h"
#endif

sem_t test_sem;

int main(void)
{
	int ret = -1;

	/****************看门狗初始化******************************/
	init_hw_watchdog();
	init_watchdog(40); //看门狗溢出时间30s

	init_led_gpio();
	led_psw(0);
	init_local_time();	//初始化系统当前时间
	init_pw_save();		//创建初始文件夹
	init_user_data();   //获取配置参数，新建SD卡文件

	printf("#### 2023-01-06-16:07\n");

	/***************初始化线程初始化************************/
	ret = init_sec_thread();
	if(ret != 0)
	{
		DEBUG("sec thread init failed!\r\n");
	}

	/*************初始化平稳诊断线程************************/
	ret = init_pw_diagnosis_thread();
	if (ret != 0)
	{
		DEBUG("pw diagnosis init failed!\n");
	}


	/*************初始化失稳诊断线程***************************/
	ret = init_sw_diagnosis_thread();
	if (ret != 0)
	{
		DEBUG("sw diagnosis init failed!\n");
	}
//#ifdef WTD_DATA_TRANSLATE_PROTOCOL
//	/*************WTD数据落地************************/
//	ret = init_wtd_thread();
//	if (ret != 0)
//	{
//		DEBUG("init_wtd_thread failed!\n");
//	}
//#endif




	/************PTU线程初始化****************************/
	//POWER_BRANCH1 ERR
//	ret = init_ptu_thread();
//	if (ret !=0)
//	{
//		DEBUG("ptu init failed!\n");
//	}
//
//
//
//
//	/************数据删除线程****************************/
	ret = init_file_del_thread();
	if(ret != 0)
	{
		DEBUG("init_file_del_thread!\n");
	}

#ifdef DATA_CONVERT_TXT_TO_BIN
	/*************初始化数据测试线程************************/
	ret = init_data_test_thread();
	if (ret != 0)
	{
		DEBUG("init_data_test_thread failed!\n");
	}
#endif

	sem_init(&test_sem,0,0);
	while(1)
	{
		sem_wait(&test_sem);
	}

	return 0;
}

