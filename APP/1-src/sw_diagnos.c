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
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <semaphore.h>
#include <linux/can.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/can/raw.h>
#include "sw_diagnos.h"
#include "self_test.h"
#include "hdd_save.h"
#include "board.h"
#include "udp_client.h"
#include "lh_math.h"
#include "ptu_app.h"
#include "pw_diagnos.h"
#ifdef WTD_DATA_TRANSLATE_PROTOCOL
#include "self_queue.h"
#endif

#define FFT_NPT 1024
#define DATA_HEAD_SIZE 2			//uint16_t 数据包号;uint16_t 速度

#define SENSITIVITY_SW 0.2f
#define STABLE_HZ40_INDEX  (uint32_t)(40.0f*FFT_NPT/FS_SW+0.5f)
#define STABLE_CAL_SENCOND 9u

extern struct SW_AD_DATA sw_ad_data;
extern struct SW_TZ_DATA sw_tz_data;
//extern struct SW_RAW_DATA sw_raw_data;
extern struct RINGBUFF_PARA sw_diagnos_ringbuf_para;

extern struct PW_CLB_CONFIG_PARA *pw_clb_config_para;
extern struct SELF_TEST_PARA self_test_para;
extern struct SW_FILE sw_file;
extern struct UDP_SEND_ADDR udp_send_addr;
extern struct PTU_DATA ptu_data;
//extern struct COMM_DATA_CNT comm_data_cnt;
extern struct SYS_STATUS_CNT sys_status_cnt;
//extern struct UPDATE_ERR_LOG_FLAG	update_err_log_flag;

#ifdef WTD_DATA_TRANSLATE_PROTOCOL
	extern struct LESS_HEAD_INFO head_info;
	extern uint8_t first_diag_result_flag;
	extern struct MSG_ALARM_TRIG msg_alarm_trigger[CHANNEL_NUM];
//	extern void save_msg_alarm_data(uint8_t ch_num, struct SW_RAW_DATA *buf, uint16_t size);
	extern void save_msg_alarm_data(uint8_t ch_num, uint8_t *buf, uint16_t size);
//	extern uint16_t little_to_big_16bit(uint16_t value);
#ifdef WTD_SIMULATION_ALARM_TRIG
	uint8_t simulation_open_flag = 0;
	uint8_t simulation_status[CHANNEL_NUM] = {TRIG_OK, TRIG_OK, TRIG_OK, TRIG_OK};
#endif
#endif

//extern uint16_t little_to_big_16bit(uint16_t value);

//extern uint8_t pw_self_test_flag;
uint16_t lastdatasw[2]={0,0};

float f_buff_outsw[4] = {0.0};		//数据滤波使用，滤掉异常点
struct SW_DIAGNOS_PARA sw_diagnos_para;
#ifdef PP_TZZ_DIAG_1S_FILLTER
struct FIVE_SEC_PP_SIZE_RECORD five_sec_pp_size_record[CHANNEL_NUM];
#endif
union SWBOARD_ERR sw_boarderr;
sem_t sw_diagnos_sem;
extern uint8_t sw_data_save_type;
extern txb_MVB_public_t app_save_public;
extern sem_t self_test_sw_sem;
extern void update_sw_tz_data(struct SW_TZ_DATA *save_tz_data,void *tzdata_temp,uint8_t update_tz_flag);
/*fs=256HZ 2-10HZ*/ //4阶
//static float FS_256_NUM_b_2_10HZ[9] = {
//5.123205967418e-06f,
//0.0f,
//-2.049282386967e-05f,
//0.0f,
//3.073923580451e-05f,
//0.0f,
//-2.049282386967e-05f,
//0.0f,
//5.123205967418e-06f
//};
//
//static float FS_256_DEN_a_2_10HZ[5] = {
//1.0f,
//-7.731838733078f,
//26.16919507888f,
//-50.64169847204f,
//61.28525943478f,
//-47.49358510896f,
//23.01687103166f,
//-6.377831450642f,
//0.773628219466f
//};


/*fs=512HZ 2-10HZ*/ //8阶
static double FS_512_NUM_b_2_10HZ[9] = {
5.123205967418e-06,
0.0,
-2.049282386967e-05,
0.0,
3.073923580451e-05,
0.0,
-2.049282386967e-05,
0.0,
5.123205967418e-06
};

static double FS_512_DEN_a_2_10HZ[9] = {
1.0,
-7.731838733078,
26.16919507888,
-50.64169847204,
61.28525943478,
-47.49358510896,
23.01687103166,
-6.377831450642,
0.773628219466
};


/*
 * 大小端 转换
 *
 * **/
static short myhtons(short n)
{
  return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}
/*
 *  原始数据转 加速度
 *
 * **/
//#ifdef WTD_DATA_TRANSLATE_PROTOCOL
//void acc_data_to_g(uint16_t *srcbuf,uint16_t *dest,uint16_t *wtd_dest,uint16_t len)//m/s^2  htons转大端
//{
//	int i=0;
//	float acc_buf = 0;
//	float testbuf = 0;
//	float wtd_testbuf = 0;
//
//	for(i=0;i<len;i++)
//	{
//		acc_buf = AD_CONVERT_ACCELERATION(srcbuf[i]);//单位g
//		testbuf = GRAVITY_PARA * AMPLIFICATION_FACTOR * acc_buf;//x100,单位m/s^2
//		wtd_testbuf = WTD_AMPLIFICATION_FACTOR * acc_buf;//x1000,单位m/s^2
//
//		dest[i] = (uint16_t)testbuf;
//		dest[i] = htons(dest[i]);//转大端
//
//		wtd_dest[i] = (uint16_t)wtd_testbuf;
//		wtd_dest[i] = htons(wtd_dest[i]);//转大端
//	}
//}
//#else
//void acc_data_to_g(uint16_t *srcbuf,uint16_t *dest,uint16_t len)//m/s^2  htons转大端
//{
//	int i=0;
//	float testbuf = 0;
//
//	for(i=0;i<len;i++)
//	{
//#ifdef AD7606_COMPLEMENTARY_TO_ORIGINAL
//		testbuf = GRAVITY_PARA * AMPLIFICATION_FACTOR * AD_CONVERT_ACCELERATION(srcbuf[i]);//单位m/s^2
//#else
//		testbuf = (srcbuf[i] - 32768) * 10 * 9.8 * 100/ 65536.0 / SENSITIVITY_SW;//m/s^2      g=9.8m/s^2  100为放大100倍以保留小数点后两位精度．
//#endif
//
//		dest[i] = (uint16_t)testbuf;
//
//		dest[i] = htons(dest[i]);//转大端
//	}
//}
//#endif
/*************************************************
Function:ADdata_Filter
Description: AD原始数据干扰处理
Input:采样值，长度
Output:磨平异常点
Return:
Others:
*************************************************/

static void ADdata_Filter(uint16_t *deal_data,uint32_t data_len ,uint8_t ch_num)
{
	int i,j=0;
	short trend;
	for( i=0;i<data_len;i++)
	{
		if(deal_data[i]!=lastdatasw[ch_num])
		{
			trend=deal_data[i]-lastdatasw[ch_num];

			if((fabs(trend)>5000))
			{
				j++;
				if(j>350)
				{
					lastdatasw[ch_num]=deal_data[i];
					j=0;
				}
				else
					deal_data[i]=lastdatasw[ch_num];
				//printf("trend[0]=%f,,deal_data[i]=%f  lastdatasw=%f \n",fabs(trend[0]),deal_data[i],lastdatasw);
			}
			else
			{
				lastdatasw[ch_num]=deal_data[i];
			}

		}

	}

}

/**
  * @author: xuzhao
  * @breif: 找出4个转向架中的最大峰峰值，取4个转向架中超出阈值波形最大的值
	* @modify:
  */
void find_max_value(struct SW_DIAGNOS_PARA *tzdeal,uint8_t ch,float ampth,uint32_t h_num)
{
	static uint16_t temp_para1;
	static uint8_t temp_para2;

	if(ch == 0)
	{
		temp_para1 = 0;
		temp_para2 = 0;
	}


	if((uint16_t)(ampth + 0.5f) > temp_para1)
	{
		//找出最大加速度
		tzdeal->max_gvalue=(uint16_t)(ampth + 0.5f);
	}
	else
	{
		tzdeal->max_gvalue=temp_para1;
	}

	if(h_num > temp_para2)
	{
		//找出超过阈值最大计数
		tzdeal->alarm_h_num = h_num;
	}
	else
	{
		tzdeal->alarm_h_num  = temp_para2;
	}

	temp_para1 = tzdeal->max_gvalue;
	temp_para2 = tzdeal->alarm_h_num;

//	printf("tzdeal->max_gvalue=%d tzdeal->alarm_h_num=%d \n",tzdeal->max_gvalue,tzdeal->alarm_h_num);
}

/*************************************************
Function:find_max_ampth
Description:
Input:
Output:
Return:
Others:
*************************************************/
void find_max_ampth(float *sour,uint32_t lenth,float *dest)
{
	if(lenth==0)
		return;

	float temp_data=sour[0];
	for(int i=1;i<lenth;i++)
	{
		if(sour[i]>temp_data)
			temp_data=sour[i];
	}
	*dest=temp_data;
}


