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
#include "user_data.h"
#include "pw_diagnos.h"
#include "self_test.h"
#include "hdd_save.h"
#include "board.h"
#include "udp_client.h"
#include "ptu_app.h"
#include "assert.h"
#include "lh_math.h"
#include "fft_data.h"

#include "pswb_record_prot.h"

#ifdef WTD_DATA_TRANSLATE_PROTOCOL
#include "self_queue.h"
#endif

#ifdef DATA_CENTER_TEST
#include "data_test.h"

extern uint16_t acc_data_cnt;
extern float *acc_data_f;
#ifdef ONE_SPEED_ONE_SEC
	extern float speed_data_f;
#else
	extern float average_speed_f;
#endif
#endif

//#define PW_DIAG_DEBUG
#ifdef PW_DIAG_DEBUG
	#define PW_DIAG_PRINTF(format,...)  printf(format,##__VA_ARGS__)//
#else
	#define PW_DIAG_PRINTF(format,...)
#endif

#ifdef DIAG_DATA_SAVE_FOR_TEST
#define YZ_CH_NUM	pw2_z
#define Z_CH_NUM	pw2_z
#endif

#define FFT_NPT 512*2
#define DATA_HEAD_SIZE 2			//uint16_t 数据包号;uint16_t 速度

//#define STABLE_HZ40_INDEX  (uint32_t)(40.0f*FFT_NPT/FS_PW+0.5f)
/*#define STABLE_CAL_SENCOND 9u*/
#define STABLE_HZ40_INDEX 80
#define STABLE_HZ13_INDEX 129 		/*0.2~13Hz,分辨率0.1,一共129个点*/
#define STABLE_TEN_SENCOND 10u
#define STABLE_CAL_SENCOND 4u		/*每sec计算１次,只需计算5次*/

#define FFT_RESOLUTION_128HZ   (0.125f)//128/1024=0.125

#ifdef BRIDGE_DIAG
#define BRIDGE_LEN_MEAN_UPDOWN_LIMIT  3//±3  20220810
#define BRIDGE_LEN_DOWN  24//20220810
#define BRIDGE_LEN_UP  38//20220810
#define BRIDGE_FACTOR  (0.02f)//20220810 //主频幅值即为桥梁因子
#define BRIDGE_FACTOR_WARN_THRESHOLD  (0.07f)
#define BRIDGE_TIME_DOMAIN_AMP_FABS  (0.15f)//20220810
#endif
//#define PW_DIAG_TEST

extern struct PW_AD_DATA pw_ad_data;
extern struct PW_TZ_DATA pw_tz_data;
#ifdef ADD_DIAG_TZZ_DATA_FILE
	extern struct DIAG_TZZ_DATA diag_tzz_data;
#endif
//extern struct PW_RAW_DATA pw_raw_data;
extern struct RINGBUFF_PARA pw_diagnos_ringbuf_para;
extern struct PW_CLB_CONFIG_PARA *pw_clb_config_para;
extern struct SELF_TEST_PARA self_test_para;
extern struct SELF_TEST_PARA self_test_para1;
//extern struct PW_FILE pw_file;
extern struct UDP_SEND_ADDR udp_send_addr;
extern struct PTU_DATA ptu_data;
extern struct COMM_DATA_CNT comm_data_cnt;
extern struct PW_SIMULATION pw_simulation;
extern struct SYS_STATUS_CNT sys_status_cnt;

#ifdef WTD_DATA_TRANSLATE_PROTOCOL
#ifndef WTD_DATA_PROTOCOL_20220822
	extern struct LESS_HEAD_INFO head_info;
#endif
	extern uint8_t first_diag_result_flag;
	extern struct MSG_ALARM_TRIG msg_alarm_trigger[CHANNEL_NUM];
//	extern void save_msg_alarm_data(uint8_t ch_num, struct PW_RAW_DATA *buf, uint16_t size);
	extern void save_msg_alarm_data(uint8_t ch_num, int8_t *buf, uint16_t size);
//	extern uint16_t little_to_big_16bit(uint16_t value);
#ifdef WTD_SIMULATION_ALARM_TRIG
	uint8_t simulation_open_flag = 0;//开启报警触发功能标志
	uint8_t simulation_status[CHANNEL_NUM] = {TRIG_OK, TRIG_OK, TRIG_OK, TRIG_OK, TRIG_OK, TRIG_OK};
#endif
#endif

void get_ten_data_res(enum FREQUENCY_BAND_E type, struct TEN_DEAL_DIAG_RES *diag_ten_res,uint16_t start_index,uint16_t end_index);
void get_ten_data_res2(enum FREQUENCY_BAND_E type, struct TEN_DEAL_DIAG_RES *diag_ten_res,uint16_t start_index,uint16_t end_index,uint16_t array_index);
void move_float_to_float(float *diagbuf,float *adbuf,uint16_t date_size);
void move_uint16_to_float(float *diagbuf,uint16_t *adbuf,uint16_t date_size);
#ifdef ADD_DIAG_TZZ_DATA_FILE
extern void save_pw_diag_tzz_data(struct DIAG_TZZ_DATA *save_tz_data, uint16_t singal_num);
#endif
extern void save_pw_tz_data(struct PW_TZ_DATA *save_tz_data,uint16_t singal_num,uint16_t target_addr,uint8_t resend_flag,uint8_t ack_flag);
extern void clear_udp_send_addr();
extern void set_udp_send_addr(uint16_t board_set,uint16_t target_num);

struct PW_DIAGNOS_PARA pw_diagnos_para;
union BOARD_ERR pw_board_st;
sem_t pw_diagnos_sem;

extern struct S1PW_TZ_DATA s1pw_tz_data; //S1 use

float f_buff_out[6] = {0.0};

static double FS_512_NUM_b_0_64HZ[7] = {
0.00105164679630761,
0.00630988077784567,
0.0157747019446142,
0.0210329359261522,
0.0157747019446142,
0.00630988077784567,
0.00105164679630761,
};

static double FS_512_DEN_a_0_64HZ[7] = {
1.0,
-2.97852992612413,
4.13608099825748,
-3.25976427975097,
1.51727884474047,
-0.391117230593912,
0.0433569884347559,
};

/*************************************************
Function:ADdata_Filter
Description: AD原始数据干扰处理
Input:采样值，长度
Output:磨平异常点
Return:
Others:
*************************************************/
uint16_t lastdata[6]={0,0,0,0,0,0};

void ADdata_Filter(uint16_t *deal_data,uint32_t data_len ,uint8_t ch_num)
{
	int i,j=0;
	short trend;
	for( i=0;i<data_len;i++)
	{
		if(deal_data[i]!=lastdata[ch_num])
		{
			trend=deal_data[i]-lastdata[ch_num];

			if((fabs(trend)>5000))
			{
				j++;
				if(j>350)
				{
					lastdata[ch_num]=deal_data[i];
					j=0;
				}
				else
					deal_data[i]=lastdata[ch_num];
			}
			else
			{
				lastdata[ch_num]=deal_data[i];
			}
		}

	}
}

//TEST-1

/*
 * 大小端 转换
 *
 * **/
short myhtons(short n)
{
  return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}

#ifdef ORIGINAL_DATA_SAVE_ACC
	#ifdef WTD_DATA_TRANSLATE_PROTOCOL
	void acc_data_to_g(uint16_t *srcbuf, int16_t *dest, int16_t *wtd_dest, uint16_t len)//m/s^2  htons转大端
	{
		int i=0;
		float acc_buf = 0;
		float testbuf = 0;
		float wtd_testbuf = 0;

		for(i=0;i<len;i++)
		{
//			printf("%f\n", AD_CONVERT_VOLTAGE(srcbuf[i]));
			acc_buf = AD_CONVERT_ACCELERATION(srcbuf[i]);//单位g
			testbuf = GRAVITY_PARA * AMPLIFICATION_FACTOR * acc_buf;//x100,单位m/s^2
			wtd_testbuf = WTD_AMPLIFICATION_FACTOR * acc_buf;//x1000,单位g
	#if 0
//			testbuf = 2*sin(2*PI*10*i/512);
//			testbuf= testbuf*100;
//			wtd_testbuf = testbuf*1000;
////			printf("%d\n", testbuf);
////			testbuf = i;
			wtd_testbuf = i;
	#endif
			dest[i] = (int16_t)testbuf;
			dest[i] = myhtons(dest[i]);//转大端


			wtd_dest[i] = (int16_t)wtd_testbuf;
			wtd_dest[i] = myhtons(wtd_dest[i]);//转大端
		}
	}
	#else
	void acc_data_to_g(uint16_t *srcbuf, int16_t *dest, uint16_t len)//AD_ORG
	{
		int i=0;
		float acc_buf = 0;
		float testbuf = 0;

		for(i=0;i<len;i++)
		{
			acc_buf = AD_CONVERT_ACCELERATION(srcbuf[i]);//单位g
			testbuf = GRAVITY_PARA * AMPLIFICATION_FACTOR * acc_buf;//x100,单位m/s^2

			dest[i] = (int16_t)testbuf;
			dest[i] = myhtons(dest[i]);//转大端
		}
	}
	#endif
#else
	void acc_data_to_g(uint16_t *srcbuf,uint16_t *dest,uint16_t len)//AD_ORG
	{
		int i=0;

		for(i=0;i<len;i++)
		{
			dest[i] = srcbuf[i];

			dest[i] = htons(dest[i]);//转大端
		}
	}
#endif
/*************************************************
Function:lost_old_data_deal
Description: 数据行数超出处理
Input:
Output:
Return:
Others:
*************************************************/
void lost_old_data_deal(struct RINGBUFF_PARA *pw_diagnos_ringbuf_para_temp,struct PW_AD_DATA *pw_ad_data_temp)
{
	pw_diagnos_ringbuf_para_temp->index = pw_ad_data_temp->row_index + 1;
	if(pw_diagnos_ringbuf_para_temp -> index > pw_ad_data_temp->row_size)
	{
		pw_diagnos_ringbuf_para_temp->index = 0;
	}
	pw_diagnos_ringbuf_para_temp->num = pw_ad_data_temp->row_size - 1;
}
/*************************************************
Function:ring_buff_index_inc
Description: 诊断行数自加
Input:
Output:
Return:
Others:
*************************************************/
void ring_buff_index_inc(struct RINGBUFF_PARA *pw_diagnos_ringbuf_para_temp)
{
	pw_diagnos_ringbuf_para_temp->index++;
	if(pw_diagnos_ringbuf_para_temp->index >= pw_diagnos_ringbuf_para_temp->size)
	{
		pw_diagnos_ringbuf_para_temp->index = 0;
	}
}

/**
 @author:seanlee
 @breif:初始化fft计算结果的二级缓存区，每一行代表一次fft计算的结果
*/
void init_stable_fft_array(
struct STABLE_FFT_ARRAY* psrc,
uint32_t row_size,
uint32_t column_size)
{
	uint32_t i = 0;
	psrc->row_size=row_size;//5秒的fft缓存
	psrc->column_size=column_size;
	psrc->row_index=0u;
	if(psrc->buff==NULL)
	{
		psrc->buff=(float **)malloc(psrc->row_size*sizeof(float *));
		if(psrc->buff == NULL)
		{
			DEBUG("malloc stable fft buff err1\n");
			return;
		}

		for(i=0;i<psrc->row_size;i++)
		{

			psrc->buff[i]=(float *)malloc(psrc->column_size*sizeof(float));
			if(psrc->buff[i] == NULL)
			{
				DEBUG("malloc stable fft buff err2\n");
				return;
			}
		}
	}
}
//init_array_float
/**
 @author:seanlee
 @breif:fft滑动滤波窗的缓存初始化
*/
void init_filter_para(struct FILTER_STRUCT_FLOAT *psrc,uint32_t filter_order)
{
	psrc->filter_order=filter_order;
	psrc->window_size=psrc->filter_order+1;

	if(psrc->before_filter_buff==NULL)
	{
		psrc->before_filter_buff=(double *)malloc(psrc->window_size*sizeof(double));
		if(psrc->before_filter_buff==NULL)
		{
			DEBUG("malloc filter_para buff err2\n");
		}
		memset(psrc->before_filter_buff,0,psrc->window_size*sizeof(double));
	}
	if(psrc->hs_data==NULL)
	{
		psrc->hs_data=(double *)malloc(psrc->window_size*sizeof(double));
		memset(psrc->hs_data,0,psrc->window_size*sizeof(double));
	}
}

/*
 *初始化10s缓存 FFT函数
 * */
void init_ten_deal_diag(struct TEN_DEAL_DIAG_RES *ten_deal_diag_res,uint32_t size)
{
	//uint16_t cnt_i = 0;
	if(ten_deal_diag_res->fft_value == NULL)
	{
		ten_deal_diag_res->fft_value  = (float *)malloc(size*sizeof(float));

	}

	if(ten_deal_diag_res->fft_value  != NULL)
	{
		PW_DIAG_PRINTF("init_ten_deal_diag data_buff success\r\n");
		memset(ten_deal_diag_res->fft_value ,0,size*sizeof(float));
	}

	ten_deal_diag_res->main_fre1 = 0.0f;
	ten_deal_diag_res->max_value1 = 0.0f;
	ten_deal_diag_res->percent_main_fre1 = 0.0f;

	ten_deal_diag_res->main_fre2 = 0.0f;
	ten_deal_diag_res->max_value2 = 0.0f;
	ten_deal_diag_res->percent_main_fre2 = 0.0f;

#ifdef BRIDGE_DIAG
	memset(ten_deal_diag_res->main_fre3, 0, 10*sizeof(float));
	memset(ten_deal_diag_res->max_value3, 0, 10*sizeof(float));
	memset(ten_deal_diag_res->max_fabs_value3, 0, 10*sizeof(float));
//	memset(ten_deal_diag_res->percent_main_fre3, 0, 10*sizeof(float));
	memset(ten_deal_diag_res->bridge_len, 0, 10*sizeof(float));
	memset(ten_deal_diag_res->speed, 0, 10*sizeof(float));
	ten_deal_diag_res->speed_cnt = 0;
	ten_deal_diag_res->keep_len_times = 0;
	ten_deal_diag_res->bridge_flag = 0;
#endif
}


/**
 @author:xuzhao
 @breif:抖车滤波参数初始化
*/
void init_jitter_filter_para(struct FILTER_STRUCT_FLOAT *psrc,uint32_t filter_order)
{
	psrc->filter_order=filter_order;
	psrc->window_size=psrc->filter_order+1;

	if(psrc->before_filter_buff==NULL)
	{
		psrc->before_filter_buff=(double *)malloc(psrc->window_size*sizeof(double));
		if(psrc->before_filter_buff==NULL)
		{
			DEBUG("malloc filter_para buff err2\n");
		}
		memset(psrc->before_filter_buff,0,psrc->window_size*sizeof(double));
	}
	if(psrc->hs_data==NULL)
	{
		psrc->hs_data=(double *)malloc(psrc->window_size*sizeof(double));
		memset(psrc->hs_data,0,psrc->window_size*sizeof(double));
	}
}



/**
 @author:xuzhao
 @breif:晃车滤波参数初始化
*/
void init_shack_filter_para(struct FILTER_STRUCT_FLOAT *psrc,uint32_t filter_order)
{
	psrc->filter_order=filter_order;
	psrc->window_size=psrc->filter_order+1;

	if(psrc->before_filter_buff==NULL)
	{
		psrc->before_filter_buff=(double *)malloc(psrc->window_size*sizeof(double));
		if(psrc->before_filter_buff==NULL)
		{
			DEBUG("malloc filter_para buff err2\n");
		}
		memset(psrc->before_filter_buff,0,psrc->window_size*sizeof(double));
	}
	if(psrc->hs_data==NULL)
	{
		psrc->hs_data=(double *)malloc(psrc->window_size*sizeof(double));
		memset(psrc->hs_data,0,psrc->window_size*sizeof(double));
	}
}


/*************************************************
Function:    pw_diagnos_thread_entry
Description: 自检和秒线程
Input:
Output:
Return:
Others:
*************************************************/
void init_array_float(struct ARRAY_FLOAT *psrc,uint32_t size)
{
	psrc->size=size;//1024

	if(psrc->station_buff == NULL)
	{
		psrc->station_buff=(float *)malloc(psrc->size*sizeof(float));
		if(psrc->station_buff == NULL)
			DEBUG("malloc jitter_buff err\n");
	}

	if(psrc->station_buff!=NULL)
	{
		psrc->w_index=0u;
		memset(psrc->station_buff,0,(psrc->size*sizeof(float)));
	}
	else
	{
		;
	}
}

/*初始化晃车诊断数据缓存*/
void init_shack_buff(struct SHACK_FLOAT *psrc,uint32_t size)
{
	psrc->size=size;//512*1

	if(psrc->shack_buff == NULL)
	{
		psrc->shack_buff=(float *)malloc(psrc->size*sizeof(float));
		if(psrc->shack_buff == NULL)
			DEBUG("malloc jitter_buff err\n");
	}

	if(psrc->shack_buff!=NULL)
	{
		psrc->w_index=0u;
		memset(psrc->shack_buff,0,(psrc->size*sizeof(float)));
	}
	else
	{
		;
	}
}

/*初始化径跳诊断数据缓存*/
void init_wheel_buff(struct WHEEL_DIAMETER *psrc,uint32_t size)
{
	psrc->size=size;//512*1

	if(psrc->sample_speed == NULL)
	{
		psrc->sample_speed=(float *)malloc(psrc->size*sizeof(float));
		if(psrc->sample_speed == NULL)
			DEBUG("malloc jitter_buff err\n");
	}

	/*径跳需要的平均频谱数据缓存*/
	if(psrc->fft_value == NULL)
	{
		psrc->fft_value=(float *)malloc(STABLE_HZ40_INDEX*sizeof(float));
		if(psrc->fft_value == NULL)
			DEBUG("malloc fft_value err\n");
	}


	if(psrc->sample_speed!=NULL)
	{
		psrc->w_index=0u;
		memset(psrc->sample_speed,0,(psrc->size*sizeof(float)));
	}
	else
	{
		;
	}
}


/*车体10s数据长度缓存初始化*/
void init_ten_deal_buff(struct TEN_DEAL_DATA *psrc,uint32_t size,uint32_t re_size)
{
	psrc->size=size;//512*10
	psrc->resample_size = re_size;//128*10

	if(psrc->deal_buff == NULL)
	{
		psrc->deal_buff=(float *)malloc(psrc->size*sizeof(float));
		if(psrc->deal_buff == NULL)
			DEBUG("malloc deal_buff err\n");
	}
	else
	{
		;
	}

	if(psrc->resample_deal_buff == NULL)
	{
		psrc->resample_deal_buff=(float *)malloc(psrc->resample_size*sizeof(float));
		if(psrc->resample_deal_buff == NULL)
			DEBUG("malloc deal_buff err\n");
	}
	else
	{
		;
	}


	if(psrc->deal_buff_bak == NULL)
	{
		psrc->deal_buff_bak=(float *)malloc(psrc->size*sizeof(float));
		if(psrc->deal_buff_bak == NULL)
			DEBUG("malloc deal_buff_bak err\n");
	}
	else
	{
		;
	}

	if(psrc->deal_buff!=NULL)
	{
		psrc->w_index=0u;
		memset(psrc->deal_buff,0,(psrc->size*sizeof(float)));
	}

	if(psrc->resample_deal_buff!=NULL)
	{
		memset(psrc->resample_deal_buff,0,(psrc->resample_size*sizeof(float)));
	}
}


/*初始化抖车诊断数据缓存*/
void init_jitter_buff(struct JITTER_FLOAT *psrc,uint32_t size)
{
	psrc->size=size;//512

	if(psrc->jitter_buff == NULL)
	{
		psrc->jitter_buff=(float *)malloc(psrc->size*sizeof(float));
		if(psrc->jitter_buff == NULL)
			DEBUG("malloc jitter_buff err\n");
	}

	if(psrc->jitter_buff!=NULL)
	{
		psrc->w_index=0u;
		memset(psrc->jitter_buff,0,(psrc->size*sizeof(float)));
	}
	else
	{
		;
	}
}


/*************************************************
Function:ring_buff_num_dec
Description: 可诊断行数自减
Input:
Output:
Return:
Others:
*************************************************/
void ring_buff_num_dec(struct RINGBUFF_PARA *pw_diagnos_ringbuf_para_temp)
{
	pw_diagnos_ringbuf_para_temp->num--;
}


