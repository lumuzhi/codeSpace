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
#include <linux/if.h>
#include <linux/can/raw.h>
#include <semaphore.h>
#include "can_config.h"
#include "user_data.h"
#include "can_data_deal.h"
#include "pthread_policy.h"
#include "board.h"


#define PRO_HEAD_LEN   13
#define CAN_RX_BUF_SIZE			256
#define CAN_TX_BUF_SIZE			256
#define CAN_RX_RD_SIZE 1024
#define CAN_RX_SIZE 1024
#define RX_LEN_MAX  512
#define AF_CAN       PF_CAN
#define CAN_EFF_FLAG 0x80000000U //扩展帧的标识
#define CAN_RTR_FLAG 0x40000000U //远程帧的标识
#define CAN_ERR_FLAG 0x20000000U //错误帧的标识，用于错误检查
#define CAN_EFF_MASK 0x1FFFFFFFU//mask

#define CAN_LEN      0x08
#define CAN_AND      0x07
#define CAN_R_BITS   0x03
#define CAN_DEAL_FUN  recv_deal

#ifdef	INTERNAL_PROTOCOL_20210416
	#ifdef PUB_INFO_REMOVE_KM_FLAG
		#define CAN_RECV_PUBLIC 0X05
	#else
		#define CAN_RECV_PUBLIC 0X07
	#endif
#else
	#define CAN_RECV_PUBLIC 0X05
#endif


#ifdef CAN_ERR_REBOOT_TWO_TIMES
extern uint8_t software_reboot_enable;
#endif

uint8_t can_reset_flag=0,can_config_flag=0;
int can_socket = -1;  //can套接字描述符
enum BOOL can_send_init_flag = FALSE;
enum BOOL RX_data_deal = TRUE;
//extern struct UPDATE_FLAG update_flag;
//extern struct COMM_DATA_CNT comm_data_cnt;
extern struct CAN_STATUS can_status[16];
struct can_frame  frame;

struct can_tx_queue  //can的发送队列，用于发送数据时组包
{
    sem_t tx_sem;
	uint32_t tx_save;
	uint32_t tx_read;
	uint32_t size;
	struct can_frame msg_buf[CAN_TX_BUF_SIZE]; //一次最多发送256帧数据
};
typedef struct can_tx_queue *CAN_tx_queue_t;
extern struct COMMUNICATE_TYPE communicate_type;


struct can_rx_queue
{
	sem_t    rx_sem;
	uint32_t rx_save;
	uint32_t rx_read;
	uint32_t size;
	struct can_frame msg_buf[CAN_RX_BUF_SIZE];
};
typedef struct can_rx_queue *CAN_rx_queue_t;


struct can_tx_queue can_send_queue; //实例化发送队列
struct can_rx_queue can_recv_queue; //实例化接收队列

list_t frame_list;

uint8_t recv_can_public_buff_A[CAN_RECV_PUBLIC][CAN_LEN] = {0};
uint8_t recv_can_public_buff_B[CAN_RECV_PUBLIC][CAN_LEN] = {0};
struct RECV_CTRL_BOARD_DATA recv_ctrl_board_data_a_can;
struct RECV_CTRL_BOARD_DATA recv_ctrl_board_data_b_can;

