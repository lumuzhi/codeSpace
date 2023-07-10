#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
//#include <arpa/inet.h>
//#include <netinet/in.h>
#include <unistd.h>
#include "sys/time.h"
#include <setjmp.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include "board.h"
//#include "multi_timer.h"
//#include "mvb.h"
#include "crc_table.h"
#include "udp_client.h"
#include "user_data.h"
#include "ptu_app.h"
#include "can_config.h"
#include "multi_timer.h"
#include "pswb_record_prot.h"

#define RESEND_CNT 3 //表示重发3次
#define UDP_SEND_JMP_TIMEOUT 0
#define UDP_SEND_SELECT_TIMEOUT 0
#define UDP_SEND_LOCK_TIMEOUT 1

#define UDP_ACK_OK 0
#define UDP_ACK_ERR -1

#define UDP_SEND_FIRST 0x55
#define UDP_SEND_AGAIN 0XAA


pthread_mutex_t mcast_lock;

extern struct CAN_STATUS can_status[16];
extern struct PW_RAW_DATA pw_raw_data;
extern struct SW_RAW_DATA sw_raw_data;
extern struct PW_CLB_CONFIG_PARA *pw_clb_config_para;
extern struct FILE_OPERTE file_operate;
extern void ptu_send(uint8_t taget, uint8_t cmd, uint8_t *data, uint16_t len);

extern uint8_t sw_data_save_type;

pswb_record_protocol_t pswb_record_protocol;
#define env pswb_record_protocol

extern psw_info_data_t psw_info_data;

int ucast_recv_sock_fd;
int multicast_socket;

struct ACK_SIG
{
    volatile uint16_t is_need_ack_flag; //1需要应答  0不需要应答

    volatile uint16_t send_dev;       //振动板 VIBR_BOARD
    volatile uint16_t send_addr_bits; //发送地址bit集合
    volatile uint16_t send_head;
    volatile uint16_t send_life;

    volatile uint16_t recv_dev;       //振动板 VIBR_BOARD
    volatile uint16_t recv_addr_bits; //收到的源板卡bit
    volatile uint16_t recv_head;
    volatile uint16_t recv_life;
};


//uint16_t target_board_addr[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
/** */
struct sockaddr_in mcast_ctl_save_group_addr;
struct sockaddr_in ucast_io_board_addr;
struct sockaddr_in ucast_ctrla_board_addr;
struct sockaddr_in ucast_ctrlb_board_addr;
struct sockaddr_in ucast_save_board_addr;
struct sockaddr_in ucast_axle_tempa_board_addr;
struct sockaddr_in ucast_axle_tempb_board_addr;
struct sockaddr_in ucast_st_board_addr;
struct sockaddr_in ucast_ad_board_addr;
struct sockaddr_in ucast_vibr_board_addr;
struct sockaddr_in ucast_gear_board_addr;
struct sockaddr_in ucast_motor_board_addr;
struct sockaddr_in local_addr;

int send_fd_socket;
pthread_mutex_t udp_send_mutex;
pthread_cond_t udp_send_cond;
static struct ACK_SIG ack_life_sig;
struct UDP_SEND_ADDR udp_send_addr;
//static jmp_buf usp_send_jmp_buf;
//static volatile sig_atomic_t udp_recv_ok_flag = 0;
//static volatile sig_atomic_t udp_recv_id = 0;
extern struct SW_RAW_DATA sw_raw_data;
static sem_t udp_send_sem;
static struct sockaddr_in ucast_target_sockaddr[16]; 			//表示要发送的板卡ip数组
static uint16_t target_board_addr[16];
extern struct ETH_STATUS eth_status[16];
void udp_ucast_recv(void *ptr);
void udp_mcast_recv(void *ptr);
int check_ack_life_sig(void);
void set_recv_life_sig(uint16_t src_dev_bit, uint16_t board_id, uint16_t recv_head, uint16_t recv_life);
void set_send_life_sig(uint16_t src_dev_bit, uint16_t send_addr, uint16_t send_head, uint16_t send_life);
int judge_client(struct sockaddr_in client);


struct COMMUNICATE_TYPE communicate_type;
extern struct RECV_PUBLIC_PARA recv_public_para;
extern struct COMM_DATA_CNT comm_data_cnt;
extern struct SYS_STATUS_CNT sys_status_cnt;
extern void ptu_send(uint8_t taget, uint8_t cmd, uint8_t *data, uint16_t len);

struct TEMP_LIFE_NUM temp_life_num;

extern uint16_t little_to_big_16bit(uint16_t value);
extern void check_eth_status(uint16_t temp_addr_bits);
extern void reset_recv_life_sig();

#define APP_PARAS_CONFIG_IP     "255.255.255.255"
static char local_ip[15];

txb_MVB_public_t app_save_public;

#if UDP_SEND_JMP_TIMEOUT == 1
/**
 * function     :sigalrm_handler
 * description  :定时器回调函数
 * input        :sig
 * output       :none
 * return       :none
 * others       :none
 */
void udp_send_timeout_handler(int sig)
{
    //	printf("udp_send_timeout_handler\n");
    return siglongjmp(usp_send_jmp_buf, 0);
}
#endif


static const uint16_t crc16_tbl[] =
{
    0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
    0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
    0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
    0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
    0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
    0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
    0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
    0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
    0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
    0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
    0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
    0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
    0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
    0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
    0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
    0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
    0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
    0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
    0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
    0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
    0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
    0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
    0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
    0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
    0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
    0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
    0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
    0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
    0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
    0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
    0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
    0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040,
};

uint16_t sum_check_u16(uint8_t *data,uint16_t crclen)
{
	int i;
	uint16_t check_sum=0;
	for(i=0;i<crclen;i++)
	{
		check_sum+=	data[i];
	}
	 return check_sum;
}


uint16_t modbus_crc(uint8_t *data, uint16_t size)
{
    uint16_t i = 0;
    uint16_t crc_16 = ~0;

    while(size)
    {
        crc_16 = (crc_16 >> 8) ^ crc16_tbl[(crc_16 ^ data[i++]) & 0xff];
        size--;
    }

    return (uint16_t)(crc_16 >> 8 | crc_16 << 8);
}

typedef struct
{
	struct sockaddr_in send_record_addr;  //记录板组播地址
}app_multicast_thread_local_t;

app_multicast_thread_local_t app_multicast_thread_local;
#define multicast_local app_multicast_thread_local


app_paras_config_env_t app_paras_config_env;
#define app_env app_paras_config_env
/**
 * 初始化各个板卡以太网通信标志
 *
 * */
void init_eth_connect_status()
{
//	eth_status[BIT_CTRLA].connect_flag = 1;					//1故障;0正常
//	eth_status[BIT_CTRLA].not_recv_cnt = 0;
//	eth_status[BIT_CTRLB].connect_flag = 1;					//1故障;0正常
//	eth_status[BIT_CTRLB].not_recv_cnt = 0;
	eth_status[BIT_SAVE].connect_flag = 1;					//1故障;0正常
	eth_status[BIT_SAVE].not_recv_cnt = 0;
//	eth_status[BIT_TEMPA].connect_flag = 1;					//1故障;0正常
//	eth_status[BIT_TEMPA].not_recv_cnt = 0;
//	eth_status[BIT_TEMPB].connect_flag = 1;					//1故障;0正常
//	eth_status[BIT_TEMPB].not_recv_cnt = 0;
	eth_status[BIT_ST].connect_flag = 1;					//1故障;0正常
	eth_status[BIT_ST].not_recv_cnt = 0;
//	eth_status[BIT_AD].connect_flag = 1;					//1故障;0正常
//	eth_status[BIT_AD].not_recv_cnt = 0;
//	eth_status[BIT_VIBR].connect_flag = 1;					//1故障;0正常
//	eth_status[BIT_VIBR].not_recv_cnt = 0;
//	eth_status[BIT_IO].connect_flag = 1;					//1故障;0正常
//	eth_status[BIT_IO].not_recv_cnt = 0;
//	eth_status[BIT_GEAR].connect_flag = 1;					//1故障;0正常
//	eth_status[BIT_GEAR].not_recv_cnt = 0;
//	eth_status[BIT_MOTOR].connect_flag = 1;					//1故障;0正常
//	eth_status[BIT_MOTOR].not_recv_cnt = 0;
}

static void udp_send1(uint8_t *buff, uint16_t buff_size, struct sockaddr *__addr, socklen_t __addr_len)
{
    sendto(ucast_recv_sock_fd, buff, buff_size, 0, __addr, __addr_len);
}

void ucast_send(uint8_t *buff, uint16_t buff_size,uint8_t target)
{
	static uint16_t counter=0;
	uint16_t len=PROT_HEAD_LEN+buff_size;
	uint8_t sendbuf[len];
	struct SEND_DATA_HEAD heads;
	heads.head[0]=0xaa;
	heads.head[1]=0x50;
	heads.len[0]=(uint8_t)(len>>8);
	heads.len[1]=(uint8_t)(len);

	heads.company_id = 0x4;
	heads.board_id = LOCAL_BOARD;

	counter++;
	heads.life_signal[0]= (uint8_t)(counter>>8);
	heads.life_signal[1]= (uint8_t)(counter);

	memmove(sendbuf,&heads,sizeof(struct SEND_DATA_HEAD));
	memmove(&sendbuf[sizeof(struct SEND_DATA_HEAD)],buff,buff_size);
	uint16_t check_sum=sum_check_u16(sendbuf,len-2);

	sendbuf[len-2]=(uint8_t)(check_sum>>8);
	sendbuf[len-1]=(uint8_t)(check_sum);

	udp_send1(sendbuf,len,(struct sockaddr *)&ucast_save_board_addr,sizeof(ucast_save_board_addr));

}

void udp_cmd_ack(uint8_t cmd,uint8_t flag)
{
	uint8_t buf[3];
	buf[0]=cmd;
	buf[1]=LOCAL_BOARD;
	buf[2]=flag;
	ucast_send(buf, 3,0);
}


/**
 * todo 单播消息接收处理
 * */
void ucast_recv_msg_deal(uint8_t *data, uint16_t len, struct sockaddr_in client_addr)
{
	char ip[16];
	strcpy(ip,inet_ntoa(client_addr.sin_addr));
	if(strcmp(ip,local_ip)==0)
		return;
	if((*(uint16_t*)&data[0]!=0x50aa))
		return;

	uint16_t host_sum = sum_check_u16(data, len-2);
	uint16_t net_sum = (data[len-2]<<8)+data[len-1];

	printf("udp_client.c -- ucast_recv_msg_deal : \n");
	for(int i = 0; i < len; i++)
	{
		if (i%10 == 0) printf("\n");
		printf("0x%02X ",data[i]);
	}
	printf("\n");


//	printf("!111111111111111111111111  ucast_recv_msg_deal\n");
	if ((host_sum == net_sum)&&(data[25]==LOCAL_BOARD))
	{
		switch(data[24])
		{
		  case APP_UPDATE_CMD:

			  printf("pswb iap---\n");

			  if (access("/media/local_config/WZS1_PSW", F_OK) < 0)
			  {
				  break;
			  }

			  system("cp /media/local_config/WZS1_PSW  /media/local_config/app -rf");

			  udp_cmd_ack(APP_UPDATE_CMD,0);

			  sleep(2);
			  system("reboot -nf");

			  break;

		  case SYSTEM_REBOOT_CMD:
				 printf(" SYSTEM_REBOOT_CMD---\n");
				  sleep(2);
				  system("reboot -nf");

			  break;

		  case CONFIG_PARA_SET_CMD:
			  //同步数据
//			  system(RSYNC_UPDATE_GET);
//			  system("cp /media/LH_DATA/update/updat_config/*  /media/LH_DATA/local_config/config/ -rf");
//			  udp_cmd_ack(CONFIG_PARA_SET_CMD,0);
//			  sleep(1);
//			  system("reboot -nf");
			  break;

		  case CONFIG_PARA_READ_CMD:
//			  system("cp /media/LH_DATA/local_config/config/*  /media/LH_DATA/update/updat_config/ -rf");
//			  //同步数据
//			  system(RSYNC_UPDATE_PUT);
//			  udp_cmd_ack(CONFIG_PARA_SET_CMD,0);
			  break;

		  default:
			  break;
		}
	}
}