/*************************************************
Function:lost_old_data_deal
Description: 数据行数超出处理
Input:
Output:
Return:
Others:
*************************************************/
static void lost_old_data_deal(struct RINGBUFF_PARA *sw_diagnos_ringbuf_para_temp,struct SW_AD_DATA *SW_AD_DATA_temp)
{
	sw_diagnos_ringbuf_para_temp->index = SW_AD_DATA_temp->row_index + 1;
	if(sw_diagnos_ringbuf_para_temp -> index > SW_AD_DATA_temp->row_size)
	{
		sw_diagnos_ringbuf_para_temp->index = 0;
	}
	sw_diagnos_ringbuf_para_temp->num = SW_AD_DATA_temp->row_size - 1;
}
/*************************************************
Function:ring_buff_index_inc
Description: 诊断行数自加
Input:
Output:
Return:
Others:
*************************************************/
static void ring_buff_index_inc(struct RINGBUFF_PARA *sw_diagnos_ringbuf_para_temp)
{
	sw_diagnos_ringbuf_para_temp->index++;
	if(sw_diagnos_ringbuf_para_temp->index >= sw_diagnos_ringbuf_para_temp->size)
	{
		sw_diagnos_ringbuf_para_temp->index = 0;
	}
}


static void arm_move_f32(double *data_float, double *data_int, uint32_t blocksize)
{
  uint32_t blkcnt;                               /* loop counter */
//  uint8_t cnt = 0;

  double  in_f32_1, in_f32_2, in_f32_3, in_f32_4;
  double  in_int_1, in_int_2, in_int_3, in_int_4;

  /*loop Unrolling */
  blkcnt = blocksize >> 2u;


  while(blkcnt > 0u)
  {
		in_f32_1 = *(data_float - 1);
		in_f32_2 = *(data_float - 2);
		in_f32_3 = *(data_float - 3);
		in_f32_4 = *(data_float - 4);
		*(data_float - 0) = in_f32_1;
		*(data_float - 1) = in_f32_2;
		*(data_float - 2) = in_f32_3;
		*(data_float - 3) = in_f32_4;

		in_int_1 = *(data_int - 1);
		in_int_2 = *(data_int - 2);
		in_int_3 = *(data_int - 3);
		in_int_4 = *(data_int - 4);

		*(data_int - 0) = in_int_1;
		*(data_int - 1) = in_int_2;
		*(data_int - 2) = in_int_3;
		*(data_int - 3) = in_int_4;


    /* update pointers to process next samples */
    data_float -= 4u;
    data_int   -= 4u;

    /* Decrement the loop counter */
    blkcnt--;
  }

  /* If the blockSize is not a multiple of 4, compute any remaining output samples here.
   ** No loop unrolling is used. */
  blkcnt = blocksize % 0x4u;

  while(blkcnt > 0u)
  {
    *data_float = *(data_float - 1);
	*data_int   = *(data_int - 1);

	data_float--;
	data_int--;
    /* Decrement the loop counter */
	blkcnt--;
  }

}

/*******************************************************************************
* Function Name  : filter_deal
* Description    : 失稳滤波处理
* Input          : - deal_buf:待滤波数据处理及滤波结果更新
                   - filter_struct_float：滤波处理结构体
* Output         : None
* Return         : None
* Function call relation  :
* Reentrant      :
* Notes          :
*******************************************************************************/

void swfilter_deal(struct SWARRAY_FLOAT *deal_buf,struct FILTER_STRUCT_FLOAT *filter_struct_float)
{
	uint16_t cnt_i = 0;
	double temp_x = 0.0;
	double temp_y = 0.0;
	double temp_res = 0.0f;

#if 0

#endif

	for(cnt_i = 0;cnt_i < deal_buf->size;cnt_i++)
	{
		temp_x = 0.0;
		temp_y = 0.0;
		temp_res = 0.0f;

		filter_struct_float->before_filter_buff[0] = (double)deal_buf->buff[cnt_i];



		temp_x = float_sum_xy(FS_512_NUM_b_2_10HZ,filter_struct_float->before_filter_buff,filter_struct_float->window_size);
		temp_y = float_sum_xy(&FS_512_DEN_a_2_10HZ[1],&filter_struct_float->hs_data[1],filter_struct_float->window_size-1);


//		if(cnt_i > 100 && cnt_i < 110)
//					printf("temp_res = %f\n", (float)temp_y);

		temp_res = temp_x - temp_y;

		filter_struct_float->hs_data[0] = temp_res;

		arm_move_f32(&filter_struct_float->hs_data[filter_struct_float->window_size-1],
				&filter_struct_float->before_filter_buff[filter_struct_float->window_size-1],filter_struct_float->window_size-1);
		deal_buf->buff[cnt_i] = (float)temp_res;


	}
}
/*******************************************************************************
* Function Name  : calc_amp_deal
* Description    : 计算峰峰值
* Input          : - filter_para: 滤波结构体
                   - tagTimeWindowDeal:时间窗结构体
                   - HC：已经达到阈值的次数
				　　　- ch_id: 通道
* Output         : None
* Return         : None
* Function call relation  :
* Reentrant      :
* Notes          :
*******************************************************************************/
static void swcalc_amp_deal(struct SW_DIAGNOS_PARA * tzdata, uint32_t ch)
{
	uint32_t extreme_numb = tzdata->last_packet_extreme[ch].w_index;
	float *process_data;

	/*如果极值点的个数为奇数则保留1个，如果极值点的个数为偶数则保留2个*/
	uint8_t have_ffz_flag = 0,jz_offset_bit = 0;
	uint32_t remain_numb,group_numb,group_i = 0;


	process_data = tzdata->last_packet_extreme[ch].buff;
	/*至少有3个极值点(要有一个完整的波峰和波谷)才能判断一个峰峰值,每一包极值点取峰峰值时应该保证都是从正值点或者负值点取*/
	if(process_data[0] > 0)
	{
		if (extreme_numb >= 3)
		{
			have_ffz_flag = 0x01;
			jz_offset_bit = 0x0;
		}
	}
	else
	{
		if (extreme_numb >= 4)
		{
			extreme_numb--;
			have_ffz_flag = 0x01;
			jz_offset_bit = 0x1;
		}
	}

	if(have_ffz_flag>0)
	{
		/*step1. 计算峰值及其个数及保留的极值点个数*/
		group_numb = extreme_numb/2;
		remain_numb = extreme_numb & 0x01;
		if(remain_numb == 0)
		{
			group_numb--;
			remain_numb = 2;
		}

		for(group_i=0;group_i<group_numb;group_i++)
		{
			float a=tzdata->last_packet_extreme[ch].buff[group_i*2 + jz_offset_bit];
			float b=tzdata->last_packet_extreme[ch].buff[group_i*2 + 1 + jz_offset_bit];
			tzdata->data_deal_buff[ch].buff[group_i]=fabs(a-b);//波峰-波谷,取绝对值
		}
		tzdata->data_deal_buff[ch].w_index=group_numb;//峰值的个数赋值给data_deal_buff[ch].w_index
    }
    else
    {
        remain_numb = extreme_numb;
		tzdata->data_deal_buff[ch].w_index=0;
    }

    if(remain_numb==2)
    {
        tzdata->last_packet_extreme[ch].buff[0]=tzdata->last_packet_extreme[ch].buff[extreme_numb-2];
        tzdata->last_packet_extreme[ch].buff[1]=tzdata->last_packet_extreme[ch].buff[extreme_numb-1];
    }
    else if(remain_numb==1)
    {
        tzdata->last_packet_extreme[ch].buff[0]=tzdata->last_packet_extreme[ch].buff[extreme_numb-1];
    }
    else if(remain_numb==3)
    {
        tzdata->last_packet_extreme[ch].buff[0]=tzdata->last_packet_extreme[ch].buff[extreme_numb-3];
        tzdata->last_packet_extreme[ch].buff[1]=tzdata->last_packet_extreme[ch].buff[extreme_numb-2];
        tzdata->last_packet_extreme[ch].buff[2]=tzdata->last_packet_extreme[ch].buff[extreme_numb-1];
    }
    tzdata->last_packet_extreme[ch].w_index=remain_numb;//要保留的极值个数赋值给last_packet_extreme[ch].w_index

}

/*******************************************************************************
* Function Name  : reduce_data
* Description    : 将数据缓存中的数据只保存极值
* Input          : - data:待处理数据缓存
                   - IndexBuff：待处理数据标号缓存
                   - data_num：处理点数
* Output         : None
* Return         : None
* Function call relation  :
* Reentrant      :
* Notes          :
*******************************************************************************/
static int32_t swreduce_data(float *data, uint32_t data_num)
{
	enum DATASTATS_TYPE statstmp = INIT_STATS;
	uint32_t data_i=0;
	uint32_t save_i=0;

	for (data_i=0; data_i<data_num; data_i++)
	{
		switch(statstmp)
		{
			case INIT_STATS:
				data[save_i] = data[data_i];

			  if ((data_i+1) < data_num)
				{
					if (data[save_i] > 0)
					{
						if (data[data_i+1] > 0)
							statstmp = POS_STATS;
						else
						{
							save_i++;
						}
					}
					else
					{
						if (data[data_i+1] < 0)
							statstmp = NEG_STATS;
						else
						{
							save_i++;
						}
					}
				}
			break;

			case POS_STATS:
				if (data[save_i] < data[data_i])
				{
					data[save_i] = data[data_i];
				}

				if ((data_i+1) < data_num)
				{
					if (data[data_i+1] < 0)
					{
						save_i++;
						statstmp = INIT_STATS;
					}
				}
			break;

			case NEG_STATS:
				if (data[save_i] > data[data_i])
				{
					data[save_i] = data[data_i];
				}

				if ((data_i+1) < data_num)
				{
					if (data[data_i+1] > 0)
					{
						save_i++;
						statstmp = INIT_STATS;
					}
				}
			break;

			default:
				break;
		}
	}

	return (save_i+1);

}