void can_tx_init(void);
extern void update_recv_public_info(struct RECV_CTRL_BOARD_DATA *recv_ctrl_board_data_temp, struct COMMUNICATE_TYPE *temp_commmunicate_type,enum PUBLIC_INFO_UPDATE_FLAG update_flag);
extern void update_use_public_info();					//更新公共消息
/*************************************************
Function:  can_send_data
Description: can消息发送函数
Input:  要发送的数据指针:data  数据的长度:len
Output: 无
Return: 无
Others:
*************************************************/
void can_send_data(uint8_t *data, uint16_t len,uint16_t tagmask)
{
	struct can_frame *msg;
	CANID_t can_tx_id;

	uint32_t send_group = 0, send_remain = 0, send_cnt = 0, len_tmp = 0;
	uint32_t i = 0, j = 0, r_index = 0;
	uint32_t multi_flag; //TRUE即为多帧，FALSE即为单帧
    uint32_t send_flag;
    uint8_t cnt_life=0;

	if (data==NULL || len==0 || can_send_init_flag==FALSE)
	{
		return;
	}

	can_tx_id.WORD = 0;
	can_tx_id.BITS.tx_source=PW_BOARD_CANID;   //原板卡
	can_tx_id.BITS.rx_mask  = tagmask;
	can_tx_id.BITS.type =1;
	can_tx_id.BITS.nc=0;


	send_group = len>>CAN_R_BITS;  //计算多少完整帧数据
	send_remain = len&CAN_AND;    //计算剩余不足一帧的有多少数据

	if (send_group>1 || (send_group==1 && send_remain>0)) //两帧以上就判断为多帧数据
	{
	    multi_flag = TRUE;
		send_cnt = send_group;
		if (send_remain > 0) //不足一帧的算一帧
		{
			send_cnt++;
		}
	}
	else
	{
		send_cnt = 1;  //单帧数据，只需要发送一次
		multi_flag = FALSE;
	}

	//封装数据
	can_tx_id.BITS.id_total = send_cnt-1;
	can_tx_id.BITS.id_cnt =0;

	for (i=0; i<send_cnt; i++)
	{
		msg = &(can_send_queue.msg_buf[can_send_queue.tx_save]);
		if (multi_flag)//多帧
		{
			if (i == 0x0)//头帧
			{
//			  can_rx_id.BITS.type = MULTI_BEGIN;
			  len_tmp = CAN_LEN;
			}
			else if (i == send_cnt-1)//尾帧
			{
//				can_rx_id.BITS.type = MULTI_END;
				if (send_remain>0)
				{
				  len_tmp = send_remain;
				}
				else
				{
					len_tmp = CAN_LEN;
				}
			}
			else//中间帧
			{
//				can_rx_id.BITS.type = MULTI_MID;
				len_tmp = CAN_LEN;
			}
		}
		else//单帧
		{
//			can_rx_id.BITS.type = SINGLE;
			if (send_group == 1)
			{
				len_tmp = CAN_LEN;
			}
			else
			{
				len_tmp = send_remain;
			}
		}

		/*can发送生命信号*/
		can_tx_id.BITS.pag_life=cnt_life;
		cnt_life++;
#ifdef CAN_PROTOCOL_20210322
		if(cnt_life>=4)
			cnt_life=0;
#else
		if(cnt_life>=8)
		{
			cnt_life=0;
		}
#endif
		can_tx_id.BITS.id_cnt = i;
		msg->can_id = can_tx_id.WORD | CAN_EFF_FLAG; //对应到stm32上的msg->ExtId，将把每帧的标识传输到通信对方
  	    msg->can_dlc = len_tmp;
  	    //DEBUG("can_tx_id.WORD:%x\n",can_tx_id.WORD);

  	    for (j=0; j<len_tmp; j++)
  	    {
  	    	msg->data[j] = data[r_index++];
  	    	//DEBUG("%d->%d,",msg->data[j],r_index);
  	    }
  	  //DEBUG("\n");
		can_send_queue.tx_save++;
		if(can_send_queue.tx_save >= can_send_queue.size)
		{
			can_send_queue.tx_save = 0;
		}
		send_flag = TRUE;

		if (can_send_queue.tx_save == can_send_queue.tx_read)
		{
			can_send_queue.tx_read ++;
			if (can_send_queue.tx_read >= can_send_queue.size)
			{
				can_send_queue.tx_read = 0;
			}
			send_flag = FALSE;
		}

		if (send_flag)
		{
			sem_post(&can_send_queue.tx_sem);
		}
	}

}



