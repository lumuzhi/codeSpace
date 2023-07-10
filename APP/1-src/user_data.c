#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include "fftw3.h"
#include "user_data.h"
#include "ptu_app.h"
#include "pw_diagnos.h"
#include "self_test.h"
#include "hdd_save.h"
#include <dirent.h>

#ifdef DATA_CENTER_TEST
#include "data_test.h"
#endif

#ifdef WTD_DATA_TRANSLATE_PROTOCOL
#include <netdb.h>
#include "self_queue.h"
#endif

//#define SOFT_VERSION 2000
//#define UPDATE_TIME 20190417

#ifdef WTD_DATA_TRANSLATE_PROTOCOL
#define SELF_NET_PORT		12001

#define WEH01_WTD_TZ_PORT		12121
#define WEH01_WTD_ALARM_PORT	12131

#define WEH08_WTD_TZ_PORT		12122
#define WEH08_WTD_ALARM_PORT	12132

#define WEH09_WTD_TZ_PORT		12123
#define WEH09_WTD_ALARM_PORT	12133

#define WEH17_WTD_TZ_PORT		12124
#define WEH17_WTD_ALARM_PORT	12134
#endif

#define LOCAL_CONFIG_FILE   "/media/local_config/config/config.dat" 	//本地配置文件

#define LOCAL_POWER_ON_CNT  "/media/local_config/config/power.dat"		//存储上电次数
/*define data dir*/
#define LOCAL_PISTZ_FILE   "/media/LH_DATA/hdd/PIS/" 					  //本地pis特征值文件，下面目录依次放置每列车的特征值（轴承、轮对、温度）
#define LOCAL_TRAIN_FILE   "/media/LH_DATA/hdd/"						  //本地列车数据文件，下面目录依次放置每列车的轴承、轮对、温度数据文件夹，文件夹下又分原始和特征值

struct PW_CLB_CONFIG_PARA *pw_clb_config_para;
//struct PW_CLB_CONFIG_PARA *sw_clb_config_para;
//extern struct STORE_FLAG *store_flag;
struct TRAIN_SOFT_VERSION train_soft_version;  //系统软件版本
struct _SOFT_VERSION pw_soft_version;
struct PW_AD_DATA pw_ad_data;
struct PW_TZ_DATA pw_tz_data;
struct PW_RAW_DATA pw_raw_data;
#ifdef ADD_DIAG_TZZ_DATA_FILE
struct DIAG_TZZ_DATA diag_tzz_data;
#endif

#ifdef WTD_DATA_TRANSLATE_PROTOCOL
	uint8_t first_diag_result_flag = 0;//第一个算法结果产生
	struct WTD_PW_TZ_DATA wtd_pw_tz_data;
	struct WTD_PW_ALARM_DATA wtd_pw_alarm_data;
#ifndef WTD_DATA_PROTOCOL_20220822
	struct LESS_HEAD_INFO head_info;
	struct TZ_VALUE_DATA save_tz_value_data[10];
#endif
	LiQueue *alarm_data_queue[CHANNEL_NUM]={NULL,NULL,NULL,NULL,NULL,NULL};
	struct DEQUEUE_DATA dequeue_org_data[CHANNEL_NUM];
	struct MSG_ALARM_TRIG msg_alarm_trigger[CHANNEL_NUM];
	struct MESSAGE_COUNT_CTRL alarm_ctrl[CHANNEL_NUM];
	uint8_t save_wtd_ip[4];

	void reset_wtd_pw_alarm_data();
#endif

struct RINGBUFF_PARA pw_diagnos_ringbuf_para;
struct RINGBUFF_PARA pw_save_ringbuf_para;
struct RINGBUFF_PARA pw_send_ringbuf_para;
struct ETH_STATUS eth_status[16];
struct CAN_STATUS can_status[16];
struct COMM_DATA_CNT comm_data_cnt;
struct PW_SIMULATION pw_simulation;
struct FILE_OPERTE file_operate;
struct SYS_STATUS_CNT sys_status_cnt;
struct UPDATE_ERR_LOG_FLAG	update_err_log_flag;

struct POWER_CNT poer_cnt;

struct RECV_PUBLIC_PARA recv_public_para;
extern struct PTU_DATA ptu_data;


uint8_t pw_data_save_type = 0;
uint8_t sw_data_save_type = 0;

#ifdef AD7606_ERR_WATCHDOG_RESET
uint8_t self_check_over_flag = 0;
#endif

#ifdef ONLY_CAN_ETH_FEED_WATCHDOG
uint8_t comm_connect_flag = 0;
#endif


struct SW_AD_DATA sw_ad_data;
struct SW_TZ_DATA sw_tz_data;
struct SW_RAW_DATA sw_raw_data;
struct RINGBUFF_PARA sw_diagnos_ringbuf_para;
struct RINGBUFF_PARA sw_save_ringbuf_para;
struct RINGBUFF_PARA sw_send_ringbuf_para;


struct S1PW_TZ_DATA s1pw_tz_data; //S1 use
struct S1SW_TZ_DATA s1sw_tz_data; //S1 use
extern void init_eth_connect_status();
/*************************************************
Function:  check_dir_exits
Description:  检查一个目录是否存在
Input: 　目录绝对路径 dir_path
Output: 无
Return: 存在0，不存在-１
Others:
*************************************************/
int check_dir_exits(const char* dir_path)
{
	if(dir_path == NULL)
	{
		return -1;
	}
	if(opendir(dir_path) == NULL)
	{
		return -1;
	}
	return 0;
}

/*************************************************
Function:  check_file_exits
Description: 检测某个文件是否存在
Input:  无
Output: 无
Return: 存在：0
		不存在：非0
Others:
*************************************************/
int check_file_exits(const char* file_path)
{
	int ret = -1;
    ret = access(file_path,F_OK);
    return ret;
}

//struct PW_CLB_CONFIG_PARA
//{
//	struct LOCAL_NET_ADDR  local_net_para;  	//本地以太网
//	struct PTU_NET_ADDR   ptu_net_para;
//	struct MCAST_ADDR pw_send_mcast_addr;		//平稳板发送组播地址
//	struct MCAST_ADDR pw_recv_mcast_addr;		//平稳板接收组播地址
//	struct UCAST_ADDR save_board_addr;			//记录板ip地址
//	struct PW_DIAGNOS_THRESHOLD_PARA  pw_diagnos_para;	//平稳算法诊断参数
//};