void  swcopy_data_float(float *src,float *dst,uint16_t data_num)
{
	uint16_t cnt_i;
	for(cnt_i = 0;cnt_i < data_num;cnt_i ++)
	{
		dst[cnt_i] = src[cnt_i];
	}
}
/*******************************************************************************
* Function Name  : sigtoampth
* Description    : 处理峰峰值
* Input          : - tzdata: 特征参数
                   - ch_id:通道号(0、1...)
                   - numb:待处理的点数
* Output         : None
* Return         : None
* Function call relation  :
* Reentrant      :
* Notes          :
*******************************************************************************/
static void swsigtoampth(struct SW_DIAGNOS_PARA *tzdata, uint32_t ch, uint32_t size)
{
//	uint32_t i;
//	uint32_t ampth_cnt = 0;
	uint32_t numb;

  /*step1. 对新的1包滤波数据--求极值点*/
	numb = swreduce_data(tzdata->data_deal_buff[ch].buff, size);

	/*step2. 合并操作：将新的极值点 转移到 老的1包“保留”下来的极值点的后面*/
	swcopy_data_float(
	tzdata->data_deal_buff[ch].buff,
	&tzdata->last_packet_extreme[ch].buff[tzdata->last_packet_extreme[ch].w_index],
	numb);


	/*step3. 计算“合并数据”的极值点，返回“新的极值点个数”*/
	tzdata->last_packet_extreme[ch].w_index=swreduce_data(
	tzdata->last_packet_extreme[ch].buff,
	tzdata->last_packet_extreme[ch].w_index+numb);

	/*step4. 计算峰峰值，保留“可疑极值点”*/
	swcalc_amp_deal(tzdata,ch);

}
/*******************************************************************************
* Function Name  : sigtodiff
* Description    : 从信号中剔除相邻相同数和零值数
* Input          : - tzdata: 特征参数
                   - ch_id:通道号(0、1...)
* Output         : None
* Return         : None
* Function call relation  :
* Reentrant      :
* Notes          :
*******************************************************************************/
static int32_t swsigtodiff(struct SW_DIAGNOS_PARA * tzdata, uint32_t ch)
{
	uint32_t i=0, j=0;

	float *sig = tzdata->data_deal_buff[ch].buff;
	uint32_t numb = tzdata->data_deal_buff[ch].size;

	if(sig == 0x0)
		 return 0;

	for (i=0; i<numb; i++)
	{
		if (*(sig+i) != 0)
			break;
	}
	if (i < numb)
	{
		*(sig+j) = *(sig+i);

		j=1;
		for(i=i+1; i<numb; i++)
		{
			 if((*(sig+i) != *(sig+i-1)) && *(sig+i)!=0)
			 {
					*(sig+j) = *(sig+i);
					j++;
			 }
		}
	}
	return j;
}


/*******************************************************************************
* Function Name  : ampth_deal
* Description    : 峰峰值处理
* Input          : None
* Output         : None
* Return         : None
* Function call relation  :
* Reentrant      :
* Notes          :
*******************************************************************************/
void swampth_deal(struct SW_DIAGNOS_PARA *tzdata, uint32_t ch)
{
	int32_t numb_tmp = 0;

  	numb_tmp = swsigtodiff(tzdata, ch);

	if (numb_tmp > 0)
	{
		swsigtoampth(tzdata, ch, numb_tmp);
	}
}

#ifdef PP_TZZ_DIAG_1S_FILLTER
#elif defined(PP_TZZ_DIAG_20210420)
static void sigtoampth_5s(struct SW_DIAGNOS_PARA *tzdata, uint32_t ch, uint32_t size);

/*从信号中剔除相邻相同数和零值数*/
static int32_t sigtodiff_5s(struct SW_DIAGNOS_PARA * tzdata, uint32_t ch)
{
	uint32_t i=0, j=0;

	float *sig = tzdata->data_deal_buff_5s[ch].buff;
	uint32_t numb = tzdata->data_deal_buff_5s[ch].size;

	if(sig == 0x0)
		 return 0;

	for (i=0; i<numb; i++)
	{
		if (*(sig+i) != 0)
			break;
	}
	if (i < numb)
	{
		*(sig+j) = *(sig+i);

		j=1;
		for(i=i+1; i<numb; i++)
		{
			 if((*(sig+i) != *(sig+i-1)) && *(sig+i)!=0)
			 {
					*(sig+j) = *(sig+i);
					j++;
			 }
		}
	}
	return j;
}

/*计算峰峰值*/
void calc_amp_deal_5s(struct SW_DIAGNOS_PARA * tzdata, uint32_t ch)
{
	uint32_t extreme_numb = tzdata->last_packet_extreme_5s[ch].w_index;
	float *process_data;

	/*如果极值点的个数为奇数则保留1个，如果极值点的个数为偶数则保留2个*/
	uint8_t have_ffz_flag = 0,jz_offset_bit = 0;
	uint32_t remain_numb,group_numb,group_i = 0;


	process_data = tzdata->last_packet_extreme_5s[ch].buff;
	/*至少有3个极值点(要有一个完整的波峰和波谷)才能判断一个峰峰值,每一包极值点取峰峰值时应该保证都是从正值点或者负值点取*/
	if(process_data[0] > 0)
	{
		if (extreme_numb >= 3)
		{
			have_ffz_flag = 0x01;
			jz_offset_bit = 0x0;
		}
	}
	else
	{
		if (extreme_numb >= 4)
		{
			extreme_numb--;
			have_ffz_flag = 0x01;
			jz_offset_bit = 0x1;
		}
	}

	if(have_ffz_flag>0)
	{
		/*step1. 计算峰值及其个数及保留的极值点个数*/
		group_numb = extreme_numb/2;
		remain_numb = extreme_numb & 0x01;//如果极值点的个数为奇数则保留1个峰峰值
		if(remain_numb == 0)
		{
			group_numb--;
			remain_numb = 2;//如果极值点的个数为偶数则保留2个峰峰值
		}

		for(group_i=0;group_i<group_numb;group_i++)
		{
			float a=tzdata->last_packet_extreme_5s[ch].buff[group_i*2 + jz_offset_bit];
			float b=tzdata->last_packet_extreme_5s[ch].buff[group_i*2 + 1 + jz_offset_bit];
			tzdata->data_deal_buff_5s[ch].buff[group_i]=fabs(a-b);//波峰-波谷,取绝对值
		}
		tzdata->data_deal_buff_5s[ch].w_index=group_numb;//峰值的个数赋值给data_deal_buff[ch].w_index
    }
    else
    {
        remain_numb = extreme_numb;
		tzdata->data_deal_buff_5s[ch].w_index=0;
    }

    if(remain_numb==2)
    {
        tzdata->last_packet_extreme_5s[ch].buff[0]=tzdata->last_packet_extreme_5s[ch].buff[extreme_numb-2];
        tzdata->last_packet_extreme_5s[ch].buff[1]=tzdata->last_packet_extreme_5s[ch].buff[extreme_numb-1];
    }
    else if(remain_numb==1)
    {
        tzdata->last_packet_extreme_5s[ch].buff[0]=tzdata->last_packet_extreme_5s[ch].buff[extreme_numb-1];
    }
    else if(remain_numb==3)
    {
        tzdata->last_packet_extreme_5s[ch].buff[0]=tzdata->last_packet_extreme_5s[ch].buff[extreme_numb-3];
        tzdata->last_packet_extreme_5s[ch].buff[1]=tzdata->last_packet_extreme_5s[ch].buff[extreme_numb-2];
        tzdata->last_packet_extreme_5s[ch].buff[2]=tzdata->last_packet_extreme_5s[ch].buff[extreme_numb-1];
    }
    tzdata->last_packet_extreme_5s[ch].w_index=remain_numb;//要保留的极值个数赋值给last_packet_extreme[ch].w_index

}

/*峰峰值处理*/
static void sigtoampth_5s(struct SW_DIAGNOS_PARA *tzdata, uint32_t ch, uint32_t size)
{
//	uint32_t i;
//	uint32_t ampth_cnt = 0;
	uint32_t numb;

  /*step1. 对新的1包滤波数据--求极值点*/
	numb = reduce_data(tzdata->data_deal_buff_5s[ch].buff, size);

	/*step2. 合并操作：将新的极值点 转移到 老的1包“保留”下来的极值点的后面*/
	copy_data_float(
	tzdata->data_deal_buff_5s[ch].buff,
	&tzdata->last_packet_extreme_5s[ch].buff[tzdata->last_packet_extreme_5s[ch].w_index],
	numb);


	/*step3. 计算“合并数据”的极值点，返回“新的极值点个数”*/
	tzdata->last_packet_extreme_5s[ch].w_index=reduce_data(
	tzdata->last_packet_extreme_5s[ch].buff,
	tzdata->last_packet_extreme_5s[ch].w_index+numb);

	/*step4. 计算峰峰值，保留“可疑极值点”*/
	calc_amp_deal_5s(tzdata,ch);

}

/*峰峰值---接口函数*/
void ampth_deal_5s(struct SW_DIAGNOS_PARA *tzdata, uint32_t ch)
{
	int32_t numb_tmp = 0;

  	numb_tmp = sigtodiff_5s(tzdata, ch);

	if (numb_tmp > 0)
	{
		sigtoampth_5s(tzdata, ch, numb_tmp);
	}
}
#endif
/*************************************************
Function:ring_buff_num_dec
Description: 可诊断行数自减
Input:
Output:
Return:
Others:
*************************************************/
static void ring_buff_num_dec(struct RINGBUFF_PARA *sw_diagnos_ringbuf_para_temp)
{
	sw_diagnos_ringbuf_para_temp->num--;
}