/*************************************************
Function:    init_pw_diagnos_para
Description: 初始化算法诊断参数
Input:
Output:
Return:
Others:
*************************************************/
void init_pw_diagnos_para()
{
	uint8_t ch_i = 0;
	for(ch_i = 0;ch_i < CHANNEL_NUM;ch_i ++)
	{
		pw_diagnos_para.station_deal_num[ch_i] = FS_PW*DIAG_SEC;
		pw_diagnos_para.self_test_index[ch_i]=0u;
		pw_diagnos_para.diag_res[ch_i].value=0.0f;
		pw_diagnos_para.diag_res[ch_i].warn_h_num=0;
		pw_diagnos_para.diag_res[ch_i].warn_l_num=0;
		pw_diagnos_para.diag_res[ch_i].alarm_h_num=0;
		pw_diagnos_para.diag_res[ch_i].alarm_l_num=0;
		pw_diagnos_para.diag_res[ch_i].alarm_status_new=RUNNING_OK;
		pw_diagnos_para.diag_res[ch_i].alarm_status_old=RUNNING_OK;

		pw_diagnos_para.rms[ch_i]=0.0f;
		pw_diagnos_para.electric_val1[ch_i]=0.0f;
		pw_diagnos_para.electric_val2[ch_i]=0.0f;


		/*平稳性指标数据处理缓存初始化*/
		init_array_float(&pw_diagnos_para.data_deal_buff[ch_i],pw_diagnos_para.station_deal_num[ch_i]);				//算法处理数据缓存

		/*平稳性指标数据处理缓存初始化*/
		init_array_float(&pw_diagnos_para.data_dealb_buff[ch_i],pw_diagnos_para.station_deal_num[ch_i]);			//算法处理数据缓存

		/*晃车数据处理缓存初始化*/
		init_shack_buff(&pw_diagnos_para.shack_float[ch_i],FS_PW*SHACK_DIAG_SEC);

#ifdef DC_HC_USE_FFT_FILTER
	    /*新增晃车数据fft缓存初始化*/
		init_shack_buff(&pw_diagnos_para.shack_fft_in[ch_i],FS_PW*SHACK_FFT_SEC);
#endif

		/*晃车峰峰值处理缓存初始化*/
		init_shack_buff(&pw_diagnos_para.last_packet_extreme[ch_i],FS_PW*SHACK_DIAG_SEC);


		/*径跳处理缓存初始化*/
		init_wheel_buff(&pw_diagnos_para.wheel_diameter[ch_i],FS_PW);

		/*抖车数据处理缓存初始化*/
		init_jitter_buff(&pw_diagnos_para.jitter_float[ch_i],FS_PW*JITTER_DIAG_SEC);

		/*抖车数据处理缓存初始化*/
		init_jitter_buff(&pw_diagnos_para.jitter_float_deal[ch_i],FS_PW*JITTER_DIAG_SEC/5);

#ifdef DC_HC_USE_FFT_FILTER
	    /*新增晃车数据fft缓存初始化*/
		init_jitter_buff(&pw_diagnos_para.jitter_fft_in[ch_i],FS_PW*JITTER_FFT_SEC);
#endif
		/*车体10s数据长度缓存处理*/
		init_ten_deal_buff(&pw_diagnos_para.ten_deal_data[ch_i],FS_PW*TEN_DIAG_SEC,RE_FS_PW*TEN_DIAG_SEC);

		init_ten_deal_diag(&pw_diagnos_para.ten_del_diag_res[ch_i],FFT_NPT);
		//printf("tzdata->ten_del_diag_res[ch].fft_value:%f,ch:%d\r\n",pw_diagnos_para.ten_del_diag_res[ch_i].fft_value[0],ch_i);

		if(ch_i==pw1_y||ch_i==pw2_y)
		{
			pw_diagnos_para.sens_para[ch_i].sens_type=PW_Y;
			pw_diagnos_para.sens_para[ch_i].sensitivity=SENSITIVITY_PW;
			/*自检前 默认传感器是true*/
			pw_diagnos_para.sens_para[ch_i].sens_self_test.bits.open_circuit_err_power_on=SENSOR_OK;
			pw_diagnos_para.sens_para[ch_i].sens_self_test.bits.open_circuit_err_real_time=SENSOR_OK;

		}
		else if(ch_i==pw1_z||ch_i==pw2_z)
		{
			pw_diagnos_para.sens_para[ch_i].sens_type=PW_Z;
			pw_diagnos_para.sens_para[ch_i].sensitivity=SENSITIVITY_PW;
			/*自检前 默认传感器是true*/
			pw_diagnos_para.sens_para[ch_i].sens_self_test.bits.open_circuit_err_power_on=SENSOR_OK;
			pw_diagnos_para.sens_para[ch_i].sens_self_test.bits.open_circuit_err_real_time=SENSOR_OK;
		}
		else if(ch_i==pw1_x||ch_i==pw2_x)
		{
			pw_diagnos_para.sens_para[ch_i].sens_type=PW_X;
			pw_diagnos_para.sens_para[ch_i].sensitivity=SENSITIVITY_PW;
			/*自检前 默认传感器是true*/
			pw_diagnos_para.sens_para[ch_i].sens_self_test.bits.open_circuit_err_power_on=SENSOR_OK;
			pw_diagnos_para.sens_para[ch_i].sens_self_test.bits.open_circuit_err_real_time=SENSOR_OK;
		}
		else
		{
			pw_diagnos_para.sens_para[ch_i].sens_type=AD_REF;
		    pw_diagnos_para.sens_para[ch_i].sensitivity=SENSITIVITY_PW;
			/*自检前 默认传感器是true*/
		    pw_diagnos_para.sens_para[ch_i].sens_self_test.bits.open_circuit_err_power_on=SENSOR_OK;
		    pw_diagnos_para.sens_para[ch_i].sens_self_test.bits.open_circuit_err_real_time=SENSOR_OK;
		}

//		/*512阶要算死，不用，改用fft-ifft*/
//		/*晃车滤波初始化*/
//		init_shack_filter_para(&pw_diagnos_para.shack_filter_para[ch_i],512);			/*512阶*/
//
//		/*抖车滤波初始化*/
//		init_jitter_filter_para(&pw_diagnos_para.jitter_filter_para[ch_i],512);			/*512阶*/

		/*fft-缓存初始化 */
		init_stable_fft_array(&pw_diagnos_para.stable_fft_array[ch_i],STABLE_CAL_SENCOND,STABLE_HZ40_INDEX);//4s fft缓存 buf[4][80]

		/*64Hz 低通滤波使用*/
		init_filter_para(&pw_diagnos_para.filter_para[ch_i],6);							/*6阶*/

		/*车体10s-fft缓存初始化*/
		//init_stable_fft_array(&pw_diagnos_para.ten_del_diag_res[ch_i].ten_del_stable_fft,STABLE_TEN_SENCOND,STABLE_HZ13_INDEX);
//pgd


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
void average_fft_value(struct STABLE_FFT_ARRAY* psrc,float *pdes)
{
	float sum_data = 0.0f;
	uint32_t i=0;
	uint32_t j=0;
	for(i=0;i<psrc->column_size;i++) 	//列
	{
		sum_data=0.0f;
		for(j=0;j<psrc->row_size;j++) 	//按行累加
		{	//i　为频率索引
//			if((i == 19)||(i == 20)||(i == 21))
//			{
//				printf("psrc->buff:%f,i:%d,j:%d\r\n",psrc->buff[j][i],i,j);
//			}

			sum_data+=psrc->buff[j][i];
		}
		pdes[i]=sum_data/psrc->row_size;	//计算每种频率的平均幅值
	}
}


/*************************************************
Function:    check_stable_cal_para
Description: 计算平稳性指标的修正系数
Input:
Output:
Return:
Others:
*************************************************/
void check_stable_cal_para(const float f,float *modify,enum SENSOR_TYPE type)
{
	if(type==PW_Y)
	{
		if(f < 5.4f)
		{
			*modify = 0.8f * f * f;
		}
		else if(f < 26.0f)
		{
			*modify = 650.0f / f / f;
		}
		else
		{
			*modify = 1.0f;
		}
	}
	else if(type==PW_Z)
	{
		if(f < 5.9f)
		{
			*modify = 0.325f * f * f;
		}
		else if(f < 20.0f)
		{
			*modify = 400.0f / f / f;
		}
		else
		{
			*modify = 1.0f;
		}
	}
}

/*任何>=４阶的滤波算法，都可以使用该函数进行移动位置*/
void arm_move_f32(double *data_float, double *data_int, uint32_t blocksize)
{
  uint32_t blkcnt = 0;                               /* loop counter */

  double  in_f32_1 = 0, in_f32_2 = 0, in_f32_3 = 0, in_f32_4 = 0;
  double  in_int_1 = 0, in_int_2 = 0, in_int_3 = 0, in_int_4 = 0;

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



/*************************************************
Function:    filter_deal
Description: 滤波处理，处理的结果放到了滑动滤波窗里面了
Input:
Output:
Return:
Others:
*************************************************/
void filter_deal(struct TEN_DEAL_DATA *deal_buf,struct FILTER_STRUCT_FLOAT *filter_struct_float)
{
	uint16_t cnt_i = 0;
	double temp_x = 0.0;
	double temp_y = 0.0;
	double temp_res = 0.0f;

	assert(deal_buf!=NULL);
	assert(filter_struct_float!=NULL);

	for(cnt_i = 0;cnt_i < deal_buf->size;cnt_i++)
	{
		temp_x = 0.0;
		temp_y = 0.0;
		temp_res = 0.0f;

//		if(type==PW_Y||type==PW_Z)
//		{
			/*平稳滤波参数*/
			filter_struct_float->before_filter_buff[0] = (double)deal_buf->deal_buff[cnt_i];
			temp_x = double_sum_xy(FS_512_NUM_b_0_64HZ,filter_struct_float->before_filter_buff,filter_struct_float->window_size);
			temp_y = double_sum_xy(&FS_512_DEN_a_0_64HZ[1],&filter_struct_float->hs_data[1],filter_struct_float->window_size-1);

			temp_res = temp_x - temp_y;
			filter_struct_float->hs_data[0] = temp_res;
			arm_move_f32(&filter_struct_float->hs_data[filter_struct_float->window_size-1],&filter_struct_float->before_filter_buff[filter_struct_float->window_size-1],filter_struct_float->window_size-1);
			deal_buf->deal_buff[cnt_i] = (float)temp_res;

//		}
	}
}


#ifdef	DC_HC_USE_FFT_FILTER
/* ************************
 * fft ifft 滤波
 * ************************/
void fft_ifft_deal(float *fft_in_buf, float *ifft_out_buf, uint8_t fre_band)
{
	float fft_in[2048]={0};
	float fft_real[1024]={0};
	float fft_imag[1024]={0};
	int k;

#if 0
	fre_band = FRE_5_TO_13HZ;
	printf("fft_ifft_deal--real in:\n");
	for(k=0; k<1024; k++)
	{
		fft_in[k] = 2*sin(2*PI*10*k/512);//f=10Hz   采样率：512p/s    (x=k/512)  w=2*PI*10      T=2*PI/w=1/10
		fft_in[k+1024] = 0;
		printf("%f\n",fft_in[k]);
	}
#else
	for(k=0; k<1024; k++)
	{
		fft_in[k] = fft_in_buf[k];//real   f=2Hz   w=2*PI*2/1024   T=2*PI/w=512
		fft_in[k+1024] = 0;//imag
		//printf("%f\n",fft_in[k]);
	}
#endif

	complex_fft((float*)fft_in, fft_real, fft_imag, 2048, 1024, FFT_E);


	for(k=0; k<1024; k++)
	{
		fft_in[k] = fft_real[k];//2*sin(2*PI*4*k/1024);//f=2Hz   w=2*PI*2/1024   T=2*PI/w=512
		fft_in[k+1024] = fft_imag[k];
	}

	if(fre_band == FRE_0D2_TO_3HZ)
		complex_fft_convert((float*)fft_in, (float*)hc_filter_real_3hz, (float*)hc_filter_imag_3hz, FFT_NPT);
	else if(fre_band == FRE_5_TO_13HZ)
		complex_fft_convert((float*)fft_in, (float*)dc_filter_real_13hz, (float*)dc_filter_imag_13hz, FFT_NPT);
	else
		return;

	complex_fft((float*)fft_in, fft_real, fft_imag, 2048, 1024, IFFT_E);

#if 0
	printf("fft_ifft_deal--real out:\n");
	for(k=0; k<1024; k++)
	{
		printf("%f\n",fft_real[k]);
	}
#endif

	memmove(ifft_out_buf, fft_real+FS_PW, FS_PW*sizeof(float));//取后512个点
}
#endif

/*************************************************
Function:    push_stable_fft_array
Description: 平稳性指标计算
Input:
Output:
Return:
Others:
*************************************************/

void push_stable_fft_array(struct STABLE_FFT_ARRAY* pdes,float* element)
{


	//printf("one read_fft=%d,row=%d\r\n",(int)pdes ->buff [pdes->row_index][0],pdes->row_index);

	/*printf("row=%d\r\n",pdes->row_index);*/

	if(pdes->row_index>=pdes->row_size)
	{
		int t = 0;
		for ( t = 0; t < pdes->row_size-1; t++)
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
float fft_index2hz(uint32_t fft_pt,uint32_t sample_hz,uint32_t index)
{
	float hz = 0.0f;
	hz= (float)sample_hz / fft_pt * index;		//大于0.5
	return hz;
}

/*fft-频率转下标*/
uint32_t fft_hz2index(uint32_t fft_pt,uint32_t sample_hz,float hz)
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
//void bubble_sort(float *data,uint32_t length)
//{
//	uint32_t i,j,cnt=0;
//	float temp = 0.0f;
//	cnt = length ;
//	if(length<=0)
//		return;
//	for(i=1;i<length;i++)
//	{
//		cnt-=1;
//		for(j=0;j<cnt;j--)
//		{
//			if(data[j]>data[j+1])
//			{
//				temp=data[j];
//				data[j]=data[j+1];
//				data[j+1]=temp;
//			}
//		}
//	}
//}

//float ad_convert_acceleration(float value,float sensitivity)
//{
//	return value*10.0f/65536.0f/sensitivity;
//}

/**
 @author:seanlee
 @breif:冲击计算
 */
//void impulse_deal(struct PW_DIAGNOS_PARA *tzdata,uint32_t ch)
//{
//	struct ARRAY_FLOAT *array=&tzdata->data_deal_buff[ch];
//	{
//		uint32_t i;
//		float delta_y;
//		float delta_x=1.0f/FS_PW;
//
//		/*两两计算冲动值，结果放在array->buff里面，size---->size-1*/
//		for(i=0;i<array->size-1;i++)
//		{
//			delta_y=fabs(array->station_buff[i]-array->station_buff[i+1]);
//			array->station_buff[i]=delta_y/delta_x;
//		}
//
//		/*冒泡排序*/
//		bubble_sort(array->station_buff,array->size-1);
//
//		/*冲动计算的结果以95%的排序最大值为准,抛掉2.5%的最小值和2.5%最大值置信区间*/
//		i=(uint32_t)((array->size-1)*0.975f);//array->buff[i]
//		tzdata->diag_res[ch].value = ad_convert_acceleration(array->station_buff[i],SENSITIVITY_PW)*GRAVITY_PARA;
//		//tzdata->Eig_value [ch].Sta_norm = tzdata->diag_res[ch].value;					//得到平稳性指标
//	}
//}


void find_fft_max_value(float *src_data,float *dst_value,uint8_t *index_cnt)
{
	float max_value = 0.0f;
	uint8_t cnt = 0;
	max_value = src_data[0];
	*index_cnt = 0 + 1;
	for(cnt = 1;cnt < STABLE_HZ40_INDEX;cnt ++)
	{
		if(max_value < src_data[cnt])
		{
			max_value = src_data[cnt];
			*index_cnt = cnt + 1;
		}
	}
	*dst_value = max_value;
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
void calc_amp_deal(struct SHACK_FLOAT *shack_float_psrc, struct SHACK_FLOAT *shack_last_packet_extreme)
{
	uint32_t extreme_numb = shack_last_packet_extreme->w_index;
	float *process_data;

	/*如果极值点的个数为奇数则保留1个，如果极值点的个数为偶数则保留2个*/
	uint8_t have_ffz_flag = 0,jz_offset_bit = 0;
	uint32_t remain_numb = 0,group_numb,group_i = 0;


	process_data = shack_last_packet_extreme->shack_buff;//tzdata->last_packet_extreme[ch].buff;
	/*至少有3个极值点(要有一个完整的波峰和波谷)才能判断一个峰峰值,每一包极值点取峰峰值时应该保证都是从正值点或者负值点取*/
	if(process_data[0] > 0)				//如果第一个极值点为正
	{
		if (extreme_numb >= 3)
		{
			have_ffz_flag = 0x01;
			jz_offset_bit = 0x0;
		}
	}
	else							//如果第一个极值点为负
	{
		if (extreme_numb >= 4)
		{
			extreme_numb--;
			have_ffz_flag = 0x01;
			jz_offset_bit = 0x1;
		}
	}

//	if (extreme_numb >= 3)
//	{
//		have_ffz_flag = 0x01;
//		jz_offset_bit = 0x0;
//	}

	if(have_ffz_flag>0)
	{
		/*step1. 计算峰值及其个数及保留的极值点个数*/
		group_numb = extreme_numb/2;				//峰峰值个数为极值点个数除以２
		remain_numb = extreme_numb & 0x01;			//如果极值点个数为奇数，remain_numb为１,如果极值点个数为偶数，remain_numb为０
		if(remain_numb == 0)
		{
			group_numb--;
			remain_numb = 2;
		}

		for(group_i=0;group_i<group_numb;group_i++)
		{
			float a=shack_last_packet_extreme->shack_buff[group_i*2 + jz_offset_bit];
			float b=shack_last_packet_extreme->shack_buff[group_i*2 + 1 + jz_offset_bit];
			shack_float_psrc->shack_buff[group_i]=fabs(a-b);//波峰-波谷,取绝对值

			PW_DIAG_PRINTF("diag_shack_pw---peak_peak[%d]:%f\n", group_i, shack_float_psrc->shack_buff[group_i]);
		}
		shack_float_psrc->w_index=group_numb;//峰值的个数赋值给data_deal_buff[ch].w_index
		//printf("1-shack_float_psrc->w_index:%d\r\n",shack_float_psrc->w_index);
    }
    else
    {
        remain_numb = extreme_numb;
        shack_float_psrc->w_index=0;
        //printf("2-shack_float_psrc->w_index:%d\r\n",shack_float_psrc->w_index);
    }


    if(remain_numb==2)
    {
    	shack_last_packet_extreme->shack_buff[0]=shack_last_packet_extreme->shack_buff[shack_last_packet_extreme->w_index-2];	//extreme_numb
    	shack_last_packet_extreme->shack_buff[1]=shack_last_packet_extreme->shack_buff[shack_last_packet_extreme->w_index-1];
    }
    else if(remain_numb==1)
    {
    	shack_last_packet_extreme->shack_buff[0]=shack_last_packet_extreme->shack_buff[shack_last_packet_extreme->w_index-1];
    }
    else if(remain_numb==3)
    {
    	shack_last_packet_extreme->shack_buff[0]=shack_last_packet_extreme->shack_buff[shack_last_packet_extreme->w_index-3];
    	shack_last_packet_extreme->shack_buff[1]=shack_last_packet_extreme->shack_buff[shack_last_packet_extreme->w_index-2];
    	shack_last_packet_extreme->shack_buff[2]=shack_last_packet_extreme->shack_buff[shack_last_packet_extreme->w_index-1];
    }
    shack_last_packet_extreme->w_index=remain_numb;//要保留的极值个数赋值给last_packet_extreme[ch].w_index
}

/**
 * 数据拷贝
 *
 * */
void copy_data_float(float *src,float *dst,uint16_t data_num)
{
	uint16_t cnt_i;
	for(cnt_i = 0;cnt_i < data_num;cnt_i ++)
	{
		dst[cnt_i] = src[cnt_i];
	}
}



/**
 * 得到转换后的加速度
 * */
void get_acc_data(float *buff, int data_cnt)
{
	int cnt_i = 0;

	for(cnt_i = 0;cnt_i < data_cnt;cnt_i++)
	{
		buff[cnt_i] = AD_CONVERT_VOLTAGE(buff[cnt_i])/SENSITIVITY_PW;//统一以g计算, 去均值时，已去了零点电压，不用再减零点电压,/SENSITIVITY_PW转g
	}
}

void get_acc_data_pwx(float *buff, int data_cnt)
{
	int cnt_i = 0;

	for(cnt_i = 0;cnt_i < data_cnt;cnt_i++)
	{
		buff[cnt_i] = GRAVITY_PARA*AD_CONVERT_VOLTAGE(buff[cnt_i])/SENSITIVITY_PW;//平稳性指标算法以m/s^2计算
	}
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
static int32_t reduce_data(float *data, uint32_t data_num)
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






/*******************************************************************************
* Function Name  : sigtoampth
* Description    : 处理峰峰值
* Input          : - tzdata: 特征参数
                   - ch_id:通道号(0、1...)
                   - numb:待处理的点数
* Output         : None
* Return         : None
* Function call relation:
* Reentrant      :
* Notes          :
*******************************************************************************/
static void sigtoampth(struct SHACK_FLOAT *shack_float_psrc, struct SHACK_FLOAT *shack_last_packet_extreme, uint32_t size)
{
//	uint32_t i = 0;
//	uint32_t ampth_cnt = 0;
	uint32_t numb = 0;

  /*step1. 对新的1包滤波数据--返回极值点的个数,shack_float_psrc->shack_buff全部为极值点－－求极值点*/
//	printf("check_size:%d\r\n",size);
	numb = reduce_data(shack_float_psrc->shack_buff, size);
//	printf("numb:%d\r\n",numb);
//	for(i = 0;i < numb;i++)
//	{
//		printf("%f\r\n",shack_float_psrc->shack_buff[i]);
//	}


	/*step2. 合并操作：将新的极值点 转移到 老的1包“保留”下来的极值点的后面*/
	//printf("1shack_last_packet_extreme->w_index:%d\r\n",shack_last_packet_extreme->w_index);
	copy_data_float(
	shack_float_psrc->shack_buff,
	&shack_last_packet_extreme->shack_buff[shack_last_packet_extreme->w_index],
	numb);
//	for(i = 0;i < shack_last_packet_extreme->w_index+numb;i++)
//		{
//			printf("%f\r\n",shack_last_packet_extreme->shack_buff[i]);
//		}


	/*step3. 计算“合并数据”的极值点，返回“新的极值点个数”*/
	shack_last_packet_extreme->w_index=reduce_data(
	shack_last_packet_extreme->shack_buff,
	shack_last_packet_extreme->w_index+numb);
	//printf("2shack_last_packet_extreme->w_index:%d\r\n",shack_last_packet_extreme->w_index);
	/*step4. 计算峰峰值，保留“可疑极值点”*/

	calc_amp_deal(shack_float_psrc,shack_last_packet_extreme);
	/*峰峰值保存在shack_float_psrc->shack_buff中,峰峰值个数为shack_float_psrc->w_index*/

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
static int32_t sigtodiff(struct SHACK_FLOAT *shack_float_psrc)
{
	uint32_t i=0, j=0;
	float *sig = NULL;
//	float temp_value = 0.0f;
//	float temp_value1 = 0.0f;

	sig = &shack_float_psrc->shack_buff[0];
	uint32_t numb = shack_float_psrc->size;



	if(sig == 0x0)
		 return 0;

	for (i=0; i<numb; i++)
	{
		if (*(sig+i) != 0)
			break;
	}
	if (i < numb)
	{
		//printf("i=%d,j=%d\r\n",i,j);
		*(sig+j) = *(sig+i);

		j=1;

		for(i=i+1; i<numb; i++)
		{

			 if(*(sig+i) != *(sig+i-1) && (*(sig+i) != 0))
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
void ampth_deal(struct SHACK_FLOAT *shack_float_psrc, struct SHACK_FLOAT *shack_last_packet_extreme)
{
	int32_t numb_tmp = 0;
	//float test_value = 0.0f;

  	/*numb_tmp = sigtodiff(tzdata, ch);*/
	/*返回　诊断数据被剔除零点和相邻相同点后的　数据个数*/



	numb_tmp = sigtodiff(shack_float_psrc);

	//printf("numb_tmp:%d\r\n",numb_tmp);
	/*shack_float_psrc->shack_buff*/
	/*shack_float_psrc->shack_buff 中的诊断数据已经被剔除零点和相邻相同点*/
	if (numb_tmp > 0)
	{
		sigtoampth(shack_float_psrc, shack_last_packet_extreme, numb_tmp);
	}
}

/*******************************************************************************
* Function Name  : rms_deal
* Description    : 均方根处理
* Input          : data_buff 待计算数据
* 				 : data_size 数据个数
* Output         : None
* Return         : rms_value
* Function call relation:
* Reentrant      :
* Notes          :
*******************************************************************************/
float rms_deal(float *data_buff, uint32_t data_size)
{
	uint32_t cnt_i = 0;
	double sum_value = 0.0f;
	float rms_value = 0.0f;
	/*先将数据转换为加速度*/
	for(cnt_i = 0;cnt_i < data_size;cnt_i++)
	{
//#ifdef PW_DIAG_TEST
//		//data_buff[cnt_i] = AD_CONVERT_ACCELERATION(data_buff[cnt_i],1.0);			//转换加速度
//#else
//		//data_buff[cnt_i] = AD_CONVERT_ACCELERATION(data_buff[cnt_i],1.0);		//转换加速度
//#endif
		sum_value += data_buff[cnt_i]*data_buff[cnt_i];
	}
	rms_value = sqrt(sum_value/data_size);

	return rms_value;
//  /*numb_tmp = sigtodiff(tzdata, ch);*/
//	/*返回　诊断数据被剔除零点和相邻相同点后的　数据个数*/
//	numb_tmp = sigtodiff(shack_float_psrc);
//	/*shack_float_psrc->shack_buff*/
//	if (numb_tmp > 0)
//	{
//		sigtoampth(shack_float_psrc, shack_last_packet_extreme, numb_tmp);
//	}
}

#ifdef PW_DIAG
/*************************************************
Function:    stable_deal
Description: 平稳性指标计算
Input:
Output:
Return:
Others:
*************************************************/
void stable_deal(struct PW_DIAGNOS_PARA* tzdata,uint32_t ch)
{
  uint32_t index_i = 0;
//  uint32_t i = 0;
//  uint32_t index = 0;
//  uint8_t test_row = 0;
//  uint8_t test_colum = 0;
  uint8_t test_index = 0;
  uint8_t cnt_i = 0;				//移动平均频谱到径跳处理
  float A = 0;
  float raw_f = 0.0f;             //频率，Hz
  float modify_F = 0.0f;          //修正后的频率，GB5595中规定的F(f)
  float max_fft = 0.0f;
  float fft_temp[STABLE_HZ40_INDEX] = {0};		//为什么不能初始化/STABLE_HZ40_INDEX
//  float diag_value = 0.0f;
  memset(fft_temp,0,sizeof(fft_temp));

    /*转加速度,单位:m/s^2,仅平稳性算法用*/
#if defined(DATA_CENTER_TEST)
	if(acc_data_cnt == ACC_DATA_LENGTH)
	{
		get_acc_data_pwx(tzdata->data_deal_buff[ch].station_buff,FFT_NPT);
	}
#else
	get_acc_data_pwx(tzdata->data_deal_buff[ch].station_buff,FFT_NPT);
#endif

#ifdef DIAG_DATA_SAVE_FOR_TEST
	if(ch == YZ_CH_NUM)
	{
		save_pwtzz_diag_file(PWTZZ_TYPE, DIAG_DATA_ACC_TITLE, NULL, 1);
		save_pwtzz_diag_file(PWTZZ_TYPE, DIAG_DATA_ACC, &tzdata->data_deal_buff[ch].station_buff[0], tzdata->data_deal_buff[ch].size);
	}
#endif

//	if(ch == pw1_y)
//	{
//		for(i = 0;i < FFT_NPT;i++)
//		{
//			printf("%f\n",tzdata->data_deal_buff[ch].station_buff[i]);
//		}
//	}

  /*FFT变换求得信号幅值*/
  float_fft(tzdata->data_deal_buff[ch].station_buff,tzdata->data_deal_buff[ch].size,FFT_NPT);

#ifdef DIAG_DATA_SAVE_FOR_TEST
	if(ch == YZ_CH_NUM)
	{
		save_pwtzz_diag_file(PWTZZ_TYPE, DIAG_DATA_FFT_AMP_TITLE, NULL, 1);
		save_pwtzz_diag_file(PWTZZ_TYPE, DIAG_DATA_FFT_AMP, &tzdata->data_deal_buff[ch].station_buff[0], tzdata->data_deal_buff[ch].size);
	}
#endif

#if 0//def SI_FANG_DATA_TEST
  int k = 0;

  if(ch == pw1_y)
  {
	  printf("pw1_y---ch:0---filter\n");

	  for(k=0; k<FFT_NPT; k++)
		  printf("%f\n", tzdata->data_deal_buff[ch].station_buff[k]);
  }
  else if(ch == pw1_z)
  {
	  printf("pw1_z---ch:1---filter\n");

	  for(k=0; k<FFT_NPT; k++)
		  printf("%f\n", tzdata->data_deal_buff[ch].station_buff[k]);
  }
#endif

 // printf("start FFT_VALUE:\r\n");
  for(index_i=1; index_i<=STABLE_HZ40_INDEX; index_i++)
  {
		raw_f=fft_index2hz(FFT_NPT,FS_PW,index_i);				//下标转频率
		if(raw_f>=0.5)
		{
			//fft_temp[index_i-1]=tzdata->data_deal_buff[ch].buff[index_i]*2.0f/FFT_NPT * FFT_MODIFY_PARA[index_i-1];//*2/N得到真实fft幅值
			fft_temp[index_i-1]=tzdata->data_deal_buff[ch].station_buff[index_i];			//fft 函数中已经将模值转化为信号幅值。由fft后的模值计算出对应的信号幅值

		}
   }

#ifdef DIAG_DATA_SAVE_FOR_TEST
	if(ch == YZ_CH_NUM)
	{
		save_pwtzz_diag_file(PWTZZ_TYPE, DIAG_DATA_40HZ_FFT_AMP_TITLE, NULL, 1);
		save_pwtzz_diag_file(PWTZZ_TYPE, DIAG_DATA_40HZ_FFT_AMP, &fft_temp[0], STABLE_HZ40_INDEX);
	}
#endif

//  if(ch == 0)
//  {
//	  for(index_i=0; index_i<512; index_i++)
//	  {
//		  printf("%f\r\n",tzdata->data_deal_buff[ch].station_buff[index_i]);
//	  }
//  }
//  printf("end FFT_VALUE\r\n");

    find_fft_max_value(fft_temp,&max_fft,&test_index);
    raw_f=fft_index2hz(FFT_NPT,FS_PW,test_index);

	//printf("tzdata->stable_fft_array[ch].row_index:%d\r\n",tzdata->stable_fft_array[ch].row_index);
	/*已经达到５次（５s）的数据，这个是第５次的数据，也就是５s的数据--->开始计算横向平稳型指标*/
	if(tzdata->stable_fft_array[ch].row_index >= tzdata->stable_fft_array[ch].row_size)
	{

		float fft_mean[STABLE_HZ40_INDEX];
		memset(fft_mean,0,sizeof(fft_mean));

#ifdef DIAG_DATA_SAVE_FOR_TEST
		if(ch == YZ_CH_NUM)
		{
			uint16_t i=0;

			for(i=0; i<STABLE_CAL_SENCOND; i++)
			{
				save_pwtzz_diag_file(PWTZZ_TYPE, DIAG_DATA_1S_FFT_AMP_TITLE, NULL, 1);
				save_pwtzz_diag_file(PWTZZ_TYPE, DIAG_DATA_1S_FFT_AMP, tzdata->stable_fft_array[ch].buff[i], STABLE_HZ40_INDEX);
			}
		}
#endif
		/*计算平均频谱,得到前5s的平均频谱*/
		average_fft_value(&tzdata->stable_fft_array[ch],fft_mean);

#ifdef DIAG_DATA_SAVE_FOR_TEST
		if(ch == YZ_CH_NUM)
		{
			save_pwtzz_diag_file(PWTZZ_TYPE, DIAG_DATA_5S_MEAN_FFT_AMP_TITLE, NULL, 1);
			save_pwtzz_diag_file(PWTZZ_TYPE, DIAG_DATA_5S_MEAN_FFT_AMP, (float*)fft_mean, STABLE_HZ40_INDEX);
		}
#endif
		/*将本次的平均频谱赋值到径跳频谱缓存中*/
		for(cnt_i = 0;cnt_i < 80;cnt_i++)
		{
			tzdata->wheel_diameter[ch].fft_value[cnt_i] = fft_mean[cnt_i];
		}


		/*次方和 ^10 + ^10 ....*/
		tzdata->diag_res[ch].value=0.0f;					/*最开始５s的数据都为0*/
		for(index_i=1;index_i<=STABLE_HZ40_INDEX;index_i++)
		{
			raw_f=fft_index2hz(FFT_NPT,FS_PW,index_i);

			A = fft_mean[index_i-1];						/*更新加速度为m/s2*/

			if((ch==0)||(ch==3))
				check_stable_cal_para(raw_f,&modify_F,PW_Y);
			else if((ch==1)||(ch==4))
				check_stable_cal_para(raw_f,&modify_F,PW_Z);

			tzdata->diag_res[ch].value+= 3.3627e+05*A * A * A / raw_f * modify_F;      //Wi^10   3.57^10 * A^3/f*F
		}

#ifdef DIAG_DATA_SAVE_FOR_TEST
		if(ch == YZ_CH_NUM)
		{
			save_pwtzz_diag_file(PWTZZ_TYPE, DIAG_DATA_PW_TZZ_SUM_TITLE, NULL, 1);
			save_pwtzz_diag_file(PWTZZ_TYPE, DIAG_DATA_PW_TZZ_SUM, &tzdata->diag_res[ch].value, 1);
		}
#endif

		/*^0.1 求幂，最后得到平稳性指标*/
		tzdata->diag_res[ch].value=pow(tzdata->diag_res[ch].value, 0.1);

#ifdef INTERNAL_PROTOCOL_20210725
		tzdata->diag_res[ch].value += 0.005;//变更要求：所有涉及小数点的指标值，均由后一位向前一位“四舍五入”，即如平稳性指标要求传输2位小数，则这个值由第三位小数向前“四舍五入”得到；
#endif

#ifdef DIAG_DATA_SAVE_FOR_TEST
		if(ch == YZ_CH_NUM)
		{
			save_pwtzz_diag_file(PWTZZ_TYPE, DIAG_DATA_PW_TZZ_TITLE, NULL, 1);
			save_pwtzz_diag_file(PWTZZ_TYPE, DIAG_DATA_PW_TZZ, &tzdata->diag_res[ch].value, 1);
		}
#endif

		PW_DIAG_PRINTF("tzdata->diag_res[%d].value:%f\n", ch, tzdata->diag_res[ch].value);
	}

	/*将新的一组1s的频谱，放入之前的频谱队列*/
	push_stable_fft_array(&tzdata->stable_fft_array[ch],fft_temp);//row_index++

}
#endif

void ad_filter(float *buff,uint32_t size,uint8_t ch_num)
{
	uint16_t cnt = 0;
	for(cnt = 0;cnt < size;cnt ++)
	{
#if 1//20211008
		if(buff[cnt] > 39340 && buff[cnt] < 45896) //test: y向:n39375,p45885,  z向:n39350,p45886    n负向　p正向   ad 10个余量
#else
		if(buff[cnt] > 40960 && buff[cnt] < 44237)
#endif
		{
			f_buff_out[ch_num] = buff[cnt];
		}
		else
		{
			buff[cnt] = f_buff_out[ch_num];
		}
	}
}


/**
 * 移位车体10s诊断数据
 * */

void move_ten_deal_data(struct PW_DIAGNOS_PARA *tzdata,uint8_t ch_num)
{
	uint16_t cnt_i = 0;
	uint16_t size = (TEN_DIAG_SEC-1)*FS_PW;
	for(cnt_i = 0;cnt_i < size;cnt_i++)
	{
		tzdata->ten_deal_data[ch_num].deal_buff[cnt_i] = tzdata->ten_deal_data[ch_num].deal_buff_bak[cnt_i+FS_PW];
		tzdata->ten_deal_data[ch_num].deal_buff_bak[cnt_i] = tzdata->ten_deal_data[ch_num].deal_buff_bak[cnt_i+FS_PW];
	}

	tzdata->ten_deal_data[ch_num].w_index = (TEN_DIAG_SEC-1)*FS_PW;
}

/**
 * 移位抖车诊断数据
 * */

void move_jitter_data(struct PW_DIAGNOS_PARA *tzdata,uint8_t ch_num)
{
	uint16_t cnt_i = 0;
	uint16_t size = (JITTER_DIAG_SEC-1)*512;
	for(cnt_i = 0;cnt_i < size;cnt_i++)
	{
		tzdata->jitter_float[ch_num].jitter_buff[cnt_i] = tzdata->jitter_float[ch_num].jitter_buff[cnt_i+FS_PW];
	}

	tzdata->jitter_float[ch_num].w_index = (JITTER_DIAG_SEC-1)*FS_PW;
}

#ifdef DC_HC_USE_FFT_FILTER
/**
 * 移位抖车2s fft数据
 * */
void move_shack_fft_data(struct PW_DIAGNOS_PARA *tzdata,uint8_t ch_num)
{
	uint16_t cnt_i = 0;
	uint16_t size = (SHACK_FFT_SEC-1)*512;
	for(cnt_i = 0;cnt_i < size;cnt_i++)
	{
		tzdata->shack_fft_in[ch_num].shack_buff[cnt_i] = tzdata->shack_fft_in[ch_num].shack_buff[cnt_i+FS_PW];
	}

	tzdata->shack_fft_in[ch_num].w_index = (SHACK_FFT_SEC-1)*FS_PW;
}

void move_jitter_fft_data(struct PW_DIAGNOS_PARA *tzdata,uint8_t ch_num)
{
	uint16_t cnt_i = 0;
	uint16_t size = (JITTER_FFT_SEC-1)*512;
	for(cnt_i = 0;cnt_i < size;cnt_i++)
	{
		tzdata->jitter_fft_in[ch_num].jitter_buff[cnt_i] = tzdata->jitter_fft_in[ch_num].jitter_buff[cnt_i+FS_PW];
	}

	tzdata->jitter_fft_in[ch_num].w_index = (JITTER_FFT_SEC-1)*FS_PW;
}
#endif

/**
 * 平稳性指标诊断数据移位处理
 * */
void move_deal_data(struct PW_DIAGNOS_PARA *tzdata,uint8_t ch_num)
{
	uint16_t cnt_i = 0;
	uint16_t size = (DIAG_SEC-1)*FS_PW;
	for(cnt_i = 0;cnt_i < size;cnt_i++)
	{
		tzdata->data_deal_buff[ch_num].station_buff[cnt_i] = tzdata->data_dealb_buff[ch_num].station_buff[cnt_i+FS_PW];
		tzdata->data_dealb_buff[ch_num].station_buff[cnt_i] = tzdata->data_dealb_buff[ch_num].station_buff[cnt_i+FS_PW];
	}

	tzdata->data_deal_buff[ch_num].w_index = (DIAG_SEC-1)*FS_PW;
	tzdata->data_dealb_buff[ch_num].w_index = (DIAG_SEC-1)*FS_PW;
}

/**
 * 5s速度诊断数据移位处理
 * */
void move_sample_speed_data(struct PW_DIAGNOS_PARA *tzdata,uint8_t ch_num)
{
	uint16_t cnt_i = 0;
	uint16_t size = JT_DIAG_SEC-1;
	for(cnt_i = 0;cnt_i < size;cnt_i++)
	{
		tzdata->wheel_diameter[ch_num].sample_speed[cnt_i] = tzdata->wheel_diameter[ch_num].sample_speed[cnt_i+1];
	}

	tzdata->wheel_diameter[ch_num].w_index = JT_DIAG_SEC-1;
}

#ifdef CT_10S_DIAG
/*
 * 获取车体10s数据诊断结果
 * */
void get_ten_data_res(enum FREQUENCY_BAND_E type, struct TEN_DEAL_DIAG_RES *diag_ten_res,uint16_t start_index,uint16_t end_index)
{
	float left_max_fre = 0.0f;
	float right_max_fre = 0.0f;

	uint16_t left_index = 0;
	uint16_t right_index = 0;
	uint16_t cnt_i = 0;

	float max_value = 0.0f;
	float main_fre = 0.0f;
	float percent_main_fre = 0.0f;

	max_value = diag_ten_res->fft_value[start_index];
	main_fre = start_index*FFT_RESOLUTION_128HZ;
	for(cnt_i = start_index+1;cnt_i <= end_index;cnt_i++)
	{
		if(diag_ten_res->fft_value[cnt_i] > max_value)
		{
			max_value = diag_ten_res->fft_value[cnt_i];		//最大幅值
			main_fre = cnt_i*FFT_RESOLUTION_128HZ;			//最大幅值对应的频率
		}
	}

	left_max_fre = main_fre - 0.1;
	left_index = (uint16_t)(left_max_fre/FFT_RESOLUTION_128HZ);

	right_max_fre = main_fre + 0.1;
	right_index = (uint16_t)(right_max_fre/FFT_RESOLUTION_128HZ);
	//cnt_i = (uint16_t)(diag_ten_res->main_fre/0.1);
	percent_main_fre = (max_value)/(diag_ten_res->fft_value[left_index] + max_value + diag_ten_res->fft_value[right_index]);

	if(type == FRE_0D2_TO_3HZ)
	{
		diag_ten_res->max_value1 = max_value;
		diag_ten_res->main_fre1 = main_fre;
		diag_ten_res->percent_main_fre1 = percent_main_fre;
	}
	else if(type == FRE_5_TO_13HZ)
	{
		diag_ten_res->max_value2 = max_value;
		diag_ten_res->main_fre2 = main_fre;
		diag_ten_res->percent_main_fre2 = percent_main_fre;
	}
}

void get_ten_data_res2(enum FREQUENCY_BAND_E type, struct TEN_DEAL_DIAG_RES *diag_ten_res,uint16_t start_index,uint16_t end_index,uint16_t array_index)
{
	float left_max_fre = 0.0f;
	float right_max_fre = 0.0f;

	uint16_t left_index = 0;
	uint16_t right_index = 0;
	uint16_t cnt_i = 0;

	float max_value = 0.0f;
	float main_fre = 0.0f;
	float percent_main_fre = 0.0f;

	max_value = diag_ten_res->fft_value[start_index];
	main_fre = start_index*FFT_RESOLUTION_128HZ;
	for(cnt_i = start_index+1;cnt_i <= end_index;cnt_i++)
	{
		if(diag_ten_res->fft_value[cnt_i] > max_value)
		{
			max_value = diag_ten_res->fft_value[cnt_i];		//最大幅值
			main_fre = cnt_i*FFT_RESOLUTION_128HZ;			//最大幅值对应的频率
		}
	}

	left_max_fre = main_fre - 0.1;
	left_index = (uint16_t)(left_max_fre/FFT_RESOLUTION_128HZ);

	right_max_fre = main_fre + 0.1;
	right_index = (uint16_t)(right_max_fre/FFT_RESOLUTION_128HZ);
	//cnt_i = (uint16_t)(diag_ten_res->main_fre/0.1);
	percent_main_fre = (max_value)/(diag_ten_res->fft_value[left_index] + max_value + diag_ten_res->fft_value[right_index]);

	if(type == FRE_0D2_TO_3HZ)
	{
		diag_ten_res->max_value1 = max_value;
		diag_ten_res->main_fre1 = main_fre;
		diag_ten_res->percent_main_fre1 = percent_main_fre;
	}
	else if(type == FRE_5_TO_13HZ)
	{
		diag_ten_res->max_value2 = max_value;
		diag_ten_res->main_fre2 = main_fre;
		diag_ten_res->percent_main_fre2 = percent_main_fre;
	}
#ifdef BRIDGE_DIAG
	else if(type == FRE_1_TO_3HZ)
	{
		diag_ten_res->max_value3[array_index] = max_value;
		diag_ten_res->main_fre3[array_index] = main_fre;
//		diag_ten_res->percent_main_fre3[array_index] = percent_main_fre;
	}
#endif
	//return max_fre;
}

/*******************
 * *车体10s振动数据诊断*
 *
 * ******************/
void diag_ten_deal_pw(struct PW_DIAGNOS_PARA *tzdata,uint8_t ch_num)
{
	uint16_t cnt_i = 0;
	uint16_t cnt_j = 0;

	uint16_t start_index = 0;
	uint16_t end_index = 0;

	float start_fre = 0;
	float end_fre = 0;
//	float fft_index = 0.125;//0.1;	/*频谱分辨率*/ //128Hz/1024

	if(ch_num == pw1_y || ch_num == pw2_y || ch_num == pw1_z || ch_num == pw2_z)
	{
		/*去均值处理  零点处理*/
		float_sub_mean(tzdata->ten_deal_data[ch_num].deal_buff, tzdata->ten_deal_data[ch_num].size);

		/*转加速度操作*/
		get_acc_data(tzdata->ten_deal_data[ch_num].deal_buff, tzdata->ten_deal_data[ch_num].size);

	//	printf("1\r\n");
		/*低通滤波 得到滤波后的数据*/
		filter_deal(&tzdata->ten_deal_data[ch_num],&tzdata->filter_para[ch_num]);
	//	printf("2\r\n");

	#ifdef DIAG_DATA_SAVE_FOR_TEST
		if(ch_num == pw1_y || ch_num == pw1_z)
		{
			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_AFTER_FILTER_TITLE, NULL, 1);
			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_AFTER_FILTER, &tzdata->ten_deal_data[ch_num].deal_buff[0], tzdata->ten_deal_data[ch_num].size);
		}
	#endif

		/*128Hz重采样*/
		cnt_j = 0;


		for(cnt_i = 0;cnt_i < tzdata->ten_deal_data[ch_num].size;cnt_i++)
		{
			if((cnt_i+1)%4 == 0)//1280
			{
				tzdata->ten_deal_data[ch_num].resample_deal_buff[cnt_j] = tzdata->ten_deal_data[ch_num].deal_buff[cnt_i];

				if(cnt_j>=256)//从1280中的下标256开始取1024个点用于ＦＦＴ
				{
					tzdata->ten_del_diag_res[ch_num].fft_value[cnt_j-256] = tzdata->ten_deal_data[ch_num].resample_deal_buff[cnt_j];
				}

				//printf("cnt_j:%d\r\n",cnt_j);
				cnt_j++;
			}
		}

	#ifdef DIAG_DATA_SAVE_FOR_TEST
		if(ch_num == pw1_y || ch_num == pw1_z)
		{
			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_RESAMPLE_AD_TITLE, NULL, 1);
			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_RESAMPLE_AD, &tzdata->ten_deal_data[ch_num].resample_deal_buff[0], cnt_j);
		}
	#endif

		//printf("3\r\n");
		/*频谱转换*/
		 float_fft(tzdata->ten_del_diag_res[ch_num].fft_value,FFT_NPT,FFT_NPT);

	#ifdef DIAG_DATA_SAVE_FOR_TEST
		if(ch_num == pw1_y || ch_num == pw1_z)
		{
			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_FFT_AMP_TITLE, NULL, 1);
			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_FFT_AMP, &tzdata->ten_del_diag_res[ch_num].fft_value[0], FFT_NPT);
		}
	#endif

		 if(ch_num == pw1_y || ch_num == pw2_y)
		 {
			 /*晃车频域处理*/
			 start_fre = 0.2f;
			 end_fre = 3.0f;
			 start_index = (uint16_t)(start_fre/FFT_RESOLUTION_128HZ);
			 end_index = (uint16_t)(end_fre/FFT_RESOLUTION_128HZ);
			 get_ten_data_res(FRE_0D2_TO_3HZ, &tzdata->ten_del_diag_res[ch_num],start_index,end_index);

		#ifdef DIAG_DATA_SAVE_FOR_TEST
			if(ch_num == pw1_y)
			{
				save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_MAX_FFT_AMP1_TITLE, NULL, 1);
				save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_MAX_FFT_AMP1, &tzdata->ten_del_diag_res[ch_num].max_value1, 1);

				save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_PERCENT_MAIN_FREQ1_TITLE, NULL, 1);
				save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_PERCENT_MAIN_FREQ1, &tzdata->ten_del_diag_res[ch_num].percent_main_fre1, 1);

				save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_MAIN_FREQ1_TITLE, NULL, 1);
				save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_MAIN_FREQ1, &tzdata->ten_del_diag_res[ch_num].main_fre1, 1);
			}
		#endif

//			PW_DIAG_PRINTF("diag_ten_deal_pw---hc-(0.2-3)Hz---ch_num:%d, max_amp:%f, percent_main_fre:%f, main_fre:%f\n",
//					ch_num, tzdata->ten_del_diag_res[ch_num].max_value, tzdata->ten_del_diag_res[ch_num].percent_main_fre, tzdata->ten_del_diag_res[ch_num].main_fre);
		 }

		// printf("4\r\n");
		 /*抖车频域处理*/
		 start_fre = 5.0f;
		 end_fre = 13.0f;
		 start_index = (uint16_t)(start_fre/FFT_RESOLUTION_128HZ);
		 end_index = (uint16_t)(end_fre/FFT_RESOLUTION_128HZ);
		 get_ten_data_res(FRE_5_TO_13HZ, &tzdata->ten_del_diag_res[ch_num],start_index,end_index);

	#ifdef DIAG_DATA_SAVE_FOR_TEST
		if(ch_num == pw1_y || ch_num == pw1_z)
		{
			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_MAX_FFT_AMP2_TITLE, NULL, 1);
			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_MAX_FFT_AMP2, &tzdata->ten_del_diag_res[ch_num].max_value2, 1);

			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_PERCENT_MAIN_FREQ2_TITLE, NULL, 1);
			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_PERCENT_MAIN_FREQ2, &tzdata->ten_del_diag_res[ch_num].percent_main_fre2, 1);

			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_MAIN_FREQ2_TITLE, NULL, 1);
			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_MAIN_FREQ2, &tzdata->ten_del_diag_res[ch_num].main_fre2, 1);
		}
	#endif

	#ifdef BRIDGE_DIAG
		/*桥梁滤波算法*/
		float bridge_len = 0.0f;