//static inline struct PW_CLB_CONFIG_PARA *app_paras_get(void)
//{
//    return pw_clb_config_para;
//}

void app_paras_config_msg_send(uint8_t *data, uint16_t size)
{
	struct sockaddr_in send_ipaddr;

	memset(&send_ipaddr, 0, sizeof(send_ipaddr));
   send_ipaddr.sin_family = AF_INET;
   send_ipaddr.sin_addr.s_addr = inet_addr(APP_PARAS_CONFIG_IP);
   send_ipaddr.sin_port = htons(app_env.config_net_inform.port);
	sendto(ucast_recv_sock_fd, data, size, 0, (struct sockaddr *)&send_ipaddr, sizeof(send_ipaddr));
	printf("app_paras_config_msg_send :------\n");
	for(int i = 0; i < size; i++)
	{
		printf("0x%02X ",data[i]);
	}
	printf("\n");
}

static inline void app_paras_config_broadcast_ack(void)
{
//	struct PW_CLB_CONFIG_PARA *paras = OS_NULL;
	broadcast_ack_protocol_t broadcast_ack_data;

	memset(&broadcast_ack_data, 0, sizeof(broadcast_ack_protocol_t));

//	paras = app_paras_get();

	broadcast_ack_data.data_head[0] = 0xAA;
	broadcast_ack_data.data_head[1] = 0x51;
	broadcast_ack_data.data_len[0] = (uint8_t)(sizeof(broadcast_ack_protocol_t) >> 8);
	broadcast_ack_data.data_len[1] = (uint8_t)(sizeof(broadcast_ack_protocol_t));
	broadcast_ack_data.factory_code = 0x04;
	broadcast_ack_data.device_code = 0x77;
	broadcast_ack_data.cmd = APP_BROADCAST_ACK_CMD;
	pbroadcast_ack_cmd_t broadcast_ack_inform;
	broadcast_ack_inform  = (pbroadcast_ack_cmd_t)broadcast_ack_data.data_inform;
	broadcast_ack_inform->device_code = 0x77;
	broadcast_ack_inform->sn[0] = 0x77;
	broadcast_ack_inform->sn[1] = 0x77;
	memmove(broadcast_ack_inform->local_ip, pw_clb_config_para->local_net_para.self_ip, 4);
	memmove(broadcast_ack_inform->mask, pw_clb_config_para->local_net_para.self_maskaddr, 4);
	memmove(broadcast_ack_inform->gateway, pw_clb_config_para->local_net_para.self_gwaddr, 4);
	broadcast_ack_inform->port[0] = (uint8_t)(pw_clb_config_para->local_net_para.net_port >> 8);
	broadcast_ack_inform->port[1] = (uint8_t)(pw_clb_config_para->local_net_para.net_port);
	broadcast_ack_inform->train_num = pw_clb_config_para->trainnum;
	uint16_t sum = sum_check_16((uint8_t *)&broadcast_ack_data, sizeof(broadcast_ack_protocol_t) - 2);
	broadcast_ack_data.sun_crc[0] = (uint8_t)(sum >> 8);
	broadcast_ack_data.sun_crc[1] = (uint8_t)(sum);
	printf("paras_config_broadcast_ack :------\n");
	app_paras_config_msg_send((uint8_t *)&broadcast_ack_data, sizeof(broadcast_ack_protocol_t));

}

static inline void app_paras_config_ack(uint8_t state)
{
	config_ack_protocol_t config_ack_protocol;

	memset(&config_ack_protocol, 0, sizeof(config_ack_protocol_t));
	config_ack_protocol.data_head[0] = 0xAA;
	config_ack_protocol.data_head[1] = 0x51;
	config_ack_protocol.data_len[0] = (uint8_t)(sizeof(config_ack_protocol_t) >> 8);
	config_ack_protocol.data_len[1] = (uint8_t)(sizeof(config_ack_protocol_t));
	config_ack_protocol.factory_code = 0x04;
	config_ack_protocol.device_code = 0x77;
	config_ack_protocol.cmd = APP_CONFIG_INFORM_ACK_CMD;
	pconfig_inform_ack_cmd_t config_inform;
	config_inform  = (pconfig_inform_ack_cmd_t)config_ack_protocol.data_inform;
	config_inform->device_code = 0x77;
	config_inform->sn[0] = 0x77;
	config_inform->sn[1] = 0x77;
	config_inform->state = state;
	uint16_t sum = sum_check_16((uint8_t *)&config_ack_protocol, sizeof(config_ack_protocol_t) - 2);
	config_ack_protocol.sun_crc[0] = (uint8_t)(sum >> 8);
	config_ack_protocol.sun_crc[1] = (uint8_t)(sum);
	printf("app_paras_config_ack :------\n");
	app_paras_config_msg_send((uint8_t *)&config_ack_protocol, sizeof(config_ack_protocol_t));
}

void app_paras_net_printf(void)
{

	printf("\t->local_ip : %d.%d.%d.%d \n", pw_clb_config_para->local_net_para.self_ip[0], pw_clb_config_para->local_net_para.self_ip[1],
			pw_clb_config_para->local_net_para.self_ip[2], pw_clb_config_para->local_net_para.self_ip[3]);
	printf("\t->netmask : %d.%d.%d.%d \n", pw_clb_config_para->local_net_para.self_maskaddr[0], pw_clb_config_para->local_net_para.self_maskaddr[1],
			pw_clb_config_para->local_net_para.self_maskaddr[2], pw_clb_config_para->local_net_para.self_maskaddr[3]);
	printf("\t->gateway : %d.%d.%d.%d \n", pw_clb_config_para->local_net_para.self_gwaddr[0], pw_clb_config_para->local_net_para.self_gwaddr[1],
			pw_clb_config_para->local_net_para.self_gwaddr[2], pw_clb_config_para->local_net_para.self_gwaddr[3]);
	printf("\t->local_port : %d \n", pw_clb_config_para->local_net_para.net_port);
	printf("\t->train_num : %d \n", pw_clb_config_para->trainnum);

}

uint8_t app_paras_ip_check(uint8_t *ip)
{
	uint8_t result = 0;

	if((ip[0] != 192) || (ip[1] != 168) || (ip[3] != 17)) //192.168.xx.13
	{
		result = 1;
	}
   return result;
}

uint8_t app_paras_gateway_check(uint8_t *gateway)
{
	uint8_t result = 0;

	if((gateway[0] != 192) || (gateway[1] != 168) || (gateway[3] != 1)) //192.168.xx.1
	{
		result = 1;
	}
    return result;
}

/**
 * @brief
 * @param
 * @return
 * @note
**/
uint8_t app_paras_mask_check(uint8_t *mask)
{
	uint8_t result = 0;

	if((mask[0] != 255) || (mask[1] != 255)) //255.255.xx.xx
	{
		result = 1;
	}
    return result;
}

static inline void app_paras_config_inform(pconfig_inform_cmd_t config_inform)
{
//	paras_t *paras = OS_NULL;

	uint16_t crc_16 = modbus_crc((uint8_t *)config_inform, sizeof(config_inform_cmd_t) - 2);
	if((config_inform->device_code != 0x77) ||
	   (config_inform->sn[0] != 0x77) ||
	   (config_inform->sn[1] != 0x77) ||
	   (*(uint16_t *)config_inform->crc16 != crc_16))
	{
		printf("app paras config device_cord error or sn error or crc16 error \n");
		return ;
	}
	else //all check ok
	{
//		paras = app_paras_get();
		if(app_paras_ip_check(config_inform->local_ip))
		{
			printf("app paras config local ip error \n");
			app_paras_config_ack(0);
			return ;
		}
		if(app_paras_mask_check(config_inform->mask))
		{
			printf("app paras config mask error \n");
			app_paras_config_ack(0);
			return ;
		}
		if(app_paras_gateway_check(config_inform->gateway))
		{
			printf("app paras config gateway error \n");
			app_paras_config_ack(0);
			return ;
		}
		if((config_inform->train_num < 1) ||
			(config_inform->train_num > 4))
		{
			printf("app paras config train num error \n");
			app_paras_config_ack(0);
			return ;
		}
		int res =  memcmp(pw_clb_config_para->local_net_para.self_ip,config_inform->local_ip,4);
		if(res!=0)
		{
			memmove(pw_clb_config_para->local_net_para.self_ip, config_inform->local_ip, 4);
			memmove(pw_clb_config_para->local_net_para.self_maskaddr, config_inform->mask, 4);
			pw_clb_config_para->local_net_para.self_maskaddr[2] = 0;
			memmove(pw_clb_config_para->local_net_para.self_gwaddr, config_inform->gateway, 4);
			uint16_t port = (config_inform->port[0] << 8) + config_inform->port[1];
			pw_clb_config_para->local_net_para.net_port = port;
			pw_clb_config_para->trainnum = config_inform->train_num;
			printf("\t->recv config paras inform: \n");
			app_paras_net_printf();
			set_para_to_local();
			app_paras_config_ack(1);
			system("reboot -nf");
		}
	}
}


/**
 *
 */
void app_paras_config_get_data(uint8_t *data, uint16_t size, struct sockaddr_in recv_addr)
{
	pnet_config_protocol_t net_config_data;
	net_config_data = (pnet_config_protocol_t)data;


	printf("udp_client.c -- recv config msg : \n");
	for(int i = 0; i < size; i++)
	{
		if (i%10 == 0) printf("\n");
		printf("0x%02X ",data[i]);
	}
	printf("\n");


	if((*(uint16_t*)&net_config_data->data_head != htons(0xAA51))) {
		return;
	}

	uint16_t host_sum = sum_check_16(data, size - 2);
	uint16_t net_sum = (data[size - 2] << 8) + data[size - 1];
	if(host_sum != net_sum)
	{
		printf("recv msg crc failed \n");
		return;
	}
	else
	{
		if((net_config_data->factory_code != 0x04) ||
		   (net_config_data->device_code != 0xFF))
		{
			return ;
		}
		else
		{
			strcpy(app_env.config_net_inform.config_ip, inet_ntoa(recv_addr.sin_addr));
			app_env.config_net_inform.port = htons(recv_addr.sin_port);
			printf("recv config paras inform: %s, port %d\n", app_env.config_net_inform.config_ip, app_env.config_net_inform.port);
			pbroadcast_cmd_t broadcast_cmd;
			broadcast_cmd = (pbroadcast_cmd_t)net_config_data->data_inform;
			pconfig_inform_cmd_t config_inform_cmd;
			config_inform_cmd = (pconfig_inform_cmd_t)net_config_data->data_inform;
			switch(net_config_data->cmd)
			{
				case APP_BROADCAST_CMD:
					if(broadcast_cmd->bits.txb == 0x01) {
						app_paras_config_broadcast_ack();
					}
					return ;
				case APP_CONFIG_INFORM_CMD:

					printf("----APP_CONFIG_INFORM_CMD====\n");
					if(config_inform_cmd->target_broad.bits.txb == 0x01) {

						printf("--APP_CONFIG_INFORM_CMD---\n");
						app_paras_config_inform(config_inform_cmd);
					}
					return ;
				default:
					return ;
			}
		}
	}
}


