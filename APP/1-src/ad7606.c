#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "ad7606.h"
#include <stdint.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <linux/types.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <semaphore.h>
#include "user_data.h"
#include "self_test.h"
#include "ptu_app.h"

#define AD_DEV   "/dev/spidev2.0"  //SPI1 PW
#define AD_DEV2  "/dev/spidev1.0" //SPI0  SW

extern struct PW_AD_DATA pw_ad_data;
extern struct RINGBUFF_PARA pw_diagnos_ringbuf_para;
extern struct RINGBUFF_PARA pw_save_ringbuf_para;
extern struct RINGBUFF_PARA pw_send_ringbuf_para;
extern struct SELF_TEST_PARA self_test_para;

extern struct SW_AD_DATA sw_ad_data;
extern struct RINGBUFF_PARA sw_diagnos_ringbuf_para;
extern struct RINGBUFF_PARA sw_save_ringbuf_para;
extern struct RINGBUFF_PARA sw_send_ringbuf_para;
extern struct SELF_TEST_PARA self_test_para1;

//extern struct PTU_DATA ptu_data;
extern sem_t pw_diagnos_sem;
extern sem_t sw_diagnos_sem;
//uint32_t num_flag;
struct mem_config setmem;
struct mem_config setmemsw;

uint16_t channel1[512] = { 0 };
uint16_t channel2[512] = { 0 };
uint16_t channel3[512] = { 0 };
uint16_t channel4[512] = { 0 };
uint16_t channel5[512] = { 0 };
uint16_t channel6[512] = { 0 };
uint16_t channel7[512] = { 0 };
uint16_t channel8[512] = { 0 };

//uint16_t schannel1[512] = {0};
uint16_t schannel2[512] = { 0 };
uint16_t schannel3[512] = { 0 };
//uint16_t schannel4[512] = {0};
//uint16_t schannel5[512] = {0};
//uint16_t schannel6[512] = {0};
//uint16_t schannel7[512] = {0};
uint16_t schannel8[512] = { 0 };

#ifdef ONLY_CAN_ETH_FEED_WATCHDOG
uint8_t comm_first_flag = 0;
uint32_t comm_connect_cnt = 0;
extern uint8_t comm_connect_flag;
#endif

#ifdef AD7606_STOP_START_CTRL
	#ifdef AD7606_ERR_WATCHDOG_RESET
		enum AD7606_STATUS
		{
			AD_NONE,
			AD_READY,
			AD_UNSTABLE,
			AD_OK
		};

		uint8_t ad_err_flag = 0;
		uint8_t ad_err_cnt = 0;

//		enum AD7606_STATUS self_ad_status = AD_NONE;
		uint8_t self_ad_err_flag = 0;
		uint8_t self_ad_err_cnt;

		extern uint8_t self_check_over_flag;
		extern void ctrl_hw_watchdog(uint8_t ctrl_type);

	#else

		//-------------------------------------------
		#define TIMES_3M20S  400    //0.5s*2*(60*3+20)
		#define TIMES_1H     7200   //0.5s*2*60*60
		#define TIMES_1H3M20S  (TIMES_1H + TIMES_3M20S)

		//#define TIMES_FOR_TEST
		#ifdef TIMES_FOR_TEST
			#define STOP_AD_COUNTDOWN_TIMES   360//3m
			#define START_AD_COUNTDOWN_TIMES  760//6m20s
		#else
			#define STOP_AD_COUNTDOWN_TIMES   TIMES_1H
			#define START_AD_COUNTDOWN_TIMES  TIMES_1H3M20S
		#endif
		//--------------------------------------------
		uint8_t ad_flag=0;
	#endif

	static int restartad7606();
	static int stop_ad7606();
	void signal_off(void);
#endif

//float calculate_acc_voltage(uint16_t data);
void input_handler();
int start_ad7606();
int read_adraw_data();
void check_ad_spi_dev();
int init_ad7606();

#ifdef AD7606_COMPLEMENTARY_TO_ORIGINAL
/*
 * 补码转原码
 * */
