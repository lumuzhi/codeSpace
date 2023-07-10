#ifndef _SW_DIAGNOS_H_
#define _SW_DIAGNOS_H_
#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>
#include "user_data.h"
#include "global_macro.h"

//#define SWCHANNEL_NUM (1*4)
#define SWDIAG_SEC 1
#define GRAVITY_PARA 9.8f
#define FILTER_ORDER 8

#ifdef PP_TZZ_DIAG_1S_FILLTER
	#define PP_1S_NUM    30
	#define TZZ_DIAG_SEC 5
	#define CALC_PP_NUM  10
	#define TEN_PP_MAX_NUM   141//PP_1S_NUM*TZZ_DIAG_SEC-10+1=141
#elif defined(PP_TZZ_DIAG_20210420)
	#define TZZ_DIAG_SEC 5
	#define TEN_PP_MAX_NUM   150
#endif

#define VOLTAGE_RANGE 10//电压范围±5V
#define AD_RANGE   65536//ＡＤ范围
#define GRAVITY_PARA 9.8f
#define SENSITIVITY_SW 0.2f//单位：V/g
#ifdef WTD_DATA_TRANSLATE_PROTOCOL
	#define AMPLIFICATION_FACTOR 100//放大100倍
	#define WTD_AMPLIFICATION_FACTOR 1000//放大100倍
#else
	#define AMPLIFICATION_FACTOR 100//放大100倍
#endif

#define ZERO_POINT_VOLTAGE 3//零点电压3V,实际采样电压范围1-5V

#ifdef AD7606_COMPLEMENTARY_TO_ORIGINAL
	#define  AD_CONVERT_VOLTAGE(ad_original_code)   ((float)ad_original_code*VOLTAGE_RANGE/AD_RANGE)
	#define  AD_CONVERT_ACCELERATIONSW(ad_original_code)   (((float)ad_original_code*VOLTAGE_RANGE/AD_RANGE-ZERO_POINT_VOLTAGE)/SENSITIVITY_SW)
#else
	#define  AD_CONVERT_ACCELERATION(ad)   (float)ad*20.0f/65536.0f/SENSITIVITY_SW;//(float)(ad - 32768)*20.0f/65536.0f/sensitivity;
#endif

#define AD_VALID_CHECK_SIZE  256
#define ACCELERATION_OFFSET_THRESHOLD	2//m/s^2
#define AD_INVALID_TIMES	1

/*
 * 诊断结果枚举
 * 传感器报警
 * 传感器正常
 */
enum SWDIAGNOS_ALARM_STATUS
{
	SWRUNNING_OK,
	SWRUNNING_ALARM

};

enum SWSENSOR_TYPE
{
	SW_Y,
	SWAD_REF
};

//enum SENSOR_STATUS_JUDGE
//{
//	SENSOR_OK,
//	SENSOR_ERR,
//};
struct SWSENSOR_PARA
{
	enum SWSENSOR_TYPE  sens_type;
	float sensitivity;
	union SWSENS_SELF_TEST
	{
		struct
		{
		uint8_t  self_test_power_on:	1;
		uint8_t  self_test_real_time:	1;
		uint8_t  nc					:	6;
		}bits;
		uint8_t byte;
	}sens_self_test;
};

struct SWARRAY_FLOAT
{
	float *buff;
	uint32_t size;
	uint32_t w_index;
};

struct SWDIAG_RES
{
	float  value;
	uint32_t alarm_h_num; //continue > threshold cnt
	uint32_t alarm_l_num; //continue <threshold cnt
	uint32_t warn_h_num; //continue > threshold cnt
	uint32_t warn_l_num; //continue <threshold cnt
	union DIAG_ALARM_RES
	{
		struct
		{
			uint8_t alarm_status_new :1;
			uint8_t alarm_status_old :1;
			uint8_t warn_status_new  :1;
			uint8_t warn_status_old  :1;
			uint8_t nc				 :4;
		}bits;
		uint8_t alarm_byte;
	}diagnos_alarm_status;
//	enum DIAGNOS_ALARM_STATUS alarm_status_new;
//	enum DIAGNOS_ALARM_STATUS alarm_status_old;
};