/*************************************************
Function:  print_system_para
Description: 打印系统配置的相关参数
Input:
Output:
Return:
Others:
*************************************************/
void print_system_para(void)
{
	SOFT_VERSION_VAL_PRINTF;

	//打印出参数是否正确
	printf("pw_clb_config_para->local_net_para.net_port:%d\n",pw_clb_config_para->local_net_para.net_port);
	printf("pw_clb_config_para->local_net_para.self_ip:%d.%d.%d.%d\n",pw_clb_config_para->local_net_para.self_ip[0],pw_clb_config_para->local_net_para.self_ip[1],pw_clb_config_para->local_net_para.self_ip[2],pw_clb_config_para->local_net_para.self_ip[3]);
	printf("pw_clb_config_para->local_net_para.self_maskaddr:%d.%d.%d.%d\n",pw_clb_config_para->local_net_para.self_maskaddr[0],pw_clb_config_para->local_net_para.self_maskaddr[1],pw_clb_config_para->local_net_para.self_maskaddr[2],pw_clb_config_para->local_net_para.self_maskaddr[3]);
	printf("pw_clb_config_para->local_net_para.self_gwaddr:%d.%d.%d.%d\n",pw_clb_config_para->local_net_para.self_gwaddr[0],pw_clb_config_para->local_net_para.self_gwaddr[1],pw_clb_config_para->local_net_para.self_gwaddr[2],pw_clb_config_para->local_net_para.self_gwaddr[3]);

	printf("pw_clb_config_para->ptu_net_para.net_port:%d\n",pw_clb_config_para->ptu_net_para.net_port);
	printf("pw_clb_config_para->ptu_net_para.self_ip:%d.%d.%d.%d\n",pw_clb_config_para->ptu_net_para.self_ip[0],pw_clb_config_para->ptu_net_para.self_ip[1],pw_clb_config_para->ptu_net_para.self_ip[2],pw_clb_config_para->ptu_net_para.self_ip[3]);
	printf("pw_clb_config_para->ptu_net_para.self_maskaddr:%d.%d.%d.%d\n",pw_clb_config_para->ptu_net_para.self_maskaddr[0],pw_clb_config_para->ptu_net_para.self_maskaddr[1],pw_clb_config_para->ptu_net_para.self_maskaddr[2],pw_clb_config_para->ptu_net_para.self_maskaddr[3]);
	printf("pw_clb_config_para->ptu_net_para.self_gwaddr:%d.%d.%d.%d\n",pw_clb_config_para->ptu_net_para.self_gwaddr[0],pw_clb_config_para->ptu_net_para.self_gwaddr[1],pw_clb_config_para->ptu_net_para.self_gwaddr[2],pw_clb_config_para->ptu_net_para.self_gwaddr[3]);

#ifdef WTD_DATA_TRANSLATE_PROTOCOL
	printf("pw_clb_config_para->wtd_net_para.self_port:%d\n", pw_clb_config_para->wtd_net_para.self_port);
	printf("pw_clb_config_para->wtd_net_para.wtd_tz_port:%d\n", pw_clb_config_para->wtd_net_para.wtd_tz_port);
	printf("pw_clb_config_para->wtd_net_para.wtd_alarm_port:%d\n", pw_clb_config_para->wtd_net_para.wtd_alarm_port);
	printf("pw_clb_config_para->wtd_net_para.wtd_ip:%d.%d.%d.%d\n",pw_clb_config_para->wtd_net_para.wtd_ip[0],pw_clb_config_para->wtd_net_para.wtd_ip[1],pw_clb_config_para->wtd_net_para.wtd_ip[2],pw_clb_config_para->wtd_net_para.wtd_ip[3]);

	printf("pw_clb_config_para->save_board_addr0.ucast_port:%d\n", pw_clb_config_para->save_board_addr0.ucast_port);
	printf("pw_clb_config_para->save_board_addr0.ucast_addr:%d.%d.%d.%d\n",pw_clb_config_para->save_board_addr0.ucast_addr[0],pw_clb_config_para->save_board_addr0.ucast_addr[1],pw_clb_config_para->save_board_addr0.ucast_addr[2],pw_clb_config_para->save_board_addr0.ucast_addr[3]);
#endif

	printf("pw_clb_config_para->pw_send_mcast_addr.mcast_addr:%d,%d,%d,%d\n",pw_clb_config_para->pw_send_mcast_addr.mcast_addr[0],pw_clb_config_para->pw_send_mcast_addr.mcast_addr[1],pw_clb_config_para->pw_send_mcast_addr.mcast_addr[2],pw_clb_config_para->pw_send_mcast_addr.mcast_addr[3]);
	printf("pw_clb_config_para->pw_send_mcast_addr.mcast_port:%d\n",pw_clb_config_para->pw_send_mcast_addr.mcast_port);
	printf("pw_clb_config_para->pw_send_mcast_addr.is_need_ack:%d\n",pw_clb_config_para->pw_send_mcast_addr.is_need_ack);

	printf("pw_clb_config_para->pw_recv_mcast_addr.mcast_addr:%d,%d,%d,%d\n",pw_clb_config_para->pw_recv_mcast_addr.mcast_addr[0],pw_clb_config_para->pw_recv_mcast_addr.mcast_addr[1],pw_clb_config_para->pw_recv_mcast_addr.mcast_addr[2],pw_clb_config_para->pw_recv_mcast_addr.mcast_addr[3]);
	printf("pw_clb_config_para->pw_recv_mcast_addr.mcast_port:%d\n",pw_clb_config_para->pw_recv_mcast_addr.mcast_port);
	printf("pw_clb_config_para->pw_recv_mcast_addr.is_need_ack:%d\n",pw_clb_config_para->pw_recv_mcast_addr.is_need_ack);

	printf("pw_clb_config_para->save_board_addr.ucast_addr:%d,%d,%d,%d\n",pw_clb_config_para->save_board_addr.ucast_addr[0],pw_clb_config_para->save_board_addr.ucast_addr[1],pw_clb_config_para->save_board_addr.ucast_addr[2],pw_clb_config_para->save_board_addr.ucast_addr[3]);
	printf("pw_clb_config_para->save_board_addr.ucast_port:%d\n",pw_clb_config_para->save_board_addr.ucast_port);
	printf("pw_clb_config_para->save_board_addr.is_need_ack:%d\n",pw_clb_config_para->save_board_addr.is_need_ack);

//	printf("pw_clb_config_para->ctrla_board_addr.ucast_addr:%d,%d,%d,%d\n",pw_clb_config_para->ctrla_board_addr.ucast_addr[0],pw_clb_config_para->ctrla_board_addr.ucast_addr[1],pw_clb_config_para->ctrla_board_addr.ucast_addr[2],pw_clb_config_para->ctrla_board_addr.ucast_addr[3]);
//	printf("pw_clb_config_para->ctrla_board_addr.ucast_port:%d\n",pw_clb_config_para->ctrla_board_addr.ucast_port);
//	printf("pw_clb_config_para->ctrla_board_addr.is_need_ack:%d\n",pw_clb_config_para->ctrla_board_addr.is_need_ack);
//
//	printf("pw_clb_config_para->ctrlb_board_addr.ucast_addr:%d,%d,%d,%d\n",pw_clb_config_para->ctrlb_board_addr.ucast_addr[0],pw_clb_config_para->ctrlb_board_addr.ucast_addr[1],pw_clb_config_para->ctrlb_board_addr.ucast_addr[2],pw_clb_config_para->ctrlb_board_addr.ucast_addr[3]);
//	printf("pw_clb_config_para->ctrlb_board_addr.ucast_port:%d\n",pw_clb_config_para->ctrlb_board_addr.ucast_port);
//	printf("pw_clb_config_para->ctrlb_board_addr.is_need_ack:%d\n",pw_clb_config_para->ctrlb_board_addr.is_need_ack);


	printf("pw_clb_config_para->pw_diagnos_para.side1_y_para.warn_value:%f\n",pw_clb_config_para->pw_diagnos_para.side1_y_para.warn_value);
	printf("pw_clb_config_para->pw_diagnos_para.side1_y_para.alarm_value:%f\n",pw_clb_config_para->pw_diagnos_para.side1_y_para.alarm_value);
	printf("pw_clb_config_para->pw_diagnos_para.side1_y_para.warn_h_cnt:%d\n",pw_clb_config_para->pw_diagnos_para.side1_y_para.warn_h_cnt);
	printf("pw_clb_config_para->pw_diagnos_para.side1_y_para.alarm_l_cnt:%d\n",pw_clb_config_para->pw_diagnos_para.side1_y_para.alarm_l_cnt);

	printf("pw_clb_config_para->pw_diagnos_para.side1_z_para.warn_value:%f\n",pw_clb_config_para->pw_diagnos_para.side1_z_para.warn_value);
	printf("pw_clb_config_para->pw_diagnos_para.side1_z_para.alarm_value:%f\n",pw_clb_config_para->pw_diagnos_para.side1_z_para.alarm_value);
	printf("pw_clb_config_para->pw_diagnos_para.side1_z_para.warn_h_cnt:%d\n",pw_clb_config_para->pw_diagnos_para.side1_z_para.warn_h_cnt);
	printf("pw_clb_config_para->pw_diagnos_para.side1_z_para.alarm_l_cnt:%d\n",pw_clb_config_para->pw_diagnos_para.side1_z_para.alarm_l_cnt);



	printf("pw_clb_config_para->pw_diagnos_para.side2_y_para.warn_value:%f\n",pw_clb_config_para->pw_diagnos_para.side2_y_para.warn_value);
	printf("pw_clb_config_para->pw_diagnos_para.side2_y_para.alarm_value:%f\n",pw_clb_config_para->pw_diagnos_para.side2_y_para.alarm_value);
	printf("pw_clb_config_para->pw_diagnos_para.side2_y_para.warn_h_cnt:%d\n",pw_clb_config_para->pw_diagnos_para.side2_y_para.warn_h_cnt);
	printf("pw_clb_config_para->pw_diagnos_para.side2_y_para.alarm_l_cnt:%d\n",pw_clb_config_para->pw_diagnos_para.side2_y_para.alarm_l_cnt);

	printf("pw_clb_config_para->pw_diagnos_para.side2_z_para.warn_value:%f\n",pw_clb_config_para->pw_diagnos_para.side2_z_para.warn_value);
	printf("pw_clb_config_para->pw_diagnos_para.side2_z_para.alarm_value:%f\n",pw_clb_config_para->pw_diagnos_para.side2_z_para.alarm_value);
	printf("pw_clb_config_para->pw_diagnos_para.side2_z_para.warn_h_cnt:%d\n",pw_clb_config_para->pw_diagnos_para.side2_z_para.warn_h_cnt);
	printf("pw_clb_config_para->pw_diagnos_para.side2_z_para.alarm_l_cnt:%d\n",pw_clb_config_para->pw_diagnos_para.side2_z_para.alarm_l_cnt);

	printf("pw_clb_config_para->sw_diagnos_para.side1_y_para.alarm_value:%f\n",pw_clb_config_para->sw_diagnos_para.side1_y_para.alarm_value);
	printf("pw_clb_config_para->sw_diagnos_para.side1_y_para.h_cnt:%d\n",pw_clb_config_para->sw_diagnos_para.side1_y_para.h_cnt);
	printf("pw_clb_config_para->sw_diagnos_para.side1_y_para.l_cnt:%d\n",pw_clb_config_para->sw_diagnos_para.side1_y_para.l_cnt);

	printf("pw_clb_config_para->sw_diagnos_para.side1_y_para.alarm_value:%f\n",pw_clb_config_para->sw_diagnos_para.side2_y_para.alarm_value);
	printf("pw_clb_config_para->sw_diagnos_para.side1_y_para.h_cnt:%d\n",pw_clb_config_para->sw_diagnos_para.side2_y_para.h_cnt);
	printf("pw_clb_config_para->sw_diagnos_para.side1_y_para.l_cnt:%d\n",pw_clb_config_para->sw_diagnos_para.side2_y_para.l_cnt);

	printf("pw_clb_config_para->trainnum:%d\n",pw_clb_config_para->trainnum);

}