//		float d_len = 0.0f;
		float mean_len = 0.0f;
		float speed = 0.0f;
		start_fre = 1.0f;
		end_fre = 3.0f;
		start_index = (uint16_t)(start_fre/FFT_RESOLUTION_128HZ);
		end_index = (uint16_t)(end_fre/FFT_RESOLUTION_128HZ);

//		printf("-------------------------1\n");
#ifdef DATA_CENTER_TEST
		if(ch_num == pw1_z)
#else
		if(ch_num == pw1_z || ch_num == pw2_z)
#endif
		{
			tzdata->ten_del_diag_res[ch_num].max_fabs_value3[tzdata->ten_del_diag_res[ch_num].keep_len_times] = float_get_max_fabs(tzdata->ten_deal_data[ch_num].deal_buff, FS_PW*TEN_DIAG_SEC);

			get_ten_data_res2(FRE_1_TO_3HZ, &tzdata->ten_del_diag_res[ch_num], start_index, end_index, tzdata->ten_del_diag_res[ch_num].keep_len_times);

			//10秒速度算完平均后，滑移1秒，下标指数减1
			speed = float_mean(tzdata->ten_del_diag_res[ch_num].speed, BRIDGE_DATA_LENGTH);
			memmove(&tzdata->ten_del_diag_res[ch_num].speed[0], &tzdata->ten_del_diag_res[ch_num].speed[1], (BRIDGE_DATA_LENGTH-1)*sizeof(float));
			tzdata->ten_del_diag_res[ch_num].speed_cnt--;

			bridge_len = speed/(3.6*tzdata->ten_del_diag_res[ch_num].main_fre3[tzdata->ten_del_diag_res[ch_num].keep_len_times]);
			bridge_len = self_round(bridge_len, 1);
			tzdata->ten_del_diag_res[ch_num].bridge_len[tzdata->ten_del_diag_res[ch_num].keep_len_times] = bridge_len;

		#ifdef DIAG_DATA_SAVE_FOR_TEST
			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_MAX_FABS_AMP3_TITLE, NULL, 1);
			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_MAX_FABS_AMP3, &tzdata->ten_del_diag_res[ch_num].max_fabs_value3, BRIDGE_DATA_LENGTH);

			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_MAX_FFT_AMP3_TITLE, NULL, 1);
			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_MAX_FFT_AMP3, &tzdata->ten_del_diag_res[ch_num].max_value3, BRIDGE_DATA_LENGTH);

			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_MAIN_FREQ3_TITLE, NULL, 1);
			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_MAIN_FREQ3, &tzdata->ten_del_diag_res[ch_num].main_fre3, BRIDGE_DATA_LENGTH);