/*************************************************
Function:    pw_diagnos_thread_entry
Description: 自检和秒线程
Input:
Output:
Return:
Others:
*************************************************/

static void init_array_float(struct SWARRAY_FLOAT *psrc,uint32_t size)
{
	psrc->size=size;
	psrc->w_index = 0;
	if(psrc->buff == NULL)
	{
		psrc->buff=(float *)malloc(psrc->size*sizeof(float));
		if(psrc->buff == NULL)
			DEBUG("malloc array float buff err\n");
	}
	if(psrc->buff!=NULL)
	{
		//rt_kprintf ("malloc diag_buff2 ok\r\n");
		psrc->w_index=0u;
		memset(psrc->buff,0,(psrc->size*sizeof(float)));
//		psrc->mem_flag=1u;
	}
	else
	{
		;
//		psrc->mem_flag=0u;

	}
}

/**
 @author:seanlee
 @breif:fft滑动滤波窗的缓存初始化
*/
static void init_filter_para(struct FILTER_STRUCT_FLOAT *psrc,uint32_t filter_order)
{
	psrc->filter_order=filter_order;
	psrc->window_size=psrc->filter_order+1;

	if(psrc->before_filter_buff==NULL)
	{
		psrc->before_filter_buff=(double *)malloc(psrc->window_size*sizeof(double));
		memset(psrc->before_filter_buff,0,psrc->window_size*sizeof(double));
	}
	if(psrc->hs_data==NULL)
	{
		psrc->hs_data=(double *)malloc(psrc->window_size*sizeof(double));
		memset(psrc->hs_data,0,psrc->window_size*sizeof(double));
	}

	if(psrc->before_filter_buff&&psrc->hs_data)
	{
		psrc->mem_flag=1;
	}else
	{
		psrc->mem_flag=0;
	}
}

/*************************************************
Function:    init_pw_diagnos_para
Description: 初始化算法诊断参数
Input:
Output:
Return:
Others:
*************************************************/
void init_sw_diagnos_para()
{
	uint8_t ch_i = 0;
	sw_diagnos_para.sw_alarm_cnt=0;
	sw_diagnos_para.sw_30km_singl=1;
	for(ch_i = 0;ch_i < 2;ch_i ++)
	{
		sw_diagnos_para.deal_num[ch_i] = FS_SW*SWDIAG_SEC;
		sw_diagnos_para.self_test_index[ch_i]=0u;
		sw_diagnos_para.diag_res[ch_i].value=0.0f;
		sw_diagnos_para.diag_res[ch_i].alarm_h_num=0;
		sw_diagnos_para.diag_res[ch_i].alarm_l_num=0;
		sw_diagnos_para.diag_res[ch_i].warn_h_num=0;
		sw_diagnos_para.diag_res[ch_i].warn_l_num=0;
		sw_diagnos_para.diag_res[ch_i].diagnos_alarm_status.bits.alarm_status_new=RUNNING_OK;
		sw_diagnos_para.diag_res[ch_i].diagnos_alarm_status.bits.alarm_status_old=RUNNING_OK;

		sw_diagnos_para.diag_res[ch_i].diagnos_alarm_status.bits.warn_status_new=RUNNING_OK;//add warn
		sw_diagnos_para.diag_res[ch_i].diagnos_alarm_status.bits.warn_status_old=RUNNING_OK;//

		sw_diagnos_para.rms[ch_i]=0.0f;
		sw_diagnos_para.electric_val1[ch_i]=0.0f;
		sw_diagnos_para.electric_val2[ch_i]=0.0f;
		sw_diagnos_para.last_packet_extreme[ch_i].size=sw_diagnos_para.deal_num[ch_i];
		init_array_float(&sw_diagnos_para.last_packet_extreme[ch_i],sw_diagnos_para.deal_num[ch_i]);
//		init_array_float(&sw_diagnos_para.data_deal_buff[ch_i],sw_diagnos_para.deal_num[ch_i]);

		init_array_float(&sw_diagnos_para.data_deal_buff[ch_i],sw_diagnos_para.deal_num[ch_i]);				//算法处理数据缓存
		sw_diagnos_para.sens_para[ch_i].sens_type=SW_Y;
		sw_diagnos_para.sens_para[ch_i].sensitivity=SENSITIVITY_SW;
		sw_diagnos_para.sens_para[ch_i].sens_self_test.bits.self_test_power_on=SENSOR_OK;
		sw_diagnos_para.sens_para[ch_i].sens_self_test.bits.self_test_real_time = SENSOR_OK;

		init_filter_para(&sw_diagnos_para.filter_para[ch_i],FILTER_ORDER);

#ifdef PP_TZZ_DIAG_1S_FILLTER
		init_array_float(&sw_diagnos_para.peak_to_peak_value_5s[ch_i], PP_1S_NUM*TZZ_DIAG_SEC);//5s
		memset(five_sec_pp_size_record, 0, CHANNEL_NUM*sizeof(struct FIVE_SEC_PP_SIZE_RECORD));
#elif defined(PP_TZZ_DIAG_20210420)
		init_array_float(&sw_diagnos_para.data_deal_buff_5s[ch_i], FS_SW*TZZ_DIAG_SEC);//5s
		init_array_float(&sw_diagnos_para.data_deal_buff_bak_5s[ch_i], FS_SW*TZZ_DIAG_SEC);//5s
		init_array_float(&sw_diagnos_para.last_packet_extreme_5s[ch_i], FS_SW*TZZ_DIAG_SEC);//5s
#endif
	}
}

/*************************************************
Function:    average_fft_value
Description: 计算平均频谱
Input:
Output:
Return:
Others:
*************************************************/
static void average_fft_value(struct STABLE_FFT_ARRAY* psrc,float *pdes)
{
	float sum_data = 0.0f;
	uint32_t i=0;
	uint32_t j=0;
	for(i=0;i<psrc->column_size;i++) //列
	{
		sum_data=0.0f;
		for(j=0;j<psrc->row_size;j++) //按行累加
		{
			sum_data+=psrc->buff[j][i];
		}
		pdes[i]=sum_data/psrc->row_size;//计算每种频率的平均幅值
	}
}
/*************************************************
Function:    push_stable_fft_array
Description: 平稳性指标计算
Input:
Output:
Return:
Others:
*************************************************/

static void push_stable_fft_array(struct STABLE_FFT_ARRAY* pdes,float* element)
{

	//rt_kprintf ("one read_fft=%d,row=%d\r\n",(int)pdes ->buff [pdes->row_index][0],pdes->row_index);
	if(pdes->row_index>=pdes->row_size)
	{
		int t;
		for ( t = 0; t < pdes->row_size - 1; t++)
		{
			int size =pdes->column_size * sizeof(float);
			memmove(pdes->buff[t], pdes->buff[t+1], size);
		}
		pdes->row_index--;
	}
	memmove(pdes->buff[pdes->row_index],
	element,
	pdes->column_size*sizeof(float));
	pdes->row_index++;
}


/*fft-下标转频率*/
static float fft_index2hz(uint32_t fft_pt,uint32_t sample_hz,uint32_t index)
{
	float hz;
	hz= (float)sample_hz / fft_pt * index;//大于0.5
	return hz;
}

/*fft-频率转下标*/
static uint32_t fft_hz2index(uint32_t fft_pt,uint32_t sample_hz,float hz)
{
	uint32_t index;
	index = (uint32_t)hz*fft_pt/sample_hz;
	//hz= (float)sample_hz / fft_pt * index;//大于0.5

	return index;
}

/**
	@author:seanlee
	@breif:冒泡排序
*/
void bubble_sort(float *data,uint32_t length)
{
	uint32_t i,j,cnt;
	float temp;
	cnt = length ;
	if(length<=0)
		return;
	for(i=1;i<length;i++)
	{
		cnt-=1;
		for(j=0;j<cnt;j--)
		{
			if(data[j]>data[j+1])
			{
				temp=data[j];
				data[j]=data[j+1];
				data[j+1]=temp;
			}
		}
	}
}

//float ad_convert_acceleration(float value,float sensitivity)
//{
//	return value*20.0f/65536.0f/sensitivity;
//}

/**
 @author:seanlee
 @breif:冲击计算
 */
//void impulse_deal(struct SW_DIAGNOS_PARA *tzdata,uint32_t ch)
//{
//	struct ARRAY_FLOAT *array=&tzdata->data_deal_buff[ch];
//	{
//		uint32_t i;
//		float delta_y;
//		float delta_x=1.0f/FS_SW;
//
//		/*两两计算冲动值，结果放在array->buff里面，size---->size-1*/
//		for(i=0;i<array->size-1;i++)
//		{
//			delta_y=fabs(array->buff[i]-array->buff[i+1]);
//			array->buff[i]=delta_y/delta_x;
//		}
//
//		/*冒泡排序*/
//		bubble_sort(array->buff,array->size-1);
//
//		/*冲动计算的结果以95%的排序最大值为准,抛掉2.5%的最小值和2.5%最大值置信区间*/
//		i=(uint32_t)((array->size-1)*0.975f);//array->buff[i]
//		tzdata->diag_res[ch].value = ad_convert_acceleration(array->buff[i])*GRAVITY_PARA;
//		//tzdata->Eig_value [ch].Sta_norm = tzdata->diag_res[ch].value;					//得到平稳性指标
//	}
//}