/*************************************************
Function:  init_system_default_para
Description: 初始化默认参数
Input:
Output:
Return:
Others:
*************************************************/
void init_system_default_para(void)
{
//本地ip相关参数

	pw_clb_config_para->local_net_para.net_port = 8000;
	pw_clb_config_para->local_net_para.self_ip[0] = 192;
	pw_clb_config_para->local_net_para.self_ip[1] = 168;
	pw_clb_config_para->local_net_para.self_ip[2] = 1;
	pw_clb_config_para->local_net_para.self_ip[3] = 17;

	pw_clb_config_para->local_net_para.self_maskaddr[0] = 255;
	pw_clb_config_para->local_net_para.self_maskaddr[1] = 255;
	pw_clb_config_para->local_net_para.self_maskaddr[2] = 0;
	pw_clb_config_para->local_net_para.self_maskaddr[3] = 0;

	pw_clb_config_para->local_net_para.self_gwaddr[0] = 192;
	pw_clb_config_para->local_net_para.self_gwaddr[1] = 168;
	pw_clb_config_para->local_net_para.self_gwaddr[2] = 1;
	pw_clb_config_para->local_net_para.self_gwaddr[3] = 1;

//	//PTU ip 参数
//	pw_clb_config_para->ptu_net_para.net_port = 6501;
//#ifdef WTD_DNS_GW_TEST
//	pw_clb_config_para->ptu_net_para.self_ip[0] = 192;
//	pw_clb_config_para->ptu_net_para.self_ip[1] = 168;
//	pw_clb_config_para->ptu_net_para.self_ip[2] = 3;
//	pw_clb_config_para->ptu_net_para.self_ip[3] = 173;
//#else
//	pw_clb_config_para->ptu_net_para.self_ip[0] = 10;
//	pw_clb_config_para->ptu_net_para.self_ip[1] = 0;
//	pw_clb_config_para->ptu_net_para.self_ip[2] = 1;
//	pw_clb_config_para->ptu_net_para.self_ip[3] = 173;
//#endif
//
//	pw_clb_config_para->ptu_net_para.self_maskaddr[0] = 255;
//	pw_clb_config_para->ptu_net_para.self_maskaddr[1] = 255;
//	pw_clb_config_para->ptu_net_para.self_maskaddr[2] = 192;
//	pw_clb_config_para->ptu_net_para.self_maskaddr[3] = 0;
//
//	pw_clb_config_para->ptu_net_para.self_gwaddr[0] = 10;
//	pw_clb_config_para->ptu_net_para.self_gwaddr[1] = 0;
//	pw_clb_config_para->ptu_net_para.self_gwaddr[2] = 0;
//	pw_clb_config_para->ptu_net_para.self_gwaddr[3] = 1;

#ifdef WTD_DATA_TRANSLATE_PROTOCOL
	pw_clb_config_para->wtd_net_para.self_port = SELF_NET_PORT;
//	pw_clb_config_para->wtd_net_para.self_ip[0] = 10;
//	pw_clb_config_para->wtd_net_para.self_ip[1] = 0;
//	pw_clb_config_para->wtd_net_para.self_ip[2] = 1;
//	pw_clb_config_para->wtd_net_para.self_ip[3] = 173;
//
//	pw_clb_config_para->wtd_net_para.self_gwaddr[0] = 10;
//	pw_clb_config_para->wtd_net_para.self_gwaddr[1] = 0;
//	pw_clb_config_para->wtd_net_para.self_gwaddr[2] = 0;
//	pw_clb_config_para->wtd_net_para.self_gwaddr[3] = 1;

	pw_clb_config_para->wtd_net_para.wtd_tz_port = WEH01_WTD_TZ_PORT;
	pw_clb_config_para->wtd_net_para.wtd_alarm_port = WEH01_WTD_ALARM_PORT;
	pw_clb_config_para->wtd_net_para.wtd_ip[0] = 10;
	pw_clb_config_para->wtd_net_para.wtd_ip[1] = 0;
	pw_clb_config_para->wtd_net_para.wtd_ip[2] = 1;
	pw_clb_config_para->wtd_net_para.wtd_ip[3] = 210;

//记录板单播地址
	pw_clb_config_para->save_board_addr0.ucast_port = SELF_NET_PORT;
	pw_clb_config_para->save_board_addr0.ucast_addr[0] = 10;
	pw_clb_config_para->save_board_addr0.ucast_addr[1] = 0;
	pw_clb_config_para->save_board_addr0.ucast_addr[2] = 1;
	pw_clb_config_para->save_board_addr0.ucast_addr[3] = 163;
#endif

//发送组播地址
	pw_clb_config_para->pw_send_mcast_addr.mcast_port = 9002;
	pw_clb_config_para->pw_send_mcast_addr.mcast_addr[0] = 239;
	pw_clb_config_para->pw_send_mcast_addr.mcast_addr[1] = 255;
	pw_clb_config_para->pw_send_mcast_addr.mcast_addr[2] = 10;
	pw_clb_config_para->pw_send_mcast_addr.mcast_addr[3] = 2;
	pw_clb_config_para->pw_send_mcast_addr.is_need_ack = 1;

//接收组播地址
	pw_clb_config_para->pw_recv_mcast_addr.mcast_port = 9001;
	pw_clb_config_para->pw_recv_mcast_addr.mcast_addr[0] = 239;
	pw_clb_config_para->pw_recv_mcast_addr.mcast_addr[1] = 255;
	pw_clb_config_para->pw_recv_mcast_addr.mcast_addr[2] = 10;
	pw_clb_config_para->pw_recv_mcast_addr.mcast_addr[3] = 1;
	pw_clb_config_para->pw_recv_mcast_addr.is_need_ack = 1;

//记录板单播地址
	pw_clb_config_para->save_board_addr.ucast_port = 8000;
	pw_clb_config_para->save_board_addr.ucast_addr[0] = 192;
	pw_clb_config_para->save_board_addr.ucast_addr[1] = 168;
	pw_clb_config_para->save_board_addr.ucast_addr[2] = 1;
	pw_clb_config_para->save_board_addr.ucast_addr[3] = 11;   /// 记得11
	pw_clb_config_para->save_board_addr.is_need_ack = 1;


//	pw_clb_config_para->ctrla_board_addr.ucast_port = 8000;
//	pw_clb_config_para->ctrla_board_addr.ucast_addr[0] = 192;
//	pw_clb_config_para->ctrla_board_addr.ucast_addr[1] = 168;
//	pw_clb_config_para->ctrla_board_addr.ucast_addr[2] = 1;
//	pw_clb_config_para->ctrla_board_addr.ucast_addr[3] = 12;
//	pw_clb_config_para->ctrla_board_addr.is_need_ack = 1;
//
//	pw_clb_config_para->ctrlb_board_addr.ucast_port = 8000;
//	pw_clb_config_para->ctrlb_board_addr.ucast_addr[0] = 192;
//	pw_clb_config_para->ctrlb_board_addr.ucast_addr[1] = 168;
//	pw_clb_config_para->ctrlb_board_addr.ucast_addr[2] = 1;
//	pw_clb_config_para->ctrlb_board_addr.ucast_addr[3] = 13;
//	pw_clb_config_para->ctrlb_board_addr.is_need_ack = 1;

	//pw 算法相关参数
	//feed dog failed
	/*司机室预警阈值:2.75,报警阈值:3.0*/
#ifdef PW_MODIFY_20211112
	pw_clb_config_para->pw_diagnos_para.side1_y_para.warn_value = 2.75f;//2.5f;  客室\司机室相同
	pw_clb_config_para->pw_diagnos_para.side1_y_para.alarm_value = 3.0f;//2.75f; 客室\司机室相同
	pw_clb_config_para->pw_diagnos_para.side1_y_para.warn_h_cnt = 15;
	pw_clb_config_para->pw_diagnos_para.side1_y_para.warn_l_cnt = 1;
	pw_clb_config_para->pw_diagnos_para.side1_y_para.alarm_h_cnt = 15;
	pw_clb_config_para->pw_diagnos_para.side1_y_para.alarm_l_cnt = 1;

	pw_clb_config_para->pw_diagnos_para.side2_y_para.warn_value = 2.75f;//2.5f;
	pw_clb_config_para->pw_diagnos_para.side2_y_para.alarm_value = 3.0f;//2.75f;
	pw_clb_config_para->pw_diagnos_para.side2_y_para.warn_h_cnt = 15;
	pw_clb_config_para->pw_diagnos_para.side2_y_para.warn_l_cnt = 1;
	pw_clb_config_para->pw_diagnos_para.side2_y_para.alarm_h_cnt = 15;
	pw_clb_config_para->pw_diagnos_para.side2_y_para.alarm_l_cnt = 1;

	pw_clb_config_para->pw_diagnos_para.side1_z_para.warn_value = 2.75f;//2.5f;
	pw_clb_config_para->pw_diagnos_para.side1_z_para.alarm_value = 3.0f;//2.75f;
	pw_clb_config_para->pw_diagnos_para.side1_z_para.warn_h_cnt = 15;
	pw_clb_config_para->pw_diagnos_para.side1_z_para.warn_l_cnt = 1;
	pw_clb_config_para->pw_diagnos_para.side1_z_para.alarm_h_cnt = 15;
	pw_clb_config_para->pw_diagnos_para.side1_z_para.alarm_l_cnt = 1;

	pw_clb_config_para->pw_diagnos_para.side2_z_para.warn_value = 2.75f;//2.5f;
	pw_clb_config_para->pw_diagnos_para.side2_z_para.alarm_value = 3.0f;//2.75f;
	pw_clb_config_para->pw_diagnos_para.side2_z_para.warn_h_cnt = 15;
	pw_clb_config_para->pw_diagnos_para.side2_z_para.warn_l_cnt = 1;
	pw_clb_config_para->pw_diagnos_para.side2_z_para.alarm_h_cnt = 15;
	pw_clb_config_para->pw_diagnos_para.side2_z_para.alarm_l_cnt = 10;
#else
	pw_clb_config_para->pw_diagnos_para.side1_y_para.warn_value = 2.75f;//2.5f;
	pw_clb_config_para->pw_diagnos_para.side1_y_para.alarm_value = 3.0f;//2.75f;
	pw_clb_config_para->pw_diagnos_para.side1_y_para.warn_h_cnt = 10;
	pw_clb_config_para->pw_diagnos_para.side1_y_para.warn_l_cnt = 6;
	pw_clb_config_para->pw_diagnos_para.side1_y_para.alarm_h_cnt = 10;
	pw_clb_config_para->pw_diagnos_para.side1_y_para.alarm_l_cnt = 6;

	pw_clb_config_para->pw_diagnos_para.side2_y_para.warn_value = 2.75f;//2.5f;
	pw_clb_config_para->pw_diagnos_para.side2_y_para.alarm_value = 3.0f;//2.75f;
	pw_clb_config_para->pw_diagnos_para.side2_y_para.warn_h_cnt = 10;
	pw_clb_config_para->pw_diagnos_para.side2_y_para.warn_l_cnt = 6;
	pw_clb_config_para->pw_diagnos_para.side2_y_para.alarm_h_cnt = 10;
	pw_clb_config_para->pw_diagnos_para.side2_y_para.alarm_l_cnt = 6;

	pw_clb_config_para->pw_diagnos_para.side1_z_para.warn_value = 2.75f;//2.5f;
	pw_clb_config_para->pw_diagnos_para.side1_z_para.alarm_value = 3.0f;//2.75f;
	pw_clb_config_para->pw_diagnos_para.side1_z_para.warn_h_cnt = 10;
	pw_clb_config_para->pw_diagnos_para.side1_z_para.warn_l_cnt = 6;
	pw_clb_config_para->pw_diagnos_para.side1_z_para.alarm_h_cnt = 10;
	pw_clb_config_para->pw_diagnos_para.side1_z_para.alarm_l_cnt = 6;

	pw_clb_config_para->pw_diagnos_para.side2_z_para.warn_value = 2.75f;//2.5f;
	pw_clb_config_para->pw_diagnos_para.side2_z_para.alarm_value = 3.0f;//2.75f;
	pw_clb_config_para->pw_diagnos_para.side2_z_para.warn_h_cnt = 10;
	pw_clb_config_para->pw_diagnos_para.side2_z_para.warn_l_cnt = 6;
	pw_clb_config_para->pw_diagnos_para.side2_z_para.alarm_h_cnt = 10;
	pw_clb_config_para->pw_diagnos_para.side2_z_para.alarm_l_cnt = 6;
#endif

	/*抖车阈值配置*/
#ifdef DC_MODIFY_20211112
	pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_warn_value = 0.02f;		/*客室0.02f 司机室=客室+0.01 客平稳性2.5　司平稳性2.75*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_alarm_value = 0.03f;		/*客室0.03f 司机室=客室+0.01 客平稳性2.75　司平稳性3.0*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_warn_h_cnt = 10;			/*超过预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_warn_l_cnt = 10;			/*低于预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_alarm_h_cnt = 10;		/*超过报警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_alarm_l_cnt = 10;		/*低于报警阈值次数*/

	pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_warn_value = 0.03f;		/*客室0.03f 司机室=客室+0.01　客平稳性2.5　司平稳性2.75*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_alarm_value = 0.04f;		/*客室0.04f 司机室=客室+0.01　客平稳性2.75　司平稳性3.0*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_warn_h_cnt = 10;			/*超过预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_warn_l_cnt = 10;			/*低于预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_alarm_h_cnt = 10;		/*超过报警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_alarm_l_cnt = 10;		/*低于报警阈值次数*/

	pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_warn_value = 0.02f;		/*客室0.02f*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_alarm_value = 0.03f;		/*客室0.03f*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_warn_h_cnt = 10;			/*超过预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_warn_l_cnt = 10;			/*低于预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_alarm_h_cnt = 10;		/*超过报警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_alarm_l_cnt = 10;		/*低于报警阈值次数*/

	pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_warn_value = 0.03f;		/*客室0.03f*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_alarm_value = 0.04f;		/*客室0.04f*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_warn_h_cnt = 10;			/*超过预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_warn_l_cnt = 10;			/*低于预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_alarm_h_cnt = 10;		/*超过报警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_alarm_l_cnt = 10;		/*低于报警阈值次数*/
#else
	pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_warn_value = 0.02f;		/*客室0.02f*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_alarm_value = 0.03f;		/*客室0.03f*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_warn_h_cnt = 10;			/*超过预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_warn_l_cnt = 6;			/*低于预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_alarm_h_cnt = 10;		/*超过报警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_alarm_l_cnt = 6;			/*低于报警阈值次数*/

	pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_warn_value = 0.03f;		/*客室0.03f*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_alarm_value = 0.04f;		/*客室0.04f*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_warn_h_cnt = 10;			/*超过预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_warn_l_cnt = 6;			/*低于预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_alarm_h_cnt = 10;		/*超过报警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_alarm_l_cnt = 6;			/*低于报警阈值次数*/

	pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_warn_value = 0.02f;		/*客室0.02f*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_alarm_value = 0.03f;		/*客室0.03f*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_warn_h_cnt = 10;			/*超过预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_warn_l_cnt = 6;			/*低于预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_alarm_h_cnt = 10;		/*超过报警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_alarm_l_cnt = 6;			/*低于报警阈值次数*/

	pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_warn_value = 0.03f;		/*客室0.03f*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_alarm_value = 0.04f;		/*客室0.04f*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_warn_h_cnt = 10;			/*超过预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_warn_l_cnt = 6;			/*低于预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_alarm_h_cnt = 10;		/*超过报警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_alarm_l_cnt = 6;			/*低于报警阈值次数*/
#endif

	/*晃车阈值配置*/
#ifdef HC_MODIFY_20211112
	pw_clb_config_para->pw_diagnos_para.shake_side1_y_para.shake_warn_value = 0.03f;		/*客室0.03f　司机室=客室+0.01*/
	pw_clb_config_para->pw_diagnos_para.shake_side1_y_para.shake_alarm_value = 0.04f;		/*客室0.04f　司机室=客室+0.01*/
	pw_clb_config_para->pw_diagnos_para.shake_side1_y_para.shake_warn_h_cnt = 10;			/*超过预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.shake_side1_y_para.shake_warn_l_cnt = 10;			/*低于预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.shake_side1_y_para.shake_alarm_h_cnt = 10;		    /*超过报警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.shake_side1_y_para.shake_alarm_l_cnt = 10;			/*低于报警阈值次数*/

	pw_clb_config_para->pw_diagnos_para.shake_side2_y_para.shake_warn_value = 0.03f;		/*客室0.03f*/
	pw_clb_config_para->pw_diagnos_para.shake_side2_y_para.shake_alarm_value = 0.04f;		/*客室0.04f*/
	pw_clb_config_para->pw_diagnos_para.shake_side2_y_para.shake_warn_h_cnt = 10;			/*超过预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.shake_side2_y_para.shake_warn_l_cnt = 10;			/*低于预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.shake_side2_y_para.shake_alarm_h_cnt = 10;			/*超过报警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.shake_side2_y_para.shake_alarm_l_cnt = 10;			/*低于报警阈值次数*/
#else
	pw_clb_config_para->pw_diagnos_para.shake_side1_y_para.shake_warn_value = 0.03f;		/*客室0.03f*/
	pw_clb_config_para->pw_diagnos_para.shake_side1_y_para.shake_alarm_value = 0.04f;		/*客室0.04f*/
	pw_clb_config_para->pw_diagnos_para.shake_side1_y_para.shake_warn_h_cnt = 10;			/*超过预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.shake_side1_y_para.shake_warn_l_cnt = 6;			/*低于预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.shake_side1_y_para.shake_alarm_h_cnt = 10;		    /*超过报警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.shake_side1_y_para.shake_alarm_l_cnt = 6;			/*低于报警阈值次数*/

	pw_clb_config_para->pw_diagnos_para.shake_side2_y_para.shake_warn_value = 0.03f;		/*客室0.03f*/
	pw_clb_config_para->pw_diagnos_para.shake_side2_y_para.shake_alarm_value = 0.04f;		/*客室0.04f*/
	pw_clb_config_para->pw_diagnos_para.shake_side2_y_para.shake_warn_h_cnt = 10;			/*超过预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.shake_side2_y_para.shake_warn_l_cnt = 6;			/*低于预警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.shake_side2_y_para.shake_alarm_h_cnt = 10;			/*超过报警阈值次数*/
	pw_clb_config_para->pw_diagnos_para.shake_side2_y_para.shake_alarm_l_cnt = 6;			/*低于报警阈值次数*/
#endif

	pw_clb_config_para->pw_diagnos_para.wheel_side1_z_para.wheel_alarm_value = 0.005f;
	pw_clb_config_para->pw_diagnos_para.wheel_side1_z_para.wheel_alarm_h_cnt = 5;
	pw_clb_config_para->pw_diagnos_para.wheel_side1_z_para.wheel_alarm_l_cnt = 0;

	pw_clb_config_para->sw_diagnos_para.side1_y_para.alarm_value = 1.6f;
	pw_clb_config_para->sw_diagnos_para.side1_y_para.h_cnt = 10;
	pw_clb_config_para->sw_diagnos_para.side1_y_para.l_cnt = 6;

	pw_clb_config_para->sw_diagnos_para.side2_y_para.alarm_value = 1.6f;
	pw_clb_config_para->sw_diagnos_para.side2_y_para.h_cnt = 10;
	pw_clb_config_para->sw_diagnos_para.side2_y_para.l_cnt = 6;

//	pw_clb_config_para->sw_diagnos_para.side3_y_para.alarm_value = 1.6f;
//	pw_clb_config_para->sw_diagnos_para.side3_y_para.h_cnt = 10;
//	pw_clb_config_para->sw_diagnos_para.side3_y_para.l_cnt = 6;
//
//	pw_clb_config_para->sw_diagnos_para.side4_y_para.alarm_value = 1.6f;
//	pw_clb_config_para->sw_diagnos_para.side4_y_para.h_cnt = 10;
//	pw_clb_config_para->sw_diagnos_para.side4_y_para.l_cnt = 6;
	//pw_clb_config_para->

	pw_clb_config_para->trainnum = pw_clb_config_para->local_net_para.self_ip[2];
}



