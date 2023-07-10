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
#include <linux/can.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/can/raw.h>
#include <semaphore.h>
#include "user_data.h"
#include "can_data_deal.h"
#include "can_config.h"
#include "ad7606.h"
#include "update.h"
#include "speed.h"
#include "sec_app.h"
#include "board.h"
#define  PRO_HEAD_LEN    12

#define  SPEED_ID        0xfe
#define  FALSE_INT       0x00
#define  TRUE_INT        0x01
#define  DEFAULT_SPEED   3000

/*--------------------- extern para -------------------------*/
//extern struct TEMP_DATA *temp_data;
//extern struct STORE_FLAG *store_flag;
//extern struct MASTER_CLB_CONFIG_PARA *master_config_paras;
//extern struct BEARING_DATA *bearing_data;
//extern struct POLYGON_DATA *polygon_data;
//extern struct UPDATE_FILE update_file;
//extern struct UPDATE_FLAG update_flag;
//extern struct CAN_UPDATE_DATA can_update_data;
//extern struct TRAIN_SOFT_VERSION train_soft_version;  //系统软件版本
//extern struct _SYS_STAT sys_train_stat;										//系统列车各设备状态
////extern TRAIN_SENSOR_STAT_UN train_sensor_stat;								//列车传感器自检状态
//extern struct CAN_CON_ST proc1_st, proc2_st;
//extern struct AD_PUBLIC_INFO bear_public_info;
//extern struct AD_PUBLIC_INFO ploy_public_info;
//extern int fd_speed;
extern struct CAN_STATUS can_status[16];
/*------------------- private ------------------------------*/
struct _DATA_FROM_MVB_ST  mvb_data_st;
extern struct SYS_STATUS_CNT sys_status_cnt;

#ifdef CAN_ERR_REBOOT_TWO_TIMES
extern int update_reboot_log_file();
extern uint8_t software_reboot_enable;
#endif

/*************************************************
Function:  can_send_reboot_cmd
Description:  发送复位重启命令
Input:  目标ＩＤ:taget
Output:
Return:
Others:
*************************************************/
void can_send_reboot_cmd(uint8_t taget)
{
	uint8_t data[1];
	data[0] = 0;
	can_send(taget, SYSTEM_RESET_CMD, data, 1, 0);
}

/*************************************************
Function:  can_send_switch_bearing_ch
Description:  通知接口板切换振动通道的通知
Input:  目标ＩＤ:taget
		切换的通道号：icp
Output:
Return:
Others:
*************************************************/
void can_send_switch_bearing_ch(uint8_t taget,uint8_t icp)
{
	uint8_t data[1];
	data[0] = icp;
	can_send(taget, SWITCH_BEARING_CH_CMD, data, 1, 0);
}


/*************************************************
Function:  can_send_switch_polygon_ch
Description:  通知接口板切换振动通道的通知
Input:  目标ＩＤ:taget
		切换的通道号：icp
Output:
Return:
Others:
*************************************************/
void can_send_switch_polygon_ch(uint8_t taget,uint8_t icp)
{
	uint8_t data[1];
	data[0] = icp;
	//printf("switch ploygon ch !\r\n");
	can_send(taget, SWITCH_POLYGON_CH_CMD, data, 1, 0);
}
/*************************************************
Function:  can_send_tboard1
Description: 通知接口板１发送温度数据
Input:
Output:
Return:
Others:
*************************************************/
void can_send_tboard1()
{
	uint8_t data[1];
	data[0] = 1;
	can_send(PRE_BOARD1, GET_TEMP_CMD, data, 1, 0);
}
/*************************************************
Function:  can_send_tboard2
Description:  通知接口板２发送温度数据
Input:
Output:
Return:
Others:
*************************************************/
void can_send_tboard2()
{
	uint8_t data[1];
	data[0] = 2;
	can_send(PRE_BOARD2, GET_TEMP_CMD, data, 1, 0);
}
/*************************************************
Function:  get_soft_version
Description: 获取软件版本
Input:	目标板卡地址
Output:
Return:
Others:
*************************************************/
void get_soft_version(uint8_t taget)
{
	uint8_t data[1];
	data[0] = 0;
	can_send(taget, GET_VERSION_CMD, data, 1, 0);
}

