#ifndef _CAN_DATA_DEAL_H
#define _CAN_DATA_DEAL_H
#include <stdint.h>





//Board ID,用于can通信
enum BOARD_TYPE
{
	  MAIN_BOARD = 0x0f,   // 处理板
	  PRE_BOARD1 = 0x01,   // 接口板1
	  PRE_BOARD2 = 0x02,   //接口板2
      MVB_BOARD = 0x03,    //通信板

};



//命令类型
enum CAN_CMD
{
	SYSTEM_RESET_CMD = 0x00,  //
	UPDATE_PRE_BOARD_CMD = 0x01, //
	UPDATE_MVB_BOARD_CMD = 0x02, //
	SWITCH_BEARING_CH_CMD = 0x03, //
	SWITCH_POLYGON_CH_CMD = 0x04, //
	GET_TEMP_CMD = 0x05,  //
	GET_VERSION_CMD = 0x06,
	TRAIN_MESSAGE = 0x10,
	SPEED_TIME_MESSAGE = 0x11,
	WHEEL_MESSAGE  = 0x12,
	SYS_STATUS = 0x13,//
	SENSOR_STATUS = 0x14,    //
	TEMP_MESSAGE = 0x15,
	PARA_CONFIG_MESSAGE = 0x16,

	MVB_IAP_MESSAGE = 0x41,
	PRE1_IAP_MESSAGE = 0x42,
	PRE2_IAP_MESSAGE = 0x43,

	IAP_WAIT_MESSAGE = 0x50,
	IAP_RESET_MESSAGE = 0x51,


//	SYSTEM_RESET_CMD = 0x00,  //复位重启
//	UPDATE_PRE_BOARD_CMD = 0x01, //升级前置处理器
//	UPDATE_MVB_BOARD_CMD = 0x02, //升级通信板
//	SWITCH_BEARING_CH_CMD = 0x03, //切换轴承采样通道
//	SWITCH_POLYGON_CH_CMD = 0x04, //切换多边形采样通道
//	GET_TEMP_CMD = 0x05,  //从前置处理器请求温度
//	GET_VERSION_CMD = 0x06,
//
//	TRAIN1_MESSAGE = 0x10,
//	TRAIN2_MESSAGE = 0x11,
//	TRAIN3_MESSAGE = 0x12,
//	TRAIN4_MESSAGE = 0x13,
//	TRAIN5_MESSAGE = 0x14,
//	TRAIN6_MESSAGE = 0x15,
//	TRAIN7_MESSAGE = 0x16,
//	TRAIN8_MESSAGE = 0x17,
//	SPEED_TIME_MESSAGE = 0x18,
//	WHEEL_MESSAGE  = 0x19,
//	SYS_STATUS = 0x1a,//系统状态
//	SENSOR_STATUS = 0x1b,    //传感器状态
//	TEMP_MESSAGE = 0x1c,
//	PARA_CONFIG_MESSAGE = 0x1d,
//
//
//	MVB_IAP_MESSAGE = 0x41,
//	PRE1_IAP_MESSAGE = 0x42,
//	PRE2_IAP_MESSAGE = 0x43,
//
//	IAP_WAIT_MESSAGE = 0x50,
//	IAP_RESET_MESSAGE = 0x51

};


/*************************************************
Function:  can_send_switch_bearing_ch
Description:  通知接口板切换振动通道的通知
Input:  目标ＩＤ:taget
		切换的通道号：icp
Output:
Return:
Others:
*************************************************/
void can_send_switch_bearing_ch(uint8_t taget,uint8_t icp);


/*************************************************
Function:  can_send_switch_polygon_ch
Description:  通知接口板切换振动通道的通知
Input:  目标ＩＤ:taget
		切换的通道号：icp
Output:
Return:
Others:
*************************************************/
void can_send_switch_polygon_ch(uint8_t taget,uint8_t icp);


/*************************************************
	Function:  can_send_tboard1
	Description: 通知接口板１发送温度数据
	Input:
	Output:
	Return:
	Others:
*************************************************/
void can_send_tboard1();

/*************************************************
	Function:  can_send_tboard2
	Description:  通知接口板２发送温度数据
	Input:
	Output:
	Return:
	Others:
*************************************************/
void can_send_tboard2();


/*************************************************
	Function:    recv_deal
	Description: can数据解析
	Input:  要解析的数据指针：data
			数据长度:len
	Output: 无
	Return: 无
	Others:
*************************************************/
void recv_deal(uint8_t *data, uint32_t len);


void init_can_connect_status(void);

void add_can_not_recv_cnt(void);

#endif
