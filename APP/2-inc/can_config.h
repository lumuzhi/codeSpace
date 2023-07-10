#ifndef _CAN_CONFIG_H_
#define _CAN_CONFIG_H_

#include "user_data.h"
#include "global_macro.h"

extern uint8_t can_reset_flag;
/***********下面是CAN相关定义**************************/
struct list_node  //引用了rtt下面的链表节点定义
{
    struct list_node *next;
    struct list_node *prev;
};
typedef struct list_node list_t;

typedef enum FRAME_TYPE
{
    SINGLE,
	MULTI_BEGIN,
    MULTI_MID,
    MULTI_END

}Frame_type;


struct FRAME
{
	  uint32_t cnt;
	  uint32_t size;
	  enum BOOL deal_flag;
      uint8_t *rx_buf;
};

struct FRAME_DEAL
{
    list_t	flist;
	uint32_t tag_id;
	uint32_t souc_id;

	struct FRAME *frame_tmp;
    struct FRAME *frame_rd;
    struct FRAME *frame_nor;
    void (*deal)(uint8_t *data, uint32_t len);
};

#ifdef CAN_PROTOCOL_20210322
typedef union CANIDT
{
  struct{
	uint32_t id_cnt    :5;//帧计数（0-31）        Bit4-Bit0
	uint32_t id_total  :5;//帧总数（最大31帧 0-31） Bit9-Bit5
	uint32_t pag_life  :2;//包生命信号（0-3循环加）  Bit11-Bit10
	uint32_t rx_mask   :12;//接受板卡掩码          Bit23-Bit12
	uint32_t tx_source :4;//发送板卡地址           Bit27-Bit24
	uint32_t type  	   :1;//通信种类（1：正常帧）    Bit28
	uint32_t nc        :3;//                    Bit31-Bit29
  }BITS;
	uint32_t WORD;
}CANID_t;

/*can 通信接收掩码*/
typedef enum CAN_COMM_RX_MASK
{
#ifdef 	ADD_MOTOR_BOARD
	MOTOR_RX_MASK=  0x1,////0x1000,
#endif
	GEARBOX_RX_MASK=0x2,//0x2000,
	CTRLA_RX_MASK=  0x4,//0x4000,
	CTRLB_RX_MASK=  0x8,//0x8000,
	SAVE_RX_MASK=      0x10,//0x10000,
	AXLE_TEMPA_RX_MASK=0x20,//0x20000,
	AXLE_TEMPB_RX_MASK=0x40,//0x40000,
	PW_RX_MASK=        0x80,//0x80000,
	SW_RX_MASK=   0x100,//0x100000,
	VIBR_RX_MASK= 0x200,//0x200000,
	I0_RX_MASK=   0x400,//0x400000,
	POWER_RX_MASK=0x800//0x800000
}Rx_Mask;

#else

typedef union CANIDT
{
  struct{
	uint32_t id_cnt   :5;//帧计数（0-31）
	uint32_t id_total :5;//帧总数（最大31帧 0-31）
	uint32_t pag_life :3;//包生命信号（0-7循环加）
	uint32_t rx_mask  :11;//接受板卡掩码（bit13-bit23）
	uint32_t tx_source :4;//发送板卡地址
	uint32_t type  	  :1;//通信种类（1：正常帧）
	uint32_t nc       :3;
  }BITS;
	uint32_t WORD;
}CANID_t;

/*can 通信接收掩码*/
typedef enum CAN_COMM_RX_MASK
{
	GEARBOX_RX_MASK=0x001,
	CTRLA_RX_MASK=0x002,
	CTRLB_RX_MASK=0x004,
	SAVE_RX_MASK=0x008,
	AXLE_TEMPA_RX_MASK=0x010,
	AXLE_TEMPB_RX_MASK=0x020,
	PW_RX_MASK=0x040,
	SW_RX_MASK=0x080,
	VIBR_RX_MASK=0x100,
	I0_RX_MASK=0x200,
	POWER_RX_MASK=0x400
}Rx_Mask;
#endif

/*canID地址分配*/
typedef enum CAN_COMM_BOARD_ADDR
{
	CTRLA_BOARD_CANID=0x0,
	CTRLB_BOARD_CANID=0x1,
	SAVE_BOARD_CANID,
	AXLE_TEMPA_BOARD_CANID,
	AXLE_TEMPB_BOARD_CANID,
	PW_BOARD_CANID,
	SW_BOARD_CANID,
	VIBR_BOARD_CANID,
	I0_BOARD_CANID,
	POWER_BOARD_CANID=0x9,
	GEARBOX_BOARD_CANID=0xa//齿轮箱板
#ifdef 	ADD_MOTOR_BOARD
	,MOTOR_BOARD_CANID=0x0b
#endif
}CANID_Addr;



#define rt_list_entry(node, type, member) \
    ((type *)((char *)(node) - (unsigned long)(&((type *)0)->member)))


/*************************************************
Function:  can_send_data
Description: can消息发送函数
Input:  要发送的数据指针:data  数据的长度:len
Output: 无
Return: 无
Others:
*************************************************/
void can_send_data(uint8_t *data, uint16_t len,uint16_t tagmask);

/*************************************************
Function:  can_tx_thread_entry
Description: can发送线程入口
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void can_tx_thread_entry(void *parameter );

/*************************************************
Function:  can_tx_init控制板A
Description: 初始化can的发送队列
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void can_tx_init(void);

/*************************************************
Function:    can_send
Description: 组帧发送can消息
Input:  目标ID:taget
		命令类型:cmd
		发送数据指针:data
		数据长度:len
Output: 无
Return: 无
Others:
*************************************************/
void can_send(uint16_t taget, uint8_t cmd, uint8_t *data, uint16_t len,uint16_t life_signal);


/*************************************************
Function:    config_can
Description: 配置socket can的相关参数
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void config_can();

/*************************************************
Function:    protocol_deal
Description: can数据协议解析
Input:  接受的can数据指针:data
		数据的长度:data_len
Output: 无
Return: 无
Others:
*************************************************/
void protocol_deal(CANID_t can_rx_id, uint8_t *data);


/*************************************************
Function:    init_can_socket
Description: 配置和初始化can
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
int init_can_socket();
void restart_can_dev(void);
int init_can_socket_reset(void);
void config_can(void);
void init_can_data_buff();
#endif
