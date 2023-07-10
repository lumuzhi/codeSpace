#ifndef _UPDATE_H_
#define _UPDATE_H_
#include <stdint.h>





//内部CAN升级相关标志
struct CAN_UPDATE_DATA
{
	uint8_t update_count;
	uint8_t update_flag;  //1正在升级，０没有在升级
    uint8_t update_message;  //升级时的命令符标识
    uint8_t recv_normal_data;
    uint8_t recv_error_data;
    uint8_t open_count;
    uint32_t send_group_count; //发送的组数计数

};


//网络升级相关标志
struct NET_UPDATE_DATA
{
	uint8_t update_count;
	uint8_t update_flag;  //1正在升级，０没有在升级
    uint8_t update_message;  //升级时的命令符标识
    uint8_t recv_normal_data;
    uint8_t recv_error_data;
    uint8_t open_count;
    uint32_t send_group_count; //发送的组数计数
    uint8_t train_num[2];  //车号　＋　板卡号　　　处理板：１　　通信板：２　　　前置处理器１：　３　　　　　前置处理器２：　４
    uint8_t update_type; //０表示can,1表示485

};

//升级时内部命令标识
enum CAN_UPDATE_CMD
{
	START_IAP,//准备升级
	SEND_IAP,//数据发送
	ACK_OK,//正确应答
	ACK_ERR,//错误应答
	END_IAP//完成数据发送
};


/*************************************************
Function:  send_iap_reset_message
Description: 通知相关板卡跳转到底包
Input:	目标板卡地址
Output:
Return:
Others:
*************************************************/
void can_send_iap_reset_message(uint8_t taget);


/*************************************************
Function:  can_send_iap_start_message
Description: 通知开始ＩＡＰ升级
Input:	目标板卡地址，命令
Output:
Return:
Others:
*************************************************/
void can_send_iap_start_message(uint8_t taget,uint8_t cmd_id);

/*************************************************
Function:    can_send_bin_jkb1
Description: 发送bin数据给前置处理器１
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void can_send_bin_jkb1(uint8_t *bin_data,uint16_t len);


/*************************************************
Function:    can_send_bin_jkb2
Description: 发送bin数据给前置处理器２
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void can_send_bin_jkb2(uint8_t *bin_data,uint16_t len);



/*************************************************
Function:    can_send_bin_mvb
Description: 发送数据给通信板
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void can_send_bin_mvb(uint8_t *bin_data,uint16_t len);


/*************************************************
Function:  net_send_reset_message
Description: 通知相关板卡跳转到底包
Input:	目标板卡地址
Output:
Return:
Others:
*************************************************/
void net_send_bin_data(int sock_fd,uint8_t taget,uint8_t *bin_data,uint16_t len);

/*************************************************
Function:  net_send_start_iap
Description: 发送开始网络升级
Input:	目标板卡地址
Output:
Return:
Others:
*************************************************/
void net_send_start_iap(int sock_fd,uint8_t taget);


/*************************************************
Function:    can_send_start_iap
Description: 发送升级开始消息
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void can_send_start_iap(uint8_t taget,enum CAN_CMD message_type );


#endif
