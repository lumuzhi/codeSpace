#ifndef _SELF_TEST_H_
#define _SELF_TEST_H_
#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>
#include "pw_diagnos.h"
#include "global_macro.h"
#include "sw_diagnos.h"
//自检参数结构体
//自检次数
//AD数据状态:1,稳定，可读取AD数据；0，不稳定，不可读取AD数据

struct SELF_TEST_PARA
{
	uint8_t self_test_num;
	uint8_t	ad_data_status;
	uint8_t sensor_mode;
	uint8_t self_test_flag;
};

struct AD_error
{
	uint8_t pw_jkb:4;
	uint8_t sw_jkb:4;
};

struct sensor_fault_pw
{
	uint8_t pw_y_cnt1:4;
	uint8_t pw_y_cnt2:4;
	uint8_t pw_x_cnt1:4;
	uint8_t pw_x_cnt2:4;
	uint8_t pw_z_cnt1:4;
	uint8_t pw_z_cnt2:4;
	uint8_t pw_jkb:   4;
	uint8_t nc:		  4;
};

struct sensor_fault_sw
{
	uint8_t sw_y_cnt1:4;
	uint8_t sw_y_cnt2:4;
	uint8_t sw_y_cnt3:4;
	uint8_t sw_y_cnt4:4;
	uint8_t sw_jkb:   4;
	uint8_t nc:		  4;
};

#ifdef AD_REF_VOLT_ERR_REBOOT
	uint8_t pw_ref_volt_err_cnt;
#endif


void init_self_test_para();
void sensor_self_test(struct PW_DIAGNOS_PARA *tzdata,uint8_t ch_num);
void swsensor_self_test(struct SW_DIAGNOS_PARA *tzdata,uint8_t ch_num);
void open_power_in_branch2();
void open_power_out_branch2();
void open_power_in_branch1();
void open_power_out_branch1();
void check_power_branch2();
void check_power_branch1();
void power_gpio_init(void);
void read_power_stuatus(void);
void init_sensor_gpio(void);
void Sensor_fault_judge_PW(float *psrc,uint32_t size,struct PW_DIAGNOS_PARA *tz_data,uint8_t ch);
void judge_pw_board_err(uint16_t *psrc,uint32_t size,uint8_t *status);
#ifdef AD_REF_VOLT_ERR_REBOOT
#ifdef AD_GPIO_ERR_REC
void open_power_ad1();
void open_power_ad2();
void check_power_ad();
#endif
uint8_t check_ref_volt(uint16_t *psrc,uint32_t size);
#endif
#endif