//union DIAG_PARA_STATUS
//{
//  struct{
//	uint8_t ch_id             :4;
//	uint8_t nc                :1;
//    uint8_t sens_valid        :1;
//	uint8_t para_valid        :1;
//    uint8_t data_valid        :1;
//  }bits;
//	uint8_t   byte;
//};

//struct FILTER_STRUCT_FLOAT
//{
//	double    *before_filter_buff;
//	double    *hs_data;    //
//	uint32_t  filter_order;
//	uint32_t  window_size;
//	uint32_t  mem_flag;
//};

//struct STABLE_FFT_ARRAY
//{
//	uint32_t row_index;
//	uint32_t row_size;
//	uint32_t column_size;
//	float **buff;
//};

struct SW_ARRAY_FLOAT
{
	float *buff;
	uint32_t size;
	uint32_t w_index;
};

struct SW_DIAGNOS_PARA
{
	uint32_t deal_num[SWCHANNEL_NUM];
	uint32_t self_test_index[SWCHANNEL_NUM];
	uint32_t sw_alarm_cnt;
	uint16_t ad_value[512];						//用作存储基准电压AD
	uint16_t max_gvalue;
	uint16_t alarm_h_num;
	uint32_t sw_board_err;
	uint32_t sw_30km_singl;
	struct SWDIAG_RES diag_res[SWCHANNEL_NUM];
	struct SWSENSOR_PARA sens_para[SWCHANNEL_NUM];
	float rms[SWCHANNEL_NUM];
	float electric_val1[SWCHANNEL_NUM];
	float electric_val2[SWCHANNEL_NUM];
	float electric_val_real[SWCHANNEL_NUM];
	struct SWARRAY_FLOAT last_packet_extreme[SWCHANNEL_NUM];
	struct SWARRAY_FLOAT data_deal_buff[SWCHANNEL_NUM];//w_index峰值的个数  buff
	struct FILTER_STRUCT_FLOAT filter_para[SWCHANNEL_NUM];
	struct STABLE_FFT_ARRAY stable_fft_array[SWCHANNEL_NUM];
#ifdef PP_TZZ_DIAG_1S_FILLTER
	struct SW_ARRAY_FLOAT peak_to_peak_value_5s[SWCHANNEL_NUM];
	float ffz_min_tzz[SWCHANNEL_NUM];
	float ffz_mean_tzz[SWCHANNEL_NUM];
#elif defined(PP_TZZ_DIAG_20210420)
	struct ARRAY_FLOAT data_deal_buff_5s[SWCHANNEL_NUM];
	struct ARRAY_FLOAT data_deal_buff_bak_5s[SWCHANNEL_NUM];
	struct ARRAY_FLOAT last_packet_extreme_5s[SWCHANNEL_NUM];
	float ffz_min_tzz[SWCHANNEL_NUM];
	float ffz_mean_tzz[SWCHANNEL_NUM];
#endif
//	struct Diag_Eigvalue_PW Eig_value[SWCHANNEL_NUM];
};

#ifdef PP_TZZ_DIAG_1S_FILLTER
struct FIVE_SEC_PP_SIZE_RECORD
{
	uint16_t sec_cnt;
	uint16_t per_sec_num[TZZ_DIAG_SEC];
};
#endif

//enum DATASTATS_TYPE
//{
//	INIT_STATS,
//	POS_STATS,
//	NEG_STATS
//};

void init_sw_diagnos_para();
int init_sw_diagnosis_thread();

#ifdef PP_TZZ_DIAG_1S_FILLTER
	void diag_peak_peak_tzz(struct SW_DIAGNOS_PARA *tzdata,uint8_t ch_num);
#elif defined(PP_TZZ_DIAG_20210420)
	void calc_amp_deal_5s(struct SW_DIAGNOS_PARA * tzdata, uint32_t ch);
	void ampth_deal_5s(struct SW_DIAGNOS_PARA *tzdata, uint32_t ch);
	void diag_peak_peak_tzz(struct SW_DIAGNOS_PARA *tzdata,uint8_t ch_num);
#endif

#endif