/*************************************************
Function:  set_para_to_local
Description:  本地emmc配置参数
Input: 　无
Output: 无
Return: 无
Others:
*************************************************/
void set_para_to_local()
{
	int config_fd1 = -1;//,ret2 = -1;
	uint8_t buf[1000] = {0};
//	uint8_t tag_id = 0;
	if(access("/media/local_config/config",F_OK)!=0)
	{
		creat_dir("/media/local_config/config");
	}

	config_fd1 = open(LOCAL_CONFIG_FILE, O_CREAT|O_RDWR ,0777);
	if (config_fd1 < 0)
	{
		printf("open config file failed!\n");
		return;
	}

	memmove(buf,pw_clb_config_para,sizeof(struct PW_CLB_CONFIG_PARA));

	if(write(config_fd1,buf,sizeof(struct PW_CLB_CONFIG_PARA))<=0)
	{
		printf("write config file failed!\n");
		return;
	}

	fsync(config_fd1);
	close(config_fd1);
//	print_system_para();
//	if(ptu_data.ptu_net_fd>0)
//		close(ptu_data.ptu_net_fd);

//	system("reboot -nf");

}


/*************************************************
Function:  get_para_from_local
Description:  从本地emmc获取配置参数
Input: 　无
Output: 无
Return: 无
Others:
*************************************************/
void get_para_from_local()
{

	int config_fd = -1,ret = -1;
	uint8_t buf[1000] = {0};
	DEBUG("get para from local!\r\n");
	config_fd = open(LOCAL_CONFIG_FILE, O_RDWR);
	if (config_fd < 0)
	{
		DEBUG("open config file failed\n");
	}

	ret = read(config_fd,buf,sizeof(struct PW_CLB_CONFIG_PARA));
	if(ret > 0)		//从配置文件中获取到参数
	{
		DEBUG("get para from config file:%d,sizeof config:%d\n",ret,sizeof(struct PW_CLB_CONFIG_PARA));
		memmove(pw_clb_config_para,(struct PW_CLB_CONFIG_PARA*)buf,ret);
		print_system_para();
		DEBUG("get para from config file\n");
	}
	else if(ret <= 0 || (config_fd < 0))  //配置文件获取失败，使用默认参数
	{
		DEBUG("get para from default\n");
		init_system_default_para();
		print_system_para();
	}

//------>ctrl_B_ETH<-------
//	recv_public_para.recv_ctrl_board_data.train_public_info.carriage_number = pw_clb_config_para->ptu_net_para.self_ip[2]-1;
//
//	printf("recv_public_para.recv_ctrl_board_data.train_public_info.carriage_number:%d\n",recv_public_para.recv_ctrl_board_data.train_public_info.carriage_number);
	close(config_fd);

	set_para_to_local();
}