/**
 * function     :udp_ucast_recv
 * description  :接收单播消息
 * input        :none
 * output       :none
 * return       :0-ok 非0-err
 * others       :none
 */

void udp_ucast_recv(void *ptr)
{
    struct sockaddr_in client_addr;
    struct sockaddr_in my_addr;
    socklen_t len = sizeof(client_addr);
    uint8_t udp_recv_buff[1024];
    char self_ip[16] = {0};
    int on = 1;

    ucast_recv_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    sprintf(self_ip,"%d.%d.%d.%d",pw_clb_config_para->local_net_para.self_ip[0],//192.168.1.17
    		pw_clb_config_para->local_net_para.self_ip[1],
			pw_clb_config_para->local_net_para.self_ip[2],
			pw_clb_config_para->local_net_para.self_ip[3]);

    /**socket复用 */
    int reuse = 1; //必须赋值为非零常数
    if (setsockopt(ucast_recv_sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("reusing socket failed");
        return;
    }

    /**绑定本地地址 */
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(pw_clb_config_para->local_net_para.net_port);//8000
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//    my_addr.sin_addr.s_addr = inet_addr(self_ip); //htonl(INADDR_ANY); //本地任意地址 inet_addr("192.168.40.100");

    if(setsockopt(ucast_recv_sock_fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0)
	{
		printf("add broadcast group failed!\n");
		return ;
	}

    int err = bind(ucast_recv_sock_fd, (struct sockaddr *)&my_addr, sizeof(my_addr));
    if (err != 0)
    {
        perror("bind");
    }

    while (1)
    {
        int r = recvfrom(ucast_recv_sock_fd, udp_recv_buff, sizeof(udp_recv_buff) - 1, 0, (struct sockaddr *)&client_addr, &len);
        if (r > 0)
        {
        	printf("-------------ucast_recv_msg_deal -------------------\n");
        	for(int i = 0; i < r; i++)
			{
        		printf("0x%02X ",udp_recv_buff[i]);
			}
        	printf("\n");
            ucast_recv_msg_deal(udp_recv_buff, r, client_addr);
            app_paras_config_get_data(udp_recv_buff, r, client_addr);
        }
    }
}


void udp_ucast_recv2(void *ptr)
{
    struct sockaddr_in client_addr;
    struct sockaddr_in my_addr;
    socklen_t len = sizeof(client_addr);
    uint8_t udp_recv_buff[1024];
    char self_ip[15] = {0};
    int on = 1;
    ucast_recv_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    sprintf(self_ip,"%d.%d.%d.%d",pw_clb_config_para->local_net_para.self_ip[0],
    		pw_clb_config_para->local_net_para.self_ip[1],
			pw_clb_config_para->local_net_para.self_ip[2],
			pw_clb_config_para->local_net_para.self_ip[3]);

    /**socket复用 */
    int reuse = 1; //必须赋值为非零常数
    if (setsockopt(ucast_recv_sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("reusing socket failed");
        return;
    }

    /**绑定本地地址 */
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(pw_clb_config_para->local_net_para.net_port); //8000
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//    my_addr.sin_addr.s_addr = inet_addr(self_ip); //htonl(INADDR_ANY); //本地任意地址 inet_addr("192.168.40.100");
    if(setsockopt(ucast_recv_sock_fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0)
	{
		printf("add broadcast group failed!\n");
		return ;
	}

    int err = bind(ucast_recv_sock_fd, (struct sockaddr *)&my_addr, sizeof(my_addr));
    if (err != 0)
    {
        perror("bind");
    }

    while (1)
    {
        int r = recvfrom(ucast_recv_sock_fd, udp_recv_buff, sizeof(udp_recv_buff) - 1, 0, (struct sockaddr *)&client_addr, &len);
        if (r > 0)
        {
        	printf("--------- recv config!!!------------\n");
//            ucast_recv_msg_deal(udp_recv_buff, r, client_addr);
            app_paras_config_get_data(udp_recv_buff, r, client_addr);
        }
    }
}


/**
 * @brief  update time
 * @param
 * @return
 * @note
**/
static int app_time_msg_valid_deal(uint8_t *timebuf)
{
    static uint8_t time_cnt=0;
    static uint8_t set_time_flag=1;
    struct LOCAL_TIME time_t,loctime;
    int ret,return_t=0;


    if((timebuf[0] >= 20) && (timebuf[0] <= 99) && (timebuf[1] >= 1) && (timebuf[1] <= 12) && \
                           (timebuf[2] >= 1) && (timebuf[2] <= 31) && (timebuf[3] >= 0) && (timebuf[3] <= 23) && \
                           (timebuf[4] >= 0) && (timebuf[4] <= 59) && (timebuf[5] >= 0) && (timebuf[5] <= 59))
    {
		if(set_time_flag)
		{
			time_t.year= timebuf[0]+2000;
			time_t.mon = timebuf[1];
			time_t.day = timebuf[2];
			time_t.hour= timebuf[3];
			time_t.min = timebuf[4];
			time_t.sec = timebuf[5];

			set_local_time(&time_t);
	//    	app_set_local_time(&time_t);

			printf("$$$$sync mvb time.....\n");
			set_time_flag=0;
			return_t=1;
		}
		else if(time_cnt==51)
		{
			get_local_time(&loctime);
			time_t.year=timebuf[0] + 2000;
			time_t.mon=timebuf[1];
			time_t.day=timebuf[2];
			time_t.hour=timebuf[3];
			time_t.min=timebuf[4];
			time_t.sec=timebuf[5];

		  ret=memcmp(&time_t,&loctime,4);

		  if(ret!=0)
			{
			   printf("$$$$time reset....\n");
			   set_local_time(&time_t);
			}
		  time_cnt=0;
		}
		time_cnt++;
    }
    return return_t;
}

void app_save_public_info(uint8_t* public_data)
{
   if (public_data == NULL)
    {
	   printf("app_save_public_info   public_data is NULL!!!\n");
    	return;
    }
   memcpy(&app_save_public, public_data, sizeof(txb_MVB_public_t));
}

void app_protocol_paras_UDP_data(uint8_t *data, uint16_t size, struct sockaddr_in recv_addr)
{
	record_unicast_protocol_t para_config_data;
	memcpy(&para_config_data, data, sizeof(record_unicast_protocol_t)-2);
	txb_MVB_public_t app_paras_public;

	if((*(uint16_t*)&para_config_data.data_head != htons(0xAA50))) {
			return;
	}
	para_config_data.sum_crc[0] = data[size - 2];
	para_config_data.sum_crc[1] = data[size - 1];

	// 校验数据是否接收完整
	uint16_t host_sum = sum_check_16(data, size - 2);
	uint16_t net_sum = (para_config_data.sum_crc[0] << 8) + para_config_data.sum_crc[1];


//	printf("udp_client.c -- recv public : \n");
//	for(int i = 0; i < size; i++)
//	{
//		if (i%10 == 0) printf("\n");
//		printf("0x%02X ",data[i]);
//	}
//	printf("\n");


	if(host_sum != net_sum)
	{
		printf("recv msg crc failed \n");
		return;
	}
	else
	{
		if(para_config_data.factory_code != 0x04)
		{
			return ;
		}
		if (para_config_data.device_code == RECORD_BOARD) //处理记录板发送的信息
		{
			if (para_config_data.cmd == 0x00) //暂时写这个 没写宏
			{
				//	多播信息，即为公共信息
				memcpy(&app_paras_public, &data[26], sizeof(txb_MVB_public_t));
				app_save_public_info((uint8_t*)&app_paras_public);

				printf("---test--- in app_udp_mcast_recv_msg_deal recv time valid = 0x%x\n",  app_paras_public.valid.bits.time_valid);

				printf("---test---  recv time = 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",  app_paras_public.time[0], app_paras_public.time[1],app_paras_public.time[2],\
							 app_paras_public.time[3], app_paras_public.time[4],app_paras_public.time[5]);
				if (app_paras_public.valid.bits.time_valid)
				{
					app_time_msg_valid_deal(app_paras_public.time);
				}

			}
		}
	}
}


static inline void app_send_record_fill_head(void)
{
#define head_data env
	static uint16_t psw_record_life;

	uint16_t length = sizeof(pswb_record_protocol_t);
	psw_record_life++;
	head_data.data_head[0] = 0xAA;
	head_data.data_head[1] = 0x50;
	head_data.data_len[0] = (uint8_t)(length >> 8);
	head_data.data_len[1] = (uint8_t)(length);
	head_data.factory_code = DEVICE_FACTORY_CODE;
	head_data.device_code =  0x77;
	head_data.life_signal[0] = (uint8_t)(psw_record_life >> 8);
	head_data.life_signal[1] = (uint8_t)(psw_record_life);
	head_data.target_addr[0] = (uint8_t)(0x01 >> 8);
	head_data.target_addr[1] = (uint8_t)(0x01);
	head_data.udp_packet = 0x01;
#undef head_data
}

static inline void app_send_record_fill_pwb(void)
{
	psw_info_data_t * record_data_p;
	psw_info_data_t * psw_info_p;
	record_data_p = (psw_info_data_t*)env.data_inform;
	psw_info_p = &psw_info_data;

	memcpy(&env.data_inform, psw_info_p, sizeof(psw_info_data_t));
}

static inline void app_send_record_fill_crc(void)
{
	uint16_t check_sum = sum_check_16((uint8_t *)&env, sizeof(pswb_record_protocol_t) - 2);
	env.sum_crc[0] = (uint8_t)(check_sum >> 8);
	env.sum_crc[1] = (uint8_t)(check_sum);
}
void udp_mcast_send(void *ptr)
{
	multicast_socket = -1;
	if((multicast_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("multicast_send_handle failed!\n");
		return;
	}
	while(1)
	{
		app_send_record_fill_head();
		app_send_record_fill_pwb();
		app_send_record_fill_crc();
		app_multicast_send(&env,sizeof(pswb_record_protocol_t));
		sleep(1);
	}
}
//Cannot assign requested address
/**
 * function     :udp_mcast_recv
 * description  :接收组播消息
 * input        :none
 * output       :none
 * return       :none
 * others       :none
 */
void udp_mcast_recv(void *ptr)
{
    struct sockaddr_in my_addr;
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    uint8_t udp_recv_buff[1024];
    char udp_recv_ip[15] = {0};
    char self_ip[15] = {0};
    int recv_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    sprintf(udp_recv_ip,"%d.%d.%d.%d",pw_clb_config_para->pw_recv_mcast_addr.mcast_addr[0], //239.255.10.1
    		pw_clb_config_para->pw_recv_mcast_addr.mcast_addr[1],
			pw_clb_config_para->pw_recv_mcast_addr.mcast_addr[2],
			pw_clb_config_para->pw_recv_mcast_addr.mcast_addr[3]);

    sprintf(self_ip,"%d.%d.%d.%d",pw_clb_config_para->local_net_para.self_ip[0], //192.168.1.17
    		pw_clb_config_para->local_net_para.self_ip[1],
			pw_clb_config_para->local_net_para.self_ip[2],
			pw_clb_config_para->local_net_para.self_ip[3]);

    /**socket复用 */
    int reuse = 1; //必须赋值为非零常数
    if (setsockopt(recv_sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("reusing socket failed");
        return;
    }

    /**绑定本地地址 */
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(pw_clb_config_para->pw_recv_mcast_addr.mcast_port); //组播端口
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);     //本地任意地址 inet_addr("192.168.40.100");
    int err = bind(recv_sock_fd, (struct sockaddr *)&my_addr, sizeof(my_addr));
    if (err != 0)
    {
        perror("bind");
    }

    /*设置回环许可:当接收者加入到一个多播组以后，再向这个多播组发送数据，这个字段的设置是否允许再返回到本身*/

    int loop = 0;
    err = setsockopt(recv_sock_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    if (err < 0)
    {
        perror("setsockopt():IP_MULTICAST_LOOP");
        return;
    }
    /**默认情况下，多播报文的ＴＴＬ被设置成了１，也就是说到这个报文在网络传送的时候，它只能在自己所在的网络传送，当要向外发送的时候，路由器把ＴＴＬ减１以后变成了０，这个报文就已经被Discard了*/
    unsigned char ttl = 1;
    err = setsockopt(recv_sock_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
    if (err < 0)
    {
        perror("setsockopt():IP_MULTICAST_TTL");
        return;
    }

    /**参数addr是希望多播输出接口的IP地址，使用INADDR_ANY地址回送到默认接口。 */
    struct in_addr addr;
    addr.s_addr = htonl(INADDR_ANY);
    setsockopt(recv_sock_fd, IPPROTO_IP, IP_MULTICAST_IF, &addr, sizeof(addr));

    /*将本机加入多播组*/
    struct ip_mreq mreq;                                          /*加入多播组*/
    mreq.imr_multiaddr.s_addr = inet_addr(udp_recv_ip); 		  /*多播地址*/
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);                /*网络接口为默认*/
    //mreq.imr_interface.s_addr = inet_addr(self_ip);
    DEBUG("recv_sock_fd_one\n");
    err = setsockopt(recv_sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    if (err < 0)
    {
        perror("setsockopt():IP_ADD_MEMBERSHIP");
        return;
    }
    DEBUG("recv_sock_fd_two\n");

    while (1)
    {
        int r = recvfrom(recv_sock_fd, udp_recv_buff, sizeof(udp_recv_buff) - 1, 0, (struct sockaddr *)&client_addr, &len);
        if (r > 0)
        {
        	printf("mcast_recv_msg_deal \n");
        	DEBUG("mcast_recv_msg_dealmcast_recv_msg_dealmcast_recv_msg_deal\n");
        	app_protocol_paras_UDP_data(udp_recv_buff,r, client_addr);
        }
    }
}

/**
 * function     :init_udp
 * description  :初始化udp，建立发送套接字，端口复用，板卡IP地址初始化，绑定本地端口，单播、组播接收线程初始化
 * input        :none
 * output       :none
 * return       :0-ok 非0-err
 * others       :none
 */
int init_udp(void)
{
	char str[100];
	char recv_mcast_ip[15];
	char send_mcast_ip[15];
	char ucast_save_ip[15];
	char ucast_pw_ip[15];
	char ucast_pw_mask[15];
	char ctrla_ip[15];
	char ctrlb_ip[15];


    /**通信、记录板组播地址 */
	sprintf(send_mcast_ip,"%d.%d.%d.%d",
			pw_clb_config_para->pw_send_mcast_addr.mcast_addr[0], //239.255.10.2
			pw_clb_config_para->pw_send_mcast_addr.mcast_addr[1],
			pw_clb_config_para->pw_send_mcast_addr.mcast_addr[2],
			pw_clb_config_para->pw_send_mcast_addr.mcast_addr[3]);

    memset(&mcast_ctl_save_group_addr, 0, sizeof(mcast_ctl_save_group_addr));         /*初始化IP多播地址为0*/
    mcast_ctl_save_group_addr.sin_family = AF_INET;                                   /*设置协议族类行为AF*/
    mcast_ctl_save_group_addr.sin_addr.s_addr = inet_addr(send_mcast_ip); 			 /*设置多播IP地址*/
    mcast_ctl_save_group_addr.sin_port = htons(pw_clb_config_para->pw_send_mcast_addr.mcast_port);  //9002          /*设置多播端口*/

	sprintf(recv_mcast_ip,"%d.%d.%d.%d",
			pw_clb_config_para->pw_recv_mcast_addr.mcast_addr[0], //239.255.10.1
			pw_clb_config_para->pw_recv_mcast_addr.mcast_addr[1],
			pw_clb_config_para->pw_recv_mcast_addr.mcast_addr[2],
			pw_clb_config_para->pw_recv_mcast_addr.mcast_addr[3]);

    /*记录板单播地址 */
	sprintf(ucast_save_ip,"%d.%d.%d.%d",
			pw_clb_config_para->save_board_addr.ucast_addr[0], //192.168.1.11 8000
			pw_clb_config_para->save_board_addr.ucast_addr[1],
			pw_clb_config_para->save_board_addr.ucast_addr[2],
			pw_clb_config_para->save_board_addr.ucast_addr[3]);
    memset(&ucast_save_board_addr, 0, sizeof(ucast_save_board_addr));
    ucast_save_board_addr.sin_family = AF_INET;
    ucast_save_board_addr.sin_addr.s_addr = inet_addr(ucast_save_ip);
    ucast_save_board_addr.sin_port = htons(pw_clb_config_para->save_board_addr.ucast_port);


    sprintf(local_ip,"%d.%d.%d.%d",
    			pw_clb_config_para->local_net_para.self_ip[0], //192.168.1.17 8000
    			pw_clb_config_para->local_net_para.self_ip[1],
    			pw_clb_config_para->local_net_para.self_ip[2],
    			pw_clb_config_para->local_net_para.self_ip[3]);
    /*平稳板单播地址 */
	sprintf(ucast_pw_ip,"%d.%d.%d.%d",
			pw_clb_config_para->local_net_para.self_ip[0], //192.168.1.17 8000
			pw_clb_config_para->local_net_para.self_ip[1],
			pw_clb_config_para->local_net_para.self_ip[2],
			pw_clb_config_para->local_net_para.self_ip[3]);
    memset(&ucast_st_board_addr, 0, sizeof(ucast_st_board_addr));
    ucast_st_board_addr.sin_family = AF_INET;
    ucast_st_board_addr.sin_addr.s_addr = inet_addr(ucast_pw_ip);
    ucast_st_board_addr.sin_port = htons(pw_clb_config_para->local_net_para.net_port);

	sprintf(ucast_pw_mask,"%d.%d.%d.%d",
			pw_clb_config_para->local_net_para.self_maskaddr[0], //255.255.0.0
			pw_clb_config_para->local_net_para.self_maskaddr[1],
			pw_clb_config_para->local_net_para.self_maskaddr[2],
			pw_clb_config_para->local_net_para.self_maskaddr[3]);


    sprintf(str,"%s %s %s %s","ifconfig eth0",ucast_pw_ip,"netmask",ucast_pw_mask);//"netmask",local_mask
    printf("------------>%s<------\n",str);
    system(str);
    memset(str,0,100);
    sprintf(str,"%s %s %s","route add -net",recv_mcast_ip,"netmask 255.255.255.255 eth0");
    system(str);
    printf("------------>%s<------\n",str);
    memset(str,0,100);
    sprintf(str,"%s %s %s","route add -net",send_mcast_ip,"netmask 255.255.255.255 eth0");
    system(str);
    printf("------------>%s<------\n",str);
    memset(str,0,100);
	sprintf(str,"%s %s %s","route add -net","255.255.255.255","netmask 255.255.255.255 dev eth0 metric 1");
	system(str);
	printf("------------>%s<------\n",str);


    /**发送锁初始化 */
    pthread_mutex_init(&udp_send_mutex, NULL);
    pthread_cond_init(&udp_send_cond, NULL);

    sem_init(&udp_send_sem, 0, 0);

    /**单播、组播接收线程 **/
    pthread_t udp_recv_ucast_thread, udp_recv_mcast_thread,udp_send_ucast_thread,udp_recv_ucast_thread2;
    int ret1 = pthread_create(&udp_recv_ucast_thread, NULL, (void *)&udp_ucast_recv, (void *)send_fd_socket);
    if (ret1 != 0)
    {
        perror("pthread_create");
    }
    //
     int ret2 = pthread_create(&udp_recv_mcast_thread, NULL, (void *)&udp_mcast_recv, (void *)send_fd_socket);
     if (ret2 != 0)
      {
        perror("pthread_create");
       }

     int ret3 = pthread_create(&udp_send_ucast_thread, NULL, (void *)&udp_mcast_send, (void *)send_fd_socket);
     if (ret3 != 0)
	 {
    	 perror("pthread_create");
	 }

//     int ret4 = pthread_create(&udp_recv_ucast_thread2, NULL, (void *)&udp_ucast_recv2, (void *)send_fd_socket);
//	if (ret4 != 0)
//	{
//		perror("pthread_create");
//	}

    return 0;
}


void test_udp(void)
{
    init_udp();
    // printf("size struct struct SW_TZ_DATA=%d\n", sizeof(struct PW_TZ_DATA));
    char MCAST_DATA1[] = "A welcome to china 1! ";
    char MCAST_DATA2[] = "B welcome to china 2! ";
    char MCAST_DATA3[] = "C welcome to china 3! ";
    char MCAST_DATA4[] = "D welcome to china 4! ";
    // memset(&vibr, 0, sizeof(struct SW_TZ_DATA));
    // vibr.data_head.head =htons(0x55aa) ;
    // // vibr.data_head.head[1] = 0xaa;
    // // vibr.data_head.len[0] = 0x66;
    // // vibr.data_head.len[1] = 0xbb;
    // vibr.data_head.len = htons(0x66bb);

    struct UDP_SEND_STYLE style =
    {
		.is_ucast_flag = 0,
		.is_need_ack_flag = 1,
		.time_out_ms = 300,
		.resend_cnt = 3
    };

    while (1)
    {
        udp_send((uint8_t*)MCAST_DATA1, sizeof(MCAST_DATA1),
                 (struct sockaddr *)&ucast_save_board_addr,
                 sizeof(ucast_save_board_addr), style);
        udp_send((uint8_t*)MCAST_DATA2, sizeof(MCAST_DATA2),
                 (struct sockaddr *)&ucast_save_board_addr,
                 sizeof(ucast_save_board_addr), style);
        udp_send((uint8_t*)MCAST_DATA3, sizeof(MCAST_DATA2),
                 (struct sockaddr *)&ucast_save_board_addr,
                 sizeof(ucast_save_board_addr), style);
        udp_send((uint8_t*)MCAST_DATA4, sizeof(MCAST_DATA2),
                 (struct sockaddr *)&ucast_save_board_addr,
                 sizeof(ucast_save_board_addr), style);
        //		 select_sleep_s(1);
        //		 udp_send(MCAST_DATA2, sizeof(MCAST_DATA2),
        //					   (struct sockaddr *)&ucast_save_board_addr,
        //					   sizeof(ucast_save_board_addr), udp_send_time_out, RESEND_CNT);
        //		 udp_send(MCAST_DATA3, sizeof(MCAST_DATA3),
        //					   (struct sockaddr *)&ucast_save_board_addr,
        //					   sizeof(ucast_save_board_addr), udp_send_time_out, RESEND_CNT);
        //		 udp_send(MCAST_DATA4, sizeof(MCAST_DATA4),
        //					   (struct sockaddr *)&ucast_save_board_addr,
        //					   sizeof(ucast_save_board_addr), udp_send_time_out, RESEND_CNT);
        //		 printf("a\n");
        ////		 pthread_sigmask(SIG_UNBLOCK, &set, NULL);
        //		 printf("c\n");
        //		 select_sleep_s(5);
        //		 printf("b\n");
    }
}

/**
 * function     :udp_send
 * description  :udp发送函数，可设置超时时间和重发次数
 * input        :buff     -要发送的数据
 *               buff_size-要发送的数据长度
 *               _addr    -要发送的目标地址，可以是单播地址，也可以是组播地址
 *               _addr_len-目标地址的长度
 *               time_out -超时时间，若为sec和usec为0表示没有超时判断，不重发
 *              resend_cnt-超时重发次数
 * output       :none
 * return       :0-ok 非0-err
 * others       :none
 */
int udp_send(uint8_t *buff, uint16_t buff_size, struct sockaddr *__addr, socklen_t __addr_len, const struct UDP_SEND_STYLE style)
{
#if UDP_SEND_JMP_TIMEOUT == 1
    sigset_t set;
    pthread_mutex_timedlock(&udp_send_mutex, &udp_lock_ts);
    uint16_t send_cnt = 0;
    int a;

    //超时重发
    if (time_out.tv_sec > 0 || time_out.tv_usec > 0)
    {
        unsigned int tv = (time_out.tv_sec * 1000 + time_out.tv_usec / 1000) / MULTI_BASE_TIMER_MS;
    AGAIN:
        sigemptyset(&set);
        sigaddset(&set, SIGALRM);
        pthread_sigmask(SIG_UNBLOCK, &set, NULL);
        sendto(send_fd_socket, buff, buff_size, 0, __addr, __addr_len);
        send_cnt++;
        set_timer_task(TIMER_2, 29, 0, udp_send_timeout_handler, &a);
        udp_recv_id = *buff; //TODO 需要修改接收正确的判断
                             //        printf("%d: send data %s ,id=%c\n", send_cnt,buff, udp_recv_id);
        if (sigsetjmp(usp_send_jmp_buf, 1) != 0)
        {
            pthread_sigmask(SIG_BLOCK, &set, NULL);
            if (send_cnt >= (resend_cnt + 1))
            {
                cancel_timer_task(TIMER_2, CANCEL_MODE_IMMEDIATELY);
                pthread_mutex_unlock(&udp_send_mutex);
                return -1;
            }
            else
            {
                goto AGAIN;
            }
        }
        while (1)
        {
            /**判断是否接收到应答报文 */
            if (udp_recv_ok_flag)
            {
                cancel_timer_task(TIMER_2, CANCEL_MODE_IMMEDIATELY);
                udp_recv_ok_flag = 0;
                break;
            }
            select_sleep_ms(5);
        }
    }
    else
    {
        //无超时重发
        sendto(send_fd_socket, buff, buff_size, 0, __addr, __addr_len);
    }

    pthread_mutex_unlock(&udp_send_mutex);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    return 0;
#elif UDP_SEND_SELECT_TIMEOUT == 1
    int maxfdp;
    int send_cnt = 0;
    struct sockaddr_in client_addr;
    char udp_recv_buff[2048];
    fd_set fds;
    struct timeval tv;
    //    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    while (1)
    {
        sendto(send_fd_socket, buff, buff_size, 0, __addr, __addr_len);
        FD_ZERO(&fds);
        FD_SET(send_fd_socket, &fds);
        maxfdp = send_fd_socket + 1;
        tv.tv_sec = 0;
        tv.tv_usec = 300000;
        switch (select(maxfdp, &fds, NULL, NULL, &tv))
        {
        case -1: //出错
        	DEBUG("select err!\n");
            close(send_fd_socket);
            return -1;
            break;
        case 0: //超时重发
            tv.tv_sec = 0;
            tv.tv_usec = 300000;
            send_cnt++;
            DEBUG("select out!\n");
            if (send_cnt > 3)
            {
            	DEBUG("over!\n");
                return -1;
            }
            else
            {
                continue;
                break;
            }
        default:
        	DEBUG("select >0!\n");
            if (FD_ISSET(send_fd_socket, &fds))
            {
                int r = recvfrom(send_fd_socket, udp_recv_buff, sizeof(udp_recv_buff) - 1, 0, NULL, &len);
                if (r >= 0)
                {
                	DEBUG("recv:%s\n", udp_recv_buff);
                }
                else
                {
                	DEBUG("select close!\n");
                    close(send_fd_socket);
                    return -1;
                }
            }
            break;
        }
    }
#elif UDP_SEND_LOCK_TIMEOUT == 1
    lock_timedwait_millsecs(&udp_send_mutex, 10000);
    uint16_t send_cnt = 0;
    uint16_t false_wake_up_cnt = 0;
    //超时重发
    if ((style.time_out_ms > 0) && (style.resend_cnt > 0) && (style.is_need_ack_flag == 0x5a))
    {
    	//fixme 设置发送的生命信号信息
        //set_send_life_sig(0, 0, 0, LOCAL_BOARD);
    	//void set_send_life_sig(uint16_t board_id, uint16_t send_addr_bits, uint16_t send_head, uint16_t send_life)
    	set_send_life_sig(LOCAL_BOARD,ntohs(*(uint16_t *)(buff+8)),ntohs(*(uint16_t *)(buff+0)),ntohs(*(uint16_t *)(buff+6)));
    	reset_recv_life_sig();
        buff[style.ack_byte_of_buff]=0x55; //首次发送用0x55
    AGAIN:
        sendto(send_fd_socket, buff, buff_size, 0, __addr, __addr_len);
        while (1)
        {
//            struct timespec ts;
//            clock_gettime(CLOCK_REALTIME, &ts);
//            printf("sec:%d,nsec:%d,set_ms:%d\n",ts.tv_sec,ts.tv_nsec,style.time_out_ms);
            int ret = cond_timedwait_millsecs(&udp_send_mutex, &udp_send_cond, style.time_out_ms);
//            clock_gettime(CLOCK_REALTIME, &ts);
//            printf("sec:%d,nsec:%d\n",ts.tv_sec,ts.tv_nsec);

            if (ret != 0) //如果超时返回
            {
                //            	printf("time out!\n");
                if (send_cnt < style.resend_cnt)
                {
                    send_cnt++;
//                    printf("udp_send--1-check_eth_status(%x/^%x)\n",ack_life_sig.send_addr_bits,ack_life_sig.recv_addr_bits);
                    check_eth_status (ack_life_sig.send_addr_bits ^ ack_life_sig.recv_addr_bits);
                   // printf("ack_life_sig.send_addr_bits:%x,ack_life_sig.recv_addr_bits:%x\n",ack_life_sig.send_addr_bits,ack_life_sig.recv_addr_bits);
                    buff[style.ack_byte_of_buff]=0xaa;//重发用0xaa
                    goto AGAIN;
                }
                else
                {
                    break;
                }
            }
            else //如果正常收到应答
            {
                if (check_ack_life_sig() == UDP_ACK_OK) //防止虚假唤醒
                {
//                	printf("udp_send--2-check_eth_status(%x/^%x)\n",ack_life_sig.send_addr_bits,ack_life_sig.recv_addr_bits);
                	check_eth_status(ack_life_sig.send_addr_bits ^ ack_life_sig.recv_addr_bits);
                    memset(&ack_life_sig, 0, sizeof(struct ACK_SIG));
                    break;
                }
                else
                {
                    //判断此为虚假唤醒
                	DEBUG("false wake up!!\n");
                    false_wake_up_cnt++;
                    if (false_wake_up_cnt >= 10)
                    {
                        pthread_mutex_unlock(&udp_send_mutex);
                        return -1;
                    }
                }
            }
        }
    }
    else
    {
        //无超时重发
        memset(&ack_life_sig, 0, sizeof(struct ACK_SIG));
        sendto(send_fd_socket, buff, buff_size, 0, __addr, __addr_len);
        //check_eth_status(0);
        //(sizeof(_addr->sa_data)/sizeof(_addr->sa_data[0]))
//        printf("send_addr:\n");
//        for(uint8_t i = 0;i<14 ;i++)
//        {
//        printf("%d.",__addr->sa_data[i]);
//        }
//        printf("\n");
    }
    pthread_mutex_unlock(&udp_send_mutex);
    return 0;
#endif
}

void get_target_addr_ip(uint16_t target_addr)
{
//	struct sockaddr_in target_ip_addr;
	switch(target_addr)
	{
	case CTRLA_BOARD:

		break;

	case CTRLB_BOARD:

		break;

	case SAVE_BOARD:

		break;

	case AXLE_TEMPA_BOARD:

		break;

	case AXLE_TEMPB_BOARD:

		break;

	case PW_BOARD:

		break;

	case SW_BOARD:

		break;

	case VIBR_BOARD:

		break;

	case IO_BOARD:

		break;

	case GEAR_BOARD:

		break;

	case MOTOR_BOARD:

		break;

	default:
		break;
	}

}
/**
 * function     :send_pw_tz_data_can
 * description  :通过CAN发送平稳特征数据
 * input        :send_tz_data　//平稳特征数据
 * 				：singal_num	　//生命信号
 * 				:target_addr //目标板卡地址
 * 				:resend_flag //重发标志
 * 				:ack_flag //应答标志
 * output       :none
 * return       :0-ok 非0-err
 * others       :none
 */

void send_pw_tz_data_can(struct PW_TZ_DATA *send_tz_data,uint16_t singal_num,uint16_t target_addr,uint8_t resend_flag,uint8_t ack_flag)
{
	uint16_t tagetmask = 0;
	if(ack_flag == 1)
	{
		ack_flag = 0x5a;
	}
	else
	{
		ack_flag = 0;
	}

	*(uint16_t *)send_tz_data->data_head.head =little_to_big_16bit(0xAA50);
	*(uint16_t *)send_tz_data->data_head.len = little_to_big_16bit(sizeof(struct PW_TZ_DATA)-128);
	send_tz_data->data_head.company_id = LUHANG;
	send_tz_data->data_head.board_id = PW_BOARD;
	*(uint16_t *)send_tz_data->data_head.life_signal = little_to_big_16bit(singal_num);

	*(uint16_t *)send_tz_data->data_head.target_board_group = little_to_big_16bit(target_addr);
	//DEBUG("target_addr:%x\n",target_addr);
	send_tz_data->data_head.resend_flag = resend_flag;
	send_tz_data->data_head.ack_flag = ack_flag;
	send_tz_data->data_head.packet_num = 0;										//发送CAN该字节为0

#ifdef CAN_PROTOCOL_20210322
//	BIT_IP_SEQ == CAN_COMM_BOARD_ADDR ---> CAN_COMM_RX_MASK  //udp_send_addr.target_board_addr[i] = 1<<i; i=0,1,2
	if(target_addr == 0x01)//CTRLA_BOARD_CANID  1<<0
	{
		tagetmask = CTRLA_RX_MASK;
	}
	else if(target_addr == 0x02)//CTRLB_BOARD_CANID 1<<1
	{
		tagetmask = CTRLB_RX_MASK;
	}
	else if(target_addr == 0x03)//CTRLA_BOARD_CANID CTRLB_BOARD_CANID  (1<<0+1<<1)
	{
		tagetmask = CTRLA_RX_MASK|CTRLB_RX_MASK;
	}
	else if(target_addr == 0x04)//SAVE_BOARD_CANID 1<<2
	{
		tagetmask = SAVE_RX_MASK;
	}
	else if(target_addr == 0x05)//CTRLA_BOARD_CANID SAVE_BOARD_CANID
	{
		tagetmask = CTRLA_RX_MASK|SAVE_RX_MASK;
	}
	else if(target_addr == 0x06)//CTRLB_BOARD_CANID SAVE_BOARD_CANID
	{
		tagetmask = CTRLB_RX_MASK|SAVE_RX_MASK;
	}
	else if(target_addr == 0x07)//CTRLA_BOARD_CANID CTRLB_BOARD_CANID SAVE_BOARD_CANID
	{
		tagetmask = CTRLA_RX_MASK|CTRLB_RX_MASK|SAVE_RX_MASK;
	}
#else
	if(target_addr == 0x01)
	{
		tagetmask = 0x02;
	}
	else if(target_addr == 0x02)
	{
		tagetmask = 0x04;
	}
	else if(target_addr == 0x03)
	{
		tagetmask = 0x06;
	}
	else if(target_addr == 0x04)
	{
		tagetmask = 0x08;
	}
	else if(target_addr == 0x05)
	{
		tagetmask = 0x0A;
	}
	else if(target_addr == 0x06)
	{
		tagetmask = 0x0c;
	}
	else if(target_addr == 0x07)
	{
		tagetmask = 0x0e;
	}
#endif

#ifdef TWO_POWER_TO_BOARD_ERR
//	if(send_tz_data->borad_err.bits.power1_err==1 && send_tz_data->borad_err.bits.power2_err==1)//send_tz_data->borad_err.bits.save_err==1 ||
//		send_tz_data->borad_err.bits.pw_board_err=1;
#endif

	//DEBUG("------>can_send_data_start<----------2\n");
#ifdef INTERNAL_PROTOCOL_20210725
	*(uint16_t *)&send_tz_data->pw_res[0] = little_to_big_16bit(check_sum((uint8_t *)send_tz_data,sizeof(struct PW_TZ_DATA)-2-128));
#elif defined(INTERNAL_PROTOCOL_20210416)
	*(uint16_t *)&send_tz_data->pw_res[14] = little_to_big_16bit(check_sum((uint8_t *)send_tz_data,sizeof(struct PW_TZ_DATA)-2-128));
#else
	*(uint16_t *)&send_tz_data->km_scale[26] = little_to_big_16bit(check_sum((uint8_t *)send_tz_data,sizeof(struct PW_TZ_DATA)-2-128));
#endif
	//	DEBUG("------>can_send_data_start_sum:%d<----------1\n",check_sum((uint8_t *)send_tz_data,sizeof(struct SW_TZ_DATA)-2-128));
//	DEBUG("------>can_send_data_start_sum:%d<----------2\n",*(uint16_t *)&send_tz_data->km_scale[38]);
//	DEBUG("------>can_send_data_start_sum:%d<----------3\n",send_tz_data->km_scale[38]);
//	DEBUG("------>can_send_data_start_sum:%d<----------4\n",send_tz_data->km_scale[39]);

	//printf("------>can_send_data_start<----------4\n");


	can_send_data((uint8_t *)send_tz_data, sizeof(struct PW_TZ_DATA) - 128,tagetmask);
	//DEBUG("------>can_send_data_start<----------5\n");
//	uint8_t testbuf[40];
//	memmove(testbuf,send_tz_data,40);
//	for(int i=0 ;i<40;i++)
//	{
//		printf(" %x ",testbuf[i]);
//	}
//	printf("\n");

	comm_data_cnt.send_can_all_cnt++;
	comm_data_cnt.send_can_signal = singal_num;



}

void send_pw_tz_data(struct PW_TZ_DATA *send_tz_data,uint16_t singal_num,uint16_t target_addr,uint8_t resend_flag,uint8_t ack_flag)
{
//	uint16_t write_res = 0;
	struct UDP_SEND_STYLE udp_send_style;
	struct sockaddr_in target_ip_addr;
//	socklen_t target_addr_len;
	if(ack_flag == 1)
	{
		ack_flag = 0x5A;
	}
	else
	{
		ack_flag = 0;
	}

	*(uint16_t *)send_tz_data->data_head.head =little_to_big_16bit(0xAA50);
	*(uint16_t *)send_tz_data->data_head.len = little_to_big_16bit(256);
	send_tz_data->data_head.company_id = LUHANG;
	send_tz_data->data_head.board_id = PW_BOARD;
	*(uint16_t *)send_tz_data->data_head.life_signal = little_to_big_16bit(singal_num);

	*(uint16_t *)send_tz_data->data_head.target_board_group = little_to_big_16bit(target_addr);
	send_tz_data->data_head.resend_flag = resend_flag;
	send_tz_data->data_head.ack_flag = ack_flag;

#ifdef TWO_POWER_TO_BOARD_ERR
//	if(send_tz_data->borad_err.bits.power1_err==1 && send_tz_data->borad_err.bits.power2_err==1)//send_tz_data->borad_err.bits.save_err==1 ||
//		send_tz_data->borad_err.bits.pw_board_err=1;
#endif

	*(uint16_t *)send_tz_data->check_sum = little_to_big_16bit(check_sum((uint8_t *)send_tz_data,sizeof(struct PW_TZ_DATA)-2));

//	uint8_t testbuf[40];
//	memmove(testbuf,send_tz_data,40);
//	for(int i=0 ;i<40;i++)
//	{
//		printf(" %x ",testbuf[i]);
//	}
//	printf("\n");

	udp_send_style.ack_byte_of_buff = 10;
	udp_send_style.resend_cnt = 3;
	udp_send_style.is_need_ack_flag = ack_flag;
	udp_send_style.is_ucast_flag = 0;				//采用单播或者组播方式发送
	udp_send_style.time_out_ms = 300;

	//target_ip_addr = udp_send_addr.send_target_sockaddr;
	memmove(&target_ip_addr,&udp_send_addr.send_target_sockaddr,sizeof(struct sockaddr_in));
	//mcast_ctl_save_group_addr
	//printf("send_pw_tz_data\n");
	//过程数据发送组播地址
	//get_target_ip_addr();

#if 0
	printf("send_pw_tz_data---side1_y_alarm:%d, side1_z_alarm:%d, side2_y_alarm:%d, side2_z_alarm:%d\n",
			send_tz_data->alarm_status.bits.side1_y_alarm, send_tz_data->alarm_status.bits.side1_z_alarm,
			send_tz_data->alarm_status.bits.side2_y_alarm, send_tz_data->alarm_status.bits.side2_z_alarm);
#endif

//	printf("send_pw_tz_data---send mcast---len:%d\n",sizeof(struct PW_TZ_DATA));
	udp_send((uint8_t*)send_tz_data,sizeof(struct PW_TZ_DATA),(struct sockaddr *)&target_ip_addr,sizeof(target_ip_addr),udp_send_style);
	comm_data_cnt.send_eth_signal = singal_num;
	comm_data_cnt.send_eth_all_cnt++;
//	set_udp_send_style();
}

#ifdef ORIGINAL_DATA_SAVE_ACC
void send_pw_orignal_data(int16_t *send_buf,uint16_t send_head,uint16_t singal_num,uint16_t target_addr,uint8_t resend_flag,uint8_t ack_flag)
#else
void send_pw_orignal_data(uint16_t *send_buf,uint16_t send_head,uint16_t singal_num,uint16_t target_addr,uint8_t resend_flag,uint8_t ack_flag)
#endif
{
//	struct UDP_SEND_STYLE udp_send_style;
//	struct sockaddr_in target_ip_addr;
////	uint8_t cnt_i = 0;
////	socklen_t target_addr_len;
//
//	if(ack_flag == 1)
//	{
//		ack_flag = 0x5a;
//	}
//	else
//	{
//		ack_flag = 0;
//	}
//
//	*(uint16_t *)pw_raw_data.send_data_head.head = little_to_big_16bit(send_head);
//	*(uint16_t *)pw_raw_data.send_data_head.len = little_to_big_16bit(sizeof(struct PW_RAW_DATA));                             //数据头24字节
//	pw_raw_data.send_data_head.company_id = LUHANG;            									         //板卡供应商编号 参考供应商定义
//	pw_raw_data.send_data_head.board_id = PW_BOARD;             								         //本身板卡编号 参考宏定义LOCAL_BOARD
//	*(uint16_t *)pw_raw_data.send_data_head.life_signal =  little_to_big_16bit(singal_num);       		  //生命信号，每秒加1
//	*(uint16_t *)pw_raw_data.send_data_head.target_board_group = little_to_big_16bit(target_addr); 		  //目标板卡的位集合
//	pw_raw_data.send_data_head.resend_flag = resend_flag;                                          //"0x55：表示首次发送该包数据，0xAA：表示重发该包数据；重发时的数据与首次发送的数据需全部一样，且仅对未应答的单板重发数据（通过Byte8-9来选型），超时时间为300ms，最多重发3次。"
//	pw_raw_data.send_data_head.ack_flag = ack_flag;                                                //"0x5A:目标板需要返回给请求板收到一包数据的应答帧   0x00:无需应答其它无效"
//	//pw_raw_data.send_data_head.packet_num = 0x07;                                                  //当前数据类型发送的总包数
//
//	memset(pw_raw_data.send_data_head.res,0,sizeof(pw_raw_data.send_data_head.res));
//
//#ifdef ORIGINAL_DATA_SAVE_ACC
//	memmove(pw_raw_data.ad_acc,send_buf,512*2);
//#else
//	memmove(pw_raw_data.ad,send_buf,512*2);
//#endif
//
//	*(uint16_t *)pw_raw_data.check_sum = little_to_big_16bit(check_sum((uint8_t *)&pw_raw_data,sizeof(struct PW_RAW_DATA)-2));
//
//	udp_send_style.ack_byte_of_buff = 10;
//	udp_send_style.resend_cnt = 3;
//	udp_send_style.is_need_ack_flag = ack_flag;
//	udp_send_style.is_ucast_flag = 0;				//采用单播或者组播方式发送
//	udp_send_style.time_out_ms = 300;
//
//
//	//target_ip_addr = udp_send_addr.send_target_sockaddr;
//	memmove(&target_ip_addr,&udp_send_addr.send_target_sockaddr,sizeof(struct sockaddr_in));
//
////	for(cnt_i = 0;cnt_i < 16;cnt_i++)
////	{
////		if(target_addr & (1<<cnt_i))
////		{
////			target_ip_addr = ucast_target_sockaddr[cnt_i];//
////			break;
////		}
////		cnt_i ++;
////	}
//	//target_ip_addr = ucast_target_sockaddr[target_addr << target_addr];									//得到ip地址
//
////	printf("send_pw_tz_data---send ucast---len:%d\n",sizeof(struct PW_TZ_DATA));
//	udp_send((uint8_t*)&pw_raw_data,sizeof(struct PW_RAW_DATA),(struct sockaddr *)&target_ip_addr,sizeof(target_ip_addr),udp_send_style);
}


/**
 * function     :set_send_life_sig
 * description  :设置确认信号
 * input        :send_addr-要发送的目标板
 *               send_head-数据头
 *               send_life-生命信号
 * output       :none
 * return       :none
 * others       :none
 */
void set_send_life_sig(uint16_t board_id, uint16_t send_addr_bits, uint16_t send_head, uint16_t send_life)
{
    ack_life_sig.is_need_ack_flag = 1;
    ack_life_sig.send_dev = board_id;
    ack_life_sig.send_addr_bits = send_addr_bits;
    ack_life_sig.send_head = send_head;
    ack_life_sig.send_life = send_life;
}

/**
 * function     :set_recv_life_sig
 * description  :设置接收到的生命信号
 * input        : src_dev_bit-接收到对方板卡的bit
 * 				     board_id-接收到对方板卡发送数据的
 * 				    recv_head-接收到的数据头
 * 				    recv_life-接收到的生命信号
 * output       :none
 * return       :none
 * others       :none
 */
void set_recv_life_sig(uint16_t board_id, uint16_t recv_addr_bit, uint16_t recv_head, uint16_t recv_life)
{
    ack_life_sig.recv_dev = board_id;
    ack_life_sig.recv_addr_bits |= recv_addr_bit;
    ack_life_sig.recv_head = recv_head;
    ack_life_sig.recv_life = recv_life;
}

void reset_recv_life_sig()
{
    ack_life_sig.recv_dev = 0;
    ack_life_sig.recv_addr_bits = 0;
    ack_life_sig.recv_head = 0;
    ack_life_sig.recv_life = 0;
}

/**
 * function     :check_ack_life_sig
 * description  :判断是否达到应答成功的条件
 * input        :none
 * output       :none
 * return       :none
 * others       :none
 */
int check_ack_life_sig(void)
{
//	DEBUG("send_life=0x%02x send_head=0x%02x send_dev=0x%02x send_addr_bits=0x%02x\n", ack_life_sig.send_life, ack_life_sig.send_head, ack_life_sig.send_dev, ack_life_sig.send_addr_bits);
//	DEBUG("recv_life=0x%02x recv_head=0x%02x recv_dev=0x%02x recv_addr_bits=0x%02x\n", ack_life_sig.recv_life, ack_life_sig.recv_head, ack_life_sig.recv_dev, ack_life_sig.recv_addr_bits);
    if (ack_life_sig.send_life == ack_life_sig.recv_life && ack_life_sig.send_head == ack_life_sig.recv_head &&  ack_life_sig.send_addr_bits == ack_life_sig.recv_addr_bits)
    {
        return UDP_ACK_OK;
    }
    else
    {
        return UDP_ACK_ERR;
    }
}

void clear_udp_send_addr()
{
	udp_send_addr.target_num = 0;
	udp_send_addr.board_set = 0;
}

/**
 * function     :set_udp_send_addr
 * description  :设置接收消息的板卡地址
 * input        :board_set- 板卡设置
 * 				:target_num - 要发送的板卡数量
 * output       :none
 * return       :none
 * others       :none
 */
void set_udp_send_addr(uint16_t board_set,uint16_t target_num)
{
	udp_send_addr.target_num = target_num;

	udp_send_addr.board_set |= udp_send_addr.target_board_addr[board_set];
	memmove(&udp_send_addr.send_target_sockaddr,&udp_send_addr.ucast_target_sockaddr[board_set],sizeof(struct sockaddr_in));

	 if(udp_send_addr.target_num > 1)
	 {
		 memmove(&udp_send_addr.send_target_sockaddr,&mcast_ctl_save_group_addr,sizeof(struct sockaddr_in));	//如果板卡数量不止一个，那就发送组播地址
	 }

//	 printf("send_addr_one:%s\n",inet_ntoa(udp_send_addr.send_target_sockaddr.sin_addr));
//	 printf("send_port:%d\n",ntohs(udp_send_addr.send_target_sockaddr.sin_port));
}

/**
 * function     :init_udp_send_addr
 * description  :初始化udp发送地址
 * input        :无
 *
 *
 * output       :none
 * return       :none
 * others       :none
 */
void init_udp_send_addr()
{
	udp_send_addr.board_set = 0;
	udp_send_addr.target_num = 0;
	for(uint8_t i = 0;i < 16;i++)
	{
		//udp_send_addr.ucast_target_sockaddr[i] = ucast_target_sockaddr[i];
		memmove(&udp_send_addr.ucast_target_sockaddr[i],&ucast_target_sockaddr[i],sizeof(struct sockaddr_in));
		udp_send_addr.target_board_addr[i] = 1<<i;
	}
}



/**
 * function     :init_target_board_addr
 * description  :初始化目标板卡地址（协议中的第8\9个字节）
 * input        :send_addr-要发送的目标板
 *               send_head-数据头
 *               send_life-生命信号
 * output       :none
 * return       :none
 * others       :none
 */


void init_target_board_addr()
{
	uint8_t cnt_i = 0;
	for(cnt_i = 0;cnt_i < 16;cnt_i ++)
	{
		target_board_addr[cnt_i] = 1<<cnt_i;
	}
}


void check_eth_status(uint16_t temp_addr_bits)
{
	uint8_t cnt_i = 0;
	if(temp_addr_bits == 0)
	{
		eth_status[BIT_CTRLA].connect_flag = 0;					//1故障;0正常
		eth_status[BIT_CTRLA].not_recv_cnt = 0;
		eth_status[BIT_CTRLB].connect_flag = 0;					//1故障;0正常
		eth_status[BIT_CTRLB].not_recv_cnt = 0;
		eth_status[BIT_SAVE].connect_flag = 0;					//1故障;0正常
		eth_status[BIT_SAVE].not_recv_cnt = 0;
		eth_status[BIT_TEMPA].connect_flag = 0;					//1故障;0正常
		eth_status[BIT_TEMPA].not_recv_cnt = 0;
		eth_status[BIT_TEMPB].connect_flag = 0;					//1故障;0正常
		eth_status[BIT_TEMPB].not_recv_cnt = 0;
		eth_status[BIT_ST].connect_flag = 0;					//1故障;0正常
		eth_status[BIT_ST].not_recv_cnt = 0;
		eth_status[BIT_AD].connect_flag = 0;					//1故障;0正常
		eth_status[BIT_AD].not_recv_cnt = 0;
		eth_status[BIT_VIBR].connect_flag = 0;					//1故障;0正常
		eth_status[BIT_VIBR].not_recv_cnt = 0;
		eth_status[BIT_IO].connect_flag = 0;					//1故障;0正常
		eth_status[BIT_IO].not_recv_cnt = 0;
		eth_status[BIT_GEAR].connect_flag = 0;					//1故障;0正常
		eth_status[BIT_GEAR].not_recv_cnt = 0;
		eth_status[BIT_MOTOR].connect_flag = 0;					//1故障;0正常
		eth_status[BIT_MOTOR].not_recv_cnt = 0;
		return;
	}
	for(cnt_i = 0 ;cnt_i < 9;cnt_i++)
	{
		//
		//printf("(1<<cnt_i):%d,temp_addr_bits\n",(1<<cnt_i),temp_addr_bits);
		if((temp_addr_bits & (1<<cnt_i)) != 0)
		 {
			//printf("temp_addr_bits:%d,cnt_i:%d\n",temp_addr_bits,cnt_i);
		    eth_status[cnt_i].not_recv_cnt++;
		    if(eth_status[cnt_i].not_recv_cnt > 4)
		    {
//		    	 printf("check_eth_status---eth_status[%d].connect_flag:%d\n",cnt_i,eth_status[cnt_i].connect_flag);
		    	 eth_status[cnt_i].connect_flag = 1;

		    	 //printf("eth_status[cnt_i]:%d\n",cnt_i);
		    }
		 }

	}
}


void check_can_status(uint16_t temp_addr_bits)
{
	uint8_t cnt_i = 0;
	if(temp_addr_bits == 0)
	{
		can_status[BIT_CTRLA].connect_flag = 0;					//1故障;0正常
		can_status[BIT_CTRLA].not_recv_cnt = 0;
		can_status[BIT_CTRLB].connect_flag = 0;					//1故障;0正常
		can_status[BIT_CTRLB].not_recv_cnt = 0;
		can_status[BIT_SAVE].connect_flag = 0;					//1故障;0正常
		can_status[BIT_SAVE].not_recv_cnt = 0;
		can_status[BIT_TEMPA].connect_flag = 0;					//1故障;0正常
		can_status[BIT_TEMPA].not_recv_cnt = 0;
		can_status[BIT_TEMPB].connect_flag = 0;					//1故障;0正常
		can_status[BIT_TEMPB].not_recv_cnt = 0;
		can_status[BIT_ST].connect_flag = 0;					//1故障;0正常
		can_status[BIT_ST].not_recv_cnt = 0;
		can_status[BIT_AD].connect_flag = 0;					//1故障;0正常
		can_status[BIT_AD].not_recv_cnt = 0;
		can_status[BIT_VIBR].connect_flag = 0;					//1故障;0正常
		can_status[BIT_VIBR].not_recv_cnt = 0;
		can_status[BIT_IO].connect_flag = 0;					//1故障;0正常
		can_status[BIT_IO].not_recv_cnt = 0;
		can_status[BIT_GEAR].connect_flag = 0;					//1故障;0正常
		can_status[BIT_GEAR].not_recv_cnt = 0;
		can_status[BIT_MOTOR].connect_flag = 0;					//1故障;0正常
		can_status[BIT_MOTOR].not_recv_cnt = 0;

		return;
	}
	for(cnt_i = 0 ;cnt_i < 9;cnt_i++)
	{
		//
		//printf("(1<<cnt_i):%d,temp_addr_bits\n",(1<<cnt_i),temp_addr_bits);
		if((temp_addr_bits & (1<<cnt_i)) != 0)
		 {
			//printf("temp_addr_bits:%d,cnt_i:%d\n",temp_addr_bits,cnt_i);
//		    eth_status[cnt_i].not_recv_cnt++;
//		    if(eth_status[cnt_i].not_recv_cnt > 4)
//		    {
//		    	 eth_status[cnt_i].connect_flag = 1;
//
//		    	 //printf("eth_status[cnt_i]:%d\n",cnt_i);
//		    }
			can_status[cnt_i].not_recv_cnt++;
			if(can_status[cnt_i].not_recv_cnt > 4)
			{
				 can_status[cnt_i].connect_flag = 1;

				 //printf("eth_status[cnt_i]:%d\n",cnt_i);
			}
		 }

	}
}

/**
 * 判断客户端
 * */
int judge_client(struct sockaddr_in client)
{
    if (client.sin_addr.s_addr == ucast_ctrla_board_addr.sin_addr.s_addr)
    {
    	eth_status[BIT_CTRLA].connect_flag = 0;					//1故障;0正常
    	eth_status[BIT_CTRLA].not_recv_cnt = 0;
        return BIT_CTRLA;
    }
    else if (client.sin_addr.s_addr == ucast_ctrlb_board_addr.sin_addr.s_addr)
    {
    	eth_status[BIT_CTRLB].connect_flag = 0;					//1故障;0正常
    	eth_status[BIT_CTRLB].not_recv_cnt = 0;
        return BIT_CTRLB;
    }
    else if (client.sin_addr.s_addr == ucast_save_board_addr.sin_addr.s_addr)
    {
    	eth_status[BIT_SAVE].connect_flag = 0;					//1故障;0正常
    	eth_status[BIT_SAVE].not_recv_cnt = 0;
        return BIT_SAVE;
    }
    else if (client.sin_addr.s_addr == ucast_axle_tempa_board_addr.sin_addr.s_addr)
    {
    	eth_status[BIT_TEMPA].connect_flag = 0;					//1故障;0正常
    	eth_status[BIT_TEMPA].not_recv_cnt = 0;
        return BIT_TEMPA;
    }
    else if (client.sin_addr.s_addr == ucast_axle_tempb_board_addr.sin_addr.s_addr)
    {
    	eth_status[BIT_TEMPB].connect_flag = 0;					//1故障;0正常
    	eth_status[BIT_TEMPB].not_recv_cnt = 0;
        return BIT_TEMPB;
    }
    else if (client.sin_addr.s_addr == ucast_st_board_addr.sin_addr.s_addr)
    {
    	eth_status[BIT_ST].connect_flag = 0;					//1故障;0正常
    	eth_status[BIT_ST].not_recv_cnt = 0;
        return BIT_ST;
    }
    else if (client.sin_addr.s_addr == ucast_ad_board_addr.sin_addr.s_addr)
    {
    	eth_status[BIT_AD].connect_flag = 0;					//1故障;0正常
    	eth_status[BIT_AD].not_recv_cnt = 0;
        return BIT_AD;
    }
    else if (client.sin_addr.s_addr == ucast_io_board_addr.sin_addr.s_addr)
    {
    	eth_status[BIT_IO].connect_flag = 0;					//1故障;0正常
    	eth_status[BIT_IO].not_recv_cnt = 0;
        return BIT_IO;
    }
    else
    {
        return -1;
    }
}

#ifdef AD7606_STOP_DATA_TIMER_TRIGGER
void comm_to_ctrl_board_check()
{
	struct PW_TZ_DATA pw_tz_data_temp;
	extern struct PW_TZ_DATA pw_tz_data;
	extern struct PW_AD_DATA pw_ad_data;
	extern struct RINGBUFF_PARA pw_diagnos_ringbuf_para;
//	extern struct UDP_SEND_ADDR udp_send_addr;

	clear_udp_send_addr();
	set_udp_send_addr(BIT_CTRLA,1);
	set_udp_send_addr(BIT_CTRLB,2);

	memmove(&pw_tz_data_temp, &pw_tz_data, sizeof(struct PW_TZ_DATA));

	pw_tz_data_temp.borad_err.bits.ctrla_eth_err = 0;
	pw_tz_data_temp.borad_err.bits.ctrla_can_err = 0;
	pw_tz_data_temp.borad_err.bits.ctrlb_eth_err = 0;
	pw_tz_data_temp.borad_err.bits.ctrlb_can_err = 0;

	//发送特征数据
	send_pw_tz_data(&pw_tz_data_temp,pw_ad_data.buff[pw_diagnos_ringbuf_para.index][0],udp_send_addr.board_set,0x55,0);

	send_pw_tz_data_can(&pw_tz_data_temp,pw_ad_data.buff[pw_diagnos_ringbuf_para.index][0],udp_send_addr.board_set,0x55,0);

	//更新接收端板卡
	clear_udp_send_addr();
}
#endif



void app_multicast_send(void *data, uint16_t size)
{
	sendto(multicast_socket, (char *)data, size, 0, (const struct sockaddr *)&mcast_ctl_save_group_addr, sizeof(mcast_ctl_save_group_addr));
}


void send_sw_tz_data(struct SW_TZ_DATA *send_tz_data,uint16_t singal_num,uint16_t target_addr,uint8_t resend_flag,uint8_t ack_flag)
{
//	uint16_t write_res = 0;
	struct UDP_SEND_STYLE udp_send_style;
	struct sockaddr_in target_ip_addr;
//	socklen_t target_addr_len;

	if(ack_flag == 1)
	{
		ack_flag = 0x5a;
	}
	else
	{
		ack_flag = 0;
	}

	*(uint16_t *)send_tz_data->data_head.head =little_to_big_16bit(0xAA50);
	*(uint16_t *)send_tz_data->data_head.len = little_to_big_16bit(sizeof(struct SW_TZ_DATA));
	send_tz_data->data_head.company_id = LUHANG;
	send_tz_data->data_head.board_id = PW_BOARD;
	*(uint16_t *)send_tz_data->data_head.life_signal = little_to_big_16bit(singal_num);

	*(uint16_t *)send_tz_data->data_head.target_board_group = little_to_big_16bit(target_addr);
//	DEBUG("target_addr:%x\n",target_addr);
	send_tz_data->data_head.resend_flag = resend_flag;
	send_tz_data->data_head.ack_flag = ack_flag;

#ifdef TWO_POWER_TO_BOARD_ERR
//	if(send_tz_data->borad_err.bits.power1_err==1 && send_tz_data->borad_err.bits.power2_err==1)
//		send_tz_data->borad_err.bits.sw_board_err=1;
#endif

	*(uint16_t *)send_tz_data->check_sum = little_to_big_16bit(check_sum((uint8_t *)send_tz_data,sizeof(struct SW_TZ_DATA)-2));


	udp_send_style.ack_byte_of_buff = 10;
	udp_send_style.resend_cnt = 3;
	udp_send_style.is_need_ack_flag = ack_flag;
	udp_send_style.is_ucast_flag = 0;				//采用单播或者组播方式发送
	udp_send_style.time_out_ms = 300;

	//target_ip_addr = udp_send_addr.send_target_sockaddr;

	memmove(&target_ip_addr,&udp_send_addr.send_target_sockaddr,sizeof(struct sockaddr_in));

//	udp_send_style.ack_byte_of_buff = 10;
//	udp_send_style.is_need_ack_flag = ack_flag;
//	udp_send_style.is_ucast_flag = 0;
//	udp_send_style.resend_cnt = 3;
//	udp_send_style.time_out_ms = 300;
	//过程数据发送组播地址
	//get_target_ip_addr();
	//int udp_send(uint8_t *buff, uint16_t buff_size, struct sockaddr *__addr, socklen_t __addr_len, const struct UDP_SEND_STYLE style)

//	printf("send_sw_tz_data---len:%d\n",sizeof(struct SW_TZ_DATA));
	udp_send((uint8_t *)send_tz_data,sizeof(struct SW_TZ_DATA),(struct sockaddr *)&target_ip_addr,sizeof(target_ip_addr),udp_send_style);

	comm_data_cnt.send_eth_all_cnt++;
	comm_data_cnt.send_eth_signal = singal_num;
	//DEBUG("send_eth->signal,cnt:%d,%d\n",comm_data_cnt.send_eth_signal,comm_data_cnt.send_eth_all_cnt);


//	set_udp_send_style();
}

void send_sw_orignal_data(uint16_t *send_buf,uint16_t send_head,uint16_t singal_num,uint16_t target_addr,uint8_t resend_flag,uint8_t ack_flag)
{
//	struct UDP_SEND_STYLE udp_send_style;
//	struct sockaddr_in target_ip_addr;
////	uint8_t cnt_i = 0;
////	socklen_t target_addr_len;
//
//	if(ack_flag == 1)
//	{
//		ack_flag = 0x5a;
//	}
//	else
//	{
//		ack_flag = 0;
//	}
//
//	*(uint16_t *)sw_raw_data.send_data_head.head = little_to_big_16bit(send_head);
//	*(uint16_t *)sw_raw_data.send_data_head.len = little_to_big_16bit(sizeof(struct SW_RAW_DATA));                      //数据头24字节
//	sw_raw_data.send_data_head.company_id = LUHANG;            									  //板卡供应商编号 参考供应商定义
//	sw_raw_data.send_data_head.board_id = PW_BOARD;             								  //本身板卡编号 参考宏定义LOCAL_BOARD
//	*(uint16_t *)sw_raw_data.send_data_head.life_signal =  little_to_big_16bit(singal_num);       //生命信号，每秒加1
//	*(uint16_t *)sw_raw_data.send_data_head.target_board_group = little_to_big_16bit(target_addr);		  //目标板卡的位集合
//	sw_raw_data.send_data_head.resend_flag = resend_flag;                                          //"0x55：表示首次发送该包数据，0xAA：表示重发该包数据；重发时的数据与首次发送的数据需全部一样，且仅对未应答的单板重发数据（通过Byte8-9来选型），超时时间为300ms，最多重发3次。"
//	sw_raw_data.send_data_head.ack_flag = ack_flag;                                                //"0x5A:目标板需要返回给请求板收到一包数据的应答帧   0x00:无需应答其它无效"
//	sw_raw_data.send_data_head.packet_num = 0x05;                                                  //当前数据类型发送的总包数
//	memset(sw_raw_data.send_data_head.res,0,sizeof(sw_raw_data.send_data_head.res));
//	sw_raw_data.send_data_head.res[0] = sw_data_save_type;
//	memmove(sw_raw_data.ad,send_buf,512*2);
//	*(uint16_t *)sw_raw_data.check_sum = little_to_big_16bit(check_sum((uint8_t *)&sw_raw_data,sizeof(struct SW_RAW_DATA)-2));
//
//	udp_send_style.ack_byte_of_buff = 10;
//	udp_send_style.resend_cnt = 3;
//	udp_send_style.is_need_ack_flag = ack_flag;
//	udp_send_style.is_ucast_flag = 0;				//采用单播或者组播方式发送
//	udp_send_style.time_out_ms = 300;
//
//
//	target_ip_addr = udp_send_addr.send_target_sockaddr;
//	memmove(&target_ip_addr,&udp_send_addr.send_target_sockaddr,sizeof(struct sockaddr_in));
//
//
//	//printf("send data head len:%d,send data len:%d\n",sizeof( struct SEND_DATA_HEAD),sizeof(struct SW_RAW_DATA));
//
////	for(cnt_i = 0;cnt_i < 16;cnt_i++)
////	{
////		if(target_addr & (1<<cnt_i))
////		{
////			target_ip_addr = ucast_target_sockaddr[cnt_i];//
////			break;
////		}
////		cnt_i ++;
////	}
//	//target_ip_addr = ucast_target_sockaddr[target_addr << target_addr];									//得到ip地址
//
//	udp_send((uint8_t *)&sw_raw_data,sizeof(struct SW_RAW_DATA),(struct sockaddr *)&target_ip_addr,sizeof(target_ip_addr),udp_send_style);
//	//comm_data_cnt.send_eth_all_cnt++;
}

