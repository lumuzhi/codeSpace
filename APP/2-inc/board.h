#ifndef _BOARD_H_
#define _BOARD_H_
#include <stdint.h>
//#include "mvb.h"

//#ifndef __DEBUG__
//#define DEBUG(format,...)  printf(format,##__VA_ARGS__)
//#else
//#define DEBUG(format,...)
//#endif

/*京雄高铁相关定义*/
//sw_recv_mcast_msg_from_CTRLB

//    "0x01：时代
//     0x02：华高
//     0x03：纵横
//     0x04：路航
//     0x05：四方所
//     0x06：紫台
//     0x07：柏盛源"
//enum COMPANY_ID
//{
//    SHIDAI = 0x01,
//    HUAGAO = 0x02,
//    ZONGHENG = 0x03,
//    LUHANG = 0x04,
//    SIFANGSUO = 0x05,
//    ZITAI = 0x06,
//    BAISHENGYUAN = 0x07
//};

// 1	IO	    192.168.1.11	   8000
// 2	控制板A	 192.168.1.12	    8000
// 3	控制板B	 192.168.1.13	    8000
// 4	记录板	 192.168.1.14	    8000
// 5	轴温A	 192.168.1.15	    8000
// 6	轴温B	 192.168.1.16	    8000
// 7	平稳	 192.168.1.17	    8000
// 8	失稳	 192.168.1.18	    8000
// 9	振动	 192.168.1.19	    8000
//#define MCAST_CTL_SAVE_GROUP_ADDR "239.255.17.10"
//#define MCAST_CTL_SAVE_GROUP_PORT 2710
//
//#define TRAIN_INFO_GROUP_ADDR "239.255.2.0"
//#define TRAIN_INFO_GROUP_PORT 1100
//
//#define UCAST_IO_BOARD_ADDR "192.168.1.11"
//#define UCAST_IO_BOARD_PORT 8000
//
//#define UCAST_CTRLA_BOARD_ADDR "192.168.1.12"
//#define UCAST_CTRLA_BOARD_PORT 8000
//
//#define UCAST_CTRLB_BOARD_ADDR "192.168.1.13"
//#define UCAST_CTRLB_BOARD_PORT 8000
//
//#define UCAST_SAVE_BOARD_ADDR "192.168.30.100"
//#define UCAST_SAVE_BOARD_PORT 8000
////#define UCAST_SAVE_BOARD_ADDR "192.168.1.14"
////#define UCAST_SAVE_BOARD_PORT 8000
//
//#define UCAST_AXLE_TEMPA_BOARD_ADDR "192.168.1.15"
//#define UCAST_AXLE_TEMPA_BOARD_PORT 8000
//
//#define UCAST_AXLE_TEMPB_BOARD_ADDR "192.168.1.16"
//#define UCAST_AXLE_TEMPB_BOARD_PORT 8000
//
//#define UCAST_ST_BOARD_ADDR "192.168.1.17"
//#define UCAST_ST_BOARD_PORT 8000
//
//#define UCAST_AD_BOARD_ADDR "192.168.1.18"
//#define UCAST_AD_BOARD_PORT 8000
//
//#define UCAST_VIBR_BOARD_ADDR "192.168.1.19"
//#define UCAST_VIBR_BOARD_PORT 8000


//struct UDP_SEND_ADDR
//{
//    uint16_t board_set;                           //表示要发送的板卡地址Bit
//    uint16_t target_num;                          //表示要发送多少个板卡
//    uint16_t target_board_addr[16];				 //要发送的板卡地址
//    struct sockaddr_in send_target_sockaddr;
//    struct sockaddr_in ucast_target_sockaddr[16]; //表示要发送的板卡ip数组
//};

//// 0x11：控制板A
//// 0x22：控制板B
//// 0x33：记录板
//// 0x44：轴温A
//// 0x55：轴温B
//// 0x66：平稳
//// 0x77：失稳
//// 0x88：振动
//// 0x99：IO板
//// 其他预留
//
//#define CTRLA_BOARD 0x11
//#define CTRLB_BOARD 0x22
//#define SAVE_BOARD 0x33
//#define AXLE_TEMPA_BOARD 0x44
//#define AXLE_TEMPB_BOARD 0x55
//#define PW_BOARD 0x66
//#define SW_BOARD 0x77
//#define VIBR_BOARD 0x88
//#define IO_BOARD 0X99

enum BIT_IP_SEQ
{
	BIT_CTRLA=0,
	BIT_CTRLB=1,
	BIT_SAVE=2,
	BIT_TEMPA=3,
	BIT_TEMPB=4,
	BIT_ST=5,
	BIT_AD=6,
	BIT_VIBR=7,
	BIT_IO=8,
	BIT_GEAR=9,
	BIT_MOTOR=0xA
};
#define LOCAL_BOARD PW_BOARD

#define FTP_VIR_DIR_NAME_SIZE 16
#define FTP_TEMP_DIR_NAME_SIZE 36

#endif

