#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
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
#include "ptu_app.h"
#include "update.h"
#include "user_data.h"
#include "pthread_policy.h"

#define PRO_HEAD_LEN  12


//struct MASTER_CLB_CONFIG_PARA train8_config_paras;

struct PTU_DATA ptu_data;
struct NET_UPDATE_DATA net_update_data;
struct PTU_MONITOR_DATA  ptu_monitor_data;
extern struct PW_TZ_DATA pw_tz_data;
//extern struct _SOFT_VERSION pw_soft_version;
struct PTU_SIMULATION_DATA ptu_simulation_data;
extern struct PW_CLB_CONFIG_PARA *pw_clb_config_para;
extern struct STORE_FLAG store_flag;
extern struct TRAIN_SOFT_VERSION train_soft_version;  //系统软件版本
extern struct PW_SIMULATION pw_simulation;

#ifdef WTD_DATA_TRANSLATE_PROTOCOL
	extern struct sockaddr_in save_ip_sockaddr;
	extern struct sockaddr_in tz_wtd_ip_sockaddr;
	extern struct sockaddr_in alarm_wtd_ip_sockaddr;
	extern int send_wtd_fd_socket;

#ifdef WTD_SIMULATION_ALARM_TRIG
	extern uint8_t simulation_open_flag;
	extern uint8_t simulation_status[CHANNEL_NUM];
	void send_wtd_simulation_test(struct PW_SIMULATION *simulation_data);

	extern void reset_wtd_trig_alarm_ctrl();
#endif
#endif

extern void get_para_from_local();

/*************************************************
Function:  app_iap_check
Description: 应用程序升级检测
Input:  无
Output:
Return: 无
Others:
*************************************************/
void app_iap_check(void )
{
//	int ret = -1;
//	ret = access("/media/local_config/JXDS_PW", F_OK);
//	if(ret==0)
//	{
//		DEBUG("check iap update pw clb app!\r\n");
//		system("cp /media/local_config/JXDS_PW /media/local_config/app/");
//		system("rm /media/local_config/JXDS_PW");
//		sleep(1);
//    	close(ptu_data.ptu_net_fd);
//    	system("reboot -nf");
//	}
}

/*************************************************
Function:    ptu_send
Description: ptu发送函数
Input:　发送目标:taget
	   命令类型:cmd
	   发送数据:data
	   数据长度:len
Output:
Return:
Others:
*************************************************/
void ptu_send(uint8_t taget, uint8_t cmd, uint8_t *data, uint16_t len)
{
//	uint8_t ptu_send_data[1024];
//	uint8_t crc=0;
//	uint16_t i=0;
//    ptu_send_data[0] = 0x55;
//	ptu_send_data[1] = 0xAA;
//	ptu_send_data[2] = len+PRO_HEAD_LEN;
//	ptu_send_data[3] = (uint8_t)((len+PRO_HEAD_LEN) >> 8);
//	ptu_send_data[4] = cmd;   //命令类型
//	ptu_send_data[5] = taget;  //目标ID
//	ptu_send_data[6] = BOARD_NUMB;  //源ID
//	ptu_send_data[7] = 0x00;
//	ptu_send_data[8] = 0x00;
//	ptu_send_data[9] = 0x00;
//	ptu_send_data[10] = 0x00;
//	for(i=0; i<len; i++)
//	{
//		ptu_send_data[11+i] = data[i];
//	}
//	crc=0;
//	for(i=0; i<PRO_HEAD_LEN+len-1; i++)
//	{
//		crc += ptu_send_data[i];
//	}
//	ptu_send_data[PRO_HEAD_LEN+len-1] = crc;
//
////	for(i=0;i<38;i++)
////	{
////		printf("%x ",ptu_send_data[11+i]);
////		if(i==25)
////			printf("\n");
////	}
////	printf("\n");
//
//	if(send(ptu_data.ptu_net_fd,ptu_send_data,PRO_HEAD_LEN+len,0) != -1)
//	{
////		DEBUG("ptu net data send is ok!\r\n");
//	}
//	else
//	{
//		close(ptu_data.ptu_net_fd);
////		DEBUG("ptu net data send is err!\r\n");
//	}

}