uint16_t ad_complementary_to_original(uint16_t ad) {
	int16_t ad_org = 0;
	uint16_t ad_org_u16 = 0;

	if (ad & 0x8000) {
		ad_org = -(int16_t)((ad ^ 0xffff) + 1); //int16_t用uint16_t保存,最高位不丢数
		ad_org_u16 = (uint16_t) ad_org;
	} else {
		ad_org_u16 = ad;
	}

	return ad_org_u16;
}
#endif

void input_handler()						//每秒进中断一次
{
	uint16_t i = 0;
	{
//		printf("----(SIGIO)input_handler--->\r\n");
		read_adraw_data();

		for (i = 0; i < 512; i++) {
			//	   if(ptu_data.ptu_simulation_flag == 0)
			//	   {
#if defined(AD7606_COMPLEMENTARY_TO_ORIGINAL)
			channel1[i] = ad_complementary_to_original(setmem.channel1[i]);	//补码转原码
			channel2[i] = ad_complementary_to_original(setmem.channel2[i]);
			channel3[i] = ad_complementary_to_original(setmem.channel3[i]);
			channel4[i] = ad_complementary_to_original(setmem.channel4[i]);
			channel5[i] = ad_complementary_to_original(setmem.channel5[i]);
			channel6[i] = ad_complementary_to_original(setmem.channel6[i]);
			channel7[i] = ad_complementary_to_original(setmem.channel7[i]);
			channel8[i] = ad_complementary_to_original(setmem.channel8[i]);
#else
		   channel1[i] = setmem.channel1[i]+0x8000;//补码转原码
		   channel2[i] = setmem.channel2[i]+0x8000;
		   channel3[i] = setmem.channel3[i]+0x8000;
		   channel4[i] = setmem.channel4[i]+0x8000;
		   channel5[i] = setmem.channel5[i]+0x8000;
		   channel6[i] = setmem.channel6[i]+0x8000;
		   channel7[i] = setmem.channel7[i]+0x8000;
		   channel8[i] = setmem.channel8[i]+0x8000;
#endif
		}
		if (self_test_para.ad_data_status == 0)				//数据不稳定则丢弃
				{
			return;
		}
	}

#if  1
//		printf("read7606_value1:%d\n",channel1[0]);
//		printf("read7606_value2:%d\n",channel2[0]);
//		printf("read7606_value3:%d\n",channel3[0]);
//		printf("read7606_value4:%d\n",channel4[0]);
//		printf("read7606_value5:%d\n",channel5[0]);
//		printf("read7606_value6:%d\n",channel6[0]);
//		printf("read7606_value7:%d\n",channel7[0]);
//		printf("read7606_value8:%d\n",channel8[0]);

//		printf("AD7606---channel1,REF_VOLT: %f V\n", AD_CONVERT_VOLTAGE(channel1[0]));
//		printf("AD7606---channel2,REF_VOLT: %f V\n", AD_CONVERT_VOLTAGE(channel2[0]));
//		printf("AD7606---channel3,REF_VOLT: %f V\n", AD_CONVERT_VOLTAGE(channel3[0]));
//		printf("AD7606---channel4,REF_VOLT: %f V\n", AD_CONVERT_VOLTAGE(channel4[0]));
//		printf("AD7606---channel5,REF_VOLT: %f V\n", AD_CONVERT_VOLTAGE(channel5[0]));
//		printf("AD7606---channel6,REF_VOLT: %f V\n", AD_CONVERT_VOLTAGE(channel6[0]));
//		printf("AD7606---channel7,REF_VOLT: %f V\n", AD_CONVERT_VOLTAGE(channel7[0]));
//		printf("AD7606---channel8,REF_VOLT: %f V\n", AD_CONVERT_VOLTAGE(channel8[0]));
	#endif

//	printf("----(SIGIO)########input_handler--->\r\n");
	read_adraw_data1();
	for (i = 0; i < 512; i++) {
		schannel2[i] = ad_complementary_to_original(setmemsw.channel2[i]);
		schannel3[i] = ad_complementary_to_original(setmemsw.channel3[i]);
		schannel8[i] = ad_complementary_to_original(setmemsw.channel8[i]);
	}

//
//		printf("##read7606_value2:%d\n",schannel2[0]);
//		printf("##read7606_value3:%d\n",schannel3[0]);

#ifdef AD7606_ERR_WATCHDOG_RESET
	ad_err_flag = 1;

//	self_ad_status = AD_OK;
//	self_ad_err_flag = 0;
#endif

	// printf("read7606_two\n");
	pw_ad_data.column_index = pw_ad_data.column_index + 2;
	memmove(&pw_ad_data.buff[pw_ad_data.row_index][pw_ad_data.column_index],
			channel1, FS_PW * 2);
//   printf("input_handler---pw_ad_data.buff[%d][%d]:%d, ch:1\n", pw_ad_data.row_index, pw_ad_data.column_index, pw_ad_data.buff[pw_ad_data.row_index][pw_ad_data.column_index]);
	pw_ad_data.column_index = pw_ad_data.column_index + FS_PW;
	// printf("read7606_value1:%d\n",channel1[0]);
	// printf("read7606_value1:%f\n",calculate_acc_voltage(channel1[0]));

	memmove(&pw_ad_data.buff[pw_ad_data.row_index][pw_ad_data.column_index],
			channel2, FS_PW * 2);
//   printf("input_handler---pw_ad_data.buff[%d][%d]:%d, ch:2\n", pw_ad_data.row_index, pw_ad_data.column_index, pw_ad_data.buff[pw_ad_data.row_index][pw_ad_data.column_index]);
	pw_ad_data.column_index = pw_ad_data.column_index + FS_PW;
//  printf("read7606_value2:%d\n",channel2[0]);

	memmove(&pw_ad_data.buff[pw_ad_data.row_index][pw_ad_data.column_index],
			channel3, FS_PW * 2);
//   printf("input_handler---pw_ad_data.buff[%d][%d]:%d, ch:3\n", pw_ad_data.row_index, pw_ad_data.column_index, pw_ad_data.buff[pw_ad_data.row_index][pw_ad_data.column_index]);
	pw_ad_data.column_index = pw_ad_data.column_index + FS_PW;
//   printf("read7606_value3:%d\n",channel3[0]);

	memmove(&pw_ad_data.buff[pw_ad_data.row_index][pw_ad_data.column_index],
			channel4, FS_PW * 2);
//   printf("input_handler---pw_ad_data.buff[%d][%d]:%d, ch:4\n", pw_ad_data.row_index, pw_ad_data.column_index, pw_ad_data.buff[pw_ad_data.row_index][pw_ad_data.column_index]);

	pw_ad_data.column_index = pw_ad_data.column_index + FS_PW;
//   printf("read7606_value4:%d\n",channel4[0]);

	memmove(&pw_ad_data.buff[pw_ad_data.row_index][pw_ad_data.column_index],
			channel5, FS_PW * 2);
//   printf("input_handler---pw_ad_data.buff[%d][%d]:%d, ch:5\n", pw_ad_data.row_index, pw_ad_data.column_index, pw_ad_data.buff[pw_ad_data.row_index][pw_ad_data.column_index]);

	pw_ad_data.column_index = pw_ad_data.column_index + FS_PW;
//   printf("read7606_value5:%d\n",channel5[0]);

	memmove(&pw_ad_data.buff[pw_ad_data.row_index][pw_ad_data.column_index],
			channel6, FS_PW * 2);
//   printf("input_handler---pw_ad_data.buff[%d][%d]:%d, ch:6\n", pw_ad_data.row_index, pw_ad_data.column_index, pw_ad_data.buff[pw_ad_data.row_index][pw_ad_data.column_index]);

	pw_ad_data.column_index = pw_ad_data.column_index + FS_PW;
//   printf("read7606_value6:%d\n",channel6[0]);

	memmove(pw_ad_data.ad_chnnel7, channel8, FS_PW * 2);
//   printf("input_handler---channel7[0]:%d, ad_channel7[0]:%d\n", channel7[0], pw_ad_data.ad_chnnel7[0]);

//   printf("read7606_value7:%f,%d\n",calculate_acc_voltage(channel7[0]),channel7[0]);
	//TEST-1
//   printf("read7606_value7:%d\n",channel7[0]);
	//  printf("read7606_value7:%f\n",calculate_acc_voltage(channel7[0]));

	pw_ad_data.packet_id++;
	pw_ad_data.buff[pw_ad_data.row_index][0] = pw_ad_data.packet_id;//数据包号,也做生命信号
	//AD缓存行数自加

	pw_ad_data.row_index++;
	//可诊断行数自加

	pw_diagnos_ringbuf_para.num++;
	//可存储行数自加

	pw_save_ringbuf_para.num++;
	//可发送行数自加
	pw_send_ringbuf_para.num++;

	if (pw_ad_data.row_index == PW_AD_DATA_ROW) {
		pw_ad_data.row_index = 0;
	}

//   DEBUG("sig_num:%d,row_index:%d,column_index:%d\n",pw_ad_data.packet_id,pw_ad_data.row_index,pw_ad_data.column_index);
	if (pw_ad_data.column_index == PW_AD_DATA_COL) {
		pw_ad_data.column_index = 0;
//	   printf("sem_post \n");
		sem_post(&pw_diagnos_sem);
//	   printf("sem_post \n");
	}


	sw_ad_data.column_index = sw_ad_data.column_index + 2;
	memmove(&sw_ad_data.buff[sw_ad_data.row_index][sw_ad_data.column_index],
			schannel2, FS_SW * 2);

	sw_ad_data.column_index = sw_ad_data.column_index + FS_SW;
	//printf("read7606_value1:%d\n",channel1[0]);
	// printf("read7606_value1:%f\n",calculate_acc_voltage(channel1[0]));

	memmove(&sw_ad_data.buff[sw_ad_data.row_index][sw_ad_data.column_index],
			schannel3, FS_SW * 2);
	sw_ad_data.column_index = sw_ad_data.column_index + FS_SW;
	//printf("read7606_value2:%d\n",channel2[0]);

	memmove(sw_ad_data.ad_chnnel5, schannel8, FS_SW * 2);
	//printf("read7606_value5:%d\n",channel5[0]);
	sw_ad_data.packet_id++;
	sw_ad_data.buff[sw_ad_data.row_index][0] = sw_ad_data.packet_id;//数据包号,也做生命信号
	//AD缓存行数自加
	sw_ad_data.row_index++;
	//可诊断行数自加
	sw_diagnos_ringbuf_para.num++;
	//可存储行数自加
	sw_save_ringbuf_para.num++;
	//可发送行数自加
	sw_send_ringbuf_para.num++;

	if (sw_ad_data.row_index == SW_AD_DATA_ROW) {
		sw_ad_data.row_index = 0;
	}
	//printf("sig_num:%d,row_index:%d,column_index:%d\n",sw_ad_data.packet_id,sw_ad_data.row_index,sw_ad_data.column_index);
	if (sw_ad_data.column_index == SW_AD_DATA_COL) {
		sw_ad_data.column_index = 0;
		sem_post(&sw_diagnos_sem);
//		printf("###sem_post\n");
	}

}

