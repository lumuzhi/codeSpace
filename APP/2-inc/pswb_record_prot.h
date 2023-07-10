/*
 * pswb_record_prot.h
 *
 *  Created on: 2022年11月21日
 *      Author: Administrator
 */

#ifndef PSWB_RECORD_PROT_H_
#define PSWB_RECORD_PROT_H_

#define DEVICE_FACTORY_CODE                 0x04

//自定义时间结构体
typedef struct
{
	uint8_t year;
	uint8_t mon;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
}pswb_collect_time_t;

/**
 * MVB的公共信息
 */
typedef struct
{
	uint8_t nc[10];
	union VALID
	{
		struct BITS
		{
			uint8_t time_valid :1;
			uint8_t wheel_valid :1;
			uint8_t speed_valid :1;
			uint8_t ncc :5;
		} bits;
		uint8_t byte ;
	} valid;
	uint8_t time[6];
	uint8_t line[2];
	uint8_t train_num[2];
	uint8_t wheel[16];
	uint8_t speed[2];
	uint8_t curr_id[2];    // 当前站ID
	uint8_t next_id[2];
	uint8_t ncc2;
	uint8_t mileage_total[4];	//44-47	总运营里程0-4294967296，1=1km
	uint8_t km_sign[2];		//48-49	公里标
	uint8_t start_station[2];	//50-51	起点站
	uint8_t end_station[2];	//52-53	终点站
	uint8_t run_state;	//54	运行状态，包括上行下行司机室状态
}txb_MVB_public_t, *ptxb_MVB_public_t;

typedef struct
{
	uint8_t data_head[2];
	uint8_t data_len[2];
	uint8_t factory_code;
	uint8_t device_code;
	uint8_t life_signal[2];
	uint8_t target_addr[2];
	uint8_t resend_flag;
	uint8_t answer_flag;
	uint8_t udp_packet;
	uint8_t reserve[10];
	uint8_t train_id;
	uint8_t cmd;
	uint8_t target_broad;
	uint8_t add_cmd;
	uint8_t sum_crc[2];
}record_unicast_protocol_t, *precord_unicast_protocol_t;

//总线板错误信息表
typedef union
{
	struct
	{
		uint8_t pw_sensor1_err :1;            //1位端平稳传感器故障
		uint8_t pw_sensor2_err :1;            //2位端平稳传感器故障
		uint8_t pw_sensor1_self_err :1;       //1位端平稳传感器自检故障
		uint8_t pw_sensor2_self_err :1;       //2位端平稳传感器自检故障
		uint8_t nc :4;
	} bits;
	uint8_t byte;
}pwb_err_t;

typedef union
{
	struct
	{
		uint8_t pw1_y_alarm :1;              //1位端平稳横向报警
		uint8_t pw1_z_alarm :1;
		uint8_t pw2_y_alarm :1;
		uint8_t pw2_z_alarm :1;
		uint8_t pw1_y_warning :1;
		uint8_t pw1_z_warning :1;
		uint8_t pw2_y_warning :1;
		uint8_t pw2_z_warning :1;		     //2位端平稳垂向预警
	} bits;
	uint8_t byte;
}pwb_alarm_t;

typedef union
{
	struct
	{
		uint8_t sw_sensor1_err :1;              //1位失稳传感器故障
		uint8_t sw_sensor2_err :1;
		uint8_t sw_sensor1_self_err :1;
		uint8_t sw_sensor2_self_err :1;
		uint8_t sw1_y_alarm :1;
		uint8_t sw2_y_alarm :1;
		uint8_t sw1_y_judge :1;
		uint8_t sw2_y_judge :1;		             //2位端失稳预判
	} bits;
	uint8_t byte;
}swb_err_alarm_t;

typedef struct
{
	uint8_t soft_version[2];
	uint8_t hard_version[2];
	uint8_t update_time[4];
}pwb_speed_version_t;

typedef struct
{
	uint8_t wh;	//高位字节
	uint8_t wl;	//低位字节
}uint_16_type_t;//大端模式
// zxb转速信息表
typedef struct
{
	pwb_err_t pwb_err;                 // 故障信息
	pwb_alarm_t pw_alarm;
	swb_err_alarm_t swb_err;
	uint_16_type_t side1_y_quota;            //平稳性指标
	uint_16_type_t side1_z_quota;
	uint_16_type_t side2_y_quota;            //
	uint_16_type_t side2_z_quota;
	uint_16_type_t side1_y_adval;            //电压
	uint_16_type_t side1_z_adval;
	uint_16_type_t side2_y_adval;            //
	uint_16_type_t side2_z_adval;
	uint_16_type_t sw1_y_adval;
	uint_16_type_t sw2_y_adval;
	uint8_t sw1_min_tzz;
	uint8_t sw2_min_tzz;
	uint8_t sw1_mean_tzz;
	uint8_t sw2_mean_tzz;
	uint8_t rc[13];
}psw_info_data_t;

//typedef struct
//{
//	swb_err_alarm_t swb_err;                 // 故障信息
//	uint_16_type_t side1_y_adval;
//	uint_16_type_t side2_y_adval;
//	uint8_t rc[15];
//}sw_info_data_t;

typedef struct
{
	uint8_t data_head[2];
	uint8_t data_len[2];
	uint8_t factory_code;
	uint8_t device_code;
	uint8_t life_signal[2];
	uint8_t target_addr[2];
	uint8_t resend_flag;
	uint8_t answer_flag;
	uint8_t udp_packet;
	uint8_t reserve[13];
	uint8_t data_inform[sizeof(psw_info_data_t)];
	uint8_t sum_crc[2];
}pswb_record_protocol_t, *ppswb_record_protocol_t;

//typedef struct
//{
//	uint8_t data_head[2];
//	uint8_t data_len[2];
//	uint8_t factory_code;
//	uint8_t device_code;
//	uint8_t life_signal[2];
//	uint8_t target_addr[2];
//	uint8_t resend_flag;
//	uint8_t answer_flag;
//	uint8_t udp_packet;
//	uint8_t reserve[13];
//	uint8_t data_inform[sizeof(sw_info_data_t)];
//	uint8_t sum_crc[2];
//}swb_record_protocol_t, *pswb_record_protocol_t;


#endif /* PSWB_RECORD_PROT_H_ */