/*************************************************
Function:  can_tx_thread_entry
Description: can发送线程入口
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void can_tx_thread_entry(void *parameter )
{
//	struct can_frame *msg;  //一帧can消息的结构体
//	uint16_t send_cnt=0;
//	uint32_t i;
//	int size = 0;
//	CAN_tx_queue_t tx_queue = (CAN_tx_queue_t)parameter;
//    tx_queue = &can_send_queue;
//    tx_queue->tx_save = 0;
//    tx_queue->tx_read = 0;
//    tx_queue->size = CAN_TX_BUF_SIZE;
//    can_tx_init();
//    sem_init(&(tx_queue->tx_sem), 0, 0);  //信号量初始化
//    can_send_init_flag = TRUE;
//
//#ifdef RT_LINUX
//    pthread_attr_t attr;       // 线程属性
//    struct sched_param sched;  // 调度策略
//    printf ("set SCHED_RR policy\n");
//    api_set_thread_policy(&attr, SCHED_RR);
//#endif
//
//	//can_tx_init();
//    while(1)
//    {
//      /* wait receive */
//	  sem_wait(&can_send_queue.tx_sem);  //一直等待信号量值不为0,并将信号量值减1
//      {
////		 feed_dog();
////		  DEBUG("sem_wait_can_send_queue.tx_sem\n");
//         for (i=0;i<500000;i++); //延时一段时间
//         if (tx_queue->tx_read == tx_queue->tx_save)
//		 {
//             continue;
//		 }
//        msg = &(tx_queue->msg_buf[tx_queue->tx_read]);
//        size = sizeof(tx_queue->msg_buf[tx_queue->tx_read]);
//        //printf("msg= %d \r\n",size);
//        if(write(can_socket, msg, size))
//        {
//    	    tx_queue->tx_read++;
//            if(tx_queue->tx_read >= tx_queue->size)
//            {
//        	  tx_queue->tx_read = 0;
//            }
//			send_cnt = 0;
//         // DEBUG("Can send ok\r\n");
//		//	CAN_LED(1);
//        }
//        else
//        {
//			 send_cnt++;
//			 if (send_cnt > 10)
//			 {
//			   send_cnt = 0;
//			   usleep(40);
//			 }
//            //rt_kDEBUG("no mb temp\r\n", trans_mb_no);
//		    sem_post(&can_send_queue.tx_sem);  //释放发送信号量
//         }
//      }
//    }
}



/*************************************************
Function:  can_tx_init
Description: 初始化can的发送队列
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void can_tx_init(void)
{
    CAN_tx_queue_t tx_queue = &can_send_queue;
    tx_queue->tx_save = 0;
    tx_queue->tx_read = 0;
    tx_queue->size = CAN_TX_BUF_SIZE;
    sem_init(&(tx_queue->tx_sem), 0, 0);  //信号量初始化
	can_send_init_flag = TRUE;
}




/*************************************************
Function:    can_send
Description: 平台间 can通信数据发送
Input:  目标ID:taget
		命令类型:cmd
		发送数据指针:data
		数据长度:len
Output: 无
Return: 无
Others: 适用于 控制板 io板 save板间的通信
*************************************************/
void can_send(uint16_t tagetmask, uint8_t cmd, uint8_t *data, uint16_t len,uint16_t life_signal)
{
    uint8_t can_data[500];
    uint16_t crc=0;
    uint16_t i=0;
	can_data[0] = 0xaa;
	can_data[1] = 0x50;
	can_data[2] = (uint8_t)((len+PRO_HEAD_LEN) >> 8);
	can_data[3] = len+PRO_HEAD_LEN;
	can_data[4] = LUHANG;   //
	can_data[5] = LOCAL_BOARD;  //源ID
	can_data[6] = (uint8_t)(life_signal>>8);  //
	can_data[7] = (uint8_t)life_signal;//
	can_data[8] = 0;
	can_data[9] = 0;
	can_data[10] = cmd;
	for(i=0; i<len; i++)
	{
		can_data[11+i] = data[i];
	}
	crc=0;
	for(i=0; i<PRO_HEAD_LEN+len-2; i++)
	{
		crc += can_data[i];
	}
	can_data[PRO_HEAD_LEN+len-2] = (uint8_t)(crc>>8);
	can_data[PRO_HEAD_LEN+len-1] = (uint8_t)crc;
	can_send_data(can_data, PRO_HEAD_LEN+len,tagetmask);

}
/*************************************************
Function:    init_can_data_buff
Description: can数据缓存初始化
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void init_can_data_buff()
{
//	uint8_t cnt_i = 0;
//	if(recv_can_public_buff_A == NULL)
//	{
//		cnt_i = 0;
//		recv_can_public_buff_A = (uint8_t **)malloc(sizeof(uint8_t *) * CAN_RECV_PUBLIC);
//		if(recv_can_public_buff_A != NULL)
//		{
//			for(cnt_i = 0;cnt_i < CAN_RECV_PUBLIC;cnt_i ++)
//			{
//				recv_can_public_buff_A[cnt_i] = (uint8_t *)malloc(sizeof(uint8_t)*CAN_LEN);
//			}
//		}
//	}
//
//	if(recv_can_public_buff_B == NULL)
//	{
//		cnt_i = 0;
//		recv_can_public_buff_B = (uint8_t **)malloc(sizeof(uint8_t *) * CAN_RECV_PUBLIC);
//		if(recv_can_public_buff_B != NULL)
//		{
//			for(cnt_i = 0;cnt_i < CAN_RECV_PUBLIC;cnt_i ++)
//			{
//				recv_can_public_buff_B[cnt_i] = (uint8_t *)malloc(sizeof(uint8_t)*CAN_LEN);
//			}
//		}
//	}
}


/*************************************************
Function:    config_can
Description: 配置socket can的相关参数
Input:  无
Output: 无
Return: 无
Others:,can_data[i]
*************************************************/
void config_can()
{

//	int ret = -1;
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_filter rfilter[1];

//	if(can_reset_flag)
//	{
//		can_config_flag=1;
//		close(can_socket);
//		printf("*****RESTORE CONFIG CAN0 DEV*****\n");
//		system("ifconfig can0 down");//
//		can_reset_flag=0;
//	}

	system("ip link set can0 type can bitrate 1000000");
	system("ifconfig can0 up");//使能can驱动设备

	can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);//创建套接字
	strcpy(ifr.ifr_name, "can0" );
	ioctl(can_socket, SIOCGIFINDEX, &ifr); //指定 can0 设备
	int loopback = 0; /* 0 = disabled, 1 = enabled (default) */
    setsockopt(can_socket, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback, sizeof(loopback)); // Setting lookback options
	//DEBUG("set can0 sock loopback unable\n");
    //定义接收规则，只接收表示符 0x18000000的报文