void input_handler1()						//每秒进中断一次
{
	uint16_t i = 0;

#ifdef AD7606_STOP_START_CTRL
	#ifndef AD7606_ERR_WATCHDOG_RESET
   if(ad_flag)
   {
	   printf("----(SIGALRM)input_handler--->\r\n");
   }
   else
	#endif
#endif
	{
		printf("----(SIGIO)########input_handler--->\r\n");
		read_adraw_data1();

		for (i = 0; i < 512; i++) {
#ifdef AD7606_COMPLEMENTARY_TO_ORIGINAL
//		   schannel1[i] = ad_complementary_to_original(setmemsw.channel1[i]);//补码转原码
			schannel2[i] = ad_complementary_to_original(setmemsw.channel2[i]);
			schannel3[i] = ad_complementary_to_original(setmemsw.channel3[i]);
//		   schannel4[i] = ad_complementary_to_original(setmemsw.channel4[i]);
//		   schannel5[i] = ad_complementary_to_original(setmemsw.channel5[i]);
//		   schannel6[i] = ad_complementary_to_original(setmemsw.channel6[i]);
//		   schannel7[i] = ad_complementary_to_original(setmemsw.channel7[i]);
//		   schannel8[i] = ad_complementary_to_original(setmemsw.channel8[i]);
#else
		   channel1[i] = setmem.channel1[i]|0x8000;//补码转原码
		   channel2[i] = setmem.channel2[i]|0x8000;
		   channel3[i] = setmem.channel3[i]|0x8000;
		   channel4[i] = setmem.channel4[i]|0x8000;
		   channel5[i] = setmem.channel5[i]|0x8000;
		   channel6[i] = setmem.channel6[i]|0x8000;
		   channel7[i] = setmem.channel7[i]|0x8000;
		   channel8[i] = setmem.channel8[i]|0x8000;
#endif
		}
		//  printf("read7606_one\n");

//	   if(self_test_para1.ad_data_status == 0)				//数据不稳定则丢弃
//	   {
//		   return;
//	   }
	}

#if  1
//	printf("##read7606_value1:%d\n",schannel1[0]);
	printf("##read7606_value2:%d\n", schannel2[0]);
	printf("##read7606_value3:%d\n", schannel3[0]);
//	printf("##read7606_value4:%d\n",schannel4[0]);
//	printf("##read7606_value5:%d\n",schannel5[0]);
//	printf("##read7606_value6:%d\n",schannel6[0]);
//	printf("##read7606_value7:%d\n",schannel7[0]);
//	printf("##read7606_value8:%d\n",schannel8[0]);

//		printf("AD7606---channel1,REF_VOLT: %f V\n", AD_CONVERT_VOLTAGE(channel1[0]));
//		printf("AD7606---channel2,REF_VOLT: %f V\n", AD_CONVERT_VOLTAGE(channel2[0]));
//		printf("AD7606---channel3,REF_VOLT: %f V\n", AD_CONVERT_VOLTAGE(channel3[0]));
//		printf("AD7606---channel4,REF_VOLT: %f V\n", AD_CONVERT_VOLTAGE(channel4[0]));
//		printf("AD7606---channel5,REF_VOLT: %f V\n", AD_CONVERT_VOLTAGE(channel5[0]));
//		printf("AD7606---channel6,REF_VOLT: %f V\n", AD_CONVERT_VOLTAGE(channel6[0]));
//		printf("AD7606---channel7,REF_VOLT: %f V\n", AD_CONVERT_VOLTAGE(channel7[0]));
//		printf("AD7606---channel8,REF_VOLT: %f V\n", AD_CONVERT_VOLTAGE(channel8[0]));
#endif

#ifdef AD7606_ERR_WATCHDOG_RESET
	extern uint8_t ad_err_flag;
	ad_err_flag = 1;
#endif

	sw_ad_data.column_index = sw_ad_data.column_index + 2;
	memmove(&sw_ad_data.buff[sw_ad_data.row_index][sw_ad_data.column_index],
			schannel2, FS_SW * 2);

	sw_ad_data.column_index = sw_ad_data.column_index + FS_SW;
	//printf("read7606_value1:%d\n",channel1[0]);
	// printf("read7606_value1:%f\n",calculate_acc_voltage(channel1[0]));

	memmove(&sw_ad_data.buff[sw_ad_data.row_index][sw_ad_data.column_index],
			schannel3, FS_SW * 2);
	sw_ad_data.column_index = sw_ad_data.column_index + FS_SW;
	//printf("read7606_value2:%d\n",channel2[0]);

	memmove(sw_ad_data.ad_chnnel5, schannel8, FS_SW * 2);
	//printf("read7606_value5:%d\n",channel5[0]);
	sw_ad_data.packet_id++;
	sw_ad_data.buff[sw_ad_data.row_index][0] = sw_ad_data.packet_id;//数据包号,也做生命信号
	//AD缓存行数自加
	sw_ad_data.row_index++;
	//可诊断行数自加
	sw_diagnos_ringbuf_para.num++;
	//可存储行数自加
	sw_save_ringbuf_para.num++;
	//可发送行数自加
	sw_send_ringbuf_para.num++;

	if (sw_ad_data.row_index == SW_AD_DATA_ROW) {
		sw_ad_data.row_index = 0;
	}
	//printf("sig_num:%d,row_index:%d,column_index:%d\n",sw_ad_data.packet_id,sw_ad_data.row_index,sw_ad_data.column_index);
	if (sw_ad_data.column_index == SW_AD_DATA_COL) {
		sw_ad_data.column_index = 0;
		sem_post(&sw_diagnos_sem);
	}

}