//			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_PERCENT_MAIN_FREQ3_TITLE, NULL, 1);
//			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_PERCENT_MAIN_FREQ3, &tzdata->ten_del_diag_res[ch_num].percent_main_fre3, BRIDGE_DATA_LENGTH);

			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_AVERAGE_SPEED_TITLE, NULL, 1);
			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_AVERAGE_SPEED, &speed, 1);

			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_BRIDGE_LEN_TITLE, NULL, 1);
			save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_BRIDGE_LEN, &tzdata->ten_del_diag_res[ch_num].bridge_len, BRIDGE_DATA_LENGTH);
		#endif
		#ifdef ADD_DIAG_TZZ_DATA_FILE
			if(ch_num == pw1_z)
			{
				diag_tzz_data.ten_bridge_z_max_fabs_amp[0] = tzdata->ten_del_diag_res[ch_num].max_fabs_value3[tzdata->ten_del_diag_res[ch_num].keep_len_times];
				diag_tzz_data.ten_bridge_z_max_fre[0] = tzdata->ten_del_diag_res[ch_num].main_fre3[tzdata->ten_del_diag_res[ch_num].keep_len_times];
				diag_tzz_data.ten_bridge_z_max_amp[0] = tzdata->ten_del_diag_res[ch_num].max_value3[tzdata->ten_del_diag_res[ch_num].keep_len_times];
				diag_tzz_data.ten_bridge_avg_speed[0] = speed;
				diag_tzz_data.ten_bridge_len[0] = bridge_len;
			}
			else if(ch_num == pw2_z)
			{
				diag_tzz_data.ten_bridge_z_max_fabs_amp[1] = tzdata->ten_del_diag_res[ch_num].max_fabs_value3[tzdata->ten_del_diag_res[ch_num].keep_len_times];
				diag_tzz_data.ten_bridge_z_max_fre[1] = tzdata->ten_del_diag_res[ch_num].main_fre3[tzdata->ten_del_diag_res[ch_num].keep_len_times];
				diag_tzz_data.ten_bridge_z_max_amp[1] = tzdata->ten_del_diag_res[ch_num].max_value3[tzdata->ten_del_diag_res[ch_num].keep_len_times];
				diag_tzz_data.ten_bridge_avg_speed[1] = speed;
				diag_tzz_data.ten_bridge_len[1] = bridge_len;
			}
		#endif
//			printf("1---speed=%f\n", speed);
//			printf("1---max_value3=%f\n", tzdata->ten_del_diag_res[ch_num].max_value3[tzdata->ten_del_diag_res[ch_num].keep_len_times]);
//			printf("1---main_fre3=%f\n", tzdata->ten_del_diag_res[ch_num].main_fre3[tzdata->ten_del_diag_res[ch_num].keep_len_times]);
//			printf("1---bridge_len=%f\n", bridge_len);

			tzdata->ten_del_diag_res[ch_num].keep_len_times++;

			if(tzdata->ten_del_diag_res[ch_num].keep_len_times == BRIDGE_DATA_LENGTH)
			{
				mean_len = self_round(float_mean(tzdata->ten_del_diag_res[ch_num].bridge_len, BRIDGE_DATA_LENGTH), 1);

				for(cnt_i=0; cnt_i<BRIDGE_DATA_LENGTH; cnt_i++)
				{
					if(fabs(tzdata->ten_del_diag_res[ch_num].bridge_len[cnt_i] - mean_len)<BRIDGE_LEN_MEAN_UPDOWN_LIMIT
						&& (tzdata->ten_del_diag_res[ch_num].bridge_len[cnt_i]>=BRIDGE_LEN_DOWN && tzdata->ten_del_diag_res[ch_num].bridge_len[cnt_i]<=BRIDGE_LEN_UP)
						&& tzdata->ten_del_diag_res[ch_num].max_value3[cnt_i]>BRIDGE_FACTOR && tzdata->ten_del_diag_res[ch_num].max_fabs_value3[cnt_i]<BRIDGE_TIME_DOMAIN_AMP_FABS)
					{
						if(cnt_i == BRIDGE_DATA_LENGTH-1)
						{
							tzdata->ten_del_diag_res[ch_num].bridge_flag = 1;
						}
						else
						{
							tzdata->ten_del_diag_res[ch_num].bridge_flag = 0;
						}
					}
					else
					{
						tzdata->ten_del_diag_res[ch_num].bridge_flag = 0;

						break;
					}
				}

//				printf("1---mean_len=%f\n", mean_len);
//				printf("1---bridge_flag=%d\n", tzdata->ten_del_diag_res[ch_num].bridge_flag);

			#ifdef DIAG_DATA_SAVE_FOR_TEST
				save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_AVERAGE_BRIDGE_LEN_TITLE, NULL, 1);
				save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_AVERAGE_BRIDGE_LEN, &mean_len, 1);
				save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_BRIDGE_FLAG_TITLE, NULL, 1);
				save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_BRIDGE_FLAG, &tzdata->ten_del_diag_res[ch_num].bridge_flag, 1);
			#endif
			#ifdef ADD_DIAG_TZZ_DATA_FILE
				if(ch_num == pw1_z)
				{
					diag_tzz_data.ten_bridge_avg_len[0] = mean_len;
					diag_tzz_data.ten_bridge_flag[0] = tzdata->ten_del_diag_res[ch_num].bridge_flag;
				}
				else if(ch_num == pw2_z)
				{
					diag_tzz_data.ten_bridge_avg_len[1] = mean_len;
					diag_tzz_data.ten_bridge_flag[1] = tzdata->ten_del_diag_res[ch_num].bridge_flag;
				}
			#endif
				//10个数比较完后，依次前移一个数据
				memmove(&tzdata->ten_del_diag_res[ch_num].max_value3[0], &tzdata->ten_del_diag_res[ch_num].max_value3[1], (BRIDGE_DATA_LENGTH-1)*sizeof(float));
				memmove(&tzdata->ten_del_diag_res[ch_num].max_fabs_value3[0], &tzdata->ten_del_diag_res[ch_num].max_fabs_value3[1], (BRIDGE_DATA_LENGTH-1)*sizeof(float));
				memmove(&tzdata->ten_del_diag_res[ch_num].main_fre3[0], &tzdata->ten_del_diag_res[ch_num].main_fre3[1], (BRIDGE_DATA_LENGTH-1)*sizeof(float));
//				memmove(&tzdata->ten_del_diag_res[ch_num].percent_main_fre3[0], &tzdata->ten_del_diag_res[ch_num].bridge_len[1], BRIDGE_DATA_LENGTH-1);
				memmove(&tzdata->ten_del_diag_res[ch_num].bridge_len[0], &tzdata->ten_del_diag_res[ch_num].bridge_len[1], (BRIDGE_DATA_LENGTH-1)*sizeof(float));

				tzdata->ten_del_diag_res[ch_num].keep_len_times = BRIDGE_DATA_LENGTH - 1;
			}
		}
	#endif
//		 PW_DIAG_PRINTF("diag_ten_deal_pw---dc-(5-13)Hz---ch_num:%d, max_amp:%f, percent_main_fre:%f, main_fre:%f\n",
//				 ch_num, tzdata->ten_del_diag_res[ch_num].max_value, tzdata->ten_del_diag_res[ch_num].percent_main_fre, tzdata->ten_del_diag_res[ch_num].main_fre);
	}
}
#endif

#ifdef DC_DIAG
/********************
 * 抖车阈值诊断
 * *****************/
void jitter_diag_res_judge(struct PW_DIAGNOS_PARA *tzdata,uint8_t ch_num)
{
	float warn_threshold_val=0.0f;
	float alarm_threshold_val=0.0f;
	uint32_t warn_threshold_hn = 0;
	uint32_t warn_threshold_ln = 0;
	uint32_t alarm_threshold_hn = 0;
	uint32_t alarm_threshold_ln = 0;
#ifdef DC_DIAG_ADD_PW_JUDGE
	float pw_warn_threshold_val=0.0f;
	float pw_alarm_threshold_val=0.0f;
#endif
//	uint16_t cnt_i = 0;

	if(ch_num == pw1_y)
	{
		/*抖车预警阈值*/
		warn_threshold_val = pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_warn_value;
		warn_threshold_hn = pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_warn_h_cnt;
		warn_threshold_ln = pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_warn_l_cnt;
		/*抖车报警阈值*/
		alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_alarm_value;
		alarm_threshold_hn = pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_alarm_h_cnt;
		alarm_threshold_ln = pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_alarm_l_cnt;
#ifdef DC_DIAG_ADD_PW_JUDGE
		if(pw_tz_data.train_public_info.carriage_number == 0 || pw_tz_data.train_public_info.carriage_number == 7)//8编车 头＼尾车为司机室
		{
			warn_threshold_val += 0.01;
			alarm_threshold_val += 0.01;
			pw_warn_threshold_val = pw_clb_config_para->pw_diagnos_para.side1_y_para.warn_value;//2.75
			pw_alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side1_y_para.alarm_value;//3.0
		}
		else
		{
			pw_warn_threshold_val = pw_clb_config_para->pw_diagnos_para.side1_y_para.warn_value-0.25;//2.5
			pw_alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side1_y_para.alarm_value-0.25;//2.75
		}
#endif
	}
	else if(ch_num == pw2_y)
	{
		/*抖车预警阈值*/
		warn_threshold_val = pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_warn_value;
		warn_threshold_hn = pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_warn_h_cnt;
		warn_threshold_ln = pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_warn_l_cnt;
		/*抖车报警阈值*/
		alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_alarm_value;
		alarm_threshold_hn = pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_alarm_h_cnt;
		alarm_threshold_ln = pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_alarm_l_cnt;
#ifdef DC_DIAG_ADD_PW_JUDGE
		if(pw_tz_data.train_public_info.carriage_number == 0 || pw_tz_data.train_public_info.carriage_number == 7)//8编车 头＼尾车为司机室
		{
			warn_threshold_val += 0.01;
			alarm_threshold_val += 0.01;
			pw_warn_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_y_para.warn_value;//2.75
			pw_alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_y_para.alarm_value;//3.0
		}
		else
		{
			pw_warn_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_y_para.warn_value-0.25;//2.5
			pw_alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_y_para.alarm_value-0.25;//2.75
		}
#endif
	}
	else if(ch_num == pw1_z)
	{
		/*抖车预警阈值*/
		warn_threshold_val = pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_warn_value;
		warn_threshold_hn = pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_warn_h_cnt;
		warn_threshold_ln = pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_warn_l_cnt;
		/*抖车报警阈值*/
		alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_alarm_value;
		alarm_threshold_hn = pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_alarm_h_cnt;
		alarm_threshold_ln = pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_alarm_l_cnt;
#ifdef DC_DIAG_ADD_PW_JUDGE
		if(pw_tz_data.train_public_info.carriage_number == 0 || pw_tz_data.train_public_info.carriage_number == 7)//8编车 头＼尾车为司机室
		{
			warn_threshold_val += 0.01;
			alarm_threshold_val += 0.01;
			pw_warn_threshold_val = pw_clb_config_para->pw_diagnos_para.side1_z_para.warn_value;//2.75
			pw_alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side1_z_para.alarm_value;//3.0
		}
		else
		{
			pw_warn_threshold_val = pw_clb_config_para->pw_diagnos_para.side1_z_para.warn_value-0.25;//2.5
			pw_alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side1_z_para.alarm_value-0.25;//2.75
		}
#endif
	}
	else if(ch_num == pw2_z)
	{
		/*抖车预警阈值*/
		warn_threshold_val = pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_warn_value;
		warn_threshold_hn = pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_warn_h_cnt;
		warn_threshold_ln = pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_warn_l_cnt;
		/*抖车报警阈值*/
		alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_alarm_value;
		alarm_threshold_hn = pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_alarm_h_cnt;
		alarm_threshold_ln = pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_alarm_l_cnt;
#ifdef DC_DIAG_ADD_PW_JUDGE
		if(pw_tz_data.train_public_info.carriage_number == 0 || pw_tz_data.train_public_info.carriage_number == 7)//8编车 头＼尾车为司机室
		{
			warn_threshold_val += 0.01;
			alarm_threshold_val += 0.01;
			pw_warn_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_z_para.warn_value;//2.75
			pw_alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_z_para.alarm_value;//3.0
		}
		else
		{
			pw_warn_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_z_para.warn_value-0.25;//2.5
			pw_alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_z_para.alarm_value-0.25;//2.75
		}
#endif
	}


//	if(ch_num == 0)
//	{
//		printf("tzdata->jitter_diag_res[ch_num].value:%f,ch_num:%d\r\n",tzdata->jitter_diag_res[ch_num].value,ch_num);
//	}

	if(tzdata->jitter_diag_res[ch_num].value > alarm_threshold_val && tzdata->diag_res[ch_num].alarm_status_new == RUNNING_ALARM
#ifdef DC_DIAG_ADD_PW_JUDGE
		&& tzdata->diag_res[ch_num].value > pw_alarm_threshold_val
#endif
			)//大于报警阈值
	{
		tzdata->jitter_diag_res[ch_num].alarm_l_num = 0;
		tzdata->jitter_diag_res[ch_num].warn_l_num = 0;
		tzdata->jitter_diag_res[ch_num].warn_h_num++;
		tzdata->jitter_diag_res[ch_num].alarm_h_num++;
	}
	else if(tzdata->jitter_diag_res[ch_num].value > warn_threshold_val && tzdata->diag_res[ch_num].alarm_status_new == RUNNING_WARN
#ifdef DC_DIAG_ADD_PW_JUDGE
		&& tzdata->diag_res[ch_num].value > pw_warn_threshold_val
#endif
			)//大于预警阈值
	{
		tzdata->jitter_diag_res[ch_num].alarm_l_num ++;
		tzdata->jitter_diag_res[ch_num].warn_l_num = 0;
		tzdata->jitter_diag_res[ch_num].warn_h_num++;
		tzdata->jitter_diag_res[ch_num].alarm_h_num = 0;
	}
	else if((tzdata->jitter_diag_res[ch_num].value < warn_threshold_val && tzdata->diag_res[ch_num].alarm_status_new == RUNNING_OK)
#ifdef DC_DIAG_ADD_PW_JUDGE
		|| tzdata->diag_res[ch_num].value < pw_warn_threshold_val
#endif
			)//低于预警阈值
	{
		tzdata->jitter_diag_res[ch_num].alarm_l_num ++;
		tzdata->jitter_diag_res[ch_num].warn_l_num ++;
		tzdata->jitter_diag_res[ch_num].warn_h_num = 0;
		tzdata->jitter_diag_res[ch_num].alarm_h_num = 0;
	}


	/*抖车预报警判断*/
	if(tzdata->jitter_diag_res[ch_num].alarm_h_num > alarm_threshold_hn)//10
	{
		tzdata->jitter_diag_res[ch_num].alarm_status_new=RUNNING_ALARM;//左移１，以便update_pw_tz_data或运算
		if(tzdata->jitter_diag_res[ch_num].alarm_status_old != tzdata->jitter_diag_res[ch_num].alarm_status_new)
		{
			tzdata->jitter_diag_res[ch_num].alarm_status_old = tzdata->jitter_diag_res[ch_num].alarm_status_new;	//抖车报警

			if(sys_status_cnt.err_type[ch_num].jitter_diag_alarm_save_flag == 0)
			{
				sys_status_cnt.err_type[ch_num].jitter_diag_alarm_save_flag = 1;										//报警存储标志置１
				sys_status_cnt.err_type[ch_num].jitter_diag_normal_save_flag = 0;
			}
		}

		if(tzdata->jitter_diag_res[ch_num].warn_status_new != RUNNING_OK)
		{
			tzdata->jitter_diag_res[ch_num].warn_status_old = RUNNING_OK;
			tzdata->jitter_diag_res[ch_num].warn_status_new = RUNNING_OK;
			if(sys_status_cnt.err_type[ch_num].jitter_diag_warn_save_flag == 0)
			{
				sys_status_cnt.err_type[ch_num].jitter_diag_warn_save_flag = 1;										//预警存储标志置１
				sys_status_cnt.err_type[ch_num].jitter_diag_normal_save_flag = 0;
			}
		}
	}
	else if(tzdata->jitter_diag_res[ch_num].warn_h_num > warn_threshold_hn)//10
	{
		tzdata->jitter_diag_res[ch_num].warn_status_new=RUNNING_WARN;
		if(tzdata->jitter_diag_res[ch_num].warn_status_old != tzdata->jitter_diag_res[ch_num].warn_status_new)
		{
			tzdata->jitter_diag_res[ch_num].warn_status_old = tzdata->jitter_diag_res[ch_num].warn_status_new;	//抖车预警

			if(sys_status_cnt.err_type[ch_num].jitter_diag_warn_save_flag == 0)
			{
				sys_status_cnt.err_type[ch_num].jitter_diag_warn_save_flag = 1;										//预警存储标志置１
				sys_status_cnt.err_type[ch_num].jitter_diag_normal_save_flag = 0;
			}
		}

		if(tzdata->jitter_diag_res[ch_num].alarm_status_new != RUNNING_OK)
		{
			tzdata->jitter_diag_res[ch_num].alarm_status_old = RUNNING_OK;
			tzdata->jitter_diag_res[ch_num].alarm_status_new = RUNNING_OK;
			if(sys_status_cnt.err_type[ch_num].jitter_diag_alarm_save_flag == 0)
			{
				sys_status_cnt.err_type[ch_num].jitter_diag_alarm_save_flag = 1;										//报警存储标志置１
				sys_status_cnt.err_type[ch_num].jitter_diag_normal_save_flag = 0;
			}
		}
	}
	else if(tzdata->jitter_diag_res[ch_num].warn_l_num > warn_threshold_ln && tzdata->jitter_diag_res[ch_num].alarm_l_num > alarm_threshold_ln)
	{
		tzdata->jitter_diag_res[ch_num].alarm_status_new = RUNNING_OK;						//抖车恢复正常
		if(tzdata->jitter_diag_res[ch_num].alarm_status_old != tzdata->jitter_diag_res[ch_num].alarm_status_new)
		{
			tzdata->jitter_diag_res[ch_num].alarm_status_old = tzdata->jitter_diag_res[ch_num].alarm_status_new;
			if(sys_status_cnt.err_type[ch_num].jitter_diag_normal_save_flag == 0)
			{
				sys_status_cnt.err_type[ch_num].jitter_diag_normal_save_flag = 1;
				sys_status_cnt.err_type[ch_num].jitter_diag_alarm_save_flag = 0;				//抖车报警存储标志置0
			}
		}

		tzdata->jitter_diag_res[ch_num].warn_status_new = RUNNING_OK;						//抖车恢复正常
		if(tzdata->jitter_diag_res[ch_num].warn_status_old != tzdata->jitter_diag_res[ch_num].warn_status_new)
		{
			tzdata->jitter_diag_res[ch_num].warn_status_old = tzdata->jitter_diag_res[ch_num].warn_status_new;
			if(sys_status_cnt.err_type[ch_num].jitter_diag_normal_save_flag == 0)
			{
				sys_status_cnt.err_type[ch_num].jitter_diag_normal_save_flag = 1;
				sys_status_cnt.err_type[ch_num].jitter_diag_warn_save_flag = 0;					//抖车预警存储标志置0
			}
		}
	}

#ifdef WTD_DATA_TRANSLATE_PROTOCOL
#ifdef WTD_SIMULATION_ALARM_TRIG//仅测试时用
	if(!simulation_open_flag)
#endif
	{
		if(tzdata->jitter_diag_res[ch_num].alarm_status_new == RUNNING_ALARM || tzdata->jitter_diag_res[ch_num].warn_status_new == RUNNING_WARN)
		{
			if(msg_alarm_trigger[ch_num].alarm_tz_type == NO_ALARM_TYPE
					&& msg_alarm_trigger[ch_num].alarm_trig_status == TRIG_OK)
			{
				msg_alarm_trigger[ch_num].alarm_trig_status = TRIG_ALARM;
				msg_alarm_trigger[ch_num].alarm_valid_flag = ALARM_VALID;

				if(ch_num == pw1_y)
					msg_alarm_trigger[ch_num].alarm_tz_type = PW1_Y_DC_ALARM_TYPE;
				else if(ch_num == pw1_z)
					msg_alarm_trigger[ch_num].alarm_tz_type = PW1_Z_DC_ALARM_TYPE;
				else if(ch_num == pw2_y)
					msg_alarm_trigger[ch_num].alarm_tz_type = PW2_Y_DC_ALARM_TYPE;
				else if(ch_num == pw2_z)
					msg_alarm_trigger[ch_num].alarm_tz_type = PW2_Z_DC_ALARM_TYPE;
			}
		}
		else if(tzdata->jitter_diag_res[ch_num].alarm_status_new == RUNNING_OK && tzdata->jitter_diag_res[ch_num].warn_status_new == RUNNING_OK)
		{
			if( ((msg_alarm_trigger[ch_num].alarm_tz_type == PW1_Y_DC_ALARM_TYPE && ch_num == pw1_y)
			  || (msg_alarm_trigger[ch_num].alarm_tz_type == PW1_Z_DC_ALARM_TYPE && ch_num == pw1_z)
			  || (msg_alarm_trigger[ch_num].alarm_tz_type == PW2_Y_DC_ALARM_TYPE && ch_num == pw2_y)
			  || (msg_alarm_trigger[ch_num].alarm_tz_type == PW2_Z_DC_ALARM_TYPE && ch_num == pw2_z))
					&& msg_alarm_trigger[ch_num].alarm_trig_status == TRIG_ALARM )
			{
				msg_alarm_trigger[ch_num].alarm_trig_status = TRIG_ALARM_REMOVE;
				msg_alarm_trigger[ch_num].alarm_valid_flag = ALARM_VALID;//30s后才无效
			}
		}
	}
#endif
}