static void ad_filter(float *buff,uint32_t size,uint8_t ch_num)
{
	uint16_t cnt = 0;
	for(cnt = 0;cnt < size;cnt ++)
	{
		if(buff[cnt] > 40960 && buff[cnt] < 44237)
		{
			f_buff_outsw[ch_num] = buff[cnt];
		}
		else
		{
			buff[cnt] = f_buff_outsw[ch_num];
		}
	}
}


uint8_t ad_valid_check(float *buff, uint32_t size)
{
	float sum_ad = 0.0f;
	float avg_ad = 0.0f;
	uint16_t i = 0;
	uint8_t invalid_cnt = 0;
	uint8_t valid_flag = 0;

	while(size)
	{
		for(i=0; i<AD_VALID_CHECK_SIZE; i++)
		{
			sum_ad = sum_ad + GRAVITY_PARA * AD_CONVERT_ACCELERATION(buff[i]);
		}

		avg_ad = sum_ad/AD_VALID_CHECK_SIZE;
		//printf("ad_valid_check-----------avg_ad: %f\n", avg_ad);

		if(fabs(avg_ad) > ACCELERATION_OFFSET_THRESHOLD)
		{
			invalid_cnt++;
		}

		size -= AD_VALID_CHECK_SIZE;
	}

	if(invalid_cnt >= AD_INVALID_TIMES)
	{
		valid_flag = 0;//invalid
	}
	else
	{
		valid_flag = 1;//valid
	}

	//printf("ad_valid_check-----------valid_flag: %d\n", valid_flag);

	return valid_flag;
}
/*************************************************
Function:    diag_deal_sw
Description: 传感器单个通道数据诊断程序
Input:
Output:
Return:
Others:
*************************************************/

void diag_deal_sw(struct SW_DIAGNOS_PARA *tzdata,uint8_t ch_num)
{
//	static uint16_t h_num;
//	float threshold_val=0.0f;
//	float ad_vol;
	float max_ampth;
//	uint8_t alarm_data[2] = {0};
//	uint32_t HN=0;
//	uint32_t LN=0;

	/*****TEST****/
//	uint16_t data_cnt = 0;
//	int write_res = -1;
//	char test_buff[10] = {0};

#ifndef AD7606_COMPLEMENTARY_TO_ORIGINAL
	/**************滤波*******************/
	ad_filter(tzdata->data_deal_buff[ch_num].buff,tzdata->data_deal_buff[ch_num].size,ch_num);
#endif

	/***** SENSOR REAL TIME SELFTEST***/
//	sensor_fault_judge_sw(tzdata ,ch_num);


	/*step1.均值处理  零点处理*/
	float_sub_mean(tzdata->data_deal_buff[ch_num].buff,tzdata->data_deal_buff[ch_num].size);

//	for(int i = 100;i<110;i++ )
//		printf("1111111111  tzdata->data_deal_buff[%d].buff = %f \n",ch_num,tzdata->data_deal_buff[ch_num].buff[i]);
//	printf("\n");

#if defined(DIAG_DATA_SAVE_FOR_TEST)&&defined(PP_TZZ_DIAG_1S_FILLTER)
	if(ch_num == 0)
	{
		save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_BEFORE_FILTER_TITLE, NULL, 1);
		save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_BEFORE_FILTER, &tzdata->data_deal_buff[ch_num].buff[0], tzdata->data_deal_buff[ch_num].size);
	}
#endif

	/*step2 滤波处理*/
	swfilter_deal(&tzdata->data_deal_buff[ch_num],&tzdata->filter_para[ch_num]);


//	for(int i = 100;i<110;i++ )
//			printf("22222  tzdata->data_deal_buff[%d].buff = %f \n",ch_num,tzdata->data_deal_buff[ch_num].buff[i]);
//	printf("\n");

#if defined(DIAG_DATA_SAVE_FOR_TEST)&&defined(PP_TZZ_DIAG_1S_FILLTER)
	if(ch_num == 0)
	{
		save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_AFTER_FILTER_TITLE, NULL, 1);
		save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_AFTER_FILTER, &tzdata->data_deal_buff[ch_num].buff[0], tzdata->data_deal_buff[ch_num].size);
	}
#endif
//#ifdef PP_TZZ_DIAG_20210420 //若5s滤波算不过来才在此处增加代码．
////5s (1s滑移)
//	memmove(tzdata->data_deal_buff_5s[ch_num], tzdata->data_deal_buff[ch_num], );
//#endif



//	printf("tzdata->sw_30km_singl:%d,ch_num:%d\n",tzdata->sw_30km_singl,ch_num);

	sw_data_save_type = SENSOR_WORKIONG_DATA;

	float speed = 0.0;
	speed = ((app_save_public.speed[0]<<0x8) + app_save_public.speed[1]) * 0.01;

//	printf("### speed = %f\n",speed);
	if(tzdata->sens_para[ch_num].sens_type==SW_Y && (speed > 30) )
