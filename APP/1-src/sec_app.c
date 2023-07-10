#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <signal.h>
#include <sys/time.h>
#include <semaphore.h>
#include <linux/can.h>
#include <sys/socket.h>
#include <net/if.h>
#include <signal.h>
#include <linux/can/raw.h>
#include "user_data.h"
#include "pthread_policy.h"
#include "update.h"
#include "sec_app.h"
#include "led.h"
#include "self_test.h"
#include "hdd_save.h"
#include "can_config.h"
#include "board.h"
#include "udp_client.h"
#include "ptu_app.h"
#include "ad7606.h"
#include "watchdog.h"

#define SDK_CHECK_TIME  600

#define USB_NET  "/sys/bus/usb/devices/1-1"


/*--------------------------- private para -------------------*/

extern struct STORE_FLAG store_flag;
extern struct PW_TZ_DATA pw_tz_data;
extern struct SW_TZ_DATA sw_tz_data;
extern struct ETH_STATUS eth_status[16];
extern struct CAN_STATUS can_status[16];
//extern struct PW_SIMULATION pw_simulation;
extern struct PTU_DATA ptu_data;
extern struct SELF_TEST_PARA self_test_para;
extern struct SELF_TEST_PARA self_test_para1;
extern struct FILE_OPERTE file_operate;
extern struct SYS_STATUS_CNT sys_status_cnt;//设备状态计数

uint32_t sec_cnt = 0;
//extern void signal_off(void);

void init_timer();
extern void show_led_ctrl(void);
extern sem_t delete_data_dir_sem; 			//数据删除信号量
sem_t self_test_sw_sem;
#ifdef NEW_DAY_RECTEATE_DIR
extern struct LOCAL_TIME old_dir_time;
#endif

#ifdef WTD_DATA_TRANSLATE_PROTOCOL
	extern sem_t send_wtd_sem;
#endif



void check_board_err()//is not called
{
	uint8_t temp_board_err_flag = 0;

//	printf("check_board_err---eth_status[BIT_CTRLA]:%x, eth_status[BIT_CTRLB]:%x, eth_status[BIT_SAVE]:%x, hdd_err_flag:%x, save_err:%x\n",
//			eth_status[BIT_CTRLA].connect_flag, eth_status[BIT_CTRLB].connect_flag, eth_status[BIT_SAVE].connect_flag, store_flag.hdd_err_flag, pw_tz_data.borad_err.bits.save_err);

	if(eth_status[BIT_CTRLA].connect_flag && eth_status[BIT_CTRLB].connect_flag && eth_status[BIT_SAVE].connect_flag)//&& comm_type.ctrlA_CAN.st_flag.err_flag == 1 &&comm_type.ctrlB_CAN.st_flag.err_flag == 1 && )
	{
		temp_board_err_flag = 1;
		//板卡内部以太网故障
	}

	if(can_status[BIT_CTRLA].connect_flag && can_status[BIT_CTRLB].connect_flag && can_status[BIT_SAVE].connect_flag)//&& comm_type.ctrlA_CAN.st_flag.err_flag == 1 &&comm_type.ctrlB_CAN.st_flag.err_flag == 1 && )
	{
		temp_board_err_flag = 1;
		//板卡内部CAN故障
	}

	if(store_flag.hdd_err_flag || pw_tz_data.borad_err.bits.save_err )//&& comm_type.ctrlA_CAN.st_flag.err_flag == 1 &&comm_type.ctrlB_CAN.st_flag.err_flag == 1 && )
	{
		temp_board_err_flag = 1;
		//板卡存储故障
	}

	if(pw_board_st.bits.power1_err || pw_board_st.bits.power2_err)
	{
		temp_board_err_flag = 1;
		//电源支路故障
	}

	if(temp_board_err_flag == 1)
	{
		//temp_board_err_flag = 1;
		if(sys_status_cnt.board_err_save_flag == 0)
		{
			//if 条件 sys_status_cnt.ctrla_can_err_save_flag == 1 目的是为了在can通信正常情况下只存储一次
//			sprintf(log_status,"ok");
//			sprintf(log_kind,"ctrla can");
//			sprintf(log_detail,"ctrla can ok");
//			write_log(log_status,log_kind,log_detail,log_file_name);
			sys_status_cnt.board_err_save_flag = 1;
			sys_status_cnt.board_normal_save_flag = 0;
		}

	}
	else
	{
		if(sys_status_cnt.board_normal_save_flag == 0)
		{
					//if 条件 sys_status_cnt.ctrla_can_err_save_flag == 1 目的是为了在can通信正常情况下只存储一次
		//			sprintf(log_status,"ok");
		//			sprintf(log_kind,"ctrla can");
		//			sprintf(log_detail,"ctrla can ok");
		//			write_log(log_status,log_kind,log_detail,log_file_name);
			sys_status_cnt.board_err_save_flag = 0;
			sys_status_cnt.board_normal_save_flag = 1;
		}
	}
}