/*************************************************
Function:    recv_deal
Description: can数据解析
Input:  要解析的数据指针：data
		数据长度:len
Output: 无
Return: 无
Others:
*************************************************/
void recv_deal(uint8_t *data, uint32_t len)
{

}




/*************************************************
Function:    init_can_connect_status
Description: can通信标志初始化
Input:  none

Output: none
Return: none
Others:
*************************************************/
void init_can_connect_status()
{
	can_status[BIT_CTRLA].connect_flag = 1;					//1故障;0正常
	can_status[BIT_CTRLA].not_recv_cnt = 0;
	can_status[BIT_CTRLB].connect_flag = 1;					//1故障;0正常
	can_status[BIT_CTRLB].not_recv_cnt = 0;
	can_status[BIT_SAVE].connect_flag = 1;					//1故障;0正常
	can_status[BIT_SAVE].not_recv_cnt = 0;
	can_status[BIT_TEMPA].connect_flag = 1;					//1故障;0正常
	can_status[BIT_TEMPA].not_recv_cnt = 0;
	can_status[BIT_TEMPB].connect_flag = 1;					//1故障;0正常
	can_status[BIT_TEMPB].not_recv_cnt = 0;
	can_status[BIT_ST].connect_flag = 1;					//1故障;0正常
	can_status[BIT_ST].not_recv_cnt = 0;
	can_status[BIT_AD].connect_flag = 1;					//1故障;0正常
	can_status[BIT_AD].not_recv_cnt = 0;
	can_status[BIT_VIBR].connect_flag = 1;					//1故障;0正常
	can_status[BIT_VIBR].not_recv_cnt = 0;
	can_status[BIT_IO].connect_flag = 1;					//1故障;0正常
	can_status[BIT_IO].not_recv_cnt = 0;
	can_status[BIT_GEAR].connect_flag = 1;					//1故障;0正常
	can_status[BIT_GEAR].not_recv_cnt = 0;
	can_status[BIT_MOTOR].connect_flag = 1;					//1故障;0正常
	can_status[BIT_MOTOR].not_recv_cnt = 0;
}