int start_ad7606() {
	int fd = open(AD_DEV, O_RDWR);
	printf("ad7606---start_ad7606\n");
	if (fd < 0) {
		printf("can not open config file\n");
		return -1;
	}

	if (ioctl(fd, START_AD, &setmem) < 0) {
		printf("start ad failed\n");
	}

	close(fd);
	return 0;
}

int start_ad76061() {
	int fd = open(AD_DEV2, O_RDWR);
	printf("ad76061---start_ad76061\n");
	if (fd < 0) {
		printf("can not open config file\n");
		return -1;
	}

	if (ioctl(fd, START_AD, &setmemsw) < 0) {
		printf("start ad failed\n");
	}

	close(fd);
	return 0;
}

int read_adraw_data() {
	int fd = open(AD_DEV, O_RDWR);

	if (fd < 0) {
		printf("can not open config file\n");
		return -1;
	}

	if (ioctl(fd, READ_DATA, &setmem) < 0) {
		printf("read ad data failed\n");
	}

	close(fd);
	return 0;

}

int read_adraw_data1() {
	int fd = open(AD_DEV2, O_RDWR);

	if (fd < 0) {
		printf("can not open config file\n");
		return -1;
	}

	if (ioctl(fd, READ_DATA, &setmemsw) < 0) {
		printf("read ad data failed\n");
	}

	close(fd);
	return 0;

}