uint8_t read_para_from_config()
{
	int res = 0,config_fd = -1,ret = -1;
	uint8_t buf[1000] = {0};
	DEBUG("get para from local!\r\n");
	config_fd = open(LOCAL_CONFIG_FILE, O_RDWR);
	if (config_fd < 0)
	{
		DEBUG("open config file failed\n");
		return res;
	}

	ret = read(config_fd,buf,sizeof(struct PW_CLB_CONFIG_PARA));
	if(ret > 0)		//从配置文件中获取到参数
	{
		printf("get para from config file:%d,sizeof config:%d\n",ret,sizeof(struct PW_CLB_CONFIG_PARA));
		memmove(pw_clb_config_para,(struct PW_CLB_CONFIG_PARA*)buf,sizeof(struct PW_CLB_CONFIG_PARA));
		print_system_para();
		DEBUG("get para from config file\n");
		res = 1;
	}
	return res;
}

void init_power_on_para()
{
	poer_cnt.power_on_cnt = 0;
	poer_cnt.power_on_type = 0;
	poer_cnt.power_on_sum = 0;
}

void printf_power_on_para()
{
	printf("poer_cnt.power_on_cnt:%d\n",poer_cnt.power_on_cnt);
	printf("poer_cnt.power_on_type:%d\n",poer_cnt.power_on_type);
}

void set_power_on_para()
{
	int config_fd = -1;//res = 0,ret = -1;
	uint8_t buf[10] = {0};
	config_fd = open(LOCAL_POWER_ON_CNT, O_CREAT|O_RDWR ,0777);
	memset(buf,0,sizeof(buf));
	memmove(buf,&poer_cnt,sizeof(struct POWER_CNT));
	if(write(config_fd,buf,sizeof(struct POWER_CNT))<=0)
	{
		printf("write config file failed!\n");
		return;
	}

	fsync(config_fd);
	close(config_fd);
}

void get_power_on_para()
{
	int config_fd = -1,ret = -1;//res = 0,
	uint8_t buf[10] = {0};

	config_fd = open(LOCAL_POWER_ON_CNT, O_CREAT|O_RDWR ,0777);
	if (config_fd < 0)
	{
		close(config_fd);
		DEBUG("open power on failed\n");
	}

	ret = read(config_fd,buf,sizeof(struct POWER_CNT));
	if(ret > 0)		//从配置文件中获取到参数
	{
		DEBUG("get power_on_para from config file:%d,sizeof config:%d\n",ret,sizeof(struct POWER_CNT));
		memmove(&poer_cnt,(struct POWER_CNT*)buf,sizeof(struct POWER_CNT));
		printf_power_on_para();
		DEBUG("get power_on_para from config file\n");
	}
	else if(ret <= 0 || (config_fd < 0))  //配置文件获取失败，使用默认参数
	{
		DEBUG("get para from default\n");
		init_power_on_para();
		set_power_on_para();
		printf_power_on_para();
	}

	close(config_fd);
	if(poer_cnt.power_on_type == 0 || poer_cnt.power_on_cnt > 2)
	{
		poer_cnt.power_on_cnt = 0;
		poer_cnt.power_on_type = 0;
	}
	else
	{
		poer_cnt.power_on_type = 0;					//设备启动方式，0:上电启动，１：软件复位启动
	}

	printf("2-poer_cnt.power_on_cnt:%d\n",poer_cnt.power_on_cnt);
	printf("2-poer_cnt.power_on_type:%d\n",poer_cnt.power_on_type);

	set_power_on_para();

}