//	if(tzdata->sens_para[ch_num].sens_type==SW_Y)
	{
		/*step3.计算峰峰值*/
	  swampth_deal(tzdata, ch_num);

#if 0
	  if(ch_num == 0)
	  {
		  printf("ampth_data--%d :",tzdata->data_deal_buff[ch_num].w_index);

		  for(int i=0;i<15;i++)
		  {
			  printf("%f  ",tzdata->data_deal_buff[ch_num].buff[i]);
		  }
		  printf("\n");
	  }
#endif

	  	  uint32_t i;
		float threshold_val=0.0f;
		uint32_t HN=0;
		uint32_t LN=0;

		if(ch_num == 0)
		{
			//pw_clb_config_para 名字不重要 暂时写pw 但是是总体结构 里面带失稳
			threshold_val = pw_clb_config_para->sw_diagnos_para.side1_y_para.alarm_value;
			HN = pw_clb_config_para->sw_diagnos_para.side1_y_para.h_cnt;
			LN = pw_clb_config_para->sw_diagnos_para.side1_y_para.l_cnt;
		}
		else if(ch_num == 1)
		{
			threshold_val = pw_clb_config_para->sw_diagnos_para.side2_y_para.alarm_value;
			HN = pw_clb_config_para->sw_diagnos_para.side2_y_para.h_cnt;
			LN = pw_clb_config_para->sw_diagnos_para.side2_y_para.l_cnt;
		}
//		else if(ch_num == 2)
//		{
//			threshold_val = pw_clb_config_para->sw_diagnos_para.side3_y_para.alarm_value;
//			HN = pw_clb_config_para->sw_diagnos_para.side3_y_para.h_cnt;
//			LN = pw_clb_config_para->sw_diagnos_para.side3_y_para.l_cnt;
//		}
//		else if(ch_num == 3)
//		{
//			threshold_val = pw_clb_config_para->sw_diagnos_para.side4_y_para.alarm_value;
//			HN = pw_clb_config_para->sw_diagnos_para.side4_y_para.h_cnt;
//			LN = pw_clb_config_para->sw_diagnos_para.side4_y_para.l_cnt;
//		}

		DEBUG("tzdata->data_deal_buff[%d].w_index %d \n", ch_num, tzdata->data_deal_buff[ch_num].w_index);
		for(i=0;i<tzdata->data_deal_buff[ch_num].w_index;i++)			//
		{
			/*tzdata->data_deal_buff[ch_num].buff[i]中的值为峰峰值*/ //保持原样GRAVITY_PARA *
		#ifdef PP_TZZ_DIAG_1S_FILLTER
			tzdata->data_deal_buff[ch_num].buff[i] = AD_CONVERT_VOLTAGE(tzdata->data_deal_buff[ch_num].buff[i])/SENSITIVITY_SW;//去均值已将零点电压3V去掉,不再减3V,/SENSITIVITY_SW转g
			tzdata->diag_res[ch_num].value = tzdata->data_deal_buff[ch_num].buff[i];
		#else
			tzdata->diag_res[ch_num].value = AD_CONVERT_ACCELERATIONSW(tzdata->data_deal_buff[ch_num].buff[i]);
		#endif

			DEBUG("tzdata->diag_res[%d].value:%f\n", ch_num, tzdata->diag_res[ch_num].value);
			/*warn */
			if(tzdata->diag_res[ch_num].value>1.3)
			{
				tzdata->diag_res[ch_num].warn_l_num=0;
				tzdata->diag_res[ch_num].warn_h_num++;
//				printf("tzdata->diag_res[%d].warn_h_num:%d\n", ch_num ,tzdata->diag_res[ch_num].warn_h_num);
				if(tzdata->diag_res[ch_num].warn_h_num>10)			//暂时屏蔽
				{
					tzdata->diag_res[ch_num].diagnos_alarm_status.bits.warn_status_new=SWRUNNING_ALARM;			//根据协议,传感器预警，标志位也置1
					sw_data_save_type = SENSOR_WARN_DATA;
					if(tzdata->diag_res[ch_num].diagnos_alarm_status.bits.warn_status_new!=tzdata->diag_res[ch_num].diagnos_alarm_status.bits.warn_status_old)
					{
						tzdata->diag_res[ch_num].diagnos_alarm_status.bits.warn_status_old=tzdata->diag_res[ch_num].diagnos_alarm_status.bits.warn_status_new;
						if(tzdata->diag_res[ch_num].diagnos_alarm_status.bits.warn_status_new==SWRUNNING_ALARM)
						{
							if(sys_status_cnt.swerr_type[ch_num].diag_warn_save_flag == 0)
							{
								sys_status_cnt.swerr_type[ch_num].diag_warn_save_flag = 1;
								sys_status_cnt.swerr_type[ch_num].diag_normal_save_flag = 0;
							}
					printf("--------------------->SW WARN!<-----------------ch_num:%d,tzdata->diag_res[ch_num].warn_h_num:%d\n",ch_num,tzdata->diag_res[ch_num].warn_h_num);
						}
					}
				}

			}//tzdata->diag_res[ch_num].alarm_h_num
			else
			{
				tzdata->diag_res[ch_num].warn_h_num=0;
				tzdata->diag_res[ch_num].warn_l_num++;
				if(tzdata->diag_res[ch_num].warn_l_num>6)
				{
					tzdata->diag_res[ch_num].diagnos_alarm_status.bits.warn_status_new=RUNNING_OK;
					if(tzdata->diag_res[ch_num].diagnos_alarm_status.bits.warn_status_new!=tzdata->diag_res[ch_num].diagnos_alarm_status.bits.warn_status_old)
					{
						tzdata->diag_res[ch_num].diagnos_alarm_status.bits.warn_status_old = tzdata->diag_res[ch_num].diagnos_alarm_status.bits.warn_status_new;

						if(sys_status_cnt.swerr_type[ch_num].diag_normal_save_flag == 0)
						{
							sys_status_cnt.swerr_type[ch_num].diag_alarm_save_flag = 0;
							sys_status_cnt.swerr_type[ch_num].diag_normal_save_flag = 1;
						}
						printf ("--------------------->SW WARN OK!<-----------------ch_num:%d,tzdata->diag_res[ch_num].warn_l_num:%d\n",ch_num,tzdata->diag_res[ch_num].warn_l_num);
					}
				}
			}

			if(tzdata->diag_res[ch_num].value>1.6)
			{
				tzdata->diag_res[ch_num].alarm_l_num=0;
				tzdata->diag_res[ch_num].alarm_h_num++;
//				printf("tzdata->diag_res[%d].alarm_h_num:%d\n",ch_num,tzdata->diag_res[ch_num].alarm_h_num);
				if(tzdata->diag_res[ch_num].alarm_h_num>10)			//暂时屏蔽
				{
					tzdata->diag_res[ch_num].diagnos_alarm_status.bits.alarm_status_new=SWRUNNING_ALARM;
					sw_data_save_type = SENSOR_ALARM_DATA;
					if(tzdata->diag_res[ch_num].diagnos_alarm_status.bits.alarm_status_new!=tzdata->diag_res[ch_num].diagnos_alarm_status.bits.alarm_status_old)
					{
						tzdata->diag_res[ch_num].diagnos_alarm_status.bits.alarm_status_old=tzdata->diag_res[ch_num].diagnos_alarm_status.bits.alarm_status_new;
						if(tzdata->diag_res[ch_num].diagnos_alarm_status.bits.alarm_status_new==SWRUNNING_ALARM)
						{
							tzdata->sw_alarm_cnt++;
							if(sys_status_cnt.swerr_type[ch_num].diag_alarm_save_flag == 0)
							{
								sys_status_cnt.swerr_type[ch_num].diag_alarm_save_flag = 1;
								sys_status_cnt.swerr_type[ch_num].diag_normal_save_flag = 0;
							}
							printf("--------------------->SW ALARM!<-----------------ch_num:%d,tzdata->diag_res[ch_num].alarm_h_num:%d\n",ch_num,tzdata->diag_res[ch_num].alarm_h_num);
						}
					}
				}
			}
			else
			{
				tzdata->diag_res[ch_num].alarm_h_num=0;
				tzdata->diag_res[ch_num].alarm_l_num++;
				if(tzdata->diag_res[ch_num].alarm_l_num>6)
				{
					tzdata->diag_res[ch_num].diagnos_alarm_status.bits.alarm_status_new=SWRUNNING_OK;
					if(tzdata->diag_res[ch_num].diagnos_alarm_status.bits.alarm_status_new!=tzdata->diag_res[ch_num].diagnos_alarm_status.bits.alarm_status_old)
					{
						tzdata->diag_res[ch_num].diagnos_alarm_status.bits.alarm_status_old = tzdata->diag_res[ch_num].diagnos_alarm_status.bits.alarm_status_new;
						if(sys_status_cnt.swerr_type[ch_num].diag_normal_save_flag == 0)
						{
							sys_status_cnt.swerr_type[ch_num].diag_alarm_save_flag = 0;
							sys_status_cnt.swerr_type[ch_num].diag_normal_save_flag = 1;
						}
							printf ("--------------------->SW ALARM OK!<-----------------ch_num:%d,tzdata->diag_res[ch_num].alarm_l_num:%d\n",ch_num,tzdata->diag_res[ch_num].alarm_l_num);
					}

				}
			}

		}			//最大的峰峰值转换为加速度

//		printf("55555555\n");
		find_max_ampth(tzdata->data_deal_buff[ch_num].buff, tzdata->data_deal_buff[ch_num].w_index, &max_ampth);
		//printf("find_max_ampth=%f \n",max_ampth);
		find_max_value(tzdata, ch_num, max_ampth, tzdata->diag_res[ch_num].alarm_h_num);
//		printf("999999999\n");
	#if defined(DIAG_DATA_SAVE_FOR_TEST) && defined(PP_TZZ_DIAG_1S_FILLTER)
		if(ch_num == 0 && tzdata->data_deal_buff[ch_num].w_index > 0)
		{
			save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_FFZ_TITLE, NULL, 1);
			save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_FFZ, &tzdata->data_deal_buff[ch_num].buff[0], tzdata->data_deal_buff[ch_num].w_index);
		}
	#endif

//	#ifdef PP_TZZ_DIAG_1S_FILLTER
//		/*1.装载连续5s的峰峰值（已转换为加速度g）*/
//		if(tzdata->peak_to_peak_value_5s[ch_num].w_index == tzdata->peak_to_peak_value_5s[ch_num].size)//防越界
//			return;
//
//		if(tzdata->peak_to_peak_value_5s[ch_num].w_index + tzdata->data_deal_buff[ch_num].w_index > tzdata->peak_to_peak_value_5s[ch_num].size)//防越界
//		{
//			uint16_t temp_size=0;
//
//			temp_size = tzdata->peak_to_peak_value_5s[ch_num].size - tzdata->peak_to_peak_value_5s[ch_num].w_index;
//
//			if(temp_size > 0)
//			{
//				memmove(tzdata->peak_to_peak_value_5s[ch_num].buff + tzdata->peak_to_peak_value_5s[ch_num].w_index, tzdata->data_deal_buff[ch_num].buff, temp_size);//5s //PP_1S_NUM*TZZ_DIAG_SEC
//				tzdata->peak_to_peak_value_5s[ch_num].w_index = tzdata->peak_to_peak_value_5s[ch_num].size;
//			}
//
//			five_sec_pp_size_record[ch_num].per_sec_num[five_sec_pp_size_record[ch_num].sec_cnt] = temp_size;
//		}
//		else
//		{
//			memmove(tzdata->peak_to_peak_value_5s[ch_num].buff + tzdata->peak_to_peak_value_5s[ch_num].w_index, tzdata->data_deal_buff[ch_num].buff, tzdata->data_deal_buff[ch_num].w_index);//5s //PP_1S_NUM*TZZ_DIAG_SEC
//			tzdata->peak_to_peak_value_5s[ch_num].w_index += tzdata->data_deal_buff[ch_num].w_index;
//
//			five_sec_pp_size_record[ch_num].per_sec_num[five_sec_pp_size_record[ch_num].sec_cnt] = tzdata->data_deal_buff[ch_num].w_index;
//		}
//	#endif
	}

	tzdata->diag_res[ch_num].diagnos_alarm_status.bits.alarm_status_old=tzdata->diag_res[ch_num].diagnos_alarm_status.bits.alarm_status_new;
}