//before delete_data_dir_sem

void check_ad_spi_dev() {
	printf("ad7606---check_ad_spi_dev\n");
	if (access(AD_DEV, F_OK) < 0)							//防止驱动未自动加载
			{
		system("insmod /lib/modules/ad7606.ko");
		//system("insmod /home/root/ad7606.ko");

		printf("insmod ad7606 ok\n");
		//sleep(1);
		//insmod("");
	}
	if (access(AD_DEV2, F_OK) < 0)							//防止驱动未自动加载
			{
		system("insmod /lib/modules/ad7606.ko");
		//system("insmod /home/root/ad7606.ko");

		printf("insmod ad7606 ok\n");
		//sleep(1);
		//insmod("");
	}
}

int init_ad7606() {
	int oflags;
	int fd = open(AD_DEV, O_RDWR);
	printf("ad7606---init_ad7606\n");
	if (fd < 0) {
		printf("can not open config file\n");
		return -1;
	}

	printf("1signal(SIGIO, input_handler)\r\n");
	signal(SIGIO, input_handler);
	printf("2signal(SIGIO, input_handler)\r\n");

	if (fcntl(fd, F_SETOWN, getpid()) < 0) {
		printf("fcntl err: F_SETOWN\r\n");
	}

	oflags = fcntl(fd, F_GETFL);
	if (oflags < 0) {
		printf("fcntl err: get flag\r\n");
	}

	if (fcntl(fd, F_SETFL, oflags | FASYNC) < 0) {
		printf("fcntl err: set flag\r\n");
	}
	return oflags;
}

