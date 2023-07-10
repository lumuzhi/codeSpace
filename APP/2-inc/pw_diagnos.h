#ifndef _PW_DIAGNOS_H_
#define _PW_DIAGNOS_H_
#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>
#include "global_macro.h"

#define CHANNEL_NUM (2*3)
#define SWCHANNEL_NUM  2

#define Z_CHANNEL_NUM 2
/*#define DIAG_SEC 2*/
#define DIAG_SEC 2			//平稳性指标诊断需要的数据长度
#define JT_DIAG_SEC 5
#define SELF_DIAG_SEC 2		//传感器自检数据长度控制
#define JITTER_DIAG_SEC 5	/*抖车时域诊断数据长度*/
#define SHACK_DIAG_SEC 1	/*晃车时域诊断数据长度*/


#ifdef DC_HC_USE_FFT_FILTER
#define SHACK_FFT_SEC 2	/*晃车时域诊断数据长度*/
#define JITTER_FFT_SEC 2/*抖车时域诊断数据长度*/
#endif

#define TEN_DIAG_SEC 10		/*车体10s长度处理*/

#define VOLTAGE_RANGE 10//电压范围±5V
#define AD_RANGE   65536//ＡＤ范围
#define GRAVITY_PARA 9.8f
#define SENSITIVITY_PW 1.0f//4mA/g->1V/g
#ifdef WTD_DATA_TRANSLATE_PROTOCOL
	#define AMPLIFICATION_FACTOR 100//放大100倍
	#define WTD_AMPLIFICATION_FACTOR 1000//放大100倍
#else
	#define AMPLIFICATION_FACTOR 100//放大100倍
#endif

#define ZERO_POINT_VOLTAGE 3//零点电压3V,实际采样电压范围1-5V

#ifdef AD7606_COMPLEMENTARY_TO_ORIGINAL
#define  AD_CONVERT_VOLTAGE(ad_original_code)   ((float)ad_original_code*VOLTAGE_RANGE/AD_RANGE)
#define  AD_CONVERT_ACCELERATION(ad_original_code)   (((float)ad_original_code*VOLTAGE_RANGE/AD_RANGE-ZERO_POINT_VOLTAGE)/SENSITIVITY_PW)//((float)ad_original_code*VOLTAGE_RANGE/AD_RANGE/SENSITIVITY_PW)
#else
#define  AD_CONVERT_VOLTAGE(ad)   ((float)(ad-32768)*VOLTAGE_RANGE/AD_RANGE)
#define  AD_CONVERT_ACCELERATION(ad)   (((float)(ad-32768)*VOLTAGE_RANGE/AD_RANGE-ZERO_POINT_VOLTAGE)/SENSITIVITY_PW)
#endif

#ifdef BRIDGE_DIAG
#define BRIDGE_DATA_LENGTH   10
#endif
//#define SENSITIVITY_PW 1.25f

enum FREQUENCY_BAND_E
{
	FRE_0D2_TO_3HZ,
	FRE_5_TO_13HZ,
	FRE_1_TO_3HZ,
	FRE_MAX,
};

/*
 * 诊断结果枚举
 * 传感器报警
 * 传感器正常
 */
enum DIAGNOS_ALARM_STATUS
{
	RUNNING_OK,
	RUNNING_WARN,
	RUNNING_ALARM,
};

enum SENSOR_TYPE
{
	PW_Y,
	PW_Z,
	PW_X,
	AD_REF
};

enum SENSOR_STATUS_JUDGE
{
	SENSOR_OK,
	SENSOR_ERR,

};
struct SENSOR_PARA
{
	enum SENSOR_TYPE  sens_type;
	float sensitivity;
	union SENS_SELF_TEST
	{
		struct
		{
		uint8_t open_circuit_err_power_on:	1;
		uint8_t open_circuit_err_real_time:	1;
		uint8_t nc:							6;
		}bits;
		uint8_t byte;
	}sens_self_test;
	//enum SENSOR_STATUS_JUDGE  sens_self_test;
};


struct STABLE_FFT_ARRAY
{
	uint32_t row_index;
	uint32_t row_size;
	uint32_t column_size;
	float **buff;
};

