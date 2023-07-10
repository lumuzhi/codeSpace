#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include "led.h"
#include "user_data.h"


#define SERIAL_IN_GPIO(x)  {set_serial_in_gpio(x)}

#define LED_GPIO_EXPORT          "/sys/class/gpio/export"
//#define SERIAL_IN_GPIO_PIN_NUM   "46"
//#define CLOCK_GPIO_PIN_NUM		 "45"
//#define LATCH_GPIO_PIN_NUM		 "44"
//#define ENABLE_GPIO_PIN_NUM		 "88"

#define LED1_GPIO_PIN_NUM      "44"
#define LED2_GPIO_PIN_NUM      "45"
#define LED3_GPIO_PIN_NUM	   "46"

#define LED1_GPIO_PIN_DIR	     "/sys/class/gpio/gpio44/direction"
#define LED2_GPIO_PIN_DIR	     "/sys/class/gpio/gpio45/direction"
#define LED3_GPIO_PIN_DIR	     "/sys/class/gpio/gpio46/direction"
//#define ENABLE_GPIO_PIN_DIR	     "/sys/class/gpio/gpio88/direction"

#define LED3_GPIO_PIN_VAL	     "/sys/class/gpio/gpio46/value"
#define LED2_GPIO_PIN_VAL	     "/sys/class/gpio/gpio45/value"
#define LED1_GPIO_PIN_VAL	     "/sys/class/gpio/gpio44/value"
//#define ENABLE_GPIO_PIN_VAL	     "/sys/class/gpio/gpio88/value"

//#define TEST_GPIO_PIN_DIR	     "/sys/class/gpio/gpio89/direction"
//#define TEST_GPIO_PIN_VAL	     "/sys/class/gpio/gpio89/value"
//#define TEST_GPIO_PIN_NUM		 "89"


#define LED_GPIO_DIR_OUT_VAL      "out"
#define LED_GPIO_DIR_IN_VAL       "in"
#define LED_GPIO_RST_VAL_H        "1"
#define LED_GPIO_RST_VAL_L        "0"