/*************************************************
Function:    send_monitor_data
Description: 发送监控数据给PTU
Input:　设备地址:dev_num
Output:
Return:
Others:
*************************************************/
void send_monitor_data(uint8_t dev_num)
{
//	uint8_t buf[2048];
//	buf[0] = dev_num;
//
//	printf("send_monitor_data---ptu_monitor_data---ethA_err:%d,ethB_err:%d\n",ptu_monitor_data.monitor_pw_tz_data.borad_err.bits.ctrla_eth_err,
//			ptu_monitor_data.monitor_pw_tz_data.borad_err.bits.ctrlb_eth_err);
//
//	memmove(&buf[1],&ptu_monitor_data,sizeof(struct PTU_MONITOR_DATA));
//	ptu_send(0xff,STATUS_MONITOR_CMD,buf,sizeof(struct PTU_MONITOR_DATA) + 1);
}




/*************************************************
Function:    config_ok_ack
Description: ptu配置成功返回
Input:　设备地址:dev_num
	   成功标识:flag:1成功，０失败
Output:
Return:
Others:
*************************************************/
void config_ok_ack(uint8_t dev_num,uint8_t flag)
{
//	uint8_t buf[2];
//	buf[0] = dev_num;
//	buf[1] = flag;
//	ptu_send(0xff,CONFIG_PARA_OK_CMD,buf,2);
}




/*************************************************
Function:    start_download_ack
Description: 开始下载确认
Input:
Output:
Return:
Others:
*************************************************/
void start_download_ack(void)
{
//	uint8_t buf[1];
//	buf[0] = 0;
//	ptu_send(0xff,START_DOWNLOAD_CMD,buf,1);
}




/*************************************************
Function:    restart_ftp_ok_ack
Description: 重启FTP成功返回
Input:
Output:
Return:
Others:
*************************************************/
void restart_ftp_ok_ack()
{
//	uint8_t buf[1];
//	buf[0] = 0xff;
//	ptu_send(0xff,SYSTEM_REBOOT_CMD,buf,1);
}


/*************************************************
Function:    update_app_ok_ack
Description: ptu配置成功返回
Input:　设备地址:dev_num
Output:
Return:
Others:
*************************************************/
void update_app_ok_ack(uint8_t update_res)
{
//	uint8_t buf[1];
//	buf[0] = update_res;
//	ptu_send(0xff,SEND_APP_FILE_CMD,buf,1);
}


/*************************************************
Function:    send_version_to_ptu
Description: 发送软件版本给PTU
Input:
Output:
Return:
Others:
*************************************************/

void send_version_to_ptu(void)
{
//	uint8_t buf[512];
//	train_soft_version.version=SOFT_VERSION_VAL;
//	train_soft_version.update_time=SOFT_UPDATE_TIME;
//	train_soft_version.small_version=SMALL_VERSION;
//	memmove(buf,&train_soft_version,sizeof(struct TRAIN_SOFT_VERSION));
//	ptu_send(0xff,PTU_GET_VERSION_CMD,buf,sizeof(struct TRAIN_SOFT_VERSION));

}


/*************************************************
Function:    send_heart_to_ptu
Description: 发送设备在线状态给PTU
Input:
Output:
Return:
Others:
*************************************************/
void send_heart_to_ptu(void)
{
//	uint8_t buf[8];
//	memmove(buf,&train_net_flag,sizeof(struct TRAIN_NET_FLAG));
//	ptu_send(0xff,HEART_COUNT_CMD,buf,sizeof(struct TRAIN_NET_FLAG));
}


