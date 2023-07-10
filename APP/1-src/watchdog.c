#include <stdio.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <asm/types.h>
#include <linux/watchdog.h>
#include <sys/stat.h>
#include <signal.h>
#include "watchdog.h"
#include "user_data.h"

#define HW_WATCHDOG_GPIO_EXPORT           "/sys/class/gpio/export"
#define HW_WATCHDOG_GPIO_RST_PIN_VAL      "23"
#define HW_WATCHDOG_GPIO_RST_DIR          "/sys/class/gpio/gpio23/direction"
#define HW_WATCHDOG_GPIO_RST_OUT     	  "out"
#define HW_WATCHDOG_GPIO_RST_VAL          "/sys/class/gpio/gpio23/value"
#define HW_WATCHDOG_GPIO_RST_VAL_H        "1"
#define HW_WATCHDOG_GPIO_RST_VAL_L        "0"

enum HW_WATCHDOG_TYPE
{
	RESET_HW_WATCHDOG_GPIO,
	SET_HW_WATCHDOG_GPIO,
};
void ctrl_hw_watchdog(uint8_t ctrl_type);

int watchdog_fd = -1;


/*************************************************
Function:  init_hw_watchdog
Description:  初始化硬件看门狗
Input: 　无
Output: 无
Return: NULL
Others:
*************************************************/
void init_hw_watchdog(void)
{
	printf("init_hw_watchdog start\n");
	int fd = -1;
	fd = open(HW_WATCHDOG_GPIO_EXPORT,O_WRONLY);
	if(fd<0)
	{
		printf("GPIO export error\r\n");
		return;
	}
	write(fd,HW_WATCHDOG_GPIO_RST_PIN_VAL,sizeof(HW_WATCHDOG_GPIO_RST_PIN_VAL));
	close(fd);

	fd = open(HW_WATCHDOG_GPIO_RST_DIR,O_WRONLY);
	if(fd<0)
	{
		printf("GPIO export error\r\n");
		return;
	}
	write(fd,HW_WATCHDOG_GPIO_RST_OUT,sizeof(HW_WATCHDOG_GPIO_RST_OUT));
	close(fd);
	ctrl_hw_watchdog(SET_HW_WATCHDOG_GPIO);

}

/*************************************************
Function:  ctrl_hw_watchdog
Description:  初始化硬件看门狗
Input: 　置高或者置低
Output: 无
Return: NULL
Others:
*************************************************/
void ctrl_hw_watchdog(uint8_t ctrl_type)
{
	int fd = -1;

	fd = open(HW_WATCHDOG_GPIO_RST_VAL,O_WRONLY);
	if(fd<0)
	{
		printf("GPIO export error\r\n");
		return;
	}
	if(ctrl_type==SET_HW_WATCHDOG_GPIO)
	{
		write(fd,HW_WATCHDOG_GPIO_RST_VAL_H,sizeof(HW_WATCHDOG_GPIO_RST_VAL_H));
	}
	else if(ctrl_type==RESET_HW_WATCHDOG_GPIO)
	{
		write(fd,HW_WATCHDOG_GPIO_RST_VAL_L,sizeof(HW_WATCHDOG_GPIO_RST_VAL_L));
	}
	close(fd);
}

/*************************************************
Function:  init_watchdog
Description:  初始化看门狗配置
Input: 　看门狗溢出时间
Output: 成功０，失败１
Return: 成功０，失败１
Others:
*************************************************/
int init_watchdog(int over_time)
{
	int tmp_data = over_time;
	int ret_val;
	struct watchdog_info info_t;

	watchdog_fd = open("/dev/watchdog", O_WRONLY); //打开看门狗设备
	if (watchdog_fd == -1)
	{
		perror("watchdog");
		return 1;
	}

	/********设置看门狗溢出时间**************/
	ret_val = ioctl(watchdog_fd, WDIOC_SETTIMEOUT, &tmp_data);
	if (ret_val)
	{
		DEBUG("\nWatchdog Timer : WDIOC_SETTIMEOUT failed");
	}
	else
	{
		DEBUG("\nNew timeout value is : %d seconds", tmp_data);
	}

	/********获取看门狗当前设置的溢出时间**************/
	ret_val = ioctl(watchdog_fd, WDIOC_GETTIMEOUT, &tmp_data);
	if (ret_val)
	{
		DEBUG("\nWatchdog Timer : WDIOC_GETTIMEOUT failed");
	}
	else
	{
		DEBUG("\nCurrent timeout value is : %d seconds\n", tmp_data);
	}



	ret_val = ioctl(watchdog_fd, WDIOC_GETSUPPORT, &info_t);
	printf("$$$$$$$$$$$$$ %x!!!!!$$$$$$$$$$$$$$$$$\n",info_t.options);
	if(info_t.options&WDIOF_CARDRESET)
	{
		printf("$$$$$$$$$$$$$ WatchDog Reset!!!!!$$$$$$$$$$$$$$$$$\n");
	}

	return 0;

}



//err_data_num
/*************************************************
Function:  feed_dog
Description:  看门狗喂狗程序
Input: 　无
Output: 成功０，失败１
Return: 成功０，失败１
Others:
*************************************************/
int feed_dog(void)
{
	//向看门狗设备写入任意值来喂狗
	if (1 != write(watchdog_fd, "\0", 1)) {
		DEBUG("feed dog failed\n");
		return 1;
	}
	else
	{
		//DEBUG("feed dog succeed\n");
	}
	return 0;
}