int init_ad76061() {
	int oflags;
	int fd = open(AD_DEV2, O_RDWR);
	printf("ad76061---init_ad76061\n");
	if (fd < 0) {
		printf("can not open config file\n");
		return -1;
	}

	printf("1signal(SIGIO, input_handler)\r\n");
//    signal(SIGIO, input_handler1);
	printf("2signal(SIGIO, input_handler)\r\n");

	if (fcntl(fd, F_SETOWN, getpid()) < 0) {
		printf("fcntl err: F_SETOWN\r\n");
	}

	oflags = fcntl(fd, F_GETFL);
	if (oflags < 0) {
		printf("fcntl err: get flag\r\n");
	}

	if (fcntl(fd, F_SETFL, oflags | FASYNC) < 0) {
		printf("fcntl err: set flag\r\n");
	}
	return oflags;
}

void start_ad_sample() {
	init_pw_data_para();
	init_sw_data_para();
	check_ad_spi_dev();
	init_ad7606();
	init_ad76061();
	start_ad7606();
	start_ad76061();
	printf("start_ad\n");
}
#ifdef AD_RESET
int ad_reset_ioctl()
{
	int fd = open(AD_DEV, O_RDWR);
	printf("ad7606---reset_ad7606\n");
	if (fd < 0)
	{
		printf("\n\nERROR:Can not open ADC interface!s\n\n");
		return -1;
	}

	if (ioctl(fd, RESET_AD, &setmem) < 0)
	{
		printf("ERROR:ioctl rest ADC!\n");
		return -1;
	}

	DEBUG("RESET : AD7606 success!\n");
	close(fd);
	return 0;
}
#endif