void init_GPIO(uint8_t num)
{
	int fd = -1;
	int ret = -1;
	char val = '1';
	if (num == 1)
		val = '1';
	else
		val = '0';
	errno = 0;
	fd = open("/sys/class/gpio/gpio89/value",O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
	if(fd == -1)
	{
		DEBUG("open clock_gpio_failed\n");
		return;
	}

	ret = write(fd,&val,1);
	if(ret == -1)
	{
		DEBUG("write clock_gpio_failed\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
		return;
	}
	close(fd);
}

//int usleep(useconds_t usec);

uint8_t contrl_led_value[16];

void init_led_gpio(void)
{
	int fd1 = -1;
	int fd2 = -1;
	int w_res = -1;
//	char *w_buf = "out";
	//*w_buf = ;
	errno = 0;
	fd1 = open(LED_GPIO_EXPORT,O_WRONLY);
	if(fd1 == -1)
	{
		DEBUG("OPEN LED_GPIO_EXPORT FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}

	//set serial_in_gpio
	w_res = write(fd1,&LED1_GPIO_PIN_NUM,2);
	if(w_res == -1)
	{
		DEBUG("WRITE LED1_GPIO_PIN_NUM FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}

	w_res = write(fd1,&LED2_GPIO_PIN_NUM,2);
	if(w_res == -1)
	{
		DEBUG("WRITE LED2_GPIO_PIN_NUM FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}

	w_res = write(fd1,&LED3_GPIO_PIN_NUM,2);
	if(w_res == -1)
	{
		DEBUG("WRITE LED3_GPIO_PIN_NUM FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}

	close(fd1);

	fd2 = -1;
	w_res = -1;
	fd2 = open(LED1_GPIO_PIN_DIR,O_RDWR);
	if(fd2 == -1)
	{
		DEBUG("OPEN LED1_GPIO_PIN_DIR FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
	//w_res = write(fd2,&LED_GPIO_DIR_OUT_VAL,3);
	w_res =write(fd2,LED_GPIO_DIR_OUT_VAL,sizeof("out"));
	if(w_res == -1)
	{
		DEBUG("WRITE LED1_GPIO_PIN_DIR FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
	close(fd2);
	//set CLOCK_GPIO

	fd2 = -1;
	w_res = -1;
	fd2 = open(LED2_GPIO_PIN_DIR,O_RDWR);
	if(fd2 == -1)
	{
		DEBUG("OPEN LED2_GPIO_PIN_DIR FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
	w_res = write(fd2,&LED_GPIO_DIR_OUT_VAL,sizeof("out"));
	if(w_res == -1)
	{
		DEBUG("WRITE LED2_GPIO_PIN_DIR FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
	close(fd2);
	//set LATCH_GPIO
	fd2 = -1;
	w_res = -1;
	fd2 = open(LED3_GPIO_PIN_DIR,O_RDWR);
	if(fd2 == -1)
	{
		DEBUG("OPEN LED3_GPIO_PIN_DIR FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
	w_res = write(fd2,&LED_GPIO_DIR_OUT_VAL,sizeof("out"));
	if(w_res == -1)
	{
		DEBUG("WRITE LED3_GPIO_PIN_DIR FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
	close(fd2);
}



void set_serial_in_gpio(uint8_t num) //buyong
{
	//system("echo 1 > /sys/class/gpio/gpio27/value");
	int fd = -1;
	int ret = -1;
	char val = '1';
	if (num == 1)
		val = '1';
	else
		val = '0';
	errno = 0;
	//fd = open("/sys/class/gpio/gpio27/value",O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
	fd = open("/sys/class/gpio/gpio89/value",O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
	if(fd == -1)
	{
		DEBUG("open serial_in_gpio_failed\n");
		return;
	}

	ret = write(fd,&val,1);
	if(ret == -1)
	{
		DEBUG("write serial_in_gpio_failed\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
		return;
	}

	close(fd);

}


void set_clock_gpio(uint8_t num)
{
	int fd = -1;
	int ret = -1;
	char val = '1';
	if (num == 1)
		val = '1';
	else
		val = '0';
	errno = 0;
	fd = open("/sys/class/gpio/gpio86/value",O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
	if(fd == -1)
	{
		DEBUG("open clock_gpio_failed\n");
		return;
	}

	ret = write(fd,&val,1);
	if(ret == -1)
	{
		DEBUG("write clock_gpio_failed\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
		return;
	}


	close(fd);
}

void set_latch_gpio(uint8_t num)
{
	int fd = -1;
	int ret = -1;
	char val = '1';
	if (num == 1)
		val = '1';
	else
		val = '0';
	errno = 0;
	fd = open("/sys/class/gpio/gpio87/value",O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
	if(fd == -1)
	{
		DEBUG("open latch_gpio_failed\n");
		return;
	}

	ret = write(fd,&val,sizeof(val));

	if(ret == -1)
	{
		DEBUG("write latch_gpio_failed\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
		return;
	}

	close(fd);
}

void set_enable_gpio(uint8_t num)
{
	int fd = -1;
	int ret = -1;
	char val = '1';
	if (num == 1)
		val = '1';
	else
		val = '0';
	errno = 0;
	fd = open("/sys/class/gpio/gpio88/value",O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
	if(fd == -1)
	{
		DEBUG("open enable_gpio_failed\n");
		return;
	}

	ret = write(fd,&val,sizeof(val));

	if(ret == -1)
	{
		DEBUG("write enable_gpio_failed\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
		return;
	}

	close(fd);
}

void read_latch_gpio(void)
{
	int fd = -1;
	int read_cnt = -1;
	char readbuf[1] = {3};

	DEBUG("read latch gpio\n");
	fd = open("/sys/class/gpio/gpio87/value",O_RDWR);
	if(fd == -1)
	{
		DEBUG("open latch_gpio_failed\n");
		return;
	}

	read_cnt = read(fd,readbuf,1);
	//usleep(100);
	if(read_cnt == -1)
	{
		DEBUG("read latch_gpio_failed\n");
		return;
	}
	DEBUG("read_cnt:%d,read_buf:%c\n",read_cnt,readbuf[0]);

}

void read_serial_in_gpio(void)
{
	int fd = -1;
	int read_cnt = -1;
	char readbuf[1] = {3};

	DEBUG("read serial_in_gpio\n");
	fd = open("/sys/class/gpio/gpio89/value",O_RDWR);
	if(fd == -1)
	{
		DEBUG("open serial_in_gpio_failed\n");
		return;
	}

	read_cnt = read(fd,readbuf,1);
	if(read_cnt == -1)
	{
		DEBUG("read serial_in_gpio_failed\n");
		return;
	}
	DEBUG("read_cnt:%d,read_buf:%c\n",read_cnt,readbuf[0]);

}

void read_enable_gpio(void)
{
	int fd = -1;
	int read_cnt = -1;
	char readbuf[1] = {3};

	DEBUG("read enable_gpio\n");
	fd = open("/sys/class/gpio/gpio88/value",O_RDWR);
	if(fd == -1)
	{
		DEBUG("open enable_gpio_failed\n");
		return;
	}

	read_cnt = read(fd,readbuf,1);
	if(read_cnt == -1)
	{
		DEBUG("read enable_gpio_failed\n");
		return;
	}
	DEBUG("read_cnt:%d,read_buf:%c\n",read_cnt,readbuf[0]);

}

void read_clock_gpio(void)
{
	int fd = -1;
	int read_cnt = -1;
	char readbuf[1] = {3};

	DEBUG("read clock_gpio\n");
	fd = open("/sys/class/gpio/gpio86/value",O_RDWR);
	if(fd == -1)
	{
		DEBUG("open clock_gpio_failed\n");
		return;
	}

	read_cnt = read(fd,readbuf,1);
	if(read_cnt == -1)
	{
		DEBUG("read clock_gpio_failed\n");
		return;
	}
	DEBUG("read_cnt:%d,read_buf:%c\n",read_cnt,readbuf[0]);

}

void show_led(uint8_t *led_buf)
{
	uint8_t i = 0;
	set_latch_gpio(0);
	set_enable_gpio(1);
//	DEBUG("test_led\n");
	for(i = 0;i<16;i++)
	{
		if(led_buf[i] == 1)
		{
			set_serial_in_gpio(1);
			//printf("test_led1\n");
		}
		else
		{
			set_serial_in_gpio(0);
			//printf("test_led2\n");

		}
		set_clock_gpio(0);
		set_clock_gpio(1);

	}
	set_latch_gpio(1);
	set_enable_gpio(0);
}



void init_led_status()
{
	control_led(PW_RUN_LED,LED_OFF);
	control_led(PW_LAN_LED,LED_OFF);
	control_led(PW_CAN_LED,LED_OFF);
	control_led(PW_USER1_LED,LED_OFF);
	control_led(PW_USER2_LED,LED_OFF);
	control_led(PW_USER3_LED,LED_OFF);
	control_led(PW_ERR_LED,LED_OFF);

}



void control_led(enum LED_TYPE led_type,enum LED_STATU led_status)
{

	switch(led_type)
	{
	case PW_RUN_LED:
		contrl_led_value[PW_RUN_LED] = led_status;
		break;

	case PW_LAN_LED:
		contrl_led_value[PW_LAN_LED] = led_status;
		break;

	case PW_CAN_LED:
		contrl_led_value[PW_CAN_LED] = led_status;
		break;

	case PW_USER1_LED:
		contrl_led_value[PW_USER1_LED] = led_status;
		break;

	case PW_USER2_LED:
		contrl_led_value[PW_USER2_LED] = led_status;
		break;

	case PW_USER3_LED:
		contrl_led_value[PW_USER3_LED] = led_status;
		break;

	case PW_ERR_LED:
		contrl_led_value[PW_ERR_LED] = led_status;
		break;

	default:
		break;
	}

	//show_led(contrl_led_value);
}

void show_led_ctrl(void)
{
	show_led(contrl_led_value);
}

void test_led()
{
	uint8_t cnt_i = 0;

	memset(contrl_led_value,1,16);

	for(cnt_i = 0;cnt_i<16;cnt_i++)
	{
		control_led(PW_RUN_LED,LED_ON);
		sleep(1);
		control_led(PW_CAN_LED,LED_ON);
		sleep(1);
		control_led(PW_LAN_LED,LED_ON);
		sleep(1);
		control_led(PW_USER1_LED,LED_ON);
		sleep(1);
		control_led(PW_USER2_LED,LED_ON);
		sleep(1);
		control_led(PW_USER3_LED,LED_ON);
		sleep(1);
		control_led(PW_ERR_LED,LED_ON);
		sleep(1);

		control_led(PW_ERR_LED,LED_OFF);
		sleep(1);
		control_led(PW_USER3_LED,LED_OFF);
		sleep(1);
		control_led(PW_USER2_LED,LED_OFF);
		sleep(1);
		control_led(PW_USER1_LED,LED_OFF);
		sleep(1);
		control_led(PW_LAN_LED,LED_OFF);
		sleep(1);
		control_led(PW_CAN_LED,LED_OFF);
		sleep(1);
		control_led(PW_RUN_LED,LED_OFF);
		sleep(1);
		if(cnt_i == 15)
			cnt_i = 0;
	}
}

int gpio_write(char* path,uint8_t value)
{
	int fd = -1;
	int ret = -1;
	char val;
	if(value == 1)
		val = '1';
	else
		val = '0';

	fd = open(path,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
	if(fd == -1)
	{
		DEBUG("open latch_gpio_failed\n");
		return -1;
	}

	ret = write(fd,&val,sizeof(val));
	if(ret == -1)
	{
		DEBUG("read latch_gpio_failed\n");
		return -1;
	}
	close(fd);
	return 0;
//	DEBUG("read_cnt:%d,read_buf:%c\n",read_cnt,readbuf[0]);

}


void led_sys(uint8_t flag)
{
	uint8_t value;
	value=flag;
	if(gpio_write(LED1_GPIO_PIN_VAL,value)==-1)
	{
		DEBUG("gpio_write err--led_sys\n");
	}
}

void led_net(uint8_t flag)
{
	uint8_t value;
	value=flag;
	if(gpio_write(LED2_GPIO_PIN_VAL,value)==-1)
	{
		DEBUG("gpio_write err--led_net\n");
	}
}

void led_psw(uint8_t flag)
{
	uint8_t value;
	value=flag;
	if(gpio_write(LED3_GPIO_PIN_VAL,value)==-1)
	{
		DEBUG("gpio_write err--led_psw\n");
	}
}