/*************************************************
Function:    send_paras_to_ptu
Description: 发送参数给PTU
Input:　设备地址:dev_num
Output:
Return:
Others:
*************************************************/
void send_paras_to_ptu(uint8_t dev_num)
{
//	uint8_t buf[300];
//	switch(dev_num)
//	{
//	case 1:
//		buf[0] = 0x01;
//		memmove(&buf[1],master_config_paras,sizeof(struct MASTER_CLB_CONFIG_PARA));
//		ptu_send(0xff,CONFIG_PARA_READ_CMD,buf,sizeof(struct MASTER_CLB_CONFIG_PARA)+1);
//		break;
//	case 2:
//		buf[0] = 0x02;
//		memmove(&buf[1],slave_config_paras,sizeof(struct SLAVE_CLB_CONFIG_PARA));
//		ptu_send(0xff,CONFIG_PARA_READ_CMD,buf,sizeof(struct SLAVE_CLB_CONFIG_PARA) + 1);
//		break;
//	case 3:
//		buf[0] = 0x03;
//		memmove(&buf[1],slave_config_paras,sizeof(struct SLAVE_CLB_CONFIG_PARA));
//		ptu_send(0xff,CONFIG_PARA_READ_CMD,buf,sizeof(struct SLAVE_CLB_CONFIG_PARA) + 1);
//		break;
//	case 4:
//		buf[0] = 0x04;
//		memmove(&buf[1],slave_config_paras,sizeof(struct SLAVE_CLB_CONFIG_PARA));
//		ptu_send(0xff,CONFIG_PARA_READ_CMD,buf,sizeof(struct SLAVE_CLB_CONFIG_PARA) + 1);
//		break;
//	case 5:
//		buf[0] = 0x05;
//		memmove(&buf[1],slave_config_paras,sizeof(struct SLAVE_CLB_CONFIG_PARA));
//		ptu_send(0xff,CONFIG_PARA_READ_CMD,buf,sizeof(struct SLAVE_CLB_CONFIG_PARA) + 1);
//		break;
//	case 6:
//		buf[0] = 0x06;
//		memmove(&buf[1],slave_config_paras,sizeof(struct SLAVE_CLB_CONFIG_PARA));
//		ptu_send(0xff,CONFIG_PARA_READ_CMD,buf,sizeof(struct SLAVE_CLB_CONFIG_PARA) + 1);
//		break;
//	case 7:
//		buf[0] = 0x07;
//		memmove(&buf[1],slave_config_paras,sizeof(struct SLAVE_CLB_CONFIG_PARA));
//		ptu_send(0xff,CONFIG_PARA_READ_CMD,buf,sizeof(struct SLAVE_CLB_CONFIG_PARA) + 1);
//		break;
//	case 8:
//		buf[0] = 0x08;
//		memmove(&buf[1],&train8_config_paras,sizeof(struct MASTER_CLB_CONFIG_PARA));
//		ptu_send(0xff,CONFIG_PARA_READ_CMD,buf,sizeof(struct MASTER_CLB_CONFIG_PARA) + 1);
//		break;
//	}
}

#ifdef WTD_DATA_TRANSLATE_PROTOCOL
#ifdef WTD_SIMULATION_ALARM_TRIG
void send_wtd_simulation_test(struct PW_SIMULATION *simulation_data)
{
	uint32_t data = *(uint32_t*)simulation_data;
	uint8_t i=0;
	uint8_t ch_num=0;

	if(data&(0x1<<8))//1位端加速度传感器状态
	{
		simulation_open_flag = 1;
	}
	else
	{
		simulation_open_flag = 0;
	}

//	printf("simulation_data y=%d, z=%d\n", simulation_data->one_warning_y, simulation_data->one_warning_z);
	for(i=9; i<=10; i++)//1位端横\垂向平稳性报警状态
	{
		if(i==10)//横
			ch_num = 0;
		else if(i==9)//垂
			ch_num = 1;

		if(data&(0x1<<i))
		{
			if(simulation_status[ch_num] == TRIG_OK)
			{
				simulation_status[ch_num] = TRIG_ALARM;
			}
		}
		else
		{
			if(simulation_status[ch_num] == TRIG_ALARM)
			{
				simulation_status[ch_num] = TRIG_ALARM_REMOVE;
			}
		}
		printf("simulation_status[%d] = %d\n", ch_num, simulation_status[ch_num]);
	}
}
#endif
#endif