#ifdef AD7606_STOP_START_CTRL
static int restartad7606()
{
	int fd = open(AD_DEV, O_RDWR);
	printf("ad7606---restartad7606\n");
	if (fd < 0)
	{
		printf("\n\nERROR:Can not open ADC interface!s\n\n");
		return -1;
	}

	if (ioctl(fd, START_AD, &setmem) < 0)
	{
		printf("ERROR:ioctl STOP ADC!\n");
		return -1;
	}

	DEBUG("STOP : AD7606 success!\n");
	close(fd);
	return 0;
}

static int stop_ad7606()
{
	int fd = open(AD_DEV, O_RDWR);
	printf("ad7606---stop_ad7606\n");
	if (fd < 0)
	{
		printf("\n\nERROR:Can not open ADC interface!s\n\n");
		return -1;
	}

	if (ioctl(fd, STOP_AD, &setmem) < 0)
	{
		printf("ERROR:ioctl STOP ADC!\n");
		return -1;
	}

	DEBUG("STOP : AD7606 success!\n");
	close(fd);
	return 0;
}

/*
 * 定时检测ad7606是否正常采样
 * 当ad跑飞(非正常采样)后,软件计数重启的功能
 */
void signal_off(void)
{
	static uint32_t counter=0;

#ifdef ONLY_CAN_ETH_FEED_WATCHDOG
	uint32_t limit_value = 100;

	if(comm_first_flag)
	{
		limit_value = 16;
	}

	printf("signal_off---comm_connect_flag:%d\n", comm_connect_flag);
	if(!comm_connect_flag)
	{
		comm_connect_cnt++;

		printf("signal_off---comm_connect_cnt:%d\n", comm_connect_cnt);
		if(comm_connect_cnt >= limit_value)
		{
			ctrl_hw_watchdog(0);
			system("reboot -nf");
		}
	}
	else
	{
		comm_connect_flag = 0;
		comm_connect_cnt = 0;
	}
#endif

#ifdef AD7606_ERR_WATCHDOG_RESET
	if(self_check_over_flag)
	{
		if(!ad_err_flag)
		{
			ad_err_cnt++;

//			printf("ad_err_cnt---->%d\n",ad_err_cnt);
			if(ad_err_cnt >= 3)
			{
//				printf("sd_exist_test---reboot\n");
				ctrl_hw_watchdog(0);
				system("reboot -nf");
			}
		}
		else
		{
			ad_err_flag = 0;
			ad_err_cnt = 0;
		}
	}
#if 0//self test20210618
	else
	{
		if(counter >= 160)
		{
//			if(self_ad_status == AD_READY)
			if(self_ad_err_flag)
			{
				self_ad_err_cnt++;

				printf("self_ad_err_cnt---->%d\n",self_ad_err_cnt);
				if(self_ad_err_cnt >= 4)
				{
	//				printf("sd_exist_test---reboot\n");
					ctrl_hw_watchdog(0);
					system("reboot -nf");
				}
			}
			else
			{
//				self_ad_status = AD_NONE;
//				self_ad_err_flag = 0;
				self_ad_err_cnt = 0;
			}
		}
	}
#endif
#endif

	counter++;
	printf("counter---->%d\n",counter);

#ifndef AD7606_ERR_WATCHDOG_RESET
	if(counter%STOP_AD_COUNTDOWN_TIMES==0) //7200
	{
		printf("SIGSTOP----\n");

		ad_flag=1;
		//signal(SIGSTOP,input_handler);
		stop_ad7606();
	}
	else if(ad_flag)
	{

		if(counter>START_AD_COUNTDOWN_TIMES)//    7600  3m20s
		{
			counter=0;
			printf("insmod /lib/modules/ad7606.ko----\n");
			ad_flag=0;
			restartad7606();
		}

	#ifdef AD7606_STOP_DATA_TIMER_TRIGGER
		if(counter%2 == 0)//2*0.5s=1s
		{
//			extern void comm_to_ctrl_board_check();
//			comm_to_ctrl_board_check();
			input_handler();
		}
	#endif
	}
#endif

}
#endif

//int main()
//{
//  init_irq();
//  start_ad();
//  while(1);
//  return 0;
//
//}