#ifdef PP_TZZ_DIAG_1S_FILLTER
uint16_t sec_cnt[CHANNEL_NUM];
void diag_peak_peak_tzz(struct SW_DIAGNOS_PARA *tzdata,uint8_t ch_num)
{
	uint16_t i = 0;
	uint16_t cnt = 0;
	float ffz_min_buf[TEN_PP_MAX_NUM] = {0.0};
	float ffz_mean_buf[TEN_PP_MAX_NUM] = {0.0};//若高频没完全滤掉，1s的数据按2－10Hz滤最多30个峰峰值，最多30＊5s=150,最大取150，10个峰峰值(1个峰峰值滑移)出一个最小或平均值，得最多150-10+1个最小或平均值

	if((tzdata->sens_para[ch_num].sens_type==SW_Y)&&(tzdata->sw_30km_singl==1))
	{
		five_sec_pp_size_record[ch_num].sec_cnt++;

		/*最小、均值特征值算法---5s数据处理*/
		if(five_sec_pp_size_record[ch_num].sec_cnt >= TZZ_DIAG_SEC)
		{
	#ifdef DIAG_DATA_SAVE_FOR_TEST
			if(ch_num == 0 && tzdata->peak_to_peak_value_5s[ch_num].w_index > 0)
			{
				save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_5S_FFZ_TITLE, NULL, 1);
				save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_5S_FFZ, &tzdata->peak_to_peak_value_5s[ch_num].buff[0], tzdata->peak_to_peak_value_5s[ch_num].w_index);
			}
	#endif
			for(i=0; i<tzdata->peak_to_peak_value_5s[ch_num].w_index; i++)
			{
				/*峰峰值转加速度g, 1s计算diag_deal_sw中已转g*/

				/*2.求10个峰峰值中最小值,平均值, 一个整波峰峰值滑移*/
				if(i>=CALC_PP_NUM-1)
				{
					ffz_min_buf[cnt] = float_get_min(&tzdata->peak_to_peak_value_5s[ch_num].buff[i-(CALC_PP_NUM-1)], CALC_PP_NUM);//i-9实现一个峰峰值滑移.

					ffz_mean_buf[cnt] = float_mean(&tzdata->peak_to_peak_value_5s[ch_num].buff[i-(CALC_PP_NUM-1)], CALC_PP_NUM);//i-9实现一个峰峰值滑移.

					cnt++;
				}
			}

			/*3.最小值中的最大值/2*/
			tzdata->ffz_min_tzz[ch_num] = float_get_max(ffz_min_buf, cnt)/2.0;//单位：g

			/*4.平均值中的最大值/2*/
			tzdata->ffz_mean_tzz[ch_num] = float_get_max(ffz_mean_buf, cnt)/2.0;

	#ifdef DIAG_DATA_SAVE_FOR_TEST
			if(ch_num == 0)
			{
				save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_MIN_TZZ_TITLE, NULL, 1);
				save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_MIN_TZZ, &tzdata->ffz_min_tzz[ch_num], 1);

				save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_MEAN_TZZ_TITLE, NULL, 1);
				save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_MEAN_TZZ, &tzdata->ffz_mean_tzz[ch_num], 1);
			}
	#endif

			/*5.1s滑移*/
			if(five_sec_pp_size_record[ch_num].per_sec_num[0] > 0)
			{
				copy_data_float(&tzdata->peak_to_peak_value_5s[ch_num].buff[0],
								&tzdata->peak_to_peak_value_5s[ch_num].buff[five_sec_pp_size_record[ch_num].per_sec_num[0]],
								tzdata->peak_to_peak_value_5s[ch_num].w_index-five_sec_pp_size_record[ch_num].per_sec_num[0]);

				tzdata->peak_to_peak_value_5s[ch_num].w_index -= five_sec_pp_size_record[ch_num].per_sec_num[0];
			}

			memmove(&five_sec_pp_size_record[ch_num].per_sec_num[0], &five_sec_pp_size_record[ch_num].per_sec_num[1], (TZZ_DIAG_SEC-1)*sizeof(uint16_t));
			five_sec_pp_size_record[ch_num].sec_cnt--;
		}
	}
}
#elif defined(PP_TZZ_DIAG_20210420)
void diag_peak_peak_tzz(struct SW_DIAGNOS_PARA *tzdata,uint8_t ch_num)
{
	uint32_t i = 0;
	uint16_t cnt = 0;
	float ffz_min_buf[TEN_PP_MAX_NUM] = {0.0};
	float ffz_mean_buf[TEN_PP_MAX_NUM] = {0.0};//若高频没完全滤掉，1s的数据按2－10Hz滤最多30个峰峰值，最多30＊5s=150,取150，10个峰峰值出一个最小或平均值，得15个最小或平均值,buffer扩大2倍即30

#ifndef AD7606_COMPLEMENTARY_TO_ORIGINAL
	/*1.滤波 范围限制*/
	ad_filter(tzdata->data_deal_buff_5s[ch_num].buff,tzdata->data_deal_buff_5s[ch_num].size,ch_num);
#endif

	/*2.去均值处理  零点处理*/
	float_sub_mean(tzdata->data_deal_buff_5s[ch_num].buff,tzdata->data_deal_buff_5s[ch_num].size);

#ifdef DIAG_DATA_SAVE_FOR_TEST
	if(ch_num == 0)
	{
		save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_BEFORE_FILTER_TITLE, NULL, 1);
		save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_BEFORE_FILTER, &tzdata->data_deal_buff_5s[ch_num].buff[0], tzdata->data_deal_buff_5s[ch_num].size);
	}
#endif

	/*3.滤波处理*/ //若算不过来则用fft-ifft变换滤波.
	filter_deal(&tzdata->data_deal_buff_5s[ch_num],&tzdata->filter_para[ch_num]);//8阶

#ifdef DIAG_DATA_SAVE_FOR_TEST
	if(ch_num == 0)
	{
		save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_AFTER_FILTER_TITLE, NULL, 1);
		save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_AFTER_FILTER, &tzdata->data_deal_buff_5s[ch_num].buff[0], tzdata->data_deal_buff_5s[ch_num].size);
	}
#endif

	if(tzdata->sw_30km_singl==1)
	{
		/*4.计算峰峰值*/
		ampth_deal_5s(tzdata, ch_num);

		printf("diag_peak_peak_tzz---ffz_num:%d\n", tzdata->data_deal_buff_5s[ch_num].w_index);

#ifdef DIAG_DATA_SAVE_FOR_TEST
		if(ch_num == 0)
		{
			save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_FFZ_TITLE, NULL, 1);
			save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_FFZ, &tzdata->data_deal_buff_5s[ch_num].buff[0], tzdata->data_deal_buff_5s[ch_num].size);
		}
#endif

		for(i=0; i<tzdata->data_deal_buff_5s[ch_num].w_index; i++)			//
		{
			/*5.峰峰值转加速度g, GRAVITY_PARA * */
			tzdata->data_deal_buff_5s[ch_num].buff[i] = AD_CONVERT_VOLTAGE(tzdata->data_deal_buff_5s[ch_num].buff[i])/SENSITIVITY_SW;

			/*6.求10个峰峰值中最小值,平均值, 一个整波峰峰值滑移*/
			if(i>=9)
			{
				ffz_min_buf[cnt] = float_get_min(&tzdata->data_deal_buff_5s[ch_num].buff[i-9], 10);//i-9实现一个峰峰值滑移.

				ffz_mean_buf[cnt] = float_mean(&tzdata->data_deal_buff_5s[ch_num].buff[i-9], 10);//i-9实现一个峰峰值滑移.

				cnt++;

				if(cnt>=TEN_PP_MAX_NUM)//防止高频未完全滤除导致点数意外的多,以致越界
					break;
			}
		}

#ifdef DIAG_DATA_SAVE_FOR_TEST
		if(ch_num == 0)
		{
			save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_FFZ_TO_ACC_TITLE, NULL, 1);
			save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_FFZ_TO_ACC, &tzdata->data_deal_buff_5s[ch_num].buff[0], tzdata->data_deal_buff_5s[ch_num].w_index);
		}
#endif

		/*7.最小值中的最大值/2*/
		tzdata->ffz_min_tzz[ch_num] = float_get_max(ffz_min_buf, cnt)/2.0;

		/*8.平均值中的最大值/2*/
		tzdata->ffz_mean_tzz[ch_num] = float_get_max(ffz_mean_buf, cnt)/2.0;

		printf("diag_peak_peak_tzz---ch:%d, ffz_min_tzz:%f, ffz_mean_tzz:%f\n", ch_num, tzdata->ffz_min_tzz[ch_num], tzdata->ffz_mean_tzz[ch_num]);

#ifdef DIAG_DATA_SAVE_FOR_TEST
		if(ch_num == 0)
		{
			save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_MIN_TZZ_TITLE, NULL, 1);
			save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_MIN_TZZ, &tzdata->ffz_min_tzz[ch_num], 1);

			save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_MEAN_TZZ_TITLE, NULL, 1);
			save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_MEAN_TZZ, &tzdata->ffz_mean_tzz[ch_num], 1);
		}
#endif

	}/*end if*/

}
#endif