void board_err_deal(void)
{
	extern struct STORE_FLAG store_flag;
	sd_exist_test();
//	printf("board_err_deal---store_flag.hdd_exist_flag:%d\n",store_flag.hdd_exist_flag);
	if(store_flag.hdd_exist_flag==0)
	{
		pw_tz_data.borad_err.bits.save_err=1;

		//故障记录
//		sys_status_cnt.save_err_cnt ++;
//		sys_status_cnt.ctrlb_eth_normal_cnt = 0;
		if(sys_status_cnt.save_err_save_flag == 0)
		{
//			sprintf(log_status,"err");
//			sprintf(log_kind,"save err");
//			sprintf(log_detail,"save status error");
//			write_log(log_status,log_kind,log_detail,log_file_name);
			sys_status_cnt.save_err_save_flag = 1;
			sys_status_cnt.ctrlb_eth_normal_save_flag = 0;
			//wirte_err_log_data(ERR,SAVE_ERR);
		}
	}
	else
	{
		pw_tz_data.borad_err.bits.save_err=0;

		//解除故障时记录
//		sys_status_cnt.save_normal_cnt ++;
		if(sys_status_cnt.save_err_save_flag == 1 && sys_status_cnt.ctrlb_eth_normal_save_flag == 0)
		{
//			sprintf(log_status,"ok");
//			sprintf(log_kind,"save ok");
//			sprintf(log_detail,"save status ok");
//			write_log(log_status,log_kind,log_detail,log_file_name);

			sys_status_cnt.ctrlb_eth_normal_save_flag = 1;
			sys_status_cnt.save_err_save_flag = 0;
			//wirte_err_log_data(OK,SAVE_ERR_REMOVE);

		}
		//sys_status_cnt.save_err_cnt  = 0;
	}

}

