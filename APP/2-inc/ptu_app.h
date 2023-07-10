#ifndef _PTU_APP_H
#define _PTU_APP_H

#include <stdint.h>
#include "user_data.h"


//enum PTU_CMD
//{
//	SEND_SYSTEM_FILE_CMD = 0x00,							//系统升级命令
//	SEND_APP_FILE_CMD = 0x01,								//应用程序升级
//	SYSTEM_REBOOT_CMD = 0x02,								//系统重启
//	CONFIG_PARA_CMD = 0x03,									//参数配置
//	CONFIG_PARA_READ_CMD = 0x04,							//参数读取
//	STATUS_MONITOR_CMD = 0x05,								//状态监控命令
//	START_DOWNLOAD_CMD = 0x06,								//开始下载命令
//	DOWNLOAD_OK_CMD = 0x07,									//下载完成命令
//	PTU_GET_VERSION_CMD = 0x08,								//获取软件版本
//	PTU_START_SIMULATE = 0X09,								//PTU模拟置位开始
//	PTU_STOP_SIMULATE = 0x0A,								//PTU模拟置位停止
//	SEND_PUBLIC_DATA_CMD ,								//发送公共数据到PTU
//	SEND_COMM_DATA_CMD
//};



//PTU相关标志
struct PTU_DATA
{
	int ptu_net_fd;
	uint8_t ptu_net_connect;
	uint8_t ptu_simulation_flag;
	uint16_t life_count;
};


//PTU监控数据
struct PTU_MONITOR_DATA
{
	struct PW_TZ_DATA monitor_pw_tz_data;
	struct SW_TZ_DATA monitor_sw_tz_data;
};

//PTU模拟
struct PTU_SIMULATION_DATA
{
	struct PW_TZ_DATA simulation_pw_tz_data;
	SW_SIMULATION simulation_sw_st;
};



/*************************************************
Function:    send_paras_to_ptu
Description: 发送参数给PTU
Input:　设备地址:dev_num
Output:
Return:
Others:
*************************************************/
void send_paras_to_ptu(uint8_t dev_num);

int init_ptu_thread();


/*************************************************
Function:    ptu_recv_thread_entry
Description: ptu接收线程
Input:
Output:
Return:
Others:
*************************************************/
void ptu_recv_thread_entry();


/*************************************************
Function:    ptu_data_deal
Description: ptu数据解析
Input:　被解析数据:data
	   数据长度:len
Output:
Return:
Others:
*************************************************/
void ptu_data_deal(uint8_t *data,uint32_t len);


/*************************************************
Function:    init_net_thread
Description: 初始化PTU网络线程
Input:
Output:
Return:
Others:
*************************************************/
int init_ptu_thread();



/*************************************************
Function:    restart_ftp_ok_ack
Description: 重启FTP成功返回
Input:
Output:
Return:
Others:
*************************************************/
void restart_ftp_ok_ack();


/*************************************************
Function:    config_ok_ack
Description: ptu配置成功返回
Input:　设备地址:dev_num
	   成功标识:flag:1成功，０失败
Output:
Return:
Others:
*************************************************/
void start_download_ack(void);
void app_iap_check(void );
void ptu_send(uint8_t taget, uint8_t cmd, uint8_t *data, uint16_t len);
#endif
