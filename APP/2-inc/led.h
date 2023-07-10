#ifndef _LED_H_
#define _LED_H_

#include <stdint.h>


//led type
enum LED_TYPE
{						//power led   		D1  常亮
	PW_RUN_LED = 15,    //running led		D2  定时闪
	PW_LAN_LED = 14,    //ethA/ethB led		D3  正常定时闪, 不正常灭
	PW_CAN_LED = 13,	//canA/canB led		D4  正常定时闪, 不正常灭
	PW_USER1_LED = 12,  //平稳1位端传感器     　D5  正常定时闪, 不正常灭
	PW_USER2_LED = 11,  //平稳2位端传感器  　  　D6  正常定时闪, 不正常灭
	PW_USER3_LED = 10,  //save led  	    D7  正常定时闪, 不正常灭
	PW_ERR_LED = 9      //board err led 	D8  正常灭,不正常常亮,
};

enum LED_STATU
{
	LED_OFF,
	LED_ON
};

void init_led_gpio(void);
void set_serial_in_gpio(uint8_t num);
void set_clock_gpio(uint8_t num);
void set_latch_gpio(uint8_t num);
void set_enable_gpio(uint8_t num);
void show_led(uint8_t *led_buf);
void show_led_test(void);
//void control_led(enum LED_TYPE led_type,enum LED_STATU led_status);
void control_led(enum LED_TYPE led_type,enum LED_STATU led_status);
void test_led(void);
void init_led_status();
void led_sys(uint8_t flag);
void led_net(uint8_t flag);
void led_psw(uint8_t flag);

#endif