/*************************************************
Function:    sec_thread_entry
Description: 自检和秒线程
Input:
Output:
Return:
Others:
*************************************************/
void sec_thread_entry()
{

#ifdef NEW_DAY_RECTEATE_DIR
	struct LOCAL_TIME time_now;
#endif


	start_ad_sample();
	init_udp();
//	init_udp_send_addr();
	init_sensor_gpio();
//	init_eth_status();
//	init_can_status();
//	adg_408_gpio_init();
//	init_led_status();
	init_timer();
	power_gpio_init();


	sem_init(&self_test_sw_sem,0,0);

	while(1)
	{

//		input_handler_test();
		sec_cnt ++;
		if(sec_cnt%10==0)
		{
#ifdef HISTORY_FEED_WATCHDOG
			feed_dog();
#endif
			get_memoccupy();
		}
		board_err_deal();  // 待定
//		if(sec_cnt == 5)
//			init_pw_save();

#ifdef NEW_DAY_RECTEATE_DIR
		get_local_time(&time_now);

		if(time_now.day != old_dir_time.day)
		{
			file_operate.time_update_flag = 1;		//文件重建标志
			printf("new day!\n");
		}
#endif

		//对时操作标志,对时完成后，发送删除数据命令
		if(file_operate.time_update_flag == 1 && sec_cnt>5)	//添加延时，是为了让信号量初始化成功后再进入
		{
			reset_pw_file();

			if(file_operate.file_delete_flag == 0)
			{
				file_operate.file_delete_flag = 1;
				if(sem_post(&delete_data_dir_sem)==-1)
				{
					printf("delete_data_dir_sem sem_post err\n");
				}
				else
				{
					printf("delete_data_dir_sem sem_post success\n");
				}
			}
			file_operate.time_update_flag = 0;
		}


		printf("------->sec_app<--------\n");

		if(self_test_para.self_test_flag && sec_cnt>20)
		{
			get_power_on_para();
			DEBUG("self_test_control\n");
			self_test_control();
//			swself_test_control();
		}

		if(sec_cnt > 15)			//控制板最开始上电，发的车厢号为0，等待15s再匹配车厢号
		{
//			check_board_err();
//			set_para_to_local();
		}

		if(sec_cnt > 30)
		{
			update_err_log_data(sys_status_cnt);
		}

//双层350无法检测电源故障、屏蔽
//		read_power_stuatus();

		if(sec_cnt >= SDK_CHECK_TIME)//启动10min后
		{
			if(sec_cnt % SDK_CHECK_TIME == 0)//每10min检查一次是否需要删除
			{
				printf("sem_post---delete_data_dir_sem\n");
				sem_post(&delete_data_dir_sem);
			}
		}
		sleep(1);
	}
}

 /*************************************************
 Function:    timer_handler
 Description: 定时器回调函数，用于需要定时的任务
 Input:
 Output:
 Return:
 Others:
 *************************************************/
void timer_handler(void)
{
	static uint8_t control_flag=0;
	control_flag = !control_flag;

	led_sys(control_flag);
	led_net(control_flag);

//	if((sw_tz_data.alarm_status.bits.bogie_alarm == 1)
//			|| (pw_tz_data.alarm_status.bits.side1_z_alarm  == 1)
//			|| (pw_tz_data.alarm_status.bits.side2_z_alarm  == 1)
//			|| (pw_tz_data.alarm_status.bits.side1_y_alarm  == 1)
//			|| (pw_tz_data.alarm_status.bits.side2_y_alarm  == 1)
//			|| (pw_tz_data.alarm_status.bits.side1_x_alarm  == 1)
//			|| (pw_tz_data.alarm_status.bits.side2_x_alarm  == 1))
//	{
//		led_psw(control_flag);
//	}
//	else
	{
//		led_psw(0);
	}

//	//如果connect_flag为１ 表示故障
//	if(eth_status[BIT_SAVE].connect_flag)
//	{
//		//内部以太网通信故障
//		led_net(LED_OFF);
//	}
//	else
//	{
//		led_net(control_flag);
//	}

}


/*************************************************
Function:    init_timer
Description: 初始化定时器
Input:
Output:
Return:
Others:
*************************************************/
void init_timer()
{

	struct itimerval val;
	signal(SIGALRM, (__sighandler_t)timer_handler);  //注册信号SIGALRM 和信号处理函数
	val.it_value.tv_sec = 0;
	val.it_value.tv_usec = 500000;

	val.it_interval = val.it_value;
	setitimer(ITIMER_REAL, &val, NULL);

}

/*************************************************
Function:    init_sec_thread
Description: 初始化自检和秒线程
Input:
Output:
Return:成功：0
	　　失败:非0
Others:
*************************************************/
 int init_sec_thread()
 {
 	pthread_t sec_thread_id;
 	int ret;
 	ret=pthread_create(&sec_thread_id,NULL,(void *)sec_thread_entry,NULL);
 	if(ret!=0)
 	{
 		DEBUG ("Create self check thread error!\n");
 		return ret;
 	}
 	return 0;
 }


