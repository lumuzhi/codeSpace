#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include "user_data.h"
#include "fftw3.h"
#include "ptu_app.h"
#include "update.h"
#include "can_data_deal.h"
#include "watchdog.h"

//struct UPDATE_FLAG update_flag;
//struct UPDATE_FILE update_file;
struct CAN_UPDATE_DATA can_update_data;
struct NET_UPDATE_DATA net_update_data;
//extern struct TRAIN_NET_FD train_net_fd;
//extern struct TRAIN_NET_FLAG train_net_flag;
//extern struct UART_UPDATE_DATA uart_update_data;

extern void can_send(uint16_t tagetmask, uint8_t cmd, uint8_t *data, uint16_t len,uint16_t life_signal);
/*************************************************
Function:  net_send_reset_message
Description: 通知相关板卡跳转到底包
Input:	目标板卡地址
Output:
Return:
Others:
*************************************************/
void net_send_reset_message(int sock_fd, uint8_t taget)
{
//	uint8_t data[1];
//	data[0] = 0;
//	net_send(sock_fd,taget, NET_RESET_TYPE, data, 1);
}



/*************************************************
Function:  net_send_reset_message
Description: 通知相关板卡跳转到底包
Input:	目标板卡地址
Output:
Return:
Others:
*************************************************/
void net_send_bin_data(int sock_fd,uint8_t taget,uint8_t *bin_data,uint16_t len)
{
	DEBUG("-----------net_update_data.send_group_count = %d----------\r\n",net_update_data.send_group_count);
    uint8_t data[1024];
    data[0] = net_update_data.update_message;
    *(uint16_t*)&data[1] = len;
    *(uint32_t*)&data[3] = net_update_data.send_group_count;
    memmove(&data[7],bin_data,len);
//	net_send(sock_fd,taget, UPDATE_SOFT_TYPE, data, len+7);
}


/*************************************************
Function:  net_send_start_iap
Description: 发送开始网络升级
Input:	目标板卡地址
Output:
Return:
Others:
*************************************************/
void net_send_start_iap(int sock_fd,uint8_t taget)
{
//	DEBUG("----------------------send start update date----------------------------------\r\n");
//    static uint8_t data[12];
//    DEBUG("net_update_data.update_message === %d \r\n",net_update_data.update_message);
//    data[0] = net_update_data.update_message;
//    data[1] = net_update_data.train_num[0];
//    data[2] = net_update_data.train_num[1];
//    data[3] = net_update_data.update_type;
//	net_send(sock_fd,taget, UPDATE_SOFT_TYPE, data, 4);
}



/*************************************************
Function:  send_iap_reset_message
Description: 通知相关板卡跳转到底包
Input:	目标板卡地址
Output:
Return:
Others:
*************************************************/
void can_send_iap_reset_message(uint8_t taget)
{
//	uint8_t data[1];
//	data[0] = 0;
//	can_send(taget, IAP_RESET_MESSAGE, data, 1);
}