#ifdef CAN_PROTOCOL_20210322
    rfilter[0].can_id = 0x10000000|(PW_RX_MASK<<12);//0x10080000;
    rfilter[0].can_mask = 0x10000000|(PW_RX_MASK<<12);//0x10080000;//CAN_EFF_MASK
#else
    rfilter[0].can_id = 0x10000000|(PW_RX_MASK<<13);//0x10080000;
    rfilter[0].can_mask = 0x10000000|(PW_RX_MASK<<13);//0x10080000;//CAN_EFF_MASK
#endif

    setsockopt(can_socket, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));//设置规则

	int recv_own_msgs = 0; /* 0 = disabled (default), 1 = enabled */
//    ret =
	setsockopt(can_socket, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recv_own_msgs, sizeof(recv_own_msgs)); //Setting receiver own massages options
	//DEBUG("set can0 do not receiver own massage\n");


	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	bind(can_socket, (struct sockaddr *)&addr, sizeof(addr));//将套接字与 can0 绑定
	can_config_flag=0;
}

void restart_can_dev(void)
{

//	int ret = -1;
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_filter rfilter[2];
	can_config_flag=1;
	close(can_socket);
	printf("*****RESTART CAN0 DEV FOR IO IAP*****\n");
	system("ifconfig can0 down");//
	system("ip link set can0 type can bitrate 1000000");
	system("ifconfig can0 up");//使能can驱动设备

	can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);//创建套接字
	strcpy(ifr.ifr_name, "can0" );
	ioctl(can_socket, SIOCGIFINDEX, &ifr); //指定 can0 设备
	int loopback = 0; /* 0 = disabled, 1 = enabled (default) */
    setsockopt(can_socket, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback, sizeof(loopback)); // Setting lookback options
	//DEBUG("set can0 sock loopback unable\n");

	int recv_own_msgs = 0; /* 0 = disabled (default), 1 = enabled */