/*用于平稳性指标计算的数据缓存*/
struct ARRAY_FLOAT
{
	float *station_buff;
	uint32_t size;				/*初始化时给size赋值*/
	uint32_t w_index;
};

/*用于抖车时域的数据缓存 5s大小*/
struct JITTER_FLOAT
{
	float *jitter_buff;
	uint32_t size;				/*初始化时给size赋值*/
	uint32_t w_index;
};

/*用于晃车时域的数据缓存 1s大小*/
struct SHACK_FLOAT
{
	float *shack_buff;
	uint32_t size;				/*初始化时给size赋值*/
	uint32_t w_index;
};

/*用于10s数据处理缓存*/
struct TEN_DEAL_DATA
{
	float *deal_buff;				/*从AD采集的数据缓存*/
	float *deal_buff_bak;			/*数据备份处理*/
	float *resample_deal_buff;		/*重采样数据缓存*/
	uint32_t resample_size;			/*重采样数据大小*/
	uint32_t size;
	uint32_t w_index;
};

struct TEN_DEAL_DIAG_RES
{
	float *fft_value;			/*fft后存放频谱幅值*/
	float main_fre1;			/*晃车主频*/
	float max_value1;			/*晃车主频对应的最大幅值*/
	float percent_main_fre1;	/*晃车主频占比*/

	float main_fre2;			/*抖车主频*/
	float max_value2;			/*抖车主频对应的最大幅值*/
	float percent_main_fre2;	/*抖车主频占比*/

#ifdef BRIDGE_DIAG
	float main_fre3[BRIDGE_DATA_LENGTH];			/*桥梁主频－垂*/
	float max_value3[BRIDGE_DATA_LENGTH];			/*桥梁主频对应的最大幅值－垂*/
	float max_fabs_value3[BRIDGE_DATA_LENGTH];		/*桥梁时域10s　5120点对应的幅值的绝对值的最大值*/
//	float percent_main_fre3[BRIDGE_DATA_LENGTH];	/*桥梁主频占比*/
	float bridge_len[BRIDGE_DATA_LENGTH];       /*桥梁长度*/
	float speed[BRIDGE_DATA_LENGTH];/*每秒平均速度*/
	uint32_t speed_cnt; /*平均速度记次数*/
	uint32_t keep_len_times;     /*桥梁长度相等或平均值±1以内的次数*/
	uint32_t bridge_flag;        /*桥梁识别标志*/
#endif
};


/*用于径跳的数据缓存*/
struct WHEEL_DIAMETER
{
	float *sample_speed;
	float *fft_value;
	uint32_t size;
	uint32_t w_index;
	float ave_speed;
};
//PW 阈值结构体
struct THRESHOLD_PARA
{
  /*平稳性指标计算结果 设置阈值使用*/
  float warn_value;		/*预警阈值*/
  float alarm_value;	/*报警阈值*/
  uint32_t warn_h_cnt;
  uint32_t warn_l_cnt;
  uint32_t alarm_h_cnt;
  uint32_t alarm_l_cnt;
};

struct JITTER_THRESHOLD_PARA
{
	/*抖车计算结果　设置阈值使用*/
  float jitter_warn_value;		/*抖车使用均方根预警阈值*/
  float jitter_alarm_value;		/*抖车使用均方根报警阈值*/
  uint32_t jitter_warn_h_cnt;
  uint32_t jitter_warn_l_cnt;
  uint32_t jitter_alarm_h_cnt;
  uint32_t jitter_alarm_l_cnt;
	  /**/
};

struct SHAKE_THRESHOLD_PARA
{
	/*晃车计算结果　设置阈值使用*/
  float shake_warn_value;		/*晃车使用均方根预警阈值*/
  float shake_alarm_value;		/*晃车使用均方根报警阈值*/
  uint32_t shake_warn_h_cnt;
  uint32_t shake_warn_l_cnt;
  uint32_t shake_alarm_h_cnt;
  uint32_t shake_alarm_l_cnt;
/**/
};