void add_can_not_recv_cnt()
{
	can_status[BIT_CTRLA].not_recv_cnt++;
	can_status[BIT_CTRLB].not_recv_cnt++;

#ifdef CAN_ERR_JUDGE_NEW_STYTLE
//	extern int first_software_reboot_times();
//	static uint32_t timeout_s = CAN_FIRST_REBOOT_TIMEOUT_S;
//
//	if(!first_software_reboot_times())
//		timeout_s = CTRLA_CAN_TIMEOUT_S;

	if( (can_status[BIT_CTRLA].not_recv_cnt>CTRLA_CAN_TIMEOUT_S) && (can_status[BIT_CTRLA].not_recv_cnt>CTRLA_CAN_TIMEOUT_S) )
	{
	#ifdef CAN_ERR_REBOOT_TWO_TIMES
		if(software_reboot_enable)
	#endif
		{
			update_reboot_log_file();
			printf("reset_can_deal---can reset!\n");
	#ifdef CAN_ERR_REBOOT
			sleep(1);
			system("reboot -nf");
	#else
			config_can();
			sleep(1);
	#endif
		}
	}
#endif


	if(can_status[BIT_CTRLA].not_recv_cnt > CTRLA_CAN_TIMEOUT_S)				//每次收到can消息后,会将not_recv_cnt变量置0
	{
#ifdef	CAN_LED_TOGGLE_NO_ERR
		can_status[BIT_CTRLA].connect_flag = 0;
#else
		can_status[BIT_CTRLA].connect_flag = 1;
#endif
		if(sys_status_cnt.ctrla_can_err_save_flag == 0)
		{
//			sprintf(log_status,"err");
//			sprintf(log_kind,"ctrla can");
//			sprintf(log_detail,"ctrla can error");
//			write_log(log_status,log_kind,log_detail,log_file_name);
			sys_status_cnt.ctrla_can_err_save_flag = 1;
			sys_status_cnt.ctrla_can_normal_save_flag = 0;
		}
	}
	else
	{
		can_status[BIT_CTRLA].connect_flag = 0;

		if(sys_status_cnt.ctrla_can_err_save_flag == 1 && sys_status_cnt.ctrla_can_normal_save_flag == 0)
		{
			//if 条件 sys_status_cnt.ctrla_can_err_save_flag == 1 目的是为了在can通信正常情况下只存储一次
//			sprintf(log_status,"ok");
//			sprintf(log_kind,"ctrla can");
//			sprintf(log_detail,"ctrla can ok");
//			write_log(log_status,log_kind,log_detail,log_file_name);
			sys_status_cnt.ctrla_can_normal_save_flag = 1;
			sys_status_cnt.ctrla_can_err_save_flag = 0;
		}
	}




	if(can_status[BIT_CTRLB].not_recv_cnt > CTRLB_CAN_TIMEOUT_S)
	{
#ifdef	CAN_LED_TOGGLE_NO_ERR
		can_status[BIT_CTRLB].connect_flag = 0;
#else
		can_status[BIT_CTRLB].connect_flag = 1;
#endif
		if(sys_status_cnt.ctrlb_can_err_save_flag == 0)
		{
//			sprintf(log_status,"err");
//			sprintf(log_kind,"ctrla can");
//			sprintf(log_detail,"ctrla can error");
//			write_log(log_status,log_kind,log_detail,log_file_name);
			sys_status_cnt.ctrlb_can_err_save_flag = 1;
			sys_status_cnt.ctrlb_can_normal_save_flag = 0;
		}
	}
	else
	{
		can_status[BIT_CTRLB].connect_flag = 0;

		if(sys_status_cnt.ctrlb_can_err_save_flag == 1 && sys_status_cnt.ctrlb_can_normal_save_flag == 0)
		{
			//if 条件 sys_status_cnt.ctrla_can_err_save_flag == 1 目的是为了在can通信正常情况下只存储一次
//			sprintf(log_status,"ok");
//			sprintf(log_kind,"ctrla can");
//			sprintf(log_detail,"ctrla can ok");
//			write_log(log_status,log_kind,log_detail,log_file_name);
			sys_status_cnt.ctrlb_can_normal_save_flag = 1;
			sys_status_cnt.ctrlb_can_err_save_flag = 0;
		}
	}


//	can_status[BIT_SAVE].not_recv_cnt++;
//
//
//	if(can_status[BIT_SAVE].not_recv_cnt > 10)
//	{
//#ifdef	CAN_LED_TOGGLE_NO_ERR
//		can_status[BIT_SAVE].connect_flag  = 0;
//#else
//		can_status[BIT_SAVE].connect_flag  = 1;
//#endif
//		if(sys_status_cnt.save_can_err_save_flag == 0)
//		{
////			sprintf(log_status,"err");
////			sprintf(log_kind,"ctrla can");
////			sprintf(log_detail,"ctrla can error");
////			write_log(log_status,log_kind,log_detail,log_file_name);
//			sys_status_cnt.save_can_err_save_flag = 1;
//			sys_status_cnt.save_can_normal_save_flag = 0;
//		}
//	}
//	else
//	{
//		can_status[BIT_SAVE].connect_flag = 0;
//
//		if(sys_status_cnt.save_can_err_save_flag == 1 && sys_status_cnt.ctrla_can_normal_save_flag == 0)
//		{
//			//if 条件 sys_status_cnt.ctrla_can_err_save_flag == 1 目的是为了在can通信正常情况下只存储一次
////			sprintf(log_status,"ok");
////			sprintf(log_kind,"ctrla can");
////			sprintf(log_detail,"ctrla can ok");
////			write_log(log_status,log_kind,log_detail,log_file_name);
//			sys_status_cnt.ctrla_can_normal_save_flag = 1;
//			sys_status_cnt.ctrla_can_err_save_flag = 0;
//		}
//	}
}