#ifdef DC_HC_USE_FFT_FILTER
/*******************
 * *抖车诊断*
 *
 * ******************/
void diag_jitter_pw(struct PW_DIAGNOS_PARA *tzdata,uint8_t ch_num)
{
//	tzdata->jitter_float_deal[ch_num].sec_status = FIRST_SEC;

	PW_DIAG_PRINTF("diag_jitter_pw:\n");
	/*抖车只需要横向和垂向数据*/
	if(ch_num == pw1_y || ch_num == pw2_y || ch_num == pw1_z || ch_num == pw2_z)
	{
#if !defined(DATA_CENTER_TEST)
//		ad_filter(tzdata->jitter_float_deal[ch_num].jitter_buff,tzdata->jitter_float_deal[ch_num].size,ch_num);
#endif

		/*去均值*/
		float_sub_mean(tzdata->jitter_float_deal[ch_num].jitter_buff, tzdata->jitter_float_deal[ch_num].size);

#ifndef DATA_CENTER_TEST
		/*转加速度操作*/
		get_acc_data(tzdata->jitter_float_deal[ch_num].jitter_buff, tzdata->jitter_float_deal[ch_num].size);
#endif

//#if 0//test
//		int k;
//
//		if(ch_num == pw1_y)
//		{
//			for(k=0; k<512; k++)
//			{
//				tzdata->jitter_float_deal[ch_num].jitter_buff[k] = 0.1*sin(2*PI*8*k/512);//real   f=8Hz   w=2*PI*2/1024   T=2*PI/w=512
//			}
//		}
//#endif

#ifdef DIAG_DATA_SAVE_FOR_TEST
		if(ch_num == pw1_y)
		{
			save_dctzz_diag_file(DCTZZ_TYPE, DIAG_DATA_ACC_TITLE, NULL, 1);
			save_dctzz_diag_file(DCTZZ_TYPE, DIAG_DATA_ACC, &tzdata->jitter_float_deal[ch_num].jitter_buff[0], tzdata->jitter_float_deal[ch_num].size);
		}
#endif
		//2s fft_in & move
		move_float_to_float(&tzdata->jitter_fft_in[ch_num].jitter_buff[tzdata->jitter_fft_in[ch_num].w_index], tzdata->jitter_float_deal[ch_num].jitter_buff, FS_PW);
		tzdata->jitter_fft_in[ch_num].w_index += FS_PW;

		if(tzdata->jitter_fft_in[ch_num].w_index == JITTER_FFT_SEC*FS_PW)
		{
	#ifdef DIAG_DATA_SAVE_FOR_TEST
			if(ch_num == pw1_y)
			{
				save_dctzz_diag_file(DCTZZ_TYPE, DIAG_DATA_BEFORE_FILTER_TITLE, NULL, 1);
				save_dctzz_diag_file(DCTZZ_TYPE, DIAG_DATA_BEFORE_FILTER, &tzdata->jitter_fft_in[ch_num].jitter_buff[0], tzdata->jitter_fft_in[ch_num].size);
			}
	#endif
			/*fft ifft 滤波*/
			fft_ifft_deal(tzdata->jitter_fft_in[ch_num].jitter_buff, tzdata->jitter_float_deal[ch_num].jitter_buff, FRE_5_TO_13HZ);

	#ifdef DIAG_DATA_SAVE_FOR_TEST
			if(ch_num == pw1_y)
			{
				save_dctzz_diag_file(DCTZZ_TYPE, DIAG_DATA_AFTER_FILTER_TITLE, NULL, 1);
				save_dctzz_diag_file(DCTZZ_TYPE, DIAG_DATA_AFTER_FILTER, &tzdata->jitter_float_deal[ch_num].jitter_buff[0], tzdata->jitter_float_deal[ch_num].size);
			}
	#endif
			move_jitter_fft_data(tzdata, ch_num);
		}

		/*均方根前　去均值wu*/
		float_sub_mean(tzdata->jitter_float_deal[ch_num].jitter_buff, FS_PW);//1s

		//5s fft_out & move
		move_float_to_float(&tzdata->jitter_float[ch_num].jitter_buff[tzdata->jitter_float[ch_num].w_index], tzdata->jitter_float_deal[ch_num].jitter_buff, FS_PW);
		tzdata->jitter_float[ch_num].w_index += FS_PW;

		if(tzdata->jitter_float[ch_num].w_index == JITTER_DIAG_SEC*FS_PW)
		{
	#ifdef DIAG_DATA_SAVE_FOR_TEST
			if(ch_num == pw1_y)
			{
				save_dctzz_diag_file(DCTZZ_TYPE, DIAG_DATA_5S_AFTER_FILTER_TITLE, NULL, 1);
				save_dctzz_diag_file(DCTZZ_TYPE, DIAG_DATA_5S_AFTER_FILTER, &tzdata->jitter_float[ch_num].jitter_buff[0], tzdata->jitter_float[ch_num].size);
			}
	#endif

			/*计算均方根 5s数据出一个均方根结果,均方根前去均值wu*/
			tzdata->jitter_diag_res[ch_num].value = rms_deal(tzdata->jitter_float[ch_num].jitter_buff, JITTER_DIAG_SEC*FS_PW);//范围0～0.256g,1=0.001g

	#ifdef DIAG_DATA_SAVE_FOR_TEST
			if(ch_num == pw1_y)
			{
				save_dctzz_diag_file(DCTZZ_TYPE, DIAG_DATA_DC_TZZ_TITLE, NULL, 1);
				save_dctzz_diag_file(DCTZZ_TYPE, DIAG_DATA_DC_TZZ, &tzdata->jitter_diag_res[ch_num].value, 1);
				//printf("diag_jitter_pw---Root_mean_square:%f\n", tzdata->jitter_diag_res[ch_num].value);
			}
	#endif

			PW_DIAG_PRINTF("diag_jitter_pw---Root_mean_square:%f\n", tzdata->jitter_diag_res[ch_num].value);

			/*抖车阈值诊断*/
			jitter_diag_res_judge(tzdata, ch_num);

			move_jitter_data(tzdata, ch_num);
		}

	}//end if
}
#else
void diag_jitter_pw(struct PW_DIAGNOS_PARA *tzdata,uint8_t ch_num)
{
	uint16_t cnt_i = 0;
	/*抖车只需要横向和垂向数据*/
	if(ch_num == pw1_y || ch_num == pw2_y || ch_num == pw1_z || ch_num == pw2_z)
	{
		float warn_threshold_val=0.0f;
		float alarm_threshold_val=0.0f;
		uint32_t warn_threshold_hn = 0;
		uint32_t warn_threshold_ln = 0;
		uint32_t alarm_threshold_hn = 0;
		uint32_t alarm_threshold_ln = 0;

#ifndef PW_DIAG_TEST
//		ad_filter(tzdata->jitter_float[ch_num].jitter_buff,tzdata->jitter_float[ch_num].size,ch_num);
#endif

		/*去均值*/
		//printf("tzdata->jitter_float[ch_num].size:%d\r\n",tzdata->jitter_float[ch_num].size);
//		float_sub_mean(tzdata->jitter_float[ch_num].jitter_buff,tzdata->jitter_float[ch_num].size);



		//		/*数据带通滤波*/
		//		if(ch_num == pw1_y || ch_num == pw2_y)
		//		{
		//			jitter_filter_deal(&tzdata->jitter_float[ch_num],&tzdata->jitter_filter_para[ch_num],PW_Y);
		//		}
		//		else if(ch_num == pw1_z || ch_num == pw2_z)
		//		{
		//			jitter_filter_deal(&tzdata->jitter_float[ch_num],&tzdata->jitter_filter_para[ch_num],PW_Z);
		//		}


		/*计算均方根 5s数据出一个均方根结果*/
//		rms_deal(tzdata->jitter_float[ch_num].jitter_buff,tzdata->jitter_float[ch_num].size);

		/**/
		if(ch_num == pw1_y)
		{
			/*抖车预警阈值*/
			warn_threshold_val = pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_warn_value;
			warn_threshold_hn = pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_warn_h_cnt;
			warn_threshold_ln = pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_warn_l_cnt;
			/*抖车报警阈值*/
			alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_alarm_value;
			alarm_threshold_hn = pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_alarm_h_cnt;
			alarm_threshold_ln = pw_clb_config_para->pw_diagnos_para.jitter_side1_y_para.jitter_alarm_l_cnt;
		}
		else if(ch_num == pw2_y)
		{
			/*抖车预警阈值*/
			warn_threshold_val = pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_warn_value;
			warn_threshold_hn = pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_warn_h_cnt;
			warn_threshold_ln = pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_warn_l_cnt;
			/*抖车报警阈值*/
			alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_alarm_value;
			alarm_threshold_hn = pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_alarm_h_cnt;
			alarm_threshold_ln = pw_clb_config_para->pw_diagnos_para.jitter_side2_y_para.jitter_alarm_l_cnt;
		}
		else if(ch_num == pw1_z)
		{
			/*抖车预警阈值*/
			warn_threshold_val = pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_warn_value;
			warn_threshold_hn = pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_warn_h_cnt;
			warn_threshold_ln = pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_warn_l_cnt;
			/*抖车报警阈值*/
			alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_alarm_value;
			alarm_threshold_hn = pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_alarm_h_cnt;
			alarm_threshold_ln = pw_clb_config_para->pw_diagnos_para.jitter_side1_z_para.jitter_alarm_l_cnt;
		}
		else if(ch_num == pw2_z)
		{
			/*抖车预警阈值*/
			warn_threshold_val = pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_warn_value;
			warn_threshold_hn = pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_warn_h_cnt;
			warn_threshold_ln = pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_warn_l_cnt;
			/*抖车报警阈值*/
			alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_alarm_value;
			alarm_threshold_hn = pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_alarm_h_cnt;
			alarm_threshold_ln = pw_clb_config_para->pw_diagnos_para.jitter_side2_z_para.jitter_alarm_l_cnt;
		}


//		for(cnt_i=0;cnt_i<tzdata->data_deal_buff[ch_num].w_index;cnt_i++)			//
//		{
			/*tzdata->data_deal_buff[ch_num].buff[i]中的值为峰峰值*/
			tzdata->jitter_diag_res[ch_num].value = rms_deal(tzdata->jitter_float[ch_num].jitter_buff,tzdata->jitter_float[ch_num].size);
			if(ch_num == 0)
			{
				printf("tzdata->jitter_diag_res[ch_num].value:%f,ch_num:%d\r\n",tzdata->jitter_diag_res[ch_num].value,ch_num);
			}

			if(tzdata->jitter_diag_res[ch_num].value > alarm_threshold_val && tzdata->diag_res[ch_num].alarm_status_new == RUNNING_ALARM)				//大于报警阈值
			{
				tzdata->jitter_diag_res[ch_num].alarm_l_num = 0;
				tzdata->jitter_diag_res[ch_num].warn_l_num = 0;
				tzdata->jitter_diag_res[ch_num].warn_h_num++;
				tzdata->jitter_diag_res[ch_num].alarm_h_num++;
				//
			}
			else if(tzdata->jitter_diag_res[ch_num].value > warn_threshold_val && tzdata->diag_res[ch_num].alarm_status_new == RUNNING_WARN)			//大于预警阈值
			{
				tzdata->jitter_diag_res[ch_num].alarm_l_num ++;
				tzdata->jitter_diag_res[ch_num].warn_l_num = 0;
				tzdata->jitter_diag_res[ch_num].warn_h_num++;
				tzdata->jitter_diag_res[ch_num].alarm_h_num = 0;
			}
			else if(tzdata->jitter_diag_res[ch_num].value < warn_threshold_val && tzdata->diag_res[ch_num].alarm_status_new == RUNNING_OK)			//低于预警阈值
			{
				tzdata->jitter_diag_res[ch_num].alarm_l_num ++;
				tzdata->jitter_diag_res[ch_num].warn_l_num ++;
				tzdata->jitter_diag_res[ch_num].warn_h_num = 0;
				tzdata->jitter_diag_res[ch_num].alarm_h_num = 0;
			}
			else
			{
				;
			}

			/*抖车预报警判断*/
			if(tzdata->jitter_diag_res[ch_num].alarm_h_num > alarm_threshold_hn)
			{
				tzdata->jitter_diag_res[ch_num].alarm_status_new = RUNNING_ALARM;					//抖车报警
				if(tzdata->jitter_diag_res[ch_num].alarm_status_new != tzdata->jitter_diag_res[ch_num].alarm_status_old)
				{
					tzdata->jitter_diag_res[ch_num].alarm_status_old = tzdata->jitter_diag_res[ch_num].alarm_status_new;
					sys_status_cnt.err_type[ch_num].jitter_diag_alarm_save_flag = 1;										//抖车报警存储标志置１
					sys_status_cnt.err_type[ch_num].jitter_diag_normal_save_flag = 0;
				}
			}
			else if(tzdata->jitter_diag_res[ch_num].warn_h_num > warn_threshold_hn)
			{
				if(tzdata->jitter_diag_res[ch_num].alarm_l_num > alarm_threshold_ln)
				{
					tzdata->jitter_diag_res[ch_num].alarm_status_new = RUNNING_WARN;						//抖车预警
					if(tzdata->jitter_diag_res[ch_num].alarm_status_new != tzdata->jitter_diag_res[ch_num].alarm_status_old)
					{
						tzdata->jitter_diag_res[ch_num].alarm_status_old = tzdata->jitter_diag_res[ch_num].alarm_status_new;
						sys_status_cnt.err_type[ch_num].jitter_diag_warn_save_flag = 1;						//抖车预警存储标志置１
						sys_status_cnt.err_type[ch_num].jitter_diag_normal_save_flag = 0;
					}
				}
			}
			else if(tzdata->jitter_diag_res[ch_num].warn_l_num > warn_threshold_ln)
			{
				tzdata->jitter_diag_res[ch_num].alarm_status_new = RUNNING_OK;						//抖车恢复正常
				sys_status_cnt.err_type[ch_num].jitter_diag_warn_save_flag = 0;						//抖车报警存储标志置0
				sys_status_cnt.err_type[ch_num].jitter_diag_alarm_save_flag = 0;					//抖车报警存储标志置0
				sys_status_cnt.err_type[ch_num].jitter_diag_normal_save_flag = 1;
			}
			else
			{;}

	}
}
#endif
#endif

#ifdef HC_DIAG
/********************
 * 晃车阈值诊断
 * *****************/