struct WHEEL_THRESHOLD_PARA
{
	/*径跳计算结果　设置阈值使用*/
  float wheel_alarm_value;		/*径跳使用报警阈值*/
  uint32_t wheel_alarm_h_cnt;	/*径跳大于报警阈值次数*/
  uint32_t wheel_alarm_l_cnt;
/**/
};

struct DIAG_RES
{
	float  value;
	uint32_t warn_h_num; //continue > threshold cnt
	uint32_t warn_l_num; //continue <threshold cnt
	uint32_t alarm_h_num;
	uint32_t alarm_l_num;
	enum DIAGNOS_ALARM_STATUS alarm_status_new;
	enum DIAGNOS_ALARM_STATUS alarm_status_old;
#ifdef INTERNAL_PROTOCOL_20210416
	enum DIAGNOS_ALARM_STATUS warn_status_new;
	enum DIAGNOS_ALARM_STATUS warn_status_old;
#endif
};

/*抖车诊断结果*/
struct JITTER_DIAG_RES
{
	float  value;
	uint32_t warn_h_num;
	uint32_t warn_l_num;
	uint32_t alarm_h_num;
	uint32_t alarm_l_num;
	enum DIAGNOS_ALARM_STATUS warn_status_new;
	enum DIAGNOS_ALARM_STATUS warn_status_old;
	enum DIAGNOS_ALARM_STATUS alarm_status_new;
	enum DIAGNOS_ALARM_STATUS alarm_status_old;
};

/*晃车诊断结果*/
struct SHACK_DIAG_RES
{
	float  value;
	uint32_t warn_h_num;
	uint32_t warn_l_num;
	uint32_t alarm_h_num;
	uint32_t alarm_l_num;
	enum DIAGNOS_ALARM_STATUS warn_status_new;
	enum DIAGNOS_ALARM_STATUS warn_status_old;
	enum DIAGNOS_ALARM_STATUS alarm_status_new;
	enum DIAGNOS_ALARM_STATUS alarm_status_old;
#ifdef HC_MODIFY_20211112
	enum DIAGNOS_ALARM_STATUS warn_status_cur;
	enum DIAGNOS_ALARM_STATUS alarm_status_cur;
	time_t   warn_first_time;
	time_t   warn_second_time;
	int32_t warn_diff_time;//14s
	time_t   alarm_first_time;
	time_t   alarm_second_time;
	int32_t alarm_diff_time;//14s
#endif
};

struct WHEEL_DIAG_RES
{
	float  avg_fre;
	float  max_fre;
	float  value;
	uint32_t alarm_h_num;
	uint32_t alarm_l_num;
	enum DIAGNOS_ALARM_STATUS alarm_status_new;
	enum DIAGNOS_ALARM_STATUS alarm_status_old;
};


union DIAG_PARA_STATUS
{
  struct{
	uint8_t ch_id             :4;
	uint8_t nc                :1;
    uint8_t sens_valid        :1;
	uint8_t para_valid        :1;
    uint8_t data_valid        :1;
  }BITS;
	uint8_t   byte;
};

struct FILTER_STRUCT_FLOAT
{
	double    *before_filter_buff;
	double    *hs_data;    //
	uint32_t  filter_order;
	uint32_t  window_size;
	uint32_t  mem_flag;
};

struct PW_DIAGNOS_PARA
{
	uint32_t station_deal_num[CHANNEL_NUM];
	uint32_t self_test_index[CHANNEL_NUM];
	uint16_t ad_value[512];
	uint32_t pw_board_err;
	/*平稳性指标诊断结果*/
	struct DIAG_RES diag_res[CHANNEL_NUM];


	/*晃车诊断结果*/
	struct SHACK_DIAG_RES shack_diag_res[CHANNEL_NUM];
	/*抖车诊断结果*/
	struct JITTER_DIAG_RES jitter_diag_res[CHANNEL_NUM];

	/*径跳诊断结果*/
	struct WHEEL_DIAG_RES wheel_diag_res[CHANNEL_NUM];