#if 0
/*************************************************
Function:    ptu_data_deal
Description: ptu数据解析
Input:　被解析数据:data
	   数据长度:len
Output:
Return:
Others:
*************************************************/
void ptu_data_deal(uint8_t *data,uint32_t len)
{
//	uint32_t cnt_i  = 0;
//	printf("precv form pw ptu:%d\n",len);
//	for(cnt_i = 0;cnt_i < len;cnt_i ++)
//	{
//		printf("%x,",data[cnt_i]);
//	}
//	printf("\n");
	uint32_t j = 0;
//	int ret = 0;//config_fd = -1,
	uint8_t reply_buf[3] = {0};
	//uint8_t buf[512] = {0};

	if (*(uint16_t*)(&data[0])==0xAA55 && *(uint16_t*)(&data[2])==len)			//发给平稳板的才解析
	{
	   uint8_t crc=0;
	   for (j=0; j<len-1; j++)
	   {
		   crc += data[j];
	   }
	   if (crc == data[len-1])
	   {
		   switch(data[4])
		   {
			    case SEND_SYSTEM_FILE_CMD:  //升级系统镜像和设备树文件
			        break;

			    case SEND_APP_FILE_CMD: //升级应用程序
			    	DEBUG("update pw clb app!\r\n");
			    	system("cp /media/local_config/JXDS_PW /media/local_config/app/");
#ifdef AD_RESET
			    	if(access("/media/local_config/ad7606.ko", F_OK) == 0)
			    	{
						system("cp /media/local_config/ad7606.ko /lib/modules/");
						DEBUG("update ad driver\n");
			    	}
#endif
			    	update_app_ok_ack(1);
			    	sleep(1);
			    	close(ptu_data.ptu_net_fd);
			    	system("reboot -nf");
			    	break;

			    case SYSTEM_REBOOT_CMD:
			    	close(ptu_data.ptu_net_fd);
			    	sleep(1);
			    	system("reboot -nf");
			    	break;
			    case CONFIG_PARA_CMD:
			    	DEBUG("recv config_para_cmd\n");
			    	reply_buf[0] = read_para_from_config();

			    	ptu_send(0xff,CONFIG_PARA_CMD,reply_buf,sizeof(reply_buf[0]));
			    	sleep(1);
			    	close(ptu_data.ptu_net_fd);
			    	//sleep(1);
			    	system("reboot -nf");
			    	//PW_CLB_CONFIG_PARA
			    	break;

			    case CONFIG_PARA_READ_CMD:

			    	get_para_from_local();
			    	ptu_send(0xff,CONFIG_PARA_READ_CMD,(uint8_t*)pw_clb_config_para,sizeof(struct PW_CLB_CONFIG_PARA));

			    	break;
			    case STATUS_MONITOR_CMD:
			    	memmove(&ptu_monitor_data.monitor_pw_tz_data,&pw_tz_data,sizeof(struct PW_TZ_DATA));
			    	ptu_send(0xff,STATUS_MONITOR_CMD,(uint8_t*)&ptu_monitor_data.monitor_pw_tz_data,sizeof(struct PW_TZ_DATA));

			    	break;
			    case START_DOWNLOAD_CMD:
			    	store_flag.hdd_save_flag = FALSE; //PTU开始下载数据，关闭存储功能

			    	DEBUG("ptu start download!\r\n");
			    	break;
			    case DOWNLOAD_OK_CMD:
			    	store_flag.hdd_save_flag = TRUE;  //PTU数据下载完成，关闭存储功能

			    	DEBUG("download_ok_cmd\n");
			    	break;
			    case PTU_GET_VERSION_CMD:
			    	send_version_to_ptu();
			    	break;

			    case PTU_START_SIMULATE:
			    	printf("PTU_START_SIMULATE...\n");
			    	ptu_data.ptu_simulation_flag = 1;

			    	memmove((uint8_t *)&pw_simulation,data+11,sizeof(struct PW_SIMULATION));
//			    	DEBUG("PTU_SIMULATION_CMD\n");

			    	//memmove((uint8_t *)&ptu_simulation_data.simulation_pw_tz_data,data+11,sizeof(struct PW_TZ_DATA));

//			    	for(cnt_i = 0;cnt_i < sizeof(struct PW_TZ_DATA);cnt_i++)
//			    	{
//			    		DEBUG("%x,",*((uint8_t *)&ptu_simulation_data.simulation_pw_tz_data+cnt_i));
//			    		if((cnt_i+1)%8 == 0)
//			    			DEBUG("\n");
//			    		//DEBUG("%x,",*(uint8_t *)(data+11+cnt_i));
//			    	}
//
//			    	DEBUG("\n");
			    	//b_eth_recv_signal_1
#ifdef WTD_DATA_TRANSLATE_PROTOCOL
#ifdef WTD_SIMULATION_ALARM_TRIG
			    	send_wtd_simulation_test(&pw_simulation);
#endif
#endif

			    	break;

			    case PTU_STOP_SIMULATE:
			    	printf("PTU_STOP_SIMULATE...\n");
			    	ptu_data.ptu_simulation_flag = 0;

#ifdef WTD_DATA_TRANSLATE_PROTOCOL
#ifdef WTD_SIMULATION_ALARM_TRIG
			    	reset_wtd_trig_alarm_ctrl();
#endif
#endif
			    	break;
		   }
	   }
	}
}