//    ret =
	setsockopt(can_socket, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recv_own_msgs, sizeof(recv_own_msgs)); // Setting receiver own massages options
	//DEBUG("set can0 do not receiver own massage\n");

    //定义接收规则，只接收表示符 0x18000000的报文
#ifdef CAN_PROTOCOL_20210322
    rfilter[0].can_id = 0x10000000|(PW_RX_MASK<<12);//0x18004000;
    rfilter[0].can_mask = 0x10000000|(PW_RX_MASK<<12);
//    rfilter[0].can_mask = CAN_EFF_MASK;
    rfilter[1].can_id = 0x10000000|(PW_RX_MASK<<12);//0x18002000;
    rfilter[1].can_mask = 0x10000000|(PW_RX_MASK<<12);
#else
    rfilter[0].can_id = 0x10000000|(PW_RX_MASK<<13);//0x18004000;
    rfilter[0].can_mask = 0x10000000|(PW_RX_MASK<<13);
//    rfilter[0].can_mask = CAN_EFF_MASK;
    rfilter[1].can_id = 0x10000000|(PW_RX_MASK<<13);//0x18002000;
    rfilter[1].can_mask = 0x10000000|(PW_RX_MASK<<13);
#endif

    setsockopt(can_socket, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));//设置规则

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	bind(can_socket, (struct sockaddr *)&addr, sizeof(addr));//将套接字与 can0 绑定

	can_reset_flag=1;
	can_config_flag=0;

}



/*************************************************
Function:    protocol_deal
Description: can数据协议解析
Input:  接受的can数据指针:data
		数据的长度:data_len
Output: 无
Return: 无
Others:
*************************************************/
void protocol_deal(CANID_t can_rx_id,uint8_t *data)
{
//	static uint8_t rx_buf[RX_LEN_MAX];
//	static uint32_t write_id=0;
//	static uint8_t rx_i;
//	static uint8_t recv_a_num;
//	static uint8_t recv_b_num;
////	static uint16_t crc;
////	uint8_t test_i = 0;
//
////	DEBUG("-------->recv_can_msg<-------:%d\n",can_rx_id.BITS.tx_source);
//	if(can_rx_id.BITS.tx_source == CTRLA_BOARD_CANID)				//接收控制板Ａ　CAN消息
//	{
//
////		printf("------>ctrl_A_CAN<-------\n");
//		can_status[BIT_CTRLA].connect_flag = 0;					//1故障;0正常
//		can_status[BIT_CTRLA].not_recv_cnt = 0;
//
//		memmove((uint8_t *)&recv_can_public_buff_A[can_rx_id.BITS.id_cnt][0],data,CAN_LEN);
//
//		if(can_rx_id.BITS.id_cnt == can_rx_id.BITS.id_total)
//		{
//
//
//			if(recv_a_num == can_rx_id.BITS.id_total)
//			{
//#ifdef CAN_ERR_JUDGE_NEW_STYTLE
//				software_reboot_enable = 0;
//#endif
//				//comm_data_cnt.ctrla_can_recv_all_cnt++;
//				memmove((uint8_t *)&recv_ctrl_board_data_a_can,&recv_can_public_buff_A[0][0],sizeof(struct RECV_CTRL_BOARD_DATA));
//				update_recv_public_info(&recv_ctrl_board_data_a_can,&communicate_type,CTRL_A_CAN);
//			}
//
//			memset((uint8_t *)&recv_can_public_buff_A[0][0],0,sizeof(uint8_t)*CAN_RECV_PUBLIC*CAN_LEN);
//			//recv_a_num = 0;
//
//		}
//		recv_a_num++;
//
//		if(recv_a_num > can_rx_id.BITS.id_total || (can_rx_id.BITS.id_cnt == can_rx_id.BITS.id_total))
//		{
//			recv_a_num = 0;
//		}
//	}
//	else if(can_rx_id.BITS.tx_source == CTRLB_BOARD_CANID)				//接收控制板Ｂ　CAN消息
//	{
//
////		printf("------>ctrl_B_CAN<-------\n");
//		can_status[BIT_CTRLB].connect_flag = 0;					//1故障;0正常
//		can_status[BIT_CTRLB].not_recv_cnt = 0;
//
//		memmove((uint8_t *)&recv_can_public_buff_B[can_rx_id.BITS.id_cnt][0],data,CAN_LEN);
//
////				DEBUG("recv_a_num:%d,\n",recv_b_num);
//
//		if(can_rx_id.BITS.id_cnt == can_rx_id.BITS.id_total)
//		{
//			//
//			if(recv_b_num == can_rx_id.BITS.id_total)
//			{
//#ifdef CAN_ERR_JUDGE_NEW_STYTLE
//				software_reboot_enable = 0;
//#endif
//				//comm_data_cnt.ctrlb_can_recv_all_cnt++;
//				memmove((uint8_t *)&recv_ctrl_board_data_b_can,&recv_can_public_buff_B[0][0],sizeof(struct RECV_CTRL_BOARD_DATA));
//				update_recv_public_info(&recv_ctrl_board_data_b_can,&communicate_type,CTRL_B_CAN);
//			}
//
//
//			memset((uint8_t *)&recv_can_public_buff_B[0][0],0,sizeof(uint8_t)*CAN_RECV_PUBLIC*CAN_LEN);
//
//		}
//		recv_b_num++;
//
//		if(recv_b_num > can_rx_id.BITS.id_total || (can_rx_id.BITS.id_cnt == can_rx_id.BITS.id_total))
//		{
//			recv_b_num = 0;
//		}
//	}

//	update_use_public_info();					//更新公共消息
}