	struct SENSOR_PARA sens_para[CHANNEL_NUM];
	float rms[CHANNEL_NUM];
	float electric_val1[CHANNEL_NUM];
	float electric_val2[CHANNEL_NUM];
	float electric_val_real[CHANNEL_NUM];
	/*平稳性指标数据缓存结构体*/
	struct ARRAY_FLOAT data_deal_buff[CHANNEL_NUM];//2s

	struct ARRAY_FLOAT data_dealb_buff[CHANNEL_NUM];

	/*新增抖车数据缓存结构体*/
	struct JITTER_FLOAT jitter_float[CHANNEL_NUM];//5s

	/*新增抖车数据缓存结构体*/
	struct JITTER_FLOAT jitter_float_deal[CHANNEL_NUM];//1s
#ifdef DC_HC_USE_FFT_FILTER
	/*新增晃车数据fft缓存结构体*/
	struct JITTER_FLOAT jitter_fft_in[CHANNEL_NUM];//2s  1024
#endif
	/*车体10s数据处理缓存*/
	struct TEN_DEAL_DATA ten_deal_data[CHANNEL_NUM];

	/*车体10s数据处理结果*/
	struct TEN_DEAL_DIAG_RES ten_del_diag_res[CHANNEL_NUM];


	/*新增晃车数据缓存结构体*/
	struct SHACK_FLOAT shack_float[CHANNEL_NUM];//1s 512

#ifdef DC_HC_USE_FFT_FILTER
	/*新增晃车数据fft缓存结构体*/
	struct SHACK_FLOAT shack_fft_in[CHANNEL_NUM];//2s  1024
#endif

	/*晃车找峰峰值缓存结构体*/
	struct SHACK_FLOAT last_packet_extreme[CHANNEL_NUM];
	/*晃车算法滤波参数*/
	struct FILTER_STRUCT_FLOAT shack_filter_para[CHANNEL_NUM];
	/*抖车算法滤波参数*/
	struct FILTER_STRUCT_FLOAT jitter_filter_para[CHANNEL_NUM];

	struct FILTER_STRUCT_FLOAT filter_para[CHANNEL_NUM];
	struct STABLE_FFT_ARRAY stable_fft_array[CHANNEL_NUM];

	/*径跳数据处理缓存*/
	struct WHEEL_DIAMETER wheel_diameter[CHANNEL_NUM];//1s  sample_speed:512 fft_value:80

};

enum DATASTATS_TYPE
{
	INIT_STATS,
	POS_STATS,
	NEG_STATS
};

/*struct*/

int init_pw_diagnosis_thread();
void pw_diagnos_thread_entry();
void shack_filter_deal(struct SHACK_FLOAT *deal_buf,struct FILTER_STRUCT_FLOAT *filter_struct_float,enum SENSOR_TYPE type);
void ampth_deal(struct SHACK_FLOAT *shack_float_psrc, struct SHACK_FLOAT *shack_last_packet_extreme);
void move_jitter_data(struct PW_DIAGNOS_PARA *tzdata,uint8_t ch_num);
void deal_jitter_data_pw(struct PW_DIAGNOS_PARA *tzdata,uint8_t ch_num);
void move_float_to_float(float *diagbuf,float *adbuf,uint16_t date_size);
void init_wheel_buff(struct WHEEL_DIAMETER *psrc,uint32_t size);
void filter_deal(struct TEN_DEAL_DATA *deal_buf,struct FILTER_STRUCT_FLOAT *filter_struct_float);
void init_ten_deal_diag(struct TEN_DEAL_DIAG_RES *ten_deal_diag_res,uint32_t size);
void get_acc_data(float *buff, int data_cnt);

#ifdef ORIGINAL_DATA_SAVE_ACC
	#ifdef WTD_DATA_TRANSLATE_PROTOCOL
	void acc_data_to_g(uint16_t *srcbuf, int16_t *dest, int16_t *wtd_dest, uint16_t len);
	#else
	void acc_data_to_g(uint16_t *srcbuf, int16_t *dest, uint16_t len);
	#endif
#else
	void acc_data_to_g(uint16_t *srcbuf,uint16_t *dest,uint16_t len);
#endif

void fft_ifft_deal(float *fft_in_buf, float *ifft_out_buf, uint8_t fre_band);


#endif