/*************************************************
Function:  printf_recv_msg
Description: 打印接收数据
Input:  待打印数据、长度
Output: 无
Return: 无
Others:
*************************************************/
void printf_recv_msg(uint8_t *temp_data,uint16_t len)
{
	for(uint16_t cnt_i = 0;cnt_i < len; cnt_i ++)
	{
		DEBUG("%x,",temp_data[cnt_i]);
	}
	DEBUG("\n");
}
/*************************************************
Function:  get_local_time
Description: 获取本机当前时间
Input:  时间结构体指针
Output: 无
Return: 无
Others:
*************************************************/
void get_local_time(struct LOCAL_TIME *timel)
{
	time_t timep;
	struct tm *ptime;
	time(&timep);
	ptime=gmtime(&timep);
	timel->year = 1900+ptime->tm_year;
	timel->mon  = 1+ptime->tm_mon;
	timel->day  = ptime->tm_mday;
	timel->hour = ptime->tm_hour;
	timel->min  = ptime->tm_min;
	timel->sec  = ptime->tm_sec;
}


/*************************************************
Function:  set_local_time
Description: 设置本机当前时间
Input:  时间结构体指针
Output: 无
Return: 无
Others:
*************************************************/
void set_local_time(struct LOCAL_TIME *timel)
{
	char date_str[50],time_str[50];
	char str[100] = {'d','a','t','e',' ','-','s'};
	char str1[100] = {'d','a','t','e',' ','-','s'};

	system("date");
	sprintf(date_str,"%d-%d-%d",timel->year,timel->mon,timel->day);
	DEBUG("date_str = %s \r\n",date_str);
	strcat(str,date_str);
	system(str);

	sprintf(time_str,"%d:%d:%d",timel->hour,timel->min,timel->sec);
	strcat(str1,time_str);
	system(str1);

	system("date");
}


/*************************************************
Function:  init_local_time
Description: init本机当前时间
Input:  时间结构体指针
Output: 无
Return: 无
Others:
*************************************************/
void init_local_time(void)
{
	struct LOCAL_TIME timel1;
	timel1.year = 2018;
	timel1.mon = 8;
	timel1.day = 8;
	timel1.hour = 8;
	timel1.min = 8;
	timel1.sec = 8;

	char date_str[50] = {0};
	char time_str[50] = {0};
	char str[100] = {'d','a','t','e',' ','-','s'};
	char str1[100] = {'d','a','t','e',' ','-','s'};

	system("date");
	sprintf(date_str,"%d-%d-%d",timel1.year,timel1.mon,timel1.day);
	printf("date_str = %s \r\n",date_str);
	strcat(str,date_str);
	system(str);

	sprintf(time_str,"%d:%d:%d",timel1.hour,timel1.min,timel1.sec);
	strcat(str1,time_str);
	system(str1);
	system("date");

	printf("init_local_time\n");

}

/*************************************************
Function:  check_sum
Description:  和校验计算
Input:  check_data 待计算数据；data_len 数据长度
Output: 无
Return: 无
Others:
*************************************************/
uint16_t check_sum(uint8_t *check_data,uint16_t data_len)
{
	uint16_t cnt_i = 0;
	uint16_t sum = 0;
	for(cnt_i = 0;cnt_i < data_len;cnt_i++)
	{
		sum += check_data[cnt_i];
	}
	return sum;
}

/*************************************************
Function:  update_system_version
Description:  更新系统软件版本到文件
Input:  系统版本结构体
Output: 无
Return: 无
Others:
*************************************************/
void update_system_version(void)
{
	uint8_t tmp_buf[50] = {0};
	int fd = open("/media/local_config/soft_version.dat",O_RDWR | O_CREAT);
	if(fd < 0)
	{
		DEBUG("open soft version file failed!\r\n");
	}

	write(fd,tmp_buf,sizeof(struct _SOFT_VERSION));
	fsync(fd);
	close(fd);
}