void shake_diag_res_judge(struct PW_DIAGNOS_PARA *tzdata,uint8_t ch_num)
{
	float warn_threshold_val=0.0f;
	float alarm_threshold_val=0.0f;
	uint32_t warn_threshold_hn = 0;
	uint32_t warn_threshold_ln = 0;
	uint32_t alarm_threshold_hn = 0;
	uint32_t alarm_threshold_ln = 0;
	uint32_t cnt_i = 0;
//#ifdef HC_MODIFY_20211112
//	struct LOCAL_TIME time_now;
//#endif

	/**/
	if(ch_num == pw1_y)
	{
		/*晃车预警阈值*/
		warn_threshold_val = pw_clb_config_para->pw_diagnos_para.shake_side1_y_para.shake_warn_value;
		warn_threshold_hn = pw_clb_config_para->pw_diagnos_para.shake_side1_y_para.shake_warn_h_cnt;
		warn_threshold_ln = pw_clb_config_para->pw_diagnos_para.shake_side1_y_para.shake_warn_l_cnt;
		/*晃车报警阈值*/
		alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.shake_side1_y_para.shake_alarm_value;
		alarm_threshold_hn = pw_clb_config_para->pw_diagnos_para.shake_side1_y_para.shake_alarm_h_cnt;
		alarm_threshold_ln = pw_clb_config_para->pw_diagnos_para.shake_side1_y_para.shake_alarm_l_cnt;
	}
	else if(ch_num == pw2_y)
	{
		/*晃车预警阈值*/
		warn_threshold_val = pw_clb_config_para->pw_diagnos_para.shake_side2_y_para.shake_warn_value;
		warn_threshold_hn = pw_clb_config_para->pw_diagnos_para.shake_side2_y_para.shake_warn_h_cnt;
		warn_threshold_ln = pw_clb_config_para->pw_diagnos_para.shake_side2_y_para.shake_warn_l_cnt;
		/*晃车报警阈值*/
		alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.shake_side2_y_para.shake_alarm_value;
		alarm_threshold_hn = pw_clb_config_para->pw_diagnos_para.shake_side2_y_para.shake_alarm_h_cnt;
		alarm_threshold_ln = pw_clb_config_para->pw_diagnos_para.shake_side2_y_para.shake_alarm_l_cnt;
	}

	if(pw_tz_data.train_public_info.carriage_number == 0 || pw_tz_data.train_public_info.carriage_number == 7)//8编车 头＼尾车为司机室
	{
		warn_threshold_val = 0.04;
		alarm_threshold_val = 0.05;
	}

	for(cnt_i=0;cnt_i<tzdata->shack_float[ch_num].w_index;cnt_i++)				//
	{
		/*tzdata->shack_float[ch_num].buff[i]中的值为峰峰值*/
#if 1//def PW_DIAG_TEST
		tzdata->shack_diag_res[ch_num].value = tzdata->shack_float[ch_num].shack_buff[cnt_i]/2.0f;
#else
		tzdata->shake_diag_res[ch_num].value = AD_CONVERT_ACCELERATION(tzdata->shack_float[ch_num].shack_buff[cnt_i],tzdata->sens_para[ch_num].sensitivity);
		tzdata->shake_diag_res[ch_num].value = tzdata->shake_diag_res[ch_num].value/2.0f;
#endif

//		PW_DIAG_PRINTF("tzdata->shake_diag_res[%d].value=%f\n", ch_num, tzdata->shake_diag_res[ch_num].value);

//		if(ch_num == pw1_y)
//		{
//			printf("shake_diag_res_judge---peak_peak_value:%f, ch_num:%d\r\n",tzdata->shake_diag_res[ch_num].value,ch_num);
//		}

		if(tzdata->shack_diag_res[ch_num].value > alarm_threshold_val)				//大于报警阈值
		{
			tzdata->shack_diag_res[ch_num].alarm_l_num = 0;
			tzdata->shack_diag_res[ch_num].warn_l_num = 0;
			tzdata->shack_diag_res[ch_num].warn_h_num++;
			tzdata->shack_diag_res[ch_num].alarm_h_num++;
			//
		}
		else if(tzdata->shack_diag_res[ch_num].value > warn_threshold_val)			//大于预警阈值
		{
			tzdata->shack_diag_res[ch_num].alarm_l_num ++;
			tzdata->shack_diag_res[ch_num].warn_l_num = 0;
			tzdata->shack_diag_res[ch_num].warn_h_num++;
			tzdata->shack_diag_res[ch_num].alarm_h_num = 0;
		}
		else if(tzdata->shack_diag_res[ch_num].value < warn_threshold_val)			//低于预警阈值
		{
			tzdata->shack_diag_res[ch_num].alarm_l_num ++;
			tzdata->shack_diag_res[ch_num].warn_l_num ++;
			tzdata->shack_diag_res[ch_num].warn_h_num = 0;
			tzdata->shack_diag_res[ch_num].alarm_h_num = 0;
		}

		/*晃车预报警判断*/
		if(tzdata->shack_diag_res[ch_num].alarm_h_num >= alarm_threshold_hn)
		{
	#ifdef HC_MODIFY_20211112
//			get_local_time(&time_now);
//			PW_DIAG_PRINTF("alarm_first_time---time_now.sec:%d\n", time_now.sec);
			if(tzdata->shack_diag_res[ch_num].alarm_status_cur != RUNNING_ALARM)
			{
				tzdata->shack_diag_res[ch_num].alarm_status_cur = RUNNING_ALARM;
				tzdata->shack_diag_res[ch_num].alarm_first_time = time(NULL);
				PW_DIAG_PRINTF("tzdata->shack_diag_res[%d].alarm_first_time=%d\n", ch_num, tzdata->shack_diag_res[ch_num].alarm_first_time);
			}

			tzdata->shack_diag_res[ch_num].alarm_second_time = time(NULL);
			PW_DIAG_PRINTF("tzdata->shack_diag_res[%d].alarm_second_time=%d\n", ch_num, tzdata->shack_diag_res[ch_num].alarm_second_time);

			tzdata->shack_diag_res[ch_num].alarm_diff_time = difftime(tzdata->shack_diag_res[ch_num].alarm_second_time, tzdata->shack_diag_res[ch_num].alarm_first_time);
			if(tzdata->shack_diag_res[ch_num].alarm_diff_time<0)//alarm_second_time翻转情况
			{
				tzdata->shack_diag_res[ch_num].alarm_diff_time = abs(tzdata->shack_diag_res[ch_num].alarm_diff_time)+14;
			}
			PW_DIAG_PRINTF("tzdata->shack_diag_res[%d].alarm_diff_time=%d\n", ch_num, tzdata->shack_diag_res[ch_num].alarm_diff_time);

			if(tzdata->shack_diag_res[ch_num].alarm_diff_time>=14)
	#endif
			{
				tzdata->shack_diag_res[ch_num].alarm_status_new=RUNNING_ALARM;//左移１，以便update_pw_tz_data或运算
				if(tzdata->shack_diag_res[ch_num].alarm_status_old != tzdata->shack_diag_res[ch_num].alarm_status_new)
				{
					tzdata->shack_diag_res[ch_num].alarm_status_old = tzdata->shack_diag_res[ch_num].alarm_status_new;				//平稳报警

					if(sys_status_cnt.err_type[ch_num].shack_diag_alarm_save_flag == 0)
					{
						sys_status_cnt.err_type[ch_num].shack_diag_alarm_save_flag = 1;										//报警存储标志置１
						sys_status_cnt.err_type[ch_num].shack_diag_normal_save_flag = 0;
					}
				}

				if(tzdata->shack_diag_res[ch_num].warn_status_new != RUNNING_OK)
				{
					tzdata->shack_diag_res[ch_num].warn_status_old = RUNNING_OK;
					tzdata->shack_diag_res[ch_num].warn_status_new = RUNNING_OK;
					if(sys_status_cnt.err_type[ch_num].shack_diag_warn_save_flag == 0)
					{
						sys_status_cnt.err_type[ch_num].shack_diag_warn_save_flag = 1;										//预警存储标志置１
						sys_status_cnt.err_type[ch_num].shack_diag_normal_save_flag = 0;
					}
				#ifdef HC_MODIFY_20211112
					tzdata->shack_diag_res[ch_num].warn_status_cur = RUNNING_OK;
				#endif
				}
			}
		}
		else if(tzdata->shack_diag_res[ch_num].warn_h_num >= warn_threshold_hn)
		{
	#ifdef HC_MODIFY_20211112
			if(tzdata->shack_diag_res[ch_num].warn_status_cur != RUNNING_WARN)
			{
				tzdata->shack_diag_res[ch_num].warn_status_cur = RUNNING_WARN;
				tzdata->shack_diag_res[ch_num].warn_first_time = time(NULL);
			}

			tzdata->shack_diag_res[ch_num].warn_second_time = time(NULL);


			tzdata->shack_diag_res[ch_num].warn_diff_time = difftime(tzdata->shack_diag_res[ch_num].warn_second_time, tzdata->shack_diag_res[ch_num].warn_first_time);
			if(tzdata->shack_diag_res[ch_num].warn_diff_time<0)//warn_second_time翻转情况
			{
				tzdata->shack_diag_res[ch_num].warn_diff_time = abs(tzdata->shack_diag_res[ch_num].warn_diff_time)+14;
			}

			if(tzdata->shack_diag_res[ch_num].warn_diff_time>=14)
	#endif
			{
				tzdata->shack_diag_res[ch_num].warn_status_new=RUNNING_WARN;
				if(tzdata->shack_diag_res[ch_num].warn_status_old != tzdata->shack_diag_res[ch_num].warn_status_new)
				{
					tzdata->shack_diag_res[ch_num].warn_status_old = tzdata->shack_diag_res[ch_num].warn_status_new;	//抖车预警

					if(sys_status_cnt.err_type[ch_num].shack_diag_warn_save_flag == 0)
					{
						sys_status_cnt.err_type[ch_num].shack_diag_warn_save_flag = 1;										//预警存储标志置１
						sys_status_cnt.err_type[ch_num].shack_diag_normal_save_flag = 0;
					}
				}

				if(tzdata->shack_diag_res[ch_num].alarm_status_new != RUNNING_OK)
				{
					tzdata->shack_diag_res[ch_num].alarm_status_old = RUNNING_OK;
					tzdata->shack_diag_res[ch_num].alarm_status_new = RUNNING_OK;
					if(sys_status_cnt.err_type[ch_num].shack_diag_alarm_save_flag == 0)
					{
						sys_status_cnt.err_type[ch_num].shack_diag_alarm_save_flag = 1;										//报警存储标志置１
						sys_status_cnt.err_type[ch_num].shack_diag_normal_save_flag = 0;
					}
				#ifdef HC_MODIFY_20211112
					tzdata->shack_diag_res[ch_num].alarm_status_cur = RUNNING_OK;
				#endif
				}
			}
		}
		else if(tzdata->shack_diag_res[ch_num].warn_l_num >= warn_threshold_ln && tzdata->shack_diag_res[ch_num].alarm_l_num >= alarm_threshold_ln)
		{
			tzdata->shack_diag_res[ch_num].alarm_status_new = RUNNING_OK; //恢复正常
			if(tzdata->shack_diag_res[ch_num].alarm_status_old != tzdata->shack_diag_res[ch_num].alarm_status_new)
			{
				tzdata->shack_diag_res[ch_num].alarm_status_old = tzdata->shack_diag_res[ch_num].alarm_status_new;
				if(sys_status_cnt.err_type[ch_num].shack_diag_normal_save_flag == 0)
				{
					sys_status_cnt.err_type[ch_num].shack_diag_normal_save_flag = 1;
					sys_status_cnt.err_type[ch_num].shack_diag_alarm_save_flag = 0;	//晃车报警存储标志置0
				}
			}

			tzdata->shack_diag_res[ch_num].warn_status_new = RUNNING_OK;  //恢复正常
			if(tzdata->shack_diag_res[ch_num].warn_status_old != tzdata->shack_diag_res[ch_num].warn_status_new)
			{
				tzdata->shack_diag_res[ch_num].warn_status_old = tzdata->shack_diag_res[ch_num].warn_status_new;
				if(sys_status_cnt.err_type[ch_num].shack_diag_normal_save_flag == 0)
				{
					sys_status_cnt.err_type[ch_num].shack_diag_normal_save_flag = 1;
					sys_status_cnt.err_type[ch_num].shack_diag_warn_save_flag = 0;	//晃车预警存储标志置0
				}
			}

		#ifdef HC_MODIFY_20211112
			tzdata->shack_diag_res[ch_num].warn_status_cur = RUNNING_OK;
			tzdata->shack_diag_res[ch_num].alarm_status_cur = RUNNING_OK;
		#endif
		}
	}

#ifdef WTD_DATA_TRANSLATE_PROTOCOL
#ifdef WTD_SIMULATION_ALARM_TRIG//仅测试时用
	if(!simulation_open_flag)
#endif
	{
		if(tzdata->shack_diag_res[ch_num].alarm_status_new == RUNNING_ALARM || tzdata->shack_diag_res[ch_num].warn_status_new == RUNNING_WARN)
		{
			if(msg_alarm_trigger[ch_num].alarm_tz_type == NO_ALARM_TYPE
					&& msg_alarm_trigger[ch_num].alarm_trig_status == TRIG_OK)
			{
				msg_alarm_trigger[ch_num].alarm_trig_status = TRIG_ALARM;
				msg_alarm_trigger[ch_num].alarm_valid_flag = ALARM_VALID;

				if(ch_num == pw1_y)
					msg_alarm_trigger[ch_num].alarm_tz_type = PW1_Y_HC_ALARM_TYPE;
				else if(ch_num == pw2_y)
					msg_alarm_trigger[ch_num].alarm_tz_type = PW2_Y_HC_ALARM_TYPE;
			}
		}
		else if(tzdata->shack_diag_res[ch_num].alarm_status_new == RUNNING_OK && tzdata->shack_diag_res[ch_num].warn_status_new == RUNNING_OK)
		{
			if( ((msg_alarm_trigger[ch_num].alarm_tz_type == PW1_Y_HC_ALARM_TYPE && ch_num == pw1_y)
			  || (msg_alarm_trigger[ch_num].alarm_tz_type == PW2_Y_HC_ALARM_TYPE && ch_num == pw2_y))
					&& msg_alarm_trigger[ch_num].alarm_trig_status == TRIG_ALARM )
			{
				msg_alarm_trigger[ch_num].alarm_trig_status = TRIG_ALARM_REMOVE;
				msg_alarm_trigger[ch_num].alarm_valid_flag = ALARM_VALID;//30s后才无效
			}
		}
	}
#endif
}

void find_max_min(struct SHACK_FLOAT *shack_float)
{
	float max=0;//, min=0;
	uint16_t i=0;

	max = shack_float->shack_buff[0];

	for(i=0;i<FS_PW;i++)
	{
		if(i>0)
		{
			if(shack_float->shack_buff[i]>max)
				max = shack_float->shack_buff[i];
		}
	}

}
/*******************
 * *晃车诊断
 *
 * ******************/
void diag_shack_pw(struct PW_DIAGNOS_PARA *tzdata,uint8_t ch_num)
{
//	printf("diag_shack_pw---ch_num:%d\n", ch_num);
	/*晃车只需要横向数据*/
	if(ch_num == pw1_y || ch_num == pw2_y)
	{
#if !defined(DATA_CENTER_TEST)
//		ad_filter(tzdata->shack_float[ch_num].shack_buff,tzdata->shack_float[ch_num].size,ch_num);
#endif

		/*去均值*/
		float_sub_mean(tzdata->shack_float[ch_num].shack_buff, tzdata->shack_float[ch_num].size);

#ifndef DATA_CENTER_TEST
		/*转加速度操作*/
		get_acc_data(tzdata->shack_float[ch_num].shack_buff, tzdata->shack_float[ch_num].size);
#endif

#ifdef DIAG_DATA_SAVE_FOR_TEST
		if(ch_num == pw1_y)
		{
			save_hctzz_diag_file(HCTZZ_TYPE, DIAG_DATA_ACC_TITLE, NULL, 1);
			save_hctzz_diag_file(HCTZZ_TYPE, DIAG_DATA_ACC, &tzdata->shack_float[ch_num].shack_buff[0], tzdata->shack_float[ch_num].size);
		}
#endif

		//2s fft_in & move
		move_float_to_float(&tzdata->shack_fft_in[ch_num].shack_buff[tzdata->shack_fft_in[ch_num].w_index], tzdata->shack_float[ch_num].shack_buff, FS_PW);
		tzdata->shack_fft_in[ch_num].w_index += FS_PW;

		if(tzdata->shack_fft_in[ch_num].w_index == SHACK_FFT_SEC*FS_PW)
		{
	#ifdef DIAG_DATA_SAVE_FOR_TEST
			if(ch_num == pw1_y)
			{
				save_hctzz_diag_file(HCTZZ_TYPE, DIAG_DATA_BEFORE_FILTER_TITLE, NULL, 1);
				save_hctzz_diag_file(HCTZZ_TYPE, DIAG_DATA_BEFORE_FILTER, &tzdata->shack_fft_in[ch_num].shack_buff[0], tzdata->shack_fft_in[ch_num].size);
			}
	#endif

			/*fft ifft 滤波*/
			fft_ifft_deal(tzdata->shack_fft_in[ch_num].shack_buff, tzdata->shack_float[ch_num].shack_buff, FRE_0D2_TO_3HZ);
			/*数据带通滤波*/
			//shack_filter_deal(&tzdata->shack_float[ch_num],&tzdata->shack_filter_para[ch_num],PW_Y);

	#ifdef DIAG_DATA_SAVE_FOR_TEST
			if(ch_num == pw1_y)
			{
				save_hctzz_diag_file(HCTZZ_TYPE, DIAG_DATA_AFTER_FILTER_TITLE, NULL, 1);
				save_hctzz_diag_file(HCTZZ_TYPE, DIAG_DATA_AFTER_FILTER, &tzdata->shack_float[ch_num].shack_buff[0], tzdata->shack_float[ch_num].size);
			}
	#endif

			/*计算峰峰值，峰峰值前不需去均值*/
			ampth_deal(&tzdata->shack_float[ch_num],&tzdata->last_packet_extreme[ch_num]);

			/*晃车阈值诊断*/
			shake_diag_res_judge(tzdata, ch_num);

	#ifdef DIAG_DATA_SAVE_FOR_TEST
			if(ch_num == pw1_y)
			{
				save_hctzz_diag_file(HCTZZ_TYPE, DIAG_DATA_FFZ_TITLE, NULL, 1);
				save_hctzz_diag_file(HCTZZ_TYPE, DIAG_DATA_FFZ, &tzdata->shack_float[ch_num].shack_buff[0], 1);

				save_hctzz_diag_file(HCTZZ_TYPE, DIAG_DATA_FZ_TITLE, NULL, 1);
				save_hctzz_diag_file(HCTZZ_TYPE, DIAG_DATA_FZ, &tzdata->shake_diag_res[ch_num].value, 1);
			}
	#endif

			move_shack_fft_data(tzdata, ch_num);

			tzdata->shack_float[ch_num].w_index = 0u;
		}

	}//end if

#if 0//test
//		float fft_in[2048]={0};
//		float fft_real[1024]={0};
//		float fft_imag[1024]={0};
//		int k;
//		printf("diag_shack_pw--fft in:\n");
//		for(k=0; k<1024; k++)
//		{
//			fft_in[k] = 2*sin(2*PI*2*k/512) + 2;//real   f=2Hz   w=2*PI*2/1024   T=2*PI/w=512
//			fft_in[k+1024] = 0;//imag
//			printf("%f\n",fft_in[k]);
//		}
////			memmove(tzdata->shack_fft_in[ch_num].shack_buff, (float*)fft_in, 1024);
//
//		printf("diag_shack_pw--fft start:\n");
//		complex_fft((float*)fft_in, fft_real, fft_imag, 2048, 1024, FFT_E);
//		for(k=0; k<1024; k++)
//		{
//			fft_in[k] = fft_real[k];//2*sin(2*PI*4*k/1024);//f=2Hz   w=2*PI*2/1024   T=2*PI/w=512
//			fft_in[k+1024] = fft_imag[k];
//		}
//
//		complex_fft_convert((float*)fft_in, (float*)hc_filter_real, (float*)hc_filter_imag, FFT_NPT);
////	complex_fft_convert((float*)fft_in, (float*)dc_filter_real, (float*)dc_filter_imag, FFT_NPT);
//
//		printf("diag_shack_pw--fft end:\n");
//		printf("diag_shack_pw--fft real out:\n");
//		for(k=0; k<1024; k++)
//		{
//			printf("%f\n",fft_in[k]);
//		}
//
//		printf("diag_shack_pw--fft imag out:\n");
//		for(k=0; k<1024; k++)
//		{
//			printf("%f\n",fft_in[k+1024]);
//		}
//
//		complex_fft((float*)fft_in, fft_real, fft_imag, 2048, 1024, IFFT_E);
//
//		printf("diag_shack_pw--ifft end:\n");
//		printf("diag_shack_pw--ifft real out:\n");
//		for(k=0; k<1024; k++)
//		{
//			printf("%f\n",fft_real[k]);
//		}
#endif
}
#endif

#ifdef JT_DIAG
/*************************************************
Function:    diag_speed_deal
Description: 获取速度平均值
Input:
Output:
Return:
Others:
*************************************************/
float diag_speed_deal(float *speed_buff,uint16_t size)
{
	float sum = 0.0f;
	float ave_value = 0.0f;
	sum = float_sum(speed_buff,size);
	ave_value = sum/size;
	return ave_value;

}

/************************************************
* 找出频谱中的最大幅值及对应频率
************************************************/
float find_max_value(struct PW_DIAGNOS_PARA *tzdata,float start_fre,float end_fre,uint8_t chu_num)
{
	float max_fre = 0.0f;
	uint16_t cnt_i = 0;
	//float
	uint16_t start_index = start_fre/(FS_PW/FFT_NPT);
	uint16_t end_index = end_fre/(FS_PW/FFT_NPT);

	tzdata->wheel_diag_res[chu_num].value = tzdata->wheel_diameter[chu_num].fft_value[start_index];
	max_fre = start_fre;
	for(cnt_i = start_index+1;cnt_i <= end_index;cnt_i++)
	{
		if(tzdata->wheel_diameter[chu_num].fft_value[cnt_i] > tzdata->wheel_diag_res[chu_num].value)
		{
			tzdata->wheel_diag_res[chu_num].value = tzdata->wheel_diameter[chu_num].fft_value[cnt_i];	//最大幅值
			max_fre = cnt_i*(FS_PW/FFT_NPT);			//最大幅值对应的频率
		}
	}

	return max_fre;
}