void move_adbuf_to_diagbuf(float *diagbuf,uint16_t *adbuf,uint16_t date_size)
{
	//数据赋值给处理缓存
	uint16_t cnt_i;
	for(cnt_i = 0;cnt_i < date_size;cnt_i ++)
	{
		diagbuf[cnt_i] = adbuf[cnt_i];
	}
}
/*************************************************
Function:    data_handling_pw
Description: AD数据诊断
Input:
Output:
Return:
Others:
*************************************************/
static void data_handling_sw(struct SW_DIAGNOS_PARA *tzdata, uint16_t *ad_buf, uint16_t *sw_ad_value,struct SW_TZ_DATA *tzdata_save,uint32_t data_size, uint32_t ch_numb)
{
//	uint32_t i_segment;
	uint8_t ch; //通道id
//	uint8_t alarm_data[3];

	memmove(tzdata->ad_value,sw_ad_value,512*2);

//	printf("ch_numb:%d\n",ch_numb);
	for(ch = 0;ch < ch_numb;ch ++)
	{
		if(lastdatasw[ch]==0)
		{
			lastdatasw[ch]=uint16_mean(ad_buf,512);
//			printf("lastdatasw[%d]:%d\n",ch,lastdatasw[ch]);
		}

		//1s数据处理
		move_adbuf_to_diagbuf(&tzdata->data_deal_buff[ch].buff[tzdata->data_deal_buff[ch].w_index],ad_buf,data_size);

		tzdata->data_deal_buff[ch].w_index = tzdata->data_deal_buff[ch].w_index + data_size;

//		printf ("tzdata->data_deal_buff[ch].w_index:%d,tzdata->data_deal_buff[ch].size:%d,ch:%d\n",tzdata->data_deal_buff[ch].w_index,tzdata->data_deal_buff[ch].size,ch);
		DEBUG ("tzdata->data_deal_buff[ch].w_index:%d,tzdata->data_deal_buff[ch].size:%d,ch:%d\n",tzdata->data_deal_buff[ch].w_index,tzdata->data_deal_buff[ch].size,ch);
		if(tzdata->data_deal_buff[ch].w_index == tzdata->data_deal_buff[ch].size)//FS_SW*DIAG_SEC
		{
//			printf("tzdata->data_deal_buff[%d].w_index = :%d\n",ch,tzdata->data_deal_buff[ch].w_index);
			if(self_test_para.self_test_flag)//满足自检条件
			{
				//传感器自检
				printf("---------------->SW   sensor_self_test<----------\n");
				swsensor_self_test(tzdata,ch);

			}
			else
			{
				if(ad_valid_check(tzdata->data_deal_buff[ch].buff, tzdata->data_deal_buff[ch].size))
				{
					sensor_fault_judge_sw(tzdata ,ch);
	//				printf("---------------->diag_deal_sw<--------------\n");
					/*转向架失稳算法---1s数据处理*/
					diag_deal_sw(tzdata, ch);//当数据达到诊断个数的时候，逐个通道依次诊断
					DEBUG("---------------->diag_peak_peak_tzz<---------\n");
					/*最小、均值特征值算法---5s数据处理*/
					diag_peak_peak_tzz(tzdata, ch);
				}
				else
				{
					tzdata->data_deal_buff[ch].w_index = 0;
					memset(tzdata->data_deal_buff[ch].buff, 0x00,  tzdata->data_deal_buff[ch].size * sizeof(float));

					tzdata->peak_to_peak_value_5s[ch].w_index = 0;
					memset(tzdata->peak_to_peak_value_5s[ch].buff, 0x00,  tzdata->peak_to_peak_value_5s[ch].size * sizeof(float));

					memset(&five_sec_pp_size_record[ch], 0x00, sizeof(struct FIVE_SEC_PP_SIZE_RECORD));
				}

			}

			tzdata->data_deal_buff[ch].w_index=0u;
		}

		//5s数据处理
	#ifdef PP_TZZ_DIAG_1S_FILLTER
	#elif defined(PP_TZZ_DIAG_20210420)
		move_adbuf_to_diagbuf(&tzdata->data_deal_buff_5s[ch].buff[tzdata->data_deal_buff_5s[ch].w_index],ad_buf,data_size);
		move_adbuf_to_diagbuf(&tzdata->data_deal_buff_bak_5s[ch].buff[tzdata->data_deal_buff_bak_5s[ch].w_index],ad_buf,data_size);

		tzdata->data_deal_buff_5s[ch].w_index = tzdata->data_deal_buff_5s[ch].w_index + data_size;
		tzdata->data_deal_buff_bak_5s[ch].w_index = tzdata->data_deal_buff_bak_5s[ch].w_index + data_size;

		if(tzdata->data_deal_buff_5s[ch].w_index == tzdata->data_deal_buff_5s[ch].size)
		{

			if(!self_test_para1.self_test_flag)
			{
			#ifdef DIAG_DATA_SAVE_FOR_TEST
				if(ch == 0)
				{
					float speed = ((sw_tz_data.train_public_info.speed[0]<<0x8) + sw_tz_data.train_public_info.speed[1]) * 0.01;

					//printf("speed---%6.2f\n", speed);

					save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_CHANNEL_TITLE, NULL, 1);
					save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_CHANNEL, &ch, 1);

					save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_SPEED_TITLE, NULL, 1);
					save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_SPEED, &speed, 1);

					save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_AD_TITLE, NULL, 1);
					save_swtzz_diag_file(SW_TZZ_TYPE, DIAG_DATA_AD, &tzdata->data_deal_buff_5s[ch].buff[0], tzdata->data_deal_buff_5s[ch].size);
				}
			#endif

				printf("---------------->diag_peak_peak_tzz<---------\n");
				/*最小、均值特征值算法---5s数据处理*/
				diag_peak_peak_tzz(tzdata, ch);

			#ifdef DIAG_DATA_SAVE_FOR_TEST
				if(ch == 0)
				{
					close_swtzz_diag_file(SW_TZZ_TYPE);
				}
			#endif

				//-----------------------------------------{将5s数据后4s移到前4s,实现1s滑移.
				tzdata->data_deal_buff_5s[ch].w_index = 0u;
				tzdata->data_deal_buff_bak_5s[ch].w_index = 0u;

				copy_data_float(&tzdata->data_deal_buff_5s[ch].buff[tzdata->data_deal_buff_5s[ch].w_index],
						&tzdata->data_deal_buff_bak_5s[ch].buff[tzdata->data_deal_buff_bak_5s[ch].w_index+data_size], data_size*4);

				copy_data_float(&tzdata->data_deal_buff_bak_5s[ch].buff[tzdata->data_deal_buff_bak_5s[ch].w_index],
						&tzdata->data_deal_buff_bak_5s[ch].buff[tzdata->data_deal_buff_bak_5s[ch].w_index+data_size], data_size*4);

				tzdata->data_deal_buff_5s[ch].w_index = tzdata->data_deal_buff_5s[ch].w_index + data_size*4;
				tzdata->data_deal_buff_bak_5s[ch].w_index = tzdata->data_deal_buff_bak_5s[ch].w_index + data_size*4;
				//-----------------------------------------}

			#ifdef WTD_DATA_TRANSLATE_PROTOCOL
				if(ch == ch_numb-1 && first_diag_result_flag==0)//首次所有通道算法完，标志置置1，以启动发送WTD数据
					first_diag_result_flag = 1;
			#endif
			}
		}

	#endif

		ad_buf = ad_buf + data_size;//指向下一通道数据
	}

	/*采集板卡故障判断*/
//	judge_sw_board_err(tzdata->ad_value,FS_SW,&tzdata->sw_board_err);

}

/*************************************************
Function:    pw_diagnos_thread_entry
Description: 自检和秒线程
Input:
Output:
Return:
Others:
*************************************************/
void sw_diagnos_thread_entry()
{
	int sem_res = -1;

	uint16_t data_cnt = 0;
	uint16_t orignal_data_head[4] = {0xAA51,0xAA52,0xAA53,0xAA54};

//	uint16_t dest[512] = {0};


	f_buff_outsw[0] = 42598.0;				//滤波赋初值
	f_buff_outsw[1] = 42598.0;				//滤波赋初值
	f_buff_outsw[2] = 42598.0;				//滤波赋初值
	f_buff_outsw[3] = 42598.0;				//滤波赋初值

	sem_res = sem_init(&sw_diagnos_sem,0,0);
	if(sem_res == -1)
	{
		DEBUG ("init sw_diagnos_sem error!\n");
	}

	while(1)
	{
		sem_wait(&sw_diagnos_sem);

		if(sw_diagnos_ringbuf_para.num > sw_diagnos_ringbuf_para.size)
		{
			//诊断行数大于数据总行数处理
			lost_old_data_deal(&sw_diagnos_ringbuf_para,&sw_ad_data);
		}

		while(sw_diagnos_ringbuf_para.num)
		{

//			printf("-------------sw_diagnos_thread_entry-------------");
			//*(uint16_t *)sw_tz_data.train_public_info.speed = little_to_big_16bit(sw_ad_data.buff[sw_diagnos_ringbuf_para.index][DATA_HEAD_SIZE-1]);//SPEED

			data_handling_sw(&sw_diagnos_para,&sw_ad_data.buff[sw_diagnos_ringbuf_para.index][DATA_HEAD_SIZE],sw_ad_data.ad_chnnel5,&sw_tz_data,FS_SW,SWCHANNEL_NUM);
			//printf("-------->after data_handling_sw<----\n");
			//更新生命信号
			*(uint16_t *)&sw_tz_data.data_head.life_signal[0] = sw_ad_data.buff[sw_diagnos_ringbuf_para.index][0];

			update_sw_tz_data(&sw_tz_data,&sw_diagnos_para,UPDATE_DIAG_DATE);

			//保存特征数据
			save_sw_tz_data(&sw_tz_data,sw_ad_data.buff[sw_diagnos_ringbuf_para.index][0],udp_send_addr.board_set,0x55,pw_clb_config_para->sw_send_mcast_addr.is_need_ack);
			//存储并发送原始数据

			for(data_cnt = 0;data_cnt < SWCHANNEL_NUM;data_cnt++)
			{
//				memset(dest,0,FS_PW*2);

//				acc_data_to_g(&sw_ad_data.buff[sw_diagnos_ringbuf_para.index][DATA_HEAD_SIZE+512*data_cnt], dest, FS_SW);
				//保存失稳原始数据
				save_sw_original_data(&sw_ad_data.buff[sw_diagnos_ringbuf_para.index][DATA_HEAD_SIZE+512*data_cnt],
						sw_ad_data.buff[sw_diagnos_ringbuf_para.index][0],data_cnt);
			}
			fsync(sw_file.sw_original_data.fd);
//			DEBUG("fsync_ok:%d\n",sw_ad_data.buff[sw_diagnos_ringbuf_para.index][0]);
			ring_buff_index_inc(&sw_diagnos_ringbuf_para);
			ring_buff_num_dec(&sw_diagnos_ringbuf_para);

		}
	}
}

int init_sw_diagnosis_thread()
{
	pthread_t sw_diagnos_thread_id;
	int ret = -1;
	ret=pthread_create(&sw_diagnos_thread_id,NULL,(void *)sw_diagnos_thread_entry,NULL);
	if(ret!=0)
	 {
	 	DEBUG ("create sw diagnos thread error!\n");
	 }
	 	return ret;
}