/*************************************************
Function:    can_send_start_iap
Description: 发送升级开始消息
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void can_send_start_iap(uint8_t taget,enum CAN_CMD message_type )
{
//	DEBUG("----------------------send start update  message----------------------------------\r\n");
//    static uint8_t data[1024];
//    can_update_data.update_message = 0;
//    data[0] = can_update_data.update_message;
//    data[1] = 0;
//	can_send(taget, message_type, data, 2);
}


/*************************************************
Function:    can_send_bin_jkb1
Description: 发送bin数据给前置处理器１
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void can_send_bin_jkb1(uint8_t *bin_data,uint16_t len)
{
	DEBUG("----------------------send update date to jkb1----------------------------------\r\n");
    static uint8_t data[1024];

    data[0] = can_update_data.update_message;
    *(uint16_t*)&data[1] = len;
    *(uint32_t*)&data[3] = can_update_data.send_group_count;
    DEBUG("can_update_data.send_group_count ======= %d \r\n",can_update_data.send_group_count);
    memmove(&data[7],bin_data,len);
	can_send(PRE_BOARD1, PRE1_IAP_MESSAGE, data, len+7, 0);
}


/*************************************************
Function:    can_send_bin_jkb2
Description: 发送bin数据给前置处理器２
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void can_send_bin_jkb2(uint8_t *bin_data,uint16_t len)
{
    //rt_kprintf("----------------------send update date to jkb2----------------------------------\r\n");
    uint8_t data[1024];
    data[0] = can_update_data.update_message;
    *(uint16_t*)&data[1] = len;
    *(uint32_t*)&data[3] = can_update_data.send_group_count;
    DEBUG("can_update_data.send_group_count ======= %d \r\n",can_update_data.send_group_count);
    memmove(&data[7],bin_data,len);
	can_send(PRE_BOARD2, PRE2_IAP_MESSAGE, data, len+7, 0);
}


/*************************************************
Function:    can_send_bin_mvb
Description: 发送数据给通信板
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void can_send_bin_mvb(uint8_t *bin_data,uint16_t len)
{
    //rt_kprintf("----------------------send update date to mvb----------------------------------\r\n");
    uint8_t data[1024];
    data[0] = can_update_data.update_message;
    *(uint16_t*)&data[1] = len;
    *(uint32_t*)&data[3] = can_update_data.send_group_count;
    DEBUG("can_update_data.send_group_count ======= %d \r\n",can_update_data.send_group_count);
    memmove(&data[7],bin_data,len);
	can_send(MVB_BOARD, MVB_IAP_MESSAGE, data, len+7, 0);
}



/*************************************************
Function:    system_update_thread_entry
Description: 系统升级线程
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void system_update_thread_entry(void*paramter)
{
//	uint8_t bin_data[512];
//	uint8_t net_bin_data[512];
//	uint8_t uart_bin_data[64];
//	int update_fd = -1,ret;
//	uint32_t length = 0;
	can_update_data.recv_normal_data = 0;
	can_update_data.send_group_count = 0;
	net_update_data.recv_normal_data = 0;
	net_update_data.send_group_count = 0;
	can_update_data.update_flag = 0;

#ifdef RT_LINUX
    pthread_attr_t attr;       // 线程属性
    struct sched_param sched;  // 调度策略
    DEBUG ("set SCHED_RR policy\n");
    api_set_thread_policy(&attr, SCHED_RR);
#endif

    while(1)
    {
#ifdef HISTORY_FEED_WATCHDOG
//    	finish:
		feed_dog();
#endif
    	//1车处理板内部ad采集板升级
    }
}

/*************************************************
Function:    init_update_thread
Description: 初始化升级线程
Input:
Output:
Return:
Others:
*************************************************/
int init_update_thread()
{
//	pthread_t recv_thread_id;
//	int ret;
//	update_file.train1_adb_update_file = "/media/LH_DATA/update/train_1/update_app/adb.bin";
//	update_file.train1_clb_update_file = "/media/LH_DATA/update/train_1/update_app/TFDSE-CLB";
//	update_file.train1_pre1_update_file = "/media/LH_DATA/update/train_1/update_app/jkb1.bin";
//	update_file.train1_pre2_update_file = "/media/LH_DATA/update/train_1/update_app/jkb2.bin";
//	update_file.train1_txb_update_file =  "/media/LH_DATA/update/train_1/update_app/txb.bin";
//	update_file.train2_clb_update_file =  "/media/LH_DATA/update/train_2/update_app/update.bin";
//	update_file.train2_pre1_update_file = "/media/LH_DATA/update/train_2/update_app/jkb1.bin";
//	update_file.train2_pre2_update_file = "/media/LH_DATA/update/train_2/update_app/jkb2.bin";
//	update_file.train3_clb_update_file =  "/media/LH_DATA/update/train_3/update_app/update.bin";
//	update_file.train3_pre1_update_file = "/media/LH_DATA/update/train_3/update_app/jkb1.bin";
//	update_file.train3_pre2_update_file = "/media/LH_DATA/update/train_3/update_app/jkb2.bin";
//	update_file.train4_clb_update_file =  "/media/LH_DATA/update/train_4/update_app/update.bin";
//	update_file.train4_pre1_update_file = "/media/LH_DATA/update/train_4/update_app/jkb1.bin";
//	update_file.train4_pre2_update_file = "/media/LH_DATA/update/train_4/update_app/jkb2.bin";
//    update_file.train5_clb_update_file =  "/media/LH_DATA/update/train_5/update_app/update.bin";
//    update_file.train5_pre1_update_file = "/media/LH_DATA/update/train_5/update_app/jkb1.bin";
//    update_file.train5_pre2_update_file = "/media/LH_DATA/update/train_5/update_app/jkb2.bin";
//    update_file.train6_clb_update_file =  "/media/LH_DATA/update/train_6/update_app/update.bin";
//    update_file.train6_pre1_update_file = "/media/LH_DATA/update/train_6/update_app/jkb1.bin";
//    update_file.train6_pre2_update_file = "/media/LH_DATA/update/train_6/update_app/jkb2.bin";
//    update_file.train7_clb_update_file =  "/media/LH_DATA/update/train_7/update_app/update.bin";
//    update_file.train7_pre1_update_file = "/media/LH_DATA/update/train_7/update_app/jkb1.bin";
//    update_file.train7_pre2_update_file = "/media/LH_DATA/update/train_7/update_app/jkb2.bin";
//    update_file.train8_adb_update_file = "/media/LH_DATA/update/train_8/update_app/adb.bin";
//    update_file.train8_clb_update_file =  "/media/LH_DATA/update/train_8/update_app/TFDSE-CLB";
//    update_file.train8_pre1_update_file = "/media/LH_DATA/update/train_8/update_app/jkb1.bin";
//    update_file.train8_pre2_update_file = "/media/LH_DATA/update/train_8/update_app/jkb2.bin";
//    update_file.train8_txb_update_file = "/media/LH_DATA/update/train_8/update_app/txb.bin";

//	ret=pthread_create(&recv_thread_id,NULL,(void *)system_update_thread_entry,NULL);
//	if(ret!=0)
//	{
//		DEBUG ("update thread error!\n");
//		return ret;
//	}
	return 0;
}