/*************************************************
Function:    diag_wheel_pw
Description: 径跳诊断
Input:
Output:
Return:
Others:
*************************************************/
void diag_wheel_pw(struct PW_DIAGNOS_PARA *tzdata,uint8_t ch_num)
{
	float alarm_threshold_val=0.0f;
	float theory_rota_fre = 0.0f;
	float fre_start = 0.0f;
	float fre_end = 0.0f;
	uint32_t ALARM_HN = 0;
//	uint32_t ALARM_LN = 0;
	static float speed_temp = 0.0f;
	static float max_fre_temp = 0.0f;
	float max_fre = 0.0f;

	if(ch_num == pw1_z || ch_num == pw2_z)
	{
		/*获取速度平均值*/
		tzdata->wheel_diameter[ch_num].ave_speed = diag_speed_deal(tzdata->wheel_diameter[ch_num].sample_speed, JT_DIAG_SEC);//tzdata->wheel_diameter[ch_num].size);

		if(tzdata->wheel_diameter[ch_num].ave_speed > 26.0f)		//保证理论转频值大于0.5
		{
#ifdef DIAG_DATA_SAVE_FOR_TEST
			if(ch_num == Z_CH_NUM)
			{
				save_jttzz_diag_file(JTTZZ_TYPE, DIAG_DATA_AVG_SPEED_TITLE, NULL, 1);
				save_jttzz_diag_file(JTTZZ_TYPE, DIAG_DATA_AVG_SPEED, &tzdata->wheel_diameter[ch_num].ave_speed, 1);

				save_jttzz_diag_file(JTTZZ_TYPE, DIAG_DATA_AD_TITLE, NULL, 1);
				save_jttzz_diag_file(JTTZZ_TYPE, DIAG_DATA_AD, &tzdata->wheel_diameter[ch_num].fft_value[0], tzdata->wheel_diameter[ch_num].size);
			}
#endif
			/*计算理论转频*/
			theory_rota_fre = tzdata->wheel_diameter[ch_num].ave_speed/(3.6*3.14*0.88);//0.89
			tzdata->wheel_diag_res[ch_num].avg_fre = theory_rota_fre;

			/*得到最大频谱范围值*/
			fre_start = theory_rota_fre-2;
			fre_end = theory_rota_fre+2;

			/*查找主频及幅值*/
			max_fre = find_max_value(tzdata,fre_start,fre_end,ch_num);

			tzdata->wheel_diag_res[ch_num].max_fre = max_fre;

#ifdef DIAG_DATA_SAVE_FOR_TEST
			if(ch_num == Z_CH_NUM)
			{
				save_jttzz_diag_file(JTTZZ_TYPE, DIAG_DATA_JT_MAX_FRE_TITLE, NULL, 1);
				save_jttzz_diag_file(JTTZZ_TYPE, DIAG_DATA_JT_MAX_FRE, &max_fre, 1);

				save_jttzz_diag_file(JTTZZ_TYPE, DIAG_DATA_JT_TZZ_TITLE, NULL, 1);
				save_jttzz_diag_file(JTTZZ_TYPE, DIAG_DATA_JT_TZZ, &tzdata->wheel_diag_res[ch_num].value, 1);
			}
#endif
			PW_DIAG_PRINTF("diag_wheel_pw---jt---fre(%f, %f), max_fre:%f, max_amp:%f\n", fre_start, fre_end, max_fre, tzdata->wheel_diag_res[ch_num].value);

			if(ch_num == pw1_z)
			{
				alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.wheel_side1_z_para.wheel_alarm_value;
				ALARM_HN = pw_clb_config_para->pw_diagnos_para.wheel_side1_z_para.wheel_alarm_h_cnt;
//				ALARM_LN = pw_clb_config_para->pw_diagnos_para.wheel_side1_z_para.wheel_alarm_l_cnt;
			}
			else if(ch_num == pw2_z)
			{
				alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.wheel_side2_z_para.wheel_alarm_value;
				ALARM_HN = pw_clb_config_para->pw_diagnos_para.wheel_side2_z_para.wheel_alarm_h_cnt;
//				ALARM_LN = pw_clb_config_para->pw_diagnos_para.wheel_side2_z_para.wheel_alarm_l_cnt;
			}

			/*判断速度差是否满足条件*/
			if(abs(tzdata->wheel_diameter[ch_num].ave_speed - speed_temp)<5 )
			{
				if(tzdata->wheel_diag_res[ch_num].value > alarm_threshold_val && (max_fre == max_fre_temp))
				{
					tzdata->wheel_diag_res[ch_num].alarm_h_num++;
					tzdata->wheel_diag_res[ch_num].alarm_l_num = 0;
				}

			}
			else
			{
				tzdata->wheel_diag_res[ch_num].alarm_h_num = 0;
				tzdata->wheel_diag_res[ch_num].alarm_l_num++;
			}

			if(tzdata->wheel_diag_res[ch_num].alarm_h_num >= ALARM_HN)
			{
				if(tzdata->wheel_diag_res[ch_num].alarm_status_new!=tzdata->wheel_diag_res[ch_num].alarm_status_old)
				{
					if(tzdata->wheel_diag_res[ch_num].alarm_status_new==RUNNING_ALARM)
					{
						tzdata->wheel_diag_res[ch_num].alarm_status_old = tzdata->wheel_diag_res[ch_num].alarm_status_new;				//平稳报警

//						if(sys_status_cnt.err_type[ch_num].diag_alarm_save_flag == 0)
//						{
//							sys_status_cnt.err_type[ch_num].diag_alarm_save_flag = 1;													//报警存储标志置１
//							sys_status_cnt.err_type[ch_num].diag_normal_save_flag = 0;
//						}
					}
				}
			}

			speed_temp = tzdata->wheel_diameter[ch_num].ave_speed;
			max_fre_temp = max_fre;
		}
	}
}
#endif

#ifdef PW_DIAG
/*************************************************
Function:    diag_deal_pw
Description: 平稳性指标计算
Input:
Output:
Return:
Others:
*************************************************/
void diag_deal_pw(struct PW_DIAGNOS_PARA *tzdata,uint8_t ch_num)
{
	float warn_threshold_val=0.0f;
	float alarm_threshold_val=0.0f;


#if 0
	uint8_t alarm_data[2] = {0};
	/***测试使用***/
	uint16_t test_data_cnt = 0;
	int test_write_res = -1;
	char test_buff[10] = {0};
	float ad_vol = 0.0f;
#endif

	/***测试使用***/
	uint32_t WARN_HN=0;
	uint32_t WARN_LN = 0;
	uint32_t ALARM_HN = 0;
	uint32_t ALARM_LN = 0;
	/*uint32_t ALARM_=0;*/



	/****滤波使用******只有横向和垂向使用****/
if(ch_num == pw1_y || ch_num == pw2_y || ch_num == pw1_z || ch_num == pw2_z)
 {
#if !defined(DATA_CENTER_TEST)
	/*用512个点*/
//	ad_filter(tzdata->data_deal_buff[ch_num].station_buff,tzdata->data_deal_buff[ch_num].size,ch_num);

	/*判断传感器故障*/
	Sensor_fault_judge_PW(tzdata->data_deal_buff[ch_num].station_buff,tzdata->data_deal_buff[ch_num].size,tzdata ,ch_num);
#endif

	/*step1.均值处理,对1024个数据求均值*/
	float_sub_mean(tzdata->data_deal_buff[ch_num].station_buff,tzdata->data_deal_buff[ch_num].size);

	/*给诊断缓存后512个数据补０*/
//	supplement_zero(tzdata->data_deal_buff[ch_num].station_buff+512,tzdata->data_deal_buff[ch_num].size/DIAG_SEC);
	/*给诊断缓存后512个数据补０*/

	/*step2.平稳算法*/
	if(tzdata->sens_para[ch_num].sens_type==PW_Y)
	{
		/*横向平稳性指标计算*/
		stable_deal(tzdata,ch_num);

		if(ch_num == pw1_y)
		{
			if(pw_clb_config_para->trainnum == 1 || pw_clb_config_para->trainnum == 4)
			{
				warn_threshold_val = pw_clb_config_para->pw_diagnos_para.side1_y_para.warn_value;
				alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side1_y_para.alarm_value;
			}
			else
			{
				warn_threshold_val = pw_clb_config_para->pw_diagnos_para.side1_y_para.warn_value - 0.25;
				alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side1_y_para.alarm_value - 0.25;
			}
			WARN_HN = pw_clb_config_para->pw_diagnos_para.side1_y_para.warn_h_cnt;
			WARN_LN = pw_clb_config_para->pw_diagnos_para.side1_y_para.warn_l_cnt;
//			printf("diag_deal_pw---y---ch0, warn_value:%f, wHcnt:%d, wLcnt:%d\n", warn_threshold_val, WARN_HN, WARN_LN);

//			alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side1_y_para.alarm_value;
			ALARM_HN = pw_clb_config_para->pw_diagnos_para.side1_y_para.alarm_h_cnt;
			ALARM_LN = pw_clb_config_para->pw_diagnos_para.side1_y_para.alarm_l_cnt;
//			printf("diag_deal_pw---y---ch0, alarm_value:%f, aHcnt:%d, aLcnt:%d\n", alarm_threshold_val, ALARM_HN, ALARM_LN);


		}
		else if(ch_num == pw2_y)
		{
			if(pw_clb_config_para->trainnum == 1 || pw_clb_config_para->trainnum == 4)
			{
				warn_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_y_para.warn_value;
				alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_y_para.alarm_value;
			}
			else
			{
				warn_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_y_para.warn_value - 0.25;
				alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_y_para.alarm_value - 0.25;
			}
//			warn_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_y_para.warn_value;
			WARN_HN = pw_clb_config_para->pw_diagnos_para.side2_y_para.warn_h_cnt;
			WARN_LN = pw_clb_config_para->pw_diagnos_para.side2_y_para.warn_l_cnt;
//			printf("diag_deal_pw---y---ch3, warn_value:%f, wHcnt:%d, wLcnt:%d\n", warn_threshold_val, WARN_HN, WARN_LN);

//			alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_y_para.alarm_value;
			ALARM_HN = pw_clb_config_para->pw_diagnos_para.side2_y_para.alarm_h_cnt;
			ALARM_LN = pw_clb_config_para->pw_diagnos_para.side2_y_para.alarm_l_cnt;
//			printf("diag_deal_pw---y---ch3, alarm_value:%f, aHcnt:%d, aLcnt:%d\n", alarm_threshold_val, ALARM_HN, ALARM_LN);
		}
	}
	else if(tzdata->sens_para[ch_num].sens_type==PW_Z)
	{


//		printf("PWZZZZZZZZZZZZZZZZZZZZZZZZZ\n");
		/*垂向平稳性指标计算-得到平稳性指标*/
		stable_deal(tzdata,ch_num);

		/*垂向平稳性指标计算-得到平稳性指标*/
		if(ch_num == pw1_z)
		{
			if(pw_clb_config_para->trainnum == 1 || pw_clb_config_para->trainnum == 4)
			{
				warn_threshold_val = pw_clb_config_para->pw_diagnos_para.side1_z_para.warn_value;
				alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side1_z_para.alarm_value;
			}
			else
			{
				warn_threshold_val = pw_clb_config_para->pw_diagnos_para.side1_z_para.warn_value - 0.25;
				alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side1_z_para.alarm_value - 0.25;
			}
//			warn_threshold_val = pw_clb_config_para->pw_diagnos_para.side1_z_para.warn_value;
			WARN_HN = pw_clb_config_para->pw_diagnos_para.side1_z_para.warn_h_cnt;
			WARN_LN = pw_clb_config_para->pw_diagnos_para.side1_z_para.warn_l_cnt;
//			printf("diag_deal_pw---z---ch1, warn_value:%f, wHcnt:%d, wLcnt:%d\n", warn_threshold_val, WARN_HN, WARN_LN);

//			alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side1_z_para.alarm_value;
			ALARM_HN = pw_clb_config_para->pw_diagnos_para.side1_z_para.alarm_h_cnt;
			ALARM_LN = pw_clb_config_para->pw_diagnos_para.side1_z_para.alarm_l_cnt;
//			printf("diag_deal_pw---z---ch1, alarm_value:%f, aHcnt:%d, aLcnt:%d\n", alarm_threshold_val, ALARM_HN, ALARM_LN);
		}
		else if(ch_num == pw2_z)
		{
			if(pw_clb_config_para->trainnum == 1 || pw_clb_config_para->trainnum == 4)
			{
				warn_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_z_para.warn_value;
				alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_z_para.alarm_value;
			}
			else
			{
				warn_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_z_para.warn_value - 0.25;
				alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_z_para.alarm_value - 0.25;
			}
//			warn_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_z_para.warn_value;
			WARN_HN = pw_clb_config_para->pw_diagnos_para.side2_z_para.warn_h_cnt;
			WARN_LN = pw_clb_config_para->pw_diagnos_para.side2_z_para.warn_l_cnt;
//			printf("diag_deal_pw---z---ch4, warn_value:%f, wHcnt:%d, wLcnt:%d\n", warn_threshold_val, WARN_HN, WARN_LN);

//			alarm_threshold_val = pw_clb_config_para->pw_diagnos_para.side2_z_para.alarm_value;
			ALARM_HN = pw_clb_config_para->pw_diagnos_para.side2_z_para.alarm_h_cnt;
			ALARM_LN = pw_clb_config_para->pw_diagnos_para.side2_z_para.alarm_l_cnt;
//			printf("diag_deal_pw---z---ch4, alarm_value:%f, aHcnt:%d, aLcnt:%d\n", alarm_threshold_val, ALARM_HN, ALARM_LN);
		}
	}

//	if(ch_num == pw1_y)
//	{
//		printf("pw_index---tzdata->diag_res[%d].value,%f\n", ch_num, tzdata->diag_res[ch_num].value);
//	}

	/*step3.平稳判断*/
	/*平稳预警判断,横向和垂向都需要判断*/


	if(tzdata->diag_res[ch_num].value>alarm_threshold_val)		/*①大于报警阈值判断*/
	{
		tzdata->diag_res[ch_num].alarm_l_num = 0;
		tzdata->diag_res[ch_num].warn_l_num = 0;
		tzdata->diag_res[ch_num].alarm_h_num++;
		tzdata->diag_res[ch_num].warn_h_num++;
	}
#ifdef BRIDGE_DIAG
	else if((tzdata->diag_res[ch_num].value>warn_threshold_val)
			&& (ch_num == pw1_z || ch_num == pw2_z)
			&& (tzdata->ten_del_diag_res[ch_num].bridge_flag)
			&& (tzdata->ten_del_diag_res[ch_num].max_value3[tzdata->ten_del_diag_res[ch_num].keep_len_times-1] > BRIDGE_FACTOR_WARN_THRESHOLD))	/*②大于预警阈值判断*/
	{
		tzdata->diag_res[ch_num].alarm_h_num = 0;
		tzdata->diag_res[ch_num].alarm_l_num++;
		tzdata->diag_res[ch_num].warn_l_num = 0;
		tzdata->diag_res[ch_num].warn_h_num++;
#ifdef ADD_DIAG_TZZ_DATA_FILE
		if(tzdata->diag_res[ch_num].warn_h_num > WARN_HN) /*满足平稳预警*/
		{
			if(ch_num == pw1_z)
			{
				diag_tzz_data.ten_bridge_pwx_warn_status[0] = RUNNING_WARN;
			}
			else if(ch_num == pw2_z)
			{
				diag_tzz_data.ten_bridge_pwx_warn_status[1] = RUNNING_WARN;
			}
		}
		else
		{
			if(ch_num == pw1_z)
			{
				diag_tzz_data.ten_bridge_pwx_warn_status[0] = RUNNING_OK;
			}
			else if(ch_num == pw2_z)
			{
				diag_tzz_data.ten_bridge_pwx_warn_status[1] = RUNNING_OK;
			}
		}
#endif
	}
#endif
	else if(tzdata->diag_res[ch_num].value>warn_threshold_val)	/*②大于预警阈值判断*/
	{
		tzdata->diag_res[ch_num].alarm_h_num = 0;
		tzdata->diag_res[ch_num].alarm_l_num++;
		tzdata->diag_res[ch_num].warn_l_num = 0;
		tzdata->diag_res[ch_num].warn_h_num++;
	}
	else														/*③小于预警阈值*/ /*编号１，２，３的判断条件不能交换顺序*/
	{
		/**/
		tzdata->diag_res[ch_num].warn_h_num = 0;
		tzdata->diag_res[ch_num].alarm_h_num = 0;
		tzdata->diag_res[ch_num].alarm_l_num++;
		tzdata->diag_res[ch_num].warn_l_num++;
	}

//	printf("diag_deal_pw---ch%d, pw_value:%f (compare to 2.75 or 3.0)\n", ch_num, tzdata->diag_res[ch_num].value);
//
//	printf("diag_deal_pw---ch%d, warn_h_num:%d, alarm_h_num:%d, warn_l_num:%d, alarm_l_num:%d\n", ch_num,
//			tzdata->diag_res[ch_num].warn_h_num, tzdata->diag_res[ch_num].alarm_h_num,
//			tzdata->diag_res[ch_num].warn_l_num, tzdata->diag_res[ch_num].alarm_l_num);

#ifdef INTERNAL_PROTOCOL_20210416
	/*超阈值次数判断*/
	//alarm

//	printf("### tzdata->diag_res[1].alarm_h_num %d\n ",tzdata->diag_res[1].alarm_h_num);
//	printf("### tzdata->diag_res[4].alarm_h_num %d\n ",tzdata->diag_res[4].alarm_h_num);
	if(tzdata->diag_res[ch_num].alarm_h_num > ALARM_HN)		/*满足平稳报警*/
	{
		pw_data_save_type = SENSOR_ALARM_DATA;
		tzdata->diag_res[ch_num].alarm_status_new=RUNNING_ALARM;//左移１，以便update_pw_tz_data或运算
		if(tzdata->diag_res[ch_num].alarm_status_old != tzdata->diag_res[ch_num].alarm_status_new)
		{
			tzdata->diag_res[ch_num].alarm_status_old = tzdata->diag_res[ch_num].alarm_status_new;				//平稳报警

			if(sys_status_cnt.err_type[ch_num].diag_alarm_save_flag == 0)
			{
				sys_status_cnt.err_type[ch_num].diag_alarm_save_flag = 1;										//报警存储标志置１
				sys_status_cnt.err_type[ch_num].diag_normal_save_flag = 0;
			}
		}

		if(tzdata->diag_res[ch_num].warn_status_new != RUNNING_OK)
		{
			tzdata->diag_res[ch_num].warn_status_old = RUNNING_OK;
			tzdata->diag_res[ch_num].warn_status_new = RUNNING_OK;
			if(sys_status_cnt.err_type[ch_num].diag_warn_save_flag == 0)
			{
				sys_status_cnt.err_type[ch_num].diag_warn_save_flag = 1;										//预警存储标志置１
				sys_status_cnt.err_type[ch_num].diag_normal_save_flag = 0;
			}
		}
	}
	//warn
	else if(tzdata->diag_res[ch_num].warn_h_num > WARN_HN) /*满足平稳预警*/
	{
		pw_data_save_type = SENSOR_WARN_DATA;
		tzdata->diag_res[ch_num].warn_status_new=RUNNING_WARN;
		if(tzdata->diag_res[ch_num].warn_status_old != tzdata->diag_res[ch_num].warn_status_new)
		{
			tzdata->diag_res[ch_num].warn_status_old = tzdata->diag_res[ch_num].warn_status_new;				//平稳预警

			if(sys_status_cnt.err_type[ch_num].diag_warn_save_flag == 0)
			{
				sys_status_cnt.err_type[ch_num].diag_warn_save_flag = 1;										//预警存储标志置１
				sys_status_cnt.err_type[ch_num].diag_normal_save_flag = 0;
			}
		}

		if(tzdata->diag_res[ch_num].alarm_status_new != RUNNING_OK)
		{
			tzdata->diag_res[ch_num].alarm_status_old = RUNNING_OK;
			tzdata->diag_res[ch_num].alarm_status_new = RUNNING_OK;
			if(sys_status_cnt.err_type[ch_num].diag_alarm_save_flag == 0)
			{
				sys_status_cnt.err_type[ch_num].diag_alarm_save_flag = 1;										//报警存储标志置１
				sys_status_cnt.err_type[ch_num].diag_normal_save_flag = 0;
			}
		}
	}
	else if((tzdata->diag_res[ch_num].alarm_l_num > ALARM_LN) && (tzdata->diag_res[ch_num].warn_l_num > WARN_LN))		/*恢复正常*/
	{
		/*pw_data_save_type = SENSOR_WARN_DATA;*/
		tzdata->diag_res[ch_num].alarm_status_new=RUNNING_OK;
		if(tzdata->diag_res[ch_num].alarm_status_old!=tzdata->diag_res[ch_num].alarm_status_new)
		{
			tzdata->diag_res[ch_num].alarm_status_old = tzdata->diag_res[ch_num].alarm_status_new;					/*平稳报警恢复正常*/

			if(sys_status_cnt.err_type[ch_num].diag_normal_save_flag == 0)
			{
				sys_status_cnt.err_type[ch_num].diag_normal_save_flag = 1;											/*正常存储标志置１*/
				sys_status_cnt.err_type[ch_num].diag_alarm_save_flag = 0;
			}
		}

		/*pw_data_save_type = SENSOR_WARN_DATA;*/
		tzdata->diag_res[ch_num].warn_status_new=RUNNING_OK;
		if(tzdata->diag_res[ch_num].warn_status_new!=tzdata->diag_res[ch_num].warn_status_old)
		{
			tzdata->diag_res[ch_num].warn_status_old = tzdata->diag_res[ch_num].warn_status_new;					/*平稳预警恢复正常*/

			if(sys_status_cnt.err_type[ch_num].diag_normal_save_flag == 0)
			{
				sys_status_cnt.err_type[ch_num].diag_normal_save_flag = 1;											/*正常存储标志置１*/
				sys_status_cnt.err_type[ch_num].diag_warn_save_flag = 0;
			}
		}
	}
//	printf("diag_deal_pw---ch%d, warn_status_new:%d, alarm_status_new:%d (0-ok, 1-warn, 2-alarm)\n", ch_num,
//			tzdata->diag_res[ch_num].warn_status_new, tzdata->diag_res[ch_num].alarm_status_new);

#ifdef WTD_DATA_TRANSLATE_PROTOCOL
#ifdef WTD_SIMULATION_ALARM_TRIG//仅测试时用
	if(!simulation_open_flag)
#endif
	{
		if(tzdata->diag_res[ch_num].alarm_status_new == RUNNING_ALARM || tzdata->diag_res[ch_num].warn_status_new == RUNNING_WARN)
		{
			if(msg_alarm_trigger[ch_num].alarm_tz_type == NO_ALARM_TYPE
					&& msg_alarm_trigger[ch_num].alarm_trig_status == TRIG_OK)
			{
				msg_alarm_trigger[ch_num].alarm_trig_status = TRIG_ALARM;
				msg_alarm_trigger[ch_num].alarm_valid_flag = ALARM_VALID;

				if(ch_num == pw1_y)
					msg_alarm_trigger[ch_num].alarm_tz_type = PW1_Y_PWX_ALARM_TYPE;
				else if(ch_num == pw1_z)
					msg_alarm_trigger[ch_num].alarm_tz_type = PW1_Z_PWX_ALARM_TYPE;
				else if(ch_num == pw2_y)
					msg_alarm_trigger[ch_num].alarm_tz_type = PW2_Y_PWX_ALARM_TYPE;
				else if(ch_num == pw2_z)
					msg_alarm_trigger[ch_num].alarm_tz_type = PW2_Z_PWX_ALARM_TYPE;
			}
		}
		else if(tzdata->diag_res[ch_num].alarm_status_new == RUNNING_OK && tzdata->diag_res[ch_num].warn_status_new == RUNNING_OK)
		{
			if( ((msg_alarm_trigger[ch_num].alarm_tz_type == PW1_Y_PWX_ALARM_TYPE && ch_num == pw1_y)
			  || (msg_alarm_trigger[ch_num].alarm_tz_type == PW1_Z_PWX_ALARM_TYPE && ch_num == pw1_z)
			  || (msg_alarm_trigger[ch_num].alarm_tz_type == PW2_Y_PWX_ALARM_TYPE && ch_num == pw2_y)
			  || (msg_alarm_trigger[ch_num].alarm_tz_type == PW2_Z_PWX_ALARM_TYPE && ch_num == pw2_z))
					&& msg_alarm_trigger[ch_num].alarm_trig_status == TRIG_ALARM )
			{
				msg_alarm_trigger[ch_num].alarm_trig_status = TRIG_ALARM_REMOVE;
				msg_alarm_trigger[ch_num].alarm_valid_flag = ALARM_VALID;//30s后才无效
			}
		}
	}
#endif

#else
	/*超阈值次数判断*/
	if(tzdata->diag_res[ch_num].alarm_h_num > ALARM_HN)		/*满足平稳报警*/
	{
		pw_data_save_type = SENSOR_ALARM_DATA;
		tzdata->diag_res[ch_num].alarm_status_new=RUNNING_ALARM;
		if(tzdata->diag_res[ch_num].alarm_status_new!=tzdata->diag_res[ch_num].alarm_status_old)
		{
			if(tzdata->diag_res[ch_num].alarm_status_new==RUNNING_ALARM)
			{
				tzdata->diag_res[ch_num].alarm_status_old = tzdata->diag_res[ch_num].alarm_status_new;				//平稳报警

				if(sys_status_cnt.err_type[ch_num].diag_alarm_save_flag == 0)
				{
					sys_status_cnt.err_type[ch_num].diag_alarm_save_flag = 1;										//报警存储标志置１
					sys_status_cnt.err_type[ch_num].diag_normal_save_flag = 0;
				}
			}
		}
	}
	else if(tzdata->diag_res[ch_num].warn_h_num > WARN_HN) /*满足平稳预警*/
	{
		pw_data_save_type = SENSOR_WARN_DATA;
#ifdef INTERNAL_PROTOCOL_20210416
		tzdata->diag_res[ch_num].warn_status_new=RUNNING_WARN;
		if(tzdata->diag_res[ch_num].warn_status_new!=tzdata->diag_res[ch_num].warn_status_old)
		{
			if(tzdata->diag_res[ch_num].warn_status_new==RUNNING_WARN)
			{
				tzdata->diag_res[ch_num].warn_status_old = tzdata->diag_res[ch_num].warn_status_new;				//平稳预警

				if(sys_status_cnt.err_type[ch_num].diag_warn_save_flag == 0)
				{
					sys_status_cnt.err_type[ch_num].diag_warn_save_flag = 1;										//预警存储标志置１
					sys_status_cnt.err_type[ch_num].diag_normal_save_flag = 0;
				}
			}
		}
#else
		tzdata->diag_res[ch_num].alarm_status_new=RUNNING_WARN;
		if(tzdata->diag_res[ch_num].alarm_status_new!=tzdata->diag_res[ch_num].alarm_status_old)
		{
			if(tzdata->diag_res[ch_num].alarm_status_new==RUNNING_WARN)
			{
				tzdata->diag_res[ch_num].alarm_status_old = tzdata->diag_res[ch_num].alarm_status_new;				//平稳预警

				if(sys_status_cnt.err_type[ch_num].diag_warn_save_flag == 0)
				{
					sys_status_cnt.err_type[ch_num].diag_warn_save_flag = 1;										//预警存储标志置１
					sys_status_cnt.err_type[ch_num].diag_normal_save_flag = 0;
				}
			}
		}
#endif
	}
	else if((tzdata->diag_res[ch_num].alarm_l_num > ALARM_LN) && (tzdata->diag_res[ch_num].warn_l_num > WARN_LN))		/*恢复正常*/
	{
		/*pw_data_save_type = SENSOR_WARN_DATA;*/
		tzdata->diag_res[ch_num].alarm_status_new=RUNNING_OK;
		if(tzdata->diag_res[ch_num].alarm_status_new!=tzdata->diag_res[ch_num].alarm_status_old)
		{
			if(tzdata->diag_res[ch_num].alarm_status_new==RUNNING_OK)
			{
				tzdata->diag_res[ch_num].alarm_status_old = tzdata->diag_res[ch_num].alarm_status_new;					/*平稳预警*/

				if(sys_status_cnt.err_type[ch_num].diag_normal_save_flag == 0)
				{
					sys_status_cnt.err_type[ch_num].diag_normal_save_flag = 1;											/*正常存储标志置１*/
					sys_status_cnt.err_type[ch_num].diag_warn_save_flag = 0;
					sys_status_cnt.err_type[ch_num].diag_alarm_save_flag = 0;
				}
			}
		}
	}
#endif
 }

}
#endif