//void malloc_data_buff(uint16_t **data_buff,uint)
/*************************************************
Function:  malloc_two_dim_pointer_buff
Description:  申请二级指针缓存
Input:  pointer_buff 二级指针地址，row_size：二级指针行数,col_size:二级指针列数
Output: 无
Return: 无
Others:
*************************************************/
//void malloc_two_dim_pointer_buff(uint16_t ***pointer_buff,uint16_t row_size,uint16_t col_size)
//{
//	uint16_t cnt_i = 0;
//	uint16_t cnt_j = 0;
//	printf("row_size:%d,col_size:%d\n",row_size,col_size);
//	if(*pointer_buff == NULL)
//	{
//		*pointer_buff = (uint16_t **)malloc(row_size*sizeof(uint16_t *));
//		if(pointer_buff == NULL)
//		{
//			DEBUG("malloc pointer_buff failed\n");
//			return;
//		}
//
//		for(cnt_i = 0;cnt_i< row_size;cnt_i ++)
//		{
//			*pointer_buff[cnt_i] = (uint16_t *)malloc(col_size*sizeof(uint16_t));
//			if(pointer_buff[cnt_i] == NULL)
//			{
//				DEBUG("malloc pointer_buff[i] failed\n");
//				return;
//			}
//		}
//	}
//
//	DEBUG("malloc pointer_buff success one\n");
//	for(cnt_i = 0;cnt_i < row_size;cnt_i ++)
//	{
//		for(cnt_j = 0;cnt_j < col_size;cnt_j ++)
//		{
//			pointer_buff[cnt_i][cnt_j] = cnt_j;
//
//			printf("pw_ad_data.buff[cnt_i][cnt_j]:%d\n",pw_ad_data.buff[cnt_i][cnt_j]);
//		}
//
//	}
//
//
//	DEBUG("malloc pointer_buff success\n");
//
//}
/*************************************************
Function:  init_pw_ad_data
Description:  申请平稳AD数据缓存
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void init_pw_ad_data()
{
	uint8_t cnt_i = 0;
//	uint8_t cnt_j = 0;
	pw_ad_data.buff = NULL;
	pw_ad_data.column_index = 0;
	pw_ad_data.column_size = PW_AD_DATA_COL;//
	pw_ad_data.row_index = 0;
	pw_ad_data.row_size = PW_AD_DATA_ROW;

	pw_ad_data.buff = (uint16_t **)malloc(pw_ad_data.row_size*sizeof(uint16_t *));//4s-----(2+512*6)/s

	if(pw_ad_data.buff == NULL)
	{
		DEBUG("malloc pw_ad_data.buff* failed\n");
		return;
	}
	for(cnt_i = 0;cnt_i < pw_ad_data.row_size;cnt_i ++)
	{
		pw_ad_data.buff[cnt_i] = (uint16_t *)malloc(sizeof(uint16_t)*pw_ad_data.column_size);
		if(pw_ad_data.buff[cnt_i] == NULL)
		{
			DEBUG("malloc pw_ad_data.buff** failed\n");
			return;
		}
	}
}

/*************************************************
Function:  init_pw_tz_data
Description:初始化平稳特征数据
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void init_pw_tz_data()
{
	//init pw_tz_data head
	pw_tz_data.data_head.head[0] = 0xAA;
	pw_tz_data.data_head.head[1] = 0x50;
	*(uint16_t *)pw_tz_data.data_head.len = 0;			//初始化为0,每次发送数据前更新
	pw_tz_data.data_head.company_id = LUHANG;
	pw_tz_data.data_head.board_id = PW_BOARD;
	*(uint16_t *)pw_tz_data.data_head.life_signal = 0;
	*(uint16_t *)pw_tz_data.data_head.target_board_group = 0;
	pw_tz_data.data_head.resend_flag = 0;
	pw_tz_data.data_head.ack_flag = 0;
	pw_tz_data.data_head.packet_num = 0;
	memset(pw_tz_data.data_head.res,0,11);

	*(uint16_t *)pw_tz_data.soft_version = SOFT_VERSION_VAL;
	pw_tz_data.sensor_status.byte = 0x00;
#ifdef INTERNAL_PROTOCOL_20210725
	pw_tz_data.side1_x_quota.wh = 0;
	pw_tz_data.side1_x_quota.wl = 0;
	pw_tz_data.side1_y_quota.wh = 0;
	pw_tz_data.side1_y_quota.wl = 0;
	pw_tz_data.side1_z_quota.wh = 0;
	pw_tz_data.side1_z_quota.wl = 0;
	pw_tz_data.side2_x_quota.wh = 0;
	pw_tz_data.side2_x_quota.wl = 0;
	pw_tz_data.side2_y_quota.wh = 0;
	pw_tz_data.side2_y_quota.wl = 0;
	pw_tz_data.side2_z_quota.wh = 0;
	pw_tz_data.side2_z_quota.wl = 0;
#else
	pw_tz_data.side1_x_quota = 0;
	pw_tz_data.side1_y_quota = 0;
	pw_tz_data.side1_z_quota = 0;
	pw_tz_data.side2_x_quota = 0;
	pw_tz_data.side2_y_quota = 0;
	pw_tz_data.side2_z_quota = 0;
#endif
	//init alarm status
	pw_tz_data.alarm_status.byte[0] = 0;
	pw_tz_data.alarm_status.byte[1] = 0;

	//init borad_err
	pw_tz_data.borad_err.bits.pw_board_err = 0;
	memset((uint8_t *)&pw_tz_data.train_public_info,0,sizeof(struct TRAIN_PUBLIC_INFO));
	//memset(pw_tz_data.res,0,6);
	pw_tz_data.total_power_on_times[0] = 0;
	pw_tz_data.total_power_on_times[1] = 0;		//累计上电次数应该从本地读取出来
	memset(pw_tz_data.total_work_on_time,0,sizeof(pw_tz_data.total_work_on_time));
	memset(pw_tz_data.total_work_on_time,0,sizeof(pw_tz_data.total_work_on_time));

	memset(pw_tz_data.sys_cur_work_on_time,0,sizeof(pw_tz_data.sys_cur_work_on_time));

	pw_tz_data.train_public_info.carriage_number = 0;
//	memset(pw_tz_data.dc_hc_res,0,sizeof(pw_tz_data.dc_hc_res));
//	memset(pw_tz_data.pw_res,0,sizeof(pw_tz_data.pw_res));
#if defined(INTERNAL_PROTOCOL_20210416)
	memset(pw_tz_data.pw_res,0,sizeof(pw_tz_data.pw_res));
	memset(pw_tz_data.company_res,0,sizeof(pw_tz_data.company_res));
#else
	memset(pw_tz_data.km_scale,0,sizeof(pw_tz_data.km_scale));
	memset(pw_tz_data.resv,0,sizeof(pw_tz_data.resv));
#endif
	//memset();
	*(uint16_t *)pw_tz_data.check_sum = check_sum((uint8_t *)&pw_tz_data,sizeof(struct PW_TZ_DATA)-2);
	//sys_cur_work_on_time
	//pw_tz_data.train_public_info
#ifdef ADD_DIAG_TZZ_DATA_FILE
	memset(&diag_tzz_data, 0, sizeof(struct DIAG_TZZ_DATA));
	*(uint16_t *)diag_tzz_data.check_sum = check_sum((uint8_t *)&diag_tzz_data,sizeof(struct DIAG_TZZ_DATA)-2);
#endif
}

void init_pw_raw_data()
{
	//pw_raw_data.send_data_head.
	memset(&pw_raw_data.send_data_head,0,sizeof(struct SEND_DATA_HEAD));
#ifdef ORIGINAL_DATA_SAVE_ACC
	memset(pw_raw_data.ad_acc,0,sizeof(pw_raw_data.ad_acc));
#else
	memset(pw_raw_data.ad,0,sizeof(pw_raw_data.ad));
#endif
	*(uint16_t *)pw_raw_data.check_sum = check_sum((uint8_t *)&pw_raw_data,sizeof(struct PW_RAW_DATA)-2);
}

#ifdef WTD_DATA_TRANSLATE_PROTOCOL
void init_wtd_pw_tz_data()
{
	memset(&wtd_pw_tz_data, 0, sizeof(struct WTD_PW_TZ_DATA));
#ifndef WTD_DATA_PROTOCOL_20220822
	memset(&save_tz_value_data, 0, sizeof(struct TZ_VALUE_DATA));
	memset(&head_info, 0, sizeof(struct LESS_HEAD_INFO));
#endif
	memset(&save_wtd_ip, 0, sizeof(save_wtd_ip));
}

void init_wtd_pw_alarm_data()
{
	uint8_t ch_num = 0;

	printf("init_wtd_pw_alarm_data---start\n");

	memset(&wtd_pw_alarm_data, 0, sizeof(struct WTD_PW_ALARM_DATA));

	for(ch_num=0; ch_num<CHANNEL_NUM; ch_num++)
	{
//		if(ch_num == pw1_x || ch_num == pw2_x)
//			continue;

		alarm_data_queue[ch_num] = NULL;

		if(alarm_data_queue[ch_num] == NULL)
		{
			alarm_data_queue[ch_num] = QueueInit();
		}

		if(alarm_data_queue[ch_num] == NULL)
		{
			printf("\r\nalarm_data_queue == NULL\r\n");
		}

		dequeue_org_data[ch_num].deq_buf_len = SAMPLE_HZ*sizeof(int16_t);//1024B    sizeof(struct PW_RAW_DATA);
		dequeue_org_data[ch_num].deq_buf = (int8_t *)malloc(dequeue_org_data[ch_num].deq_buf_len);
		memset(dequeue_org_data[ch_num].deq_buf, 0, dequeue_org_data[ch_num].deq_buf_len);
		dequeue_org_data[ch_num].deq_buf_front_len = 0;
		dequeue_org_data[ch_num].deq_buf_rear_len = 0;

		memset(&msg_alarm_trigger[ch_num], 0, sizeof(struct MSG_ALARM_TRIG));
		memset(&alarm_ctrl[ch_num], 0, sizeof(struct MESSAGE_COUNT_CTRL));
	}
	printf("init_wtd_pw_alarm_data---end\n");
}

void reset_wtd_pw_alarm_data()
{
	uint8_t ch_num = 0;

	memset(&wtd_pw_alarm_data, 0, sizeof(struct WTD_PW_ALARM_DATA));

	for(ch_num=0; ch_num<CHANNEL_NUM; ch_num++)
	{
		printf("reset_wtd_pw_alarm_data---1---ch_num:%d\n", ch_num);
		if(alarm_data_queue[ch_num])
		{
			while(alarm_data_queue[ch_num]->next)//删除头链之后的链队
			{
				Queue_DeleteLastQueue(alarm_data_queue[ch_num]->next);
			}

			printf("reset_wtd_pw_alarm_data---2---ch_num:%d\n", ch_num);
			ClearQueue(alarm_data_queue[ch_num]);//清空复位头链
		}

		printf("reset_wtd_pw_alarm_data---3---ch_num:%d\n", ch_num);
		memset(dequeue_org_data[ch_num].deq_buf, 0, dequeue_org_data[ch_num].deq_buf_len);
		dequeue_org_data[ch_num].deq_buf_front_len = 0;
		dequeue_org_data[ch_num].deq_buf_rear_len = 0;

		memset(&msg_alarm_trigger[ch_num], 0, sizeof(struct MSG_ALARM_TRIG));
		memset(&alarm_ctrl[ch_num], 0, sizeof(struct MESSAGE_COUNT_CTRL));
	}
}
#endif

void init_ringbuff_para(struct RINGBUFF_PARA *ringbuff_para,uint16_t size)
{
	ringbuff_para->size = size;
	ringbuff_para->num = 0;
	ringbuff_para->index = 0;
}

/*************************************************
Function:  init_pw_data_para
Description:初始化平稳参数
Input:  无
Output: 无
Return: 无
Others:
*************************************************/

void init_pw_data_para()
{
	init_pw_ad_data();
	init_pw_tz_data();
	init_pw_raw_data();
	init_pw_diagnos_para();
#ifdef DATA_CENTER_TEST
	init_test_data();
#endif
#ifdef WTD_DATA_TRANSLATE_PROTOCOL
	init_wtd_pw_tz_data();
	init_wtd_pw_alarm_data();
#endif
	init_ringbuff_para(&pw_diagnos_ringbuf_para,PW_AD_DATA_ROW);
	init_ringbuff_para(&pw_save_ringbuf_para,PW_AD_DATA_ROW);
	init_ringbuff_para(&pw_send_ringbuf_para,PW_AD_DATA_ROW);
}

void init_soft_version()
{
	pw_soft_version.pw_soft_version = SOFT_VERSION_VAL;
	pw_soft_version.pw_update_time = SOFT_UPDATE_TIME;
}

void init_comm_data_cnt()
{
	memset((uint8_t *)&comm_data_cnt,0,sizeof(struct COMM_DATA_CNT));

	memset(&recv_public_para,0,sizeof(struct RECV_PUBLIC_PARA));
}

void init_eth_status()
{
	init_eth_connect_status();
}

void init_can_status()
{
	init_can_connect_status();
}