/*************************************************
Function:    can_recv_thread_entery
Description: can接收线程入口
Input:  无
Output: 无
Return: 无
Others:PRO_HEAD_LEN
*************************************************/
void can_recv_thread_entery(void *parameter)  //CAN接收线程入口
{
//	    uint8_t can_data[8];
////	    int i;
//		CANID_t can_rx_id;
//		uint16_t RECV_MASK=0;
//
//
////		uint8_t cnt_i = 0;
//
//		RECV_MASK = PW_RX_MASK;
//
//#ifdef RT_LINUX
//	    pthread_attr_t attr;       // 线程属性
//	    struct sched_param sched;  // 调度策略
//	    printf ("set SCHED_RR policy\n");
//	    api_set_thread_policy(&attr, SCHED_RR);
//#endif
//		while(1)
//		{
//			int nbytes = read(can_socket, &frame, sizeof(frame)); //接收报文
//			if(can_config_flag)
//			{
//				continue;
//			}
//
//			can_rx_id.WORD = frame.can_id;
//
//			memset(can_data,0,sizeof(can_data));
//			//printf("WORD:%x,can_rx_id.BITS.tx_source:%x,can_rx_id.BITS.rx_mask:%x\n",can_rx_id.WORD,can_rx_id.BITS.tx_source,can_rx_id.BITS.rx_mask);
//
//			if(nbytes > 0 && (can_rx_id.BITS.rx_mask&RECV_MASK ))
//			{
//				memcpy(can_data,frame.data,frame.can_dlc);
//
//				protocol_deal(can_rx_id,can_data);
//
//			}
//
//			usleep(1);
//
//		}
}

/*************************************************
Function:    init_can_thread
Description: 初始化创建can的发送和接收线程
Input:  无
Output: 无
Return: 无
Others:		printf("can_id:%x ",frame.can_id);
*************************************************/
int init_can_thread(void) //can接收线程初始化
{
//	pthread_t can_recv_id,can_tx_id;
//	int ret;
//
//	/****************创建CAN接收数据的线程****************/
//	ret=pthread_create(&can_recv_id,NULL,(void *)can_recv_thread_entery,NULL);
//	if(ret!=0){
//	DEBUG ("Create can recv thread error!\n");
//	return ret;
//	}
//
//	/****************创建CAN发送数据的线程****************/
//	ret=pthread_create(&can_tx_id,NULL,(void *)can_tx_thread_entry,NULL);
//	if(ret!=0){
//	DEBUG ("Create can tx thread error!\n");
//	return ret;
//	}
//
//	return 0;
}

/*************************************************
Function:    init_can_socket
Description: 配置和初始化can
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
int init_can_socket()
{
	int ret = -1;
	config_can();
	ret = init_can_thread(); //can线程初始化
	return ret;
}