void move_float_to_float(float *diagbuf,float *adbuf,uint16_t date_size)
{
	uint16_t cnt_i;
	for(cnt_i = 0;cnt_i < date_size;cnt_i ++)
	{
		diagbuf[cnt_i] = adbuf[cnt_i];			//将采集到的16位数，赋值给浮点数
		//printf("move_data:%f,%d,%d\n",diagbuf[cnt_i],adbuf[cnt_i],cnt_i);
	}
}


void move_uint16_to_float(float *diagbuf,uint16_t *adbuf,uint16_t date_size)
{
	uint16_t cnt_i;
	for(cnt_i = 0;cnt_i < date_size;cnt_i ++)
	{
		diagbuf[cnt_i] = adbuf[cnt_i];//将采集到的16位数原码，赋值给浮点数
		//printf("move_data:%f,%d,%d\n",diagbuf[cnt_i],adbuf[cnt_i],cnt_i);
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
static void data_handling_pw(struct PW_DIAGNOS_PARA *tzdata, uint16_t *ad_buf, uint16_t *pw_ad_value,struct PW_TZ_DATA *tzdata_save,uint32_t data_size, uint32_t ch_numb)
{
	/*uint32_t i_segment = 0;*/
	uint8_t ch = 0; //通道id
//	static uint8_t test_data_index;
	/*uint8_t alarm_data[3] = {0};*/
	/*uint16_t cnt_i = 0;*/
	float curent_speed = 0.0f;

//	printf("before_move\n");
	/*512的采样率:每个数据2个字节*/
	memmove(tzdata->ad_value,pw_ad_value,512*2);

	for(ch = 0;ch < ch_numb;ch ++)
	{

		if(lastdata[ch]==0)									//只有第一次执行该程序
		{
			lastdata[ch]=uint16_mean(ad_buf,FS_PW);
			DEBUG("uint16_mean[%d]:%d\n",ch,lastdata[ch]);
		}

		/*滤波后的数据放到ad_buf*/
		ADdata_Filter(ad_buf,FS_PW,ch);

		if(self_test_para.self_test_flag)
		{
			pw_data_save_type = SENSOR_SELF_TEST_DATA;
		}
		else
		{
			pw_data_save_type = SENSOR_WORKIONG_DATA;
		}

#if defined(DATA_CENTER_TEST)

#else

		/*将无符号AD数据移动到float型数组中去,平稳性指标数据处理缓存*/
		move_uint16_to_float(&tzdata->data_deal_buff[ch].station_buff[tzdata->data_deal_buff[ch].w_index],ad_buf,data_size);
		move_uint16_to_float(&tzdata->data_dealb_buff[ch].station_buff[tzdata->data_dealb_buff[ch].w_index],ad_buf,data_size);

		/*抖车数据处理缓存*/
		move_uint16_to_float(&tzdata->jitter_float_deal[ch].jitter_buff[tzdata->jitter_float_deal[ch].w_index],ad_buf,data_size);

		/*晃车数据处理缓存*/
		move_uint16_to_float(&tzdata->shack_float[ch].shack_buff[tzdata->shack_float[ch].w_index],ad_buf,data_size);

		/*车体10s数据处理缓存*/
		move_uint16_to_float(&tzdata->ten_deal_data[ch].deal_buff[tzdata->ten_deal_data[ch].w_index],ad_buf,data_size);
		move_uint16_to_float(&tzdata->ten_deal_data[ch].deal_buff_bak[tzdata->ten_deal_data[ch].w_index], ad_buf, data_size);

		curent_speed = (pw_tz_data.train_public_info.speed[0]<<8 | pw_tz_data.train_public_info.speed[1]) * 0.01;
	#ifdef BRIDGE_DIAG
		if(ch == pw1_z || ch == pw2_z)
		{
			tzdata->ten_del_diag_res[ch].speed[tzdata->ten_del_diag_res[ch].speed_cnt++] = curent_speed;
		}
	#endif

		tzdata->wheel_diameter[ch].sample_speed[tzdata->wheel_diameter[ch].w_index] = curent_speed;

		/*平稳性指标数据缓存指针自加*/
		tzdata->data_deal_buff[ch].w_index = tzdata->data_deal_buff[ch].w_index + data_size;
		tzdata->data_dealb_buff[ch].w_index = tzdata->data_dealb_buff[ch].w_index + data_size;

		/*晃车数据缓存指针自加*/
		tzdata->shack_float[ch].w_index = tzdata->shack_float[ch].w_index + data_size;

		/*抖车数据缓存指针自加*/
		tzdata->jitter_float_deal[ch].w_index = tzdata->jitter_float_deal[ch].w_index + data_size;

		/*车体10s数据缓存自加*/
		tzdata->ten_deal_data[ch].w_index = tzdata->ten_deal_data[ch].w_index + data_size;

		/*径跳诊断缓存自加*/
		tzdata->wheel_diameter[ch].w_index ++;

		ad_buf = ad_buf + data_size;
#endif

		if(self_test_para.self_test_flag)					//满足自检条件
		{
			if(tzdata->data_deal_buff[ch].w_index == SELF_DIAG_SEC*FS_PW)		//2s数据用于自检
			{
				tzdata->data_deal_buff[ch].w_index=0u;
				tzdata->data_dealb_buff[ch].w_index=0u;

				/*传感器自检*/

//				printf("---------------->pW   sensor_self_test<----------\n");
				sensor_self_test(tzdata,ch);
			}
			tzdata->shack_float[ch].w_index = 0;
			tzdata->jitter_float[ch].w_index = 0;
			tzdata->jitter_float_deal[ch].w_index = 0;
			tzdata->ten_deal_data[ch].w_index = 0;
#ifdef BRIDGE_DIAG
			tzdata->ten_del_diag_res[ch].speed_cnt = 0;
#endif
			tzdata->wheel_diameter[ch].w_index = 0;
//			test_data_index = 0;
		}
		else
		{
//			printf("---------------->diag_deal_pw<--------------\n");
#ifdef PW_DIAG
			/*平稳性指标计算*/
			if(tzdata->data_deal_buff[ch].w_index == DIAG_SEC*FS_PW)
			{
				diag_deal_pw(tzdata, ch);								//每秒计算,前5sec平稳性指标为0,5sec后每秒出一个结果
				/*数据移动*/
				move_deal_data(tzdata, ch);

	#ifdef DIAG_DATA_SAVE_FOR_TEST
				if(ch == YZ_CH_NUM)
				{
					close_pwtzz_diag_file(PWTZZ_TYPE);
				}
	#endif
			}
#else
			tzdata->data_deal_buff[ch].w_index = 0;
			tzdata->data_dealb_buff[ch].w_index = 0;//关键指数限制，避免关算法时越界重启
#endif

#ifdef JT_DIAG
			/*径跳计算*/
			if(tzdata->wheel_diameter[ch].w_index == JT_DIAG_SEC)
			{
	#ifdef DIAG_DATA_SAVE_FOR_TEST
				if(ch == Z_CH_NUM)
				{
					save_jttzz_diag_file(JTTZZ_TYPE, DIAG_DATA_CHANNEL_TITLE, NULL, 1);
					save_jttzz_diag_file(JTTZZ_TYPE, DIAG_DATA_CHANNEL, &ch, 1);

					save_jttzz_diag_file(JTTZZ_TYPE, DIAG_DATA_SPEED_TITLE, NULL, 1);
					save_jttzz_diag_file(JTTZZ_TYPE, DIAG_DATA_SPEED, &curent_speed, 1);

					save_jttzz_diag_file(JTTZZ_TYPE, DIAG_DATA_AD_TITLE, NULL, 1);
					save_jttzz_diag_file(JTTZZ_TYPE, DIAG_DATA_AD, &tzdata->wheel_diameter[ch].fft_value[0], tzdata->wheel_diameter[ch].size);
				}
	#endif

				diag_wheel_pw(tzdata, ch);

				move_sample_speed_data(tzdata, ch);

	#ifdef DIAG_DATA_SAVE_FOR_TEST
				if(ch == Z_CH_NUM)
				{
					close_jttzz_diag_file(JTTZZ_TYPE);
				}
	#endif
			}
#else
			tzdata->wheel_diameter[ch].w_index = 0;//关键指数限制，避免关算法时越界重启
#endif

#ifdef HC_DIAG
	#ifdef DIAG_DATA_SAVE_FOR_TEST
			if(ch == pw1_y)
			{
				save_hctzz_diag_file(HCTZZ_TYPE, DIAG_DATA_CHANNEL_TITLE, NULL, 1);
				save_hctzz_diag_file(HCTZZ_TYPE, DIAG_DATA_CHANNEL, &ch, 1);

				save_hctzz_diag_file(HCTZZ_TYPE, DIAG_DATA_SPEED_TITLE, NULL, 1);
				save_hctzz_diag_file(HCTZZ_TYPE, DIAG_DATA_SPEED, &curent_speed, 1);

				save_hctzz_diag_file(HCTZZ_TYPE, DIAG_DATA_AD_TITLE, NULL, 1);
				save_hctzz_diag_file(HCTZZ_TYPE, DIAG_DATA_AD, &tzdata->shack_float[ch].shack_buff[0], tzdata->shack_float[ch].size);
			}
	#endif
			/*晃车诊断*/
			tzdata->shack_float[ch].w_index = 0u;
			diag_shack_pw(tzdata, ch);

	#ifdef DIAG_DATA_SAVE_FOR_TEST
			if(ch == pw1_y)
			{
				close_hctzz_diag_file(HCTZZ_TYPE);
			}
	#endif
#else
			tzdata->shack_float[ch].w_index = 0u;//关键指数限制，避免关算法时越界重启
#endif

#ifdef DC_DIAG
	#ifdef DC_HC_USE_FFT_FILTER
		#ifdef DIAG_DATA_SAVE_FOR_TEST
			if(ch == pw1_y)
			{
				save_dctzz_diag_file(DCTZZ_TYPE, DIAG_DATA_CHANNEL_TITLE, NULL, 1);
				save_dctzz_diag_file(DCTZZ_TYPE, DIAG_DATA_CHANNEL, &ch, 1);

				save_dctzz_diag_file(DCTZZ_TYPE, DIAG_DATA_SPEED_TITLE, NULL, 1);
				save_dctzz_diag_file(DCTZZ_TYPE, DIAG_DATA_SPEED, &curent_speed, 1);

				save_dctzz_diag_file(DCTZZ_TYPE, DIAG_DATA_AD_TITLE, NULL, 1);
				save_dctzz_diag_file(DCTZZ_TYPE, DIAG_DATA_AD, &tzdata->wheel_diameter[ch].fft_value[0], tzdata->wheel_diameter[ch].size);
			}
		#endif

			tzdata->jitter_float_deal[ch].w_index = 0u;
			diag_jitter_pw(tzdata, ch);

		#ifdef DIAG_DATA_SAVE_FOR_TEST
			if(ch == pw1_y)
			{
				close_dctzz_diag_file(DCTZZ_TYPE);
			}
		#endif
	#else
			/*抖车数据前处理*/
			deal_jitter_data_pw(tzdata, ch);
			move_float_to_float(&tzdata->jitter_float[ch].jitter_buff[tzdata->jitter_float[ch].w_index],tzdata->jitter_float_deal[ch].jitter_buff,data_size);
			tzdata->jitter_float[ch].w_index = tzdata->jitter_float[ch].w_index + data_size;

			/*抖车诊断计算　5sec处理一次*/
			if(tzdata->jitter_float[ch].w_index == JITTER_DIAG_SEC*FS_PW)
			{
				//printf("1-tzdata->jitter_float[ch].w_index:%d\r\n",tzdata->jitter_float[ch].w_index);
				/*tzdata->jitter_float[ch].w_index = 0u;*/
				diag_jitter_pw(tzdata, ch);
				/*数据移动*/
				move_jitter_data(tzdata, ch);

//				printf("2-tzdata->jitter_float[ch].w_index:%d\r\n",tzdata->jitter_float[ch].w_index);
			}
//			diag_shake_train();
	#endif
#else
			tzdata->jitter_float_deal[ch].w_index = 0u;
#endif

#ifdef CT_10S_DIAG
			/*车体10s数据处理*/
			if(tzdata->ten_deal_data[ch].w_index == TEN_DIAG_SEC*FS_PW)
			{
			#ifdef DIAG_DATA_SAVE_FOR_TEST
				if(ch == pw1_y || ch == pw1_z)
				{
					save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_CHANNEL_TITLE, NULL, 1);
					save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_CHANNEL, &ch, 1);

					save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_SPEED_TITLE, NULL, 1);
					save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_SPEED, &curent_speed, 1);

					save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_AD_TITLE, NULL, 1);
					save_tentzz_diag_file(TENTZZ_TYPE, DIAG_DATA_AD, &tzdata->ten_deal_data[ch].deal_buff[0], tzdata->ten_deal_data[ch].size);
				}
			#endif

				diag_ten_deal_pw(tzdata, ch);

				move_ten_deal_data(tzdata, ch);

			#ifdef DIAG_DATA_SAVE_FOR_TEST
				if(ch == pw1_y || ch == pw1_z)
				{
					close_tentzz_diag_file(TENTZZ_TYPE);
				}
			#endif
			}
#else
			tzdata->ten_deal_data[ch].w_index = 0;
#endif

		#ifdef WTD_DATA_TRANSLATE_PROTOCOL
			if(ch == ch_numb-1 && first_diag_result_flag==0 && tzdata->ten_deal_data[ch].w_index == DIAG_SEC*FS_PW)//首次所有通道算法完，标志置置1，以启动发送WTD数据
				first_diag_result_flag = 1;
		#endif

			tzdata->diag_res[ch].alarm_status_old=tzdata->diag_res[ch].alarm_status_new;
		}
	}

	/*采集板卡故障判断*/
#ifdef AD_REF_VOLT_ERR_REBOOT
#ifdef AD_REBOOT_FLAG
	if(self_test_para.self_test_flag )
#endif
	{
		judge_pw_board_err(tzdata->ad_value, FS_PW, (uint8_t *)&tzdata->pw_board_err);
	}
#else
	judge_pw_board_err(tzdata->ad_value, FS_PW, (uint8_t *)&tzdata->pw_board_err);
#endif

}





/*************************************************
Function:    pw_diagnos_thread_entry
Description: 自检和秒线程
Input:
Output:
Return:
Others:
*************************************************/
void pw_diagnos_thread_entry()
{
	int sem_res = -1;
	uint8_t data_cnt = 0;
	uint16_t orignal_data_head[6] = {0xAA51,0xAA52,0xAA53,0xAA54,0xAA55,0xAA56};
//	char test_buff[10]={0};
	//short dest[512];
#ifdef ORIGINAL_DATA_SAVE_ACC
		int16_t dest[512] = {0};
	#ifdef WTD_DATA_TRANSLATE_PROTOCOL
		int16_t wtd_dest[512] = {0};
	#endif
#else
	uint16_t dest[512] = {0};
#endif
	/*因采样问题导致的误报警,采用滤波进行修正,滤波初值*/
	f_buff_out[0] = 42598.0;
	f_buff_out[1] = 42598.0;
	f_buff_out[2] = 42598.0;
	f_buff_out[3] = 42598.0;
	f_buff_out[4] = 42598.0;
	f_buff_out[5] = 42598.0;
	/*因采样问题导致的误报警,采用滤波进行修正,滤波初值*/
	sem_res = sem_init(&pw_diagnos_sem,0,0);
	if(sem_res == -1)
	{
		DEBUG ("init pw_diagnos_sem error!\n");
	}


	while(1)
	{
//		printf("this is pw_diagnos_thread\n");
		sem_wait(&pw_diagnos_sem);

		if(pw_diagnos_ringbuf_para.num > pw_diagnos_ringbuf_para.size)
		{
			printf("lost_old_data_deal err\n");
			//诊断行数大于数据总行数处理
			lost_old_data_deal(&pw_diagnos_ringbuf_para,&pw_ad_data);
		}
		while(pw_diagnos_ringbuf_para.num)
		{
//			printf("-------------pw_diagnos_thread_entry-------------");
			//*(uint16_t *)pw_tz_data.train_public_info.speed = pw_ad_data.buff[pw_diagnos_ringbuf_para.index][DATA_HEAD_SIZE-1];
		    data_handling_pw(&pw_diagnos_para,&pw_ad_data.buff[pw_diagnos_ringbuf_para.index][DATA_HEAD_SIZE],pw_ad_data.ad_chnnel7,&pw_tz_data,FS_PW,CHANNEL_NUM);

			update_pw_tz_data(&pw_tz_data,&pw_diagnos_para,UPDATE_DIAG_DATE);

//			update_pw_tz_data(&pw_tz_data,&pw_diagnos_para,UPDATE_OTHER__DATE);

			pw_clb_config_para->pw_send_mcast_addr.is_need_ack = 0;

			//保存特征数据
			save_pw_tz_data(&pw_tz_data,pw_ad_data.buff[pw_diagnos_ringbuf_para.index][0],udp_send_addr.board_set,0x55,0);
			//发送特征数据
//			send_pw_tz_data(&pw_tz_data,pw_ad_data.buff[pw_diagnos_ringbuf_para.index][0],udp_send_addr.board_set,0x55,0);

			DEBUG("---------->sig_num:%d----------------->0\n",pw_ad_data.buff[pw_diagnos_ringbuf_para.index][0]);


			//存储并发送原始数据
			for(data_cnt = 0;data_cnt < CHANNEL_NUM;data_cnt++)
			{
//				memset(dest,0,FS_PW*2);

//				acc_data_to_g(&pw_ad_data.buff[pw_diagnos_ringbuf_para.index][DATA_HEAD_SIZE+512*data_cnt], dest, FS_PW);

				save_pw_original_data(&pw_ad_data.buff[pw_diagnos_ringbuf_para.index][DATA_HEAD_SIZE+512*data_cnt],
						pw_ad_data.buff[pw_diagnos_ringbuf_para.index][0],data_cnt);

//				send_pw_orignal_data(dest,orignal_data_head[data_cnt],pw_ad_data.buff[pw_diagnos_ringbuf_para.index][0],udp_send_addr.board_set,0x55,pw_clb_config_para->save_board_addr.is_need_ack);
				//DEBUG("---------->sig_num:%d----------------->%d\n",pw_ad_data.buff[pw_diagnos_ringbuf_para.index][0],data_cnt+1);
			}

			ring_buff_index_inc(&pw_diagnos_ringbuf_para);
			ring_buff_num_dec(&pw_diagnos_ringbuf_para);
		}
	}
}



int init_pw_diagnosis_thread()
{
	pthread_t pw_diagnos_thread_id;
	int ret = -1;

	ret=pthread_create(&pw_diagnos_thread_id,NULL,(void *)pw_diagnos_thread_entry,NULL);
	if(ret!=0)
	 {
	 	DEBUG ("Create pw diagnos thread error!\n");
	 }
	 	return ret;
}

