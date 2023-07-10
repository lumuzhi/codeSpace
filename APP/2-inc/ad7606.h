#ifndef _INCLUDES_H_
#define _INCLUDES_H_

#include "global_macro.h"
//#ifndef _AD7606_H_
//#define _AD7606_H_

#ifdef OUTER_COMPANY_TEST_20211026//test
struct mem_config
{
	unsigned int enumtype;
	unsigned short channel1[512];
	unsigned short channel2[512];
	unsigned short channel3[512];
	unsigned short channel4[512];
	unsigned short channel5[512];
	unsigned short channel6[512];
	unsigned short channel7[512];
	unsigned short channel8[512];
};
#else
struct mem_config
{
	unsigned int enumtype;
	unsigned short *channel1;
	unsigned short *channel2;
	unsigned short *channel3;
	unsigned short *channel4;
	unsigned short *channel5;
	unsigned short *channel6;
	unsigned short *channel7;
	unsigned short *channel8;
};
#endif

#define AD7606_IOCTL_MAGIC		'x'		//定义幻数
#ifdef AD_RESET
#define AD7606_IOCTL_MAX_NR		 4		//定义命令的最大值
#else
#define AD7606_IOCTL_MAX_NR		 3		//定义命令的最大值
#endif
#define READ_DATA  _IOWR(AD7606_IOCTL_MAGIC, 1, struct mem_config)//type,nr,size
#define START_AD   _IOWR(AD7606_IOCTL_MAGIC, 2,  struct mem_config)
#define STOP_AD    _IOWR(AD7606_IOCTL_MAGIC, 3,  struct mem_config)

void start_ad_sample();

#ifdef AD_RESET
#define RESET_AD    _IOWR(AD7606_IOCTL_MAGIC, 4,  struct mem_config)
int ad_reset_ioctl();
#endif

#ifdef AD7606_COMPLEMENTARY_TO_ORIGINAL
uint16_t ad_complementary_to_original(uint16_t ad);
#endif
#endif