void init_file_operate()
{
	file_operate.file_delete_flag = 0;
	file_operate.time_update_flag = 0;
}
/*************************************************
Function:  init_data
Description: 读取配置参数，创建存储文件
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void init_user_data(void)
{

	pw_clb_config_para =(struct PW_CLB_CONFIG_PARA*)malloc(sizeof(struct PW_CLB_CONFIG_PARA));
	memset(pw_clb_config_para, 0, sizeof(struct PW_CLB_CONFIG_PARA));



	get_para_from_local(); //从本地读取配置

	init_self_test_para();

	init_soft_version();
//	init_comm_data_cnt();

	init_file_operate();

}

uint16_t sum_check_16(uint8_t *data, uint16_t crclen)
{
	uint16_t check_sum = 0;
	for(uint16_t i = 0; i < crclen; i++)
	{
		check_sum += data[i];
	}
	return check_sum;
}

void init_sw_ad_data()
{
	uint8_t cnt_i = 0;
//	uint8_t cnt_j = 0;
	sw_ad_data.buff = NULL;
	sw_ad_data.column_index = 0;
	sw_ad_data.column_size = SW_AD_DATA_COL;//
	sw_ad_data.row_index = 0;
	sw_ad_data.row_size = SW_AD_DATA_ROW;

	sw_ad_data.buff = (uint16_t **)malloc(sw_ad_data.row_size*sizeof(uint16_t *));

	if(sw_ad_data.buff == NULL)
	{
		DEBUG("malloc SW_AD_DATA.buff* failed\n");
		return;
	}
	for(cnt_i = 0;cnt_i < sw_ad_data.row_size;cnt_i ++)
	{
		sw_ad_data.buff[cnt_i] = (uint16_t *)malloc(sizeof(uint16_t)*sw_ad_data.column_size);
		if(sw_ad_data.buff[cnt_i] == NULL)
		{
			DEBUG("malloc SW_AD_DATA.buff** failed\n");
			return;
		}
	}
}

void init_sw_tz_data()
{
	*(uint16_t *)sw_tz_data.data_head.head =little_to_big_16bit(0xAA55);
	*(uint16_t *)sw_tz_data.data_head.len = little_to_big_16bit(256);
	sw_tz_data.data_head.company_id = LUHANG;
	sw_tz_data.data_head.board_id = PW_BOARD;
	*(uint16_t *)sw_tz_data.data_head.life_signal = little_to_big_16bit(20);
	sw_tz_data.data_head.resend_flag = 24;
	*(uint16_t *)sw_tz_data.data_head.target_board_group = little_to_big_16bit(25);
	sw_tz_data.data_head.ack_flag = 26;
	sw_tz_data.data_head.packet_num =27;
	memset(sw_tz_data.data_head.res,0,sizeof(sw_tz_data.data_head.res));
	*(uint16_t *)sw_tz_data.soft_version = little_to_big_16bit(SOFT_VERSION_VAL);
	sw_tz_data.sensor_status.bits.bogie_axis1_self_test_err = 0;
	sw_tz_data.sensor_status.bits.bogie_axis2_self_test_err = 0;
	sw_tz_data.sensor_status.bits.bogie_axis3_self_test_err = 0;
	sw_tz_data.sensor_status.bits.bogie_axis4_self_test_err = 0;
	sw_tz_data.sensor_status.bits.bogie_axis1_real_time_err = 0;
	sw_tz_data.sensor_status.bits.bogie_axis2_real_time_err = 0;
	sw_tz_data.sensor_status.bits.bogie_axis3_real_time_err = 0;
	sw_tz_data.sensor_status.bits.bogie_axis4_real_time_err = 0;
	sw_tz_data.relay_status.bits.low_speed_in_relay_err = 0;
	sw_tz_data.relay_status.bits.mid_speed_in_relay_err = 1;
	sw_tz_data.relay_status.bits.high_speed_in_relay_err = 1;
	sw_tz_data.relay_status.bits.res = 0;
	sw_tz_data.relay_status.bits.bogie_out_relay_err = 0;
	sw_tz_data.relay_status.bits.device_out_relay_err = 0;
	sw_tz_data.relay_status.bits.sensor_test_out_relay_err = 0;
	sw_tz_data.alarm_status.bits.bogie_axis1_alarm = 0;
	sw_tz_data.alarm_status.bits.bogie_axis2_alarm = 0;
	sw_tz_data.alarm_status.bits.bogie_axis3_alarm = 0;
	sw_tz_data.alarm_status.bits.bogie_axis4_alarm = 0;
	sw_tz_data.alarm_status.bits.bogie_alarm = 0;
	sw_tz_data.warn_status.bits.bogie_axis1_warn = 0;
	sw_tz_data.warn_status.bits.bogie_axis2_warn = 0;
	sw_tz_data.warn_status.bits.bogie_axis3_warn = 0;
	sw_tz_data.warn_status.bits.bogie_axis4_warn = 0;
	sw_tz_data.warn_status.bits.bogie_warn = 0;
	sw_tz_data.total_bogie_err_cnt = 0;
	sw_tz_data.bogie_err_para.bits.para1_grade = 0;
	sw_tz_data.bogie_err_para.bits.para2_grade = 0;
	sw_tz_data.bogie_err_para.bits.max_amp_bai = 0;
	sw_tz_data.bogie_err_para.bits.max_amp_qian = 0;
	sw_tz_data.bogie_err_para.bits.max_amp_ge = 0;
	sw_tz_data.bogie_err_para.bits.max_amp_shi = 0;
	sw_tz_data.bogie_err_para.bits.over_amp_cycle_ge = 0;
	sw_tz_data.bogie_err_para.bits.over_amp_cycle_shi = 0;
	sw_tz_data.borad_err.bits.save_err = 0;
	sw_tz_data.borad_err.bits.power1_err = 0;
	sw_tz_data.borad_err.bits.power2_err = 0;
	sw_tz_data.borad_err.bits.ad_err = 0;
	sw_tz_data.borad_err.bits.sw_board_err = 0;
	sw_tz_data.borad_err.bits.res = 0;
	sw_tz_data.com_err.bits.ctrla_eth_err = 0;
	sw_tz_data.com_err.bits.ctrla_can_err = 0;
	sw_tz_data.com_err.bits.ctrla_eth_err = 0;
	sw_tz_data.com_err.bits.ctrla_can_err = 0;

	sw_tz_data.train_public_info.year = 20;
	sw_tz_data.train_public_info.mon = 20;
	sw_tz_data.train_public_info.day = 20;
	sw_tz_data.train_public_info.hour = 20;
	sw_tz_data.train_public_info.min = 20;
	sw_tz_data.train_public_info.sec = 20;
	sw_tz_data.train_public_info.train_type = 1;

	sw_tz_data.train_public_info.carriage_number = 1;
//	sw_tz_data.train_public_info.motor_or_trailer_flag = 20;
//	sw_tz_data.train_public_info.rev[0] = 20;
//	sw_tz_data.train_public_info.rev[1] = 20;
//	sw_tz_data.train_public_info.digital_input = 1;
//	sw_tz_data.train_public_info.digital_output = 1;
//	memset(sw_tz_data.train_public_info.company_rev,0,8);
	sw_tz_data.train_public_info.marshalling.bits.ge = 1;
	sw_tz_data.train_public_info.marshalling.bits.shi = 1;
	sw_tz_data.train_public_info.marshalling.bits.bai = 1;
	sw_tz_data.train_public_info.marshalling.bits.qian = 1;
	*(uint16_t *)sw_tz_data.train_public_info.speed = little_to_big_16bit(250);
	sw_tz_data.train_public_info.train_outer_temp = 1;
	sw_tz_data.train_public_info.ctrl_train_mode = 1;
//	sw_tz_data.train_public_info.gps_data.valid.bits.air_spring_pressure = 1;
	sw_tz_data.train_public_info.gps_data.valid.bits.gps = 1;
	sw_tz_data.train_public_info.gps_data.air_spring_pressure1 = 1;
	sw_tz_data.train_public_info.gps_data.air_spring_pressure2 = 1;
	sw_tz_data.train_public_info.gps_data.longitude_mid_down = 1;
	sw_tz_data.train_public_info.gps_data.longitude_down = 1;
	sw_tz_data.train_public_info.gps_data.longitude_mid_up = 1;
	sw_tz_data.train_public_info.gps_data.longitude_dir = 1;
	sw_tz_data.train_public_info.gps_data.latitude_dir = 1;
	sw_tz_data.train_public_info.gps_data.latitude_mid_down = 1;
	sw_tz_data.train_public_info.gps_data.latitude_down = 1;
	sw_tz_data.train_public_info.gps_data.latitude_up = 1;
	sw_tz_data.train_public_info.gps_data.latitude_mid_up = 1;
//	sw_tz_data.res = 0;											//预留
	//*(uint16_t *)sw_tz_data.total_power_on_times = little_to_big_16bit(16);
	//*(uint16_t *)sw_tz_data.total_work_on_time = little_to_big_16bit(16);

	//*(uint32_t *)sw_tz_data.sys_cur_work_on_time = little_to_big_32bit(60);

#if defined(INTERNAL_PROTOCOL_20210416)
	memset(sw_tz_data.sw_res,0x0,sizeof(sw_tz_data.sw_res));
	memset(sw_tz_data.company_res,0x0,sizeof(sw_tz_data.company_res));
#else
	memset(sw_tz_data.km_scale,0x55,sizeof(sw_tz_data.km_scale));
	memset(sw_tz_data.resv,0x66,sizeof(sw_tz_data.resv));
#endif

	*(uint16_t *)sw_tz_data.check_sum = little_to_big_16bit(check_sum((uint8_t *)&sw_tz_data,sizeof(struct SW_TZ_DATA)-2));

}

void init_sw_raw_data()
{
	//pw_raw_data.send_data_head.
	memset(&sw_raw_data.send_data_head,0,sizeof(struct SEND_DATA_HEAD));
	memset(sw_raw_data.ad,0,sizeof(sw_raw_data.ad));
	*(uint16_t *)sw_raw_data.check_sum = check_sum((uint8_t *)&sw_raw_data,sizeof(struct SW_RAW_DATA)-2);
}

void init_sw_data_para()
{
	init_sw_ad_data();
	init_sw_tz_data();
	init_sw_raw_data();


	init_sw_diagnos_para();
	init_ringbuff_para(&sw_diagnos_ringbuf_para,SW_AD_DATA_ROW);
	init_ringbuff_para(&sw_save_ringbuf_para,SW_AD_DATA_ROW);
	init_ringbuff_para(&sw_send_ringbuf_para,SW_AD_DATA_ROW);
}
