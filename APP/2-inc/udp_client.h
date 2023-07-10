#ifndef _UDP_CLIENT_H
#define _UDP_CLIENT_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "unistd.h"
#include "user_data.h"
#include "global_macro.h"


#define RSYNC_UPDATE_GET "/usr/local/sync_update_get.sh"
#define RSYNC_UPDATE_PUT "/usr/local/sync_update_put.sh"

/*内部网络命令相关定义*/
enum INSIDE_NET_CMD
{
	APP_UPDATE_CMD = 0x01,
	SYSTEM_REBOOT_CMD = 0x02,
	CONFIG_PARA_SET_CMD = 0x03,
	CONFIG_PARA_READ_CMD = 0x04,
	SYNC_DATA_CMD  = 0x55,//主动同步数据
};

/*
 *udp发送模式
 * */
struct UDP_SEND_STYLE
{
	int  is_ucast_flag;						//单播还是组播
	int  is_need_ack_flag;
	uint32_t time_out_ms;
	uint32_t resend_cnt;
	uint32_t ack_byte_of_buff;
};

struct UDP_SEND_ADDR
{
    uint16_t board_set;                          //表示要发送的板卡地址Bit
    uint16_t target_num;                         //表示要发送多少个板卡
    uint16_t target_board_addr[16];				 //要发送的板卡地址
    struct sockaddr_in send_target_sockaddr;
    struct sockaddr_in ucast_target_sockaddr[16]; //表示要发送的板卡ip数组
};

extern struct sockaddr_in mcast_ctl_save_group_addr;
extern struct sockaddr_in ucast_io_board_addr;
extern struct sockaddr_in ucast_ctrla_board_addr;
extern struct sockaddr_in ucast_ctrlb_board_addr;
extern struct sockaddr_in ucast_save_board_addr;
extern struct sockaddr_in ucast_axle_tempa_board_addr;
extern struct sockaddr_in ucast_axle_tempb_board_addr;
extern struct sockaddr_in ucast_st_board_addr;
extern struct sockaddr_in ucast_ad_board_addr;
extern struct sockaddr_in ucast_vibr_board_addr;
extern struct sockaddr_in local_addr;
int udp_send(uint8_t *buff, uint16_t buff_size, struct sockaddr *__addr, socklen_t __addr_len, const struct UDP_SEND_STYLE style);

void init_eth_connect_status(void);
void add_eth_not_recv_cnt(void);
void init_udp_send_addr();
int init_udp(void);
void init_communicate_type();
void app_multicast_send(void *data, uint16_t size);
void send_pw_tz_data(struct PW_TZ_DATA *send_tz_data,uint16_t singal_num,uint16_t target_addr,uint8_t resend_flag,uint8_t ack_flag);
void send_pw_tz_data_can(struct PW_TZ_DATA *send_tz_data,uint16_t singal_num,uint16_t target_addr,uint8_t resend_flag,uint8_t ack_flag);
#ifdef ORIGINAL_DATA_SAVE_ACC
	void send_pw_orignal_data(int16_t *send_buf,uint16_t send_head,uint16_t singal_num,uint16_t target_addr,uint8_t resend_flag,uint8_t ack_flag);
#else
	void send_pw_orignal_data(uint16_t *send_buf,uint16_t send_head,uint16_t singal_num,uint16_t target_addr,uint8_t resend_flag,uint8_t ack_flag);
#endif
#endif