#endif
/*************************************************
Function:    ptu_recv_thread_entry
Description: ptu接收线程
Input:
Output:
Return:
Others:
*************************************************/
void ptu_recv_thread_entry()
{
//	uint8_t recv_buf[1024];
//	uint8_t date_deal_buf[1024];
//	char local_ptu_ip[16];//"255.255.255.255"
//	char local_ptu_gwaddr[16];
//	char local_ptu_mask[16];
//	char str[100];
//	int32_t ret = 1;
//	uint16_t cnt_i = 0;
////	int i;
//	int ptu_sockfd = -1;
//    int addr_client_len;
//    int on = 1;
//    struct sockaddr_in addr_server, addr_client;

#ifdef RT_LINUX
    pthread_attr_t attr;       //线程属性  pthread_attr_t 为线程属性结构体
    struct sched_param sched;  //调度策略
    DEBUG ("set SCHED_RR policy\n");
    api_set_thread_policy(&attr, SCHED_RR);
#endif


//	sprintf(local_ptu_ip, "%d.%d.%d.%d",
//			pw_clb_config_para->ptu_net_para.self_ip[0],
//			pw_clb_config_para->ptu_net_para.self_ip[1],
//			pw_clb_config_para->ptu_net_para.self_ip[2],
//			pw_clb_config_para->ptu_net_para.self_ip[3]);
//
//	sprintf(local_ptu_gwaddr, "%d.%d.%d.%d",
//			pw_clb_config_para->ptu_net_para.self_gwaddr[0],
//			pw_clb_config_para->ptu_net_para.self_gwaddr[1],
//			pw_clb_config_para->ptu_net_para.self_gwaddr[2],
//			pw_clb_config_para->ptu_net_para.self_gwaddr[3]);
//
//	sprintf(local_ptu_mask, "%d.%d.%d.%d",
//			pw_clb_config_para->ptu_net_para.self_maskaddr[0],
//			pw_clb_config_para->ptu_net_para.self_maskaddr[1],
//			pw_clb_config_para->ptu_net_para.self_maskaddr[2],
//			pw_clb_config_para->ptu_net_para.self_maskaddr[3]);
//
//    //配置IP和掩码
//    sprintf(str,"%s %s %s %s","ifconfig eth1",local_ptu_ip,"netmask", local_ptu_mask);
//    system(str);
//    printf("ptu_ip_netmask:%s\n", str);
#ifdef WTD_DATA_TRANSLATE_PROTOCOL
#ifdef WTD_DNS_GW_TEST
//    uint8_t dns_ip[4]={0};
//    uint8_t gw_ip[4]={0};
//
//    dns_ip[0] = pw_clb_config_para->ptu_net_para.self_ip[0];
//    dns_ip[1] = pw_clb_config_para->ptu_net_para.self_ip[1];
//    gw_ip[0] = pw_clb_config_para->ptu_net_para.self_ip[0];
//    gw_ip[1] = pw_clb_config_para->ptu_net_para.self_ip[1];

    //DNS
    if(pw_clb_config_para->ptu_net_para.self_ip[0] == 192 && pw_clb_config_para->ptu_net_para.self_ip[1] == 168)
    {
    	sprintf(str0,"%s %d%s%d%s %s", "echo nameserver", pw_clb_config_para->ptu_net_para.self_ip[0], ".", pw_clb_config_para->ptu_net_para.self_ip[1], ".3.1", ">> /etc/resolv.conf");
    	//sprintf(str0,"%s %d%s %s", "echo nameserver", pw_clb_config_para->ptu_net_para.self_ip[0], ".0.0.1", ">> /etc/resolv.conf");
    }
    else
    {
    	sprintf(str0,"%s %d%s%d%s %s", "echo nameserver", pw_clb_config_para->ptu_net_para.self_ip[0], ".0.", pw_clb_config_para->ptu_net_para.self_ip[2], ".1", ">> /etc/resolv.conf");
//    	sprintf(str0,"%s %d%s %s", "echo nameserver", pw_clb_config_para->ptu_net_para.self_ip[0], ".0.0.1", ">> /etc/resolv.conf");
    }
    system(str0);
    printf("ptu_ip_dns:%s\n", str0);
    //gw   //udhcpc自动获取网关 并设置网关．
    //sprintf(str1,"%s %s", "route add default gw", local_ptu_gwaddr);
    if(pw_clb_config_para->ptu_net_para.self_ip[0] == 192 && pw_clb_config_para->ptu_net_para.self_ip[1] == 168)
    {
    	sprintf(str1,"%s %d%s%d%s", "route add default gw", pw_clb_config_para->ptu_net_para.self_ip[0], ".", pw_clb_config_para->ptu_net_para.self_ip[1], ".3.1");
//    	sprintf(str1,"%s %d%s", "route add default gw", pw_clb_config_para->ptu_net_para.self_ip[0], ".0.0.1");
    }
    else
    {
    	sprintf(str1,"%s %d%s", "route add default gw", pw_clb_config_para->ptu_net_para.self_ip[0], ".0.0.1");
    }
    system(str1);
    printf("ptu_ip_gw:%s\n", str1);
    system("cat /etc/resolv.conf");
    system("route -n");
#else
    //DNS
//    sprintf(str0,"%s %d%s%d%s %s", "echo nameserver", pw_clb_config_para->ptu_net_para.self_ip[0], ".0.", pw_clb_config_para->ptu_net_para.self_ip[2], ".1", ">> /etc/resolv.conf");
//    system(str0);
//    memset(str0, 0, sizeof(str0));
//    sprintf(str0,"%s", "echo nameserver 10.0.0.1 >> /etc/resolv.conf");
//    system(str0);
    system("echo DNS=10.0.0.1 > /etc/systemd/resolved.conf");
    //gw
//    sprintf(str1,"%s %d%s%d%s", "route add default gw", pw_clb_config_para->ptu_net_para.self_ip[0], ".0.", pw_clb_config_para->ptu_net_para.self_ip[2], ".1");
//    system(str1);
//    memset(str1, 0, sizeof(str1));
//    sprintf(str1,"%s", "route add default gw 10.0.0.1");
//    system(str1);
    system("route add default gw 10.0.0.1");
#endif

	/*绑定save IP*/
    memset(&save_ip_sockaddr, 0, sizeof(save_ip_sockaddr));
    save_ip_sockaddr.sin_family = AF_INET;
    save_ip_sockaddr.sin_port = htons(pw_clb_config_para->save_board_addr0.ucast_port);
    save_ip_sockaddr.sin_addr.s_addr = inet_addr(str_save_ip);

	/*绑定tz wtd IP*/
    memset(&tz_wtd_ip_sockaddr, 0, sizeof(tz_wtd_ip_sockaddr));
    tz_wtd_ip_sockaddr.sin_family = AF_INET;
    tz_wtd_ip_sockaddr.sin_port = htons(pw_clb_config_para->wtd_net_para.wtd_tz_port);
    tz_wtd_ip_sockaddr.sin_addr.s_addr = inet_addr(str_wtd_ip);

	printf("send_tz_wtd_IPaddr:before:%s after:%s\n", str_wtd_ip, inet_ntoa(tz_wtd_ip_sockaddr.sin_addr));
	printf("send_tz_wtd_port:%d\n",ntohs(tz_wtd_ip_sockaddr.sin_port));

    /*绑定alarm wtd IP*/
    memset(&alarm_wtd_ip_sockaddr, 0, sizeof(alarm_wtd_ip_sockaddr));
    alarm_wtd_ip_sockaddr.sin_family = AF_INET;
    alarm_wtd_ip_sockaddr.sin_port = htons(pw_clb_config_para->wtd_net_para.wtd_alarm_port);
    alarm_wtd_ip_sockaddr.sin_addr.s_addr = inet_addr(str_wtd_ip);

	printf("send_alarm_wtd_IPaddr:before:%s after:%s\n", str_wtd_ip, inet_ntoa(alarm_wtd_ip_sockaddr.sin_addr));
	printf("send_alarm_wtd_port:%d\n",ntohs(alarm_wtd_ip_sockaddr.sin_port));

    /*建立发送WTD套接字*/
    send_wtd_fd_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (send_wtd_fd_socket == -1)
    {
        perror("send_wtd_fd_socket create err!");
    }

    /**socket复用 */
    int reuse = 1; //必须赋值为非零常数复用才有效
    if (setsockopt(send_wtd_fd_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("reusing socket failed");
    }

    /*绑定本地IP*/
    bzero(&self_ip_sockaddr, sizeof(self_ip_sockaddr));
    self_ip_sockaddr.sin_family = AF_INET;
    self_ip_sockaddr.sin_port = htons(pw_clb_config_para->wtd_net_para.self_port);
    self_ip_sockaddr.sin_addr.s_addr = inet_addr(local_ptu_ip);//用PTU的IP绑新端口

	printf("send_ptu_IPaddr:before:%s after:%s\n", local_ptu_ip, inet_ntoa(self_ip_sockaddr.sin_addr));
	printf("send_ptu_port:%d\n",ntohs(self_ip_sockaddr.sin_port));

    /**绑定本地地址 */
    int err = bind(send_wtd_fd_socket, (struct sockaddr *)&self_ip_sockaddr, sizeof(struct sockaddr_in));
    if (err != 0)
    {
        perror("bind faild\n");
    }

    /*发送确认机制初始化*/
//    memset(&ack_life_sig, sizeof(ack_life_sig), 0);

    /**发送锁初始化 */
//    pthread_mutex_init(&udp_send_mutex, NULL);
//    pthread_cond_init(&udp_send_cond, NULL);

//    /**单播接收线程 **/
//    pthread_t udp_recv_wtd_ucast_thread;
//    int ret1 = pthread_create(&udp_recv_wtd_ucast_thread, NULL, (void *)&udp_wtd_ucast_recv, (void *)send_fd_socket);
//    if (ret1 != 0)
//    {
//        perror("pthread_create");
//    }
//
//    udp_send(&pw_raw_data,sizeof(struct PW_RAW_DATA),(struct sockaddr *)&target_ip_addr,sizeof(target_ip_addr),udp_send_style);
//    ptu_send(0xff,STATUS_MONITOR_CMD,&ptu_monitor_data.monitor_pw_tz_data,sizeof(struct PW_TZ_DATA));
#endif
#if 0
    addr_client_len = sizeof(struct sockaddr);
    if((ptu_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)				//流式 socket
    {
        perror("ptu_sockfd cteate err!");
        //exit(1);
    }
    DEBUG("sock successful\n");

    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(pw_clb_config_para->ptu_net_para.net_port);
    addr_server.sin_addr.s_addr = inet_addr(local_ptu_ip);

    setsockopt(ptu_sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)); //设置复用

    if(bind(ptu_sockfd, (struct sockaddr *)&addr_server, sizeof(struct sockaddr_in)) < 0)
    {
        perror("bind");
        //exit(1);
    }
    DEBUG("bind sucessful\n");

    if(listen(ptu_sockfd, 5))
    {
        perror("listen");
        //exit(1);
    }
    DEBUG("listen sucessful\n");

	reset_connect:
	if((ptu_data.ptu_net_fd = accept(ptu_sockfd, (struct sockaddr *)&addr_client, (socklen_t *)&addr_client_len)) < 0)		//表示3次握手成功
	{
		perror("accept");
		//exit(1);
	}
	printf("pw_accept client ip: %s\n", inet_ntoa(addr_client.sin_addr));




//	while(1)
//	{

	    while(1)
	    {


	    	ret = recv(ptu_data.ptu_net_fd, recv_buf, sizeof(recv_buf), 0);// 接收数据
	    	 if(ret > 0)
	    	{
//	    		 printf("recv data from ptu ret:%d\n",ret);
	    	  for(cnt_i = 0;cnt_i < ret;)
	    	  {
	    	    memmove(date_deal_buf,recv_buf,*(uint16_t *)(recv_buf+cnt_i+2));
	    	    ptu_data_deal(date_deal_buf,*(uint16_t *)(date_deal_buf+2));
	    	    cnt_i = cnt_i + *(uint16_t *)(date_deal_buf+2);
	    	  }

	    	}
	 		else if (ret <= 0)
	 		{
	 			close(ptu_data.ptu_net_fd);
//	 			printf("ret3 = %d \r\n",ret);
	 			goto reset_connect;
	 		}
	    	 //precv form pw ptu
//		    else if(ret == 0 || (ret <= -1))
//		    {
//		    	//printf("ret3 = %d \r\n",ret);
//		    	close(ptu_data.ptu_net_fd);
//		    	//DEBUG("ret = %d \r\n",ret);
//		    	goto reset_connect;
//		    }
	    }

	   usleep(1000);
//	}
#endif
}


/*************************************************
Function:    init_net_thread
Description: 初始化PTU网络线程
Input:
Output:
Return:
Others:
*************************************************/
int init_ptu_thread()
{
//	extern void iap_recv_thread_entry();
//	pthread_t recv_thread_id,iap_thread_id;
//	int ret;
//	sleep(10);
//	ptu_data.life_count = 0;
//	ptu_data.ptu_net_connect = 0;
//	ptu_data.ptu_net_fd = -1;
//
//	ret=pthread_create(&recv_thread_id,NULL,(void *)ptu_recv_thread_entry,NULL);
//	if(ret!=0)
//	{
//		DEBUG ("net save thread error!\n");
//		return ret;
//	}

//	ret=pthread_create(&iap_thread_id,NULL,(void *)iap_recv_thread_entry,NULL);
//	if(ret!=0)
//	{
//		DEBUG ("net save thread error!\n");
//		return ret;
//	}
	return 0;
}
