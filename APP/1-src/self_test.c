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
#include <signal.h>
#include <sys/time.h>
#include <linux/can.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/if.h>
#include <linux/can/raw.h>
#include <semaphore.h>
#include <errno.h>
#include "self_test.h"
#include "lh_math.h"
#include "user_data.h"
#include "pw_diagnos.h"
#include "watchdog.h"
#include "hdd_save.h"
#include "ad7606.h"
#include "sw_diagnos.h"

#if defined(WTD_DATA_TEST)||defined(SELF_TEST_NUM_TWO)
	#define SELF_TEST_NUM 2;				//传感器自检次数
#else
#define SELF_TEST_NUM    20 // 20
#endif

#define SENSOR_REAL_ERR_CNT 5
/*
 * 传感器有效电压1~5V，对应传感器有效电流信号4~20mA
 * 传感器电压0~5V，对应传感器有效电流信号0~20mA
 */
#define vol_to_electric(x)  4*x;

#define SENSOR_GPIO_EXPORT       "/sys/class/gpio/export"
#define MCU_TEST1_PIN_NUM        "72"     //pw
#define MCU_TEST2_PIN_NUM		 "73"

#define SWMCU_TEST1_PIN_NUM        "19"     //Sw
#define SWMCU_TEST2_PIN_NUM		   "61"

#define MCU_TEST1_GPIO_PIN_DIR	     "/sys/class/gpio/gpio72/direction"
#define MCU_TEST2_GPIO_PIN_DIR	     "/sys/class/gpio/gpio73/direction"

#define SWMCU_TEST1_GPIO_PIN_DIR	 "/sys/class/gpio/gpio19/direction"
#define SWMCU_TEST2_GPIO_PIN_DIR	 "/sys/class/gpio/gpio61/direction"

#define MCU_TEST1_GPIO_PIN_VAL	     "/sys/class/gpio/gpio72/value"
#define MCU_TEST2_GPIO_PIN_VAL	     "/sys/class/gpio/gpio73/value"

#define SWMCU_TEST1_GPIO_PIN_VAL	 "/sys/class/gpio/gpio19/value"
#define SWMCU_TEST2_GPIO_PIN_VAL	 "/sys/class/gpio/gpio61/value"

#define SENSOR_GPIO_DIR_OUT_VAL      "out"
#define SENSOR_GPIO_DIR_IN_VAL       "in"
#define SENSOR_GPIO_RST_VAL_H        '1'
#define SENSOR_GPIO_RST_VAL_L        '0'

//#define POWER_BRANCH1_GPIO_X 		"/sys/class/gpio/gpio117"
//#define POWER_BRANCH1_GPIO_EXPORT 	"echo 117 > /sys/class/gpio/export"
//#define POWER_BRANCH1_GPIO_INPUT	"echo in > /sys/class/gpio/gpio117/direction"
//#define POWER_BRANCH1_GPIO_PIN_VAL	"/sys/class/gpio/gpio117/value"
//
//#define POWER_BRANCH2_GPIO_X 		"/sys/class/gpio/gpio7"
//#define POWER_BRANCH2_GPIO_EXPORT 	"echo 7 > /sys/class/gpio/export"
//#define POWER_BRANCH2_GPIO_INPUT	"echo in > /sys/class/gpio/gpio7/direction"
//#define POWER_BRANCH2_GPIO_PIN_VAL	"/sys/class/gpio/gpio7/value"
#define POWER_BRANCH_GPIO_EXPORT  		"/sys/class/gpio/export"
#define POWER_BRANCH1_GPIO_OUT		 	"/sys/class/gpio113"
#define POWER_BRANCH2_GPIO_OUT		  	"/sys/class/gpio115"
#define POWER_12BRANCH1_GPIO_OUT		"/sys/class/gpio117"
#define POWER_12BRANCH2_GPIO_OUT		"/sys/class/gpio119"
#define POWER_BRANCH1_PIN_OUT_NUM       "113"
#define POWER_BRANCH2_PIN_OUT_NUM		"115"
#define POWER_12BRANCH1_PIN_OUT_NUM     "117"
#define POWER_12BRANCH2_PIN_OUT_NUM		"119"
#define POWER_BRANCH1_GPIO_PIN_OUT_DIR	  "/sys/class/gpio/gpio113/direction"
#define POWER_BRANCH2_GPIO_PIN_OUT_DIR	  "/sys/class/gpio/gpio115/direction"
#define POWER_12BRANCH1_GPIO_PIN_OUT_DIR  "/sys/class/gpio/gpio117/direction"
#define POWER_12BRANCH2_GPIO_PIN_OUT_DIR  "/sys/class/gpio/gpio119/direction"
#define POWER_BRANCH1_GPIO_PIN_OUT_VAL	  "/sys/class/gpio/gpio113/value"
#define POWER_BRANCH2_GPIO_PIN_OUT_VAL	  "/sys/class/gpio/gpio115/value"
#define POWER_12BRANCH1_GPIO_PIN_OUT_VAL  "/sys/class/gpio/gpio117/value"
#define POWER_12BRANCH2_GPIO_PIN_OUT_VAL  "/sys/class/gpio/gpio119/value"
#define POWER_BRANCH_GPIO_DIR_OUT_VAL    "out"
#define POWER_BRANCH_GPIO_DIR_IN_VAL     "in"

struct SELF_TEST_PARA self_test_para;
struct SELF_TEST_PARA self_test_para;
struct sensor_fault_pw sensor_fault_cnt;
sem_t self_test_sem;
extern sem_t self_test_sw_sem;
//extern struct PW_TZ_DATA pw_tz_data;
extern struct SYS_STATUS_CNT sys_status_cnt; //设备状态计数
//uint8_t pw_self_test_flag = 1;

uint8_t ad_self_test_end_flag = 0;
extern uint8_t sw_data_save_type;



uint8_t pw_self_test_flag = 1;
uint8_t sw_self_test_flag = 1;
void init_self_test_para() {
	self_test_para.ad_data_status = 0;
	self_test_para.self_test_flag = 1;
	self_test_para.self_test_num = SELF_TEST_NUM;
	self_test_para.sensor_mode = 1;					//初始化为自检状态

}

void init_sensor_gpio(void) {
	int fd1 = -1;
	int fd2 = -1;
	int w_res = -1;
	errno = 0;
	fd1 = open(SENSOR_GPIO_EXPORT, O_WRONLY);
	if (fd1 == -1) {
		DEBUG("OPEN SENSOR_GPIO_EXPORT FAILED\n");
		DEBUG("Error 11no.%d: %s\n", errno, strerror(errno));
	}

	w_res = write(fd1, &MCU_TEST1_PIN_NUM, 2);				//MCU_TEST1
	if (w_res == -1) {
		DEBUG("WRITE SENSOR_GPIO_EXPORT FAILED\n");
		DEBUG("Error 22no.%d: %s\n", errno, strerror(errno));
	}

	w_res = -1;
	w_res = write(fd1, &MCU_TEST2_PIN_NUM, 2);				//MCU_TEST2
	if (w_res == -1) {
		DEBUG("WRITE SENSOR_GPIO_EXPORT FAILED\n");
		DEBUG("Error 33no.%d: %s\n", errno, strerror(errno));
	}

	w_res = write(fd1, &SWMCU_TEST1_PIN_NUM, 2);				//MCU_TEST1
	if (w_res == -1) {
		DEBUG("WRITE SENSOR_GPIO_EXPORT FAILED\n");
		DEBUG("Error 2222no.%d: %s\n", errno, strerror(errno));
	}

	w_res = -1;
	w_res = write(fd1, &SWMCU_TEST2_PIN_NUM, 2);				//MCU_TEST2
	if (w_res == -1) {
		DEBUG("WRITE SENSOR_GPIO_EXPORT FAILED\n");
		DEBUG("Error 3333no.%d: %s\n", errno, strerror(errno));
	}

	close(fd1);

	fd2 = -1;
	w_res = -1;
	fd2 = open(MCU_TEST1_GPIO_PIN_DIR, O_WRONLY);
	if (fd2 == -1) {
		DEBUG("OPEN MCU_TEST1_GPIO_PIN_DIR FAILED\n");
		DEBUG("Error 44no.%d: %s\n", errno, strerror(errno));
	}
	w_res = write(fd2, &SENSOR_GPIO_DIR_OUT_VAL, 3);
	if (w_res == -1) {
		DEBUG("WRITE SENSOR_GPIO_DIR_OUT_VAL FAILED\n");
		DEBUG("Error 55no.%d: %s\n", errno, strerror(errno));
	}
	close(fd2);

	fd2 = -1;
	w_res = -1;
	fd2 = open(MCU_TEST2_GPIO_PIN_DIR, O_WRONLY);
	if (fd2 == -1) {
		DEBUG("OPEN MCU_TEST2_GPIO_PIN_DIR FAILED\n");
		DEBUG("Error 66no.%d: %s\n", errno, strerror(errno));
	}
	w_res = write(fd2, &SENSOR_GPIO_DIR_OUT_VAL, 3);
	if (w_res == -1) {
		DEBUG("WRITE SENSOR_GPIO_DIR_OUT_VAL FAILED\n");
		DEBUG("Error 77no.%d: %s\n", errno, strerror(errno));
	}

	fd2 = -1;
	w_res = -1;
	fd2 = open(SWMCU_TEST1_GPIO_PIN_DIR, O_WRONLY);
	if (fd2 == -1) {
		DEBUG("OPEN MCU_TEST2_GPIO_PIN_DIR FAILED\n");
		DEBUG("Error 66no.%d: %s\n", errno, strerror(errno));
	}
	w_res = write(fd2, &SENSOR_GPIO_DIR_OUT_VAL, 3);
	if (w_res == -1) {
		DEBUG("WRITE SENSOR_GPIO_DIR_OUT_VAL FAILED\n");
		DEBUG("Error 77no.%d: %s\n", errno, strerror(errno));
	}

	fd2 = -1;
	w_res = -1;
	fd2 = open(SWMCU_TEST2_GPIO_PIN_DIR, O_WRONLY);
	if (fd2 == -1) {
		DEBUG("OPEN MCU_TEST2_GPIO_PIN_DIR FAILED\n");
		DEBUG("Error 66no.%d: %s\n", errno, strerror(errno));
	}
	w_res = write(fd2, &SENSOR_GPIO_DIR_OUT_VAL, 3);
	if (w_res == -1) {
		DEBUG("WRITE SENSOR_GPIO_DIR_OUT_VAL FAILED\n");
		DEBUG("Error 77no.%d: %s\n", errno, strerror(errno));
	}
	close(fd2);
}

void control_sensor1_gpio_test(char control_value) {
	int fd = -1;
	int w_res = -1;

	DEBUG("PW1:%c\n", control_value);
	fd = open(MCU_TEST1_GPIO_PIN_VAL, O_RDWR);
	if (fd == -1) {
		DEBUG("OPEN MCU_TEST1_GPIO_PIN_VAL FAILED\n");
		DEBUG("Error 88no.%d: %s\n", errno, strerror(errno));
	}
	w_res = write(fd, &control_value, 1);	//control_value
	if (w_res == -1) {
		DEBUG("WRITE MCU_TEST1_GPIO_PIN_VAL FAILED\n");
		DEBUG("Error 99no.%d: %s\n", errno, strerror(errno));
	}
	close(fd);
}

void control_sensor2_gpio_test(char control_value) {
	int fd = -1;
	int w_res = -1;
	DEBUG("PW2:%c\n", control_value);
	fd = open(MCU_TEST2_GPIO_PIN_VAL, O_RDWR);
	if (fd == -1) {
		DEBUG("OPEN MCU_TEST2_GPIO_PIN_VAL FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
	w_res = write(fd, &control_value, 1);
	if (w_res == -1) {
		DEBUG("WRITE MCU_TEST2_GPIO_PIN_VAL FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
	close(fd);
}

void control_swsensor1_gpio_test(char control_value) {
	int fd = -1;
	int w_res = -1;

	DEBUG("SW1:%c\n", control_value);
	fd = open(SWMCU_TEST1_GPIO_PIN_VAL, O_RDWR);
	if (fd == -1) {
		DEBUG("OPEN SW MCU_TEST1_GPIO_PIN_VAL FAILED\n");
		DEBUG("Error 88no.%d: %s\n", errno, strerror(errno));
	}
	w_res = write(fd, &control_value, 1);	//control_value
	if (w_res == -1) {
		DEBUG("WRITE SW MCU_TEST1_GPIO_PIN_VAL FAILED\n");
		DEBUG("Error 99no.%d: %s\n", errno, strerror(errno));
	}
	close(fd);
}

void control_swsensor2_gpio_test(char control_value) {
	int fd = -1;
	int w_res = -1;
	DEBUG("SW2:%c\n", control_value);
	fd = open(SWMCU_TEST2_GPIO_PIN_VAL, O_RDWR);
	if (fd == -1) {
		DEBUG("OPEN MCU_TEST2_GPIO_PIN_VAL FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
	w_res = write(fd, &control_value, 1);
	if (w_res == -1) {
		DEBUG("WRITE SWMCU_TEST2_GPIO_PIN_VAL FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
	close(fd);
}

void open_power_out_12branch1() {
	int fd1 = -1;
	int fd2 = -1;
	int w_res = -1;

//	printf("<-------------1------------->\n");
	if (access(POWER_12BRANCH1_GPIO_OUT, F_OK) != -1)					//路径已经存在
			{
		//write
		fd2 = open(POWER_12BRANCH1_GPIO_PIN_OUT_DIR, O_WRONLY);
		DEBUG("POWER_12BRANCH2_GPIO_OUT access\n");

		if (fd2 != -1) {
			w_res = write(fd2, &POWER_BRANCH_GPIO_DIR_IN_VAL, 2);
		}
		return;
	}
//	printf("<-------------2------------->\n");
	fd1 = open(POWER_BRANCH_GPIO_EXPORT, O_WRONLY);
	if (fd1 == -1) {
		DEBUG("OPEN POWER_BRANCH_GPIO_EXPORT FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
//	printf("<-------------3------------->\n");
	w_res = write(fd1, &POWER_12BRANCH1_PIN_OUT_NUM, 3);			//MCU_TEST1_IN
	if (w_res == -1) {
		DEBUG("WRITE POWER_12BRANCH2_PIN_OUT_NUM FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
	close(fd1);
//	printf("<-------------4------------->\n");
	fd2 = open(POWER_12BRANCH1_GPIO_PIN_OUT_DIR, O_WRONLY);
	if (fd2 != -1) {
		printf("POWER_12BRANCH2_GPIO_PIN_OUT_DIR open successed\n");
		w_res = write(fd2, &POWER_BRANCH_GPIO_DIR_IN_VAL, 2);
	}
	close(fd2);
}

void open_power_out_branch1() {
	int fd1 = -1;
	int fd2 = -1;
	int w_res = -1;

//	printf("<-------------1------------->\n");
	if (access(POWER_BRANCH1_GPIO_OUT, F_OK) != -1)					//路径已经存在
			{
		//write
		fd2 = open(POWER_BRANCH1_GPIO_PIN_OUT_DIR, O_WRONLY);
		DEBUG("POWER_BRANCH1_GPIO access\n");

		if (fd2 != -1) {
			w_res = write(fd2, &POWER_BRANCH_GPIO_DIR_IN_VAL, 2);
		}
		return;
	}
//	printf("<-------------2------------->\n");
	fd1 = open(POWER_BRANCH_GPIO_EXPORT, O_WRONLY);
	if (fd1 == -1) {
		DEBUG("OPEN POWER_BRANCH_GPIO_EXPORT FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
//	printf("<-------------3------------->\n");
	w_res = write(fd1, &POWER_BRANCH1_PIN_OUT_NUM, 3);			//MCU_TEST1_OUT
	if (w_res == -1) {
		DEBUG("WRITE POWER_BRANCH1_PIN_OUT_NUM FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
	close(fd1);
//	printf("<-------------4------------->\n");
	fd2 = open(POWER_BRANCH1_GPIO_PIN_OUT_DIR, O_WRONLY);
	if (fd2 != -1) {
		printf("POWER_BRANCH1_GPIO_PIN_OUT_DIR open successed\n");
		w_res = write(fd2, &POWER_BRANCH_GPIO_DIR_IN_VAL, 2);
	}
	close(fd2);
}

void open_power_out_12branch2() {
	int fd1 = -1;
	int fd2 = -1;
	int w_res = -1;

//	printf("<-------------5------------->\n");
	//如果设备文件存在,就只需要配置方向属性
	if (access(POWER_12BRANCH2_GPIO_OUT, F_OK) != -1)		//路径已经存在
			{
		//write
		fd2 = open(POWER_12BRANCH2_GPIO_PIN_OUT_DIR, O_WRONLY);

		if (fd2 != -1) {
			w_res = write(fd2, &POWER_BRANCH_GPIO_DIR_IN_VAL, 2);
		}
		close(fd2);

		return;
	}

//	printf("<-------------6------------->\n");
	//设备文件不存在,就创建
	fd1 = open(POWER_BRANCH_GPIO_EXPORT, O_WRONLY);
	if (fd1 == -1) {
		DEBUG("OPEN POWER_BRANCH_GPIO_EXPORT FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
//	printf("<-------------7------------->\n");
	w_res = write(fd1, &POWER_12BRANCH2_PIN_OUT_NUM, 1);				//MCU_TEST1
	if (w_res == -1) {
		DEBUG("WRITE POWER_BRANCH_GPIO_EXPORT FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
	close(fd1);

//	printf("<-------------8------------->\n");
	fd2 = open(POWER_12BRANCH2_GPIO_PIN_OUT_DIR, O_WRONLY);
	if (fd2 != -1) {
		printf("POWER_BRANCH2_GPIO_PIN_DIR open successed\n");
		w_res = write(fd2, &POWER_BRANCH_GPIO_DIR_IN_VAL, 2);
	}
	close(fd2);
}

void open_power_out_branch2() {
	int fd1 = -1;
	int fd2 = -1;
	int w_res = -1;

//	printf("<-------------5------------->\n");
	//如果设备文件存在,就只需要配置方向属性
	if (access(POWER_BRANCH2_GPIO_OUT, F_OK) != -1)		//路径已经存在
			{
		//write
		fd2 = open(POWER_BRANCH2_GPIO_PIN_OUT_DIR, O_WRONLY);

		if (fd2 != -1) {
			w_res = write(fd2, &POWER_BRANCH_GPIO_DIR_IN_VAL, 2);
		}
		close(fd2);

		return;
	}

//	printf("<-------------6------------->\n");
	//设备文件不存在,就创建
	fd1 = open(POWER_BRANCH_GPIO_EXPORT, O_WRONLY);
	if (fd1 == -1) {
		DEBUG("OPEN POWER_BRANCH_GPIO_EXPORT FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
//	printf("<-------------7------------->\n");
	w_res = write(fd1, &POWER_BRANCH2_PIN_OUT_NUM, 3);				//MCU_TEST1
	if (w_res == -1) {
		DEBUG("WRITE POWER_BRANCH_GPIO_EXPORT FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
	close(fd1);

//	printf("<-------------8------------->\n");
	fd2 = open(POWER_BRANCH2_GPIO_PIN_OUT_DIR, O_WRONLY);
	if (fd2 != -1) {
		printf("POWER_BRANCH2_GPIO_PIN_DIR open successed\n");
		w_res = write(fd2, &POWER_BRANCH_GPIO_DIR_IN_VAL, 2);
	}
	close(fd2);
}

#ifdef AD_GPIO_ERR_REC
void open_power_ad1()
{

	int fd1 = -1;
	int w_res = -1;

//	printf("<-------------5------------->\n");
	//如果设备文件存在,就只需要配置方向属性
	if(access(POWER_AD_GPIO_1,F_OK)!=-1)		//路径已经存在
	{
		return;
	}

//	printf("<-------------6------------->\n");
	//设备文件不存在,就创建
	fd1 = open(POWER_BRANCH_GPIO_EXPORT,O_WRONLY);
	if(fd1 == -1)
	{
		DEBUG("OPEN POWER_BRANCH_GPIO_EXPORT FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
//	printf("<-------------7------------->\n");
	w_res = write(fd1,&POWER_AD_GPIO_1_NUM,2);				//MCU_TEST1
	if(w_res == -1)
	{
		DEBUG("WRITE POWER_BRANCH_GPIO_EXPORT FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
	close(fd1);
}

void open_power_ad2()
{

	int fd1 = -1;
	int w_res = -1;

//	printf("<-------------5------------->\n");
	//如果设备文件存在,就只需要配置方向属性
	if(access(POWER_AD_GPIO_2,F_OK)!=-1)		//路径已经存在
	{
		return;
	}

//	printf("<-------------6------------->\n");
	//设备文件不存在,就创建
	fd1 = open(POWER_BRANCH_GPIO_EXPORT,O_WRONLY);
	if(fd1 == -1)
	{
		DEBUG("OPEN POWER_BRANCH_GPIO_EXPORT FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
//	printf("<-------------7------------->\n");
	w_res = write(fd1,&POWER_AD_GPIO_2_NUM,2);				//MCU_TEST1
	if(w_res == -1)
	{
		DEBUG("WRITE POWER_BRANCH_GPIO_EXPORT FAILED\n");
		DEBUG("Error no.%d: %s\n", errno, strerror(errno));
	}
	close(fd1);
}
#endif

/*************************************************
 Function:    check_power_branch1()
 Description: input power err1 check
 Input:
 Output:
 Return:
 Others:
 *************************************************/
void check_power_branch1() {
	int fd_in = -1;
	int fd_out = -1;
//	int r_size = -1;
	uint8_t read_buf_in[1] = { 0 };
	uint8_t read_buf_out[1] = { 0 };
	fd_in = open(POWER_BRANCH1_GPIO_PIN_OUT_VAL, O_RDONLY);
	if (fd_in == -1) {
		DEBUG("OPEN POWER_12BRANCH1_GPIO_PIN_OUT_VAL ERR\n");
		close(fd_in);
		return;
	}
//	r_size =
	read(fd_in, read_buf_in, sizeof(read_buf_in));

//	r_size = -1;
	fd_out = open(POWER_BRANCH2_GPIO_PIN_OUT_VAL, O_RDONLY);
	if (fd_out == -1) {
		DEBUG("OPEN POWER_BRANCH1_GPIO_PIN_VAL ERR\n");
		close(fd_out);
		return;
	}
//	r_size =
	read(fd_out, read_buf_out, sizeof(read_buf_out));
	//	printf("POWER_BRANCH1:%c\n",read_buf[0]);
	if ((read_buf_in[0] == '1' && read_buf_out[0] == '1')
			|| (read_buf_in[0] == '0' && read_buf_out[0] == '0')) {
		pw_board_st.bits.power1_err = 0;
		if (sys_status_cnt.power_branch1_normal_save_flag == 0) {
//			sprintf(log_status,"ok");
//			sprintf(log_kind,"power branch1");
//			sprintf(log_detail,"power branch1 ok");
//			write_log(log_status,log_kind,log_detail,log_file_name);
			sys_status_cnt.power_branch1_normal_save_flag = 1;	//电源支路１正常可存储标志
			sys_status_cnt.power_branch1_err_save_flag = 0;
		}
		//sw_tz_data.borad_err.bits.power1_err = 0;
	} else if (read_buf_in[0] == '1' && read_buf_out[0] == '0') {
		pw_board_st.bits.power1_err = 1;
		if (sys_status_cnt.power_branch1_err_save_flag == 0) {
//			sprintf(log_status,"ok");
//			sprintf(log_kind,"power branch1");
//			sprintf(log_detail,"power branch1 ok");
//			write_log(log_status,log_kind,log_detail,log_file_name);
			sys_status_cnt.power_branch1_normal_save_flag = 0;
			sys_status_cnt.power_branch1_err_save_flag = 1;
		}

	}
	close(fd_in);
	close(fd_out);
}

/*************************************************
 Function:    check_power_branch2()
 Description: input power err2 check
 Input:
 Output:
 Return:
 Others:
 *************************************************/
void check_power_branch2() {
	int fd_in = -1;
	int fd_out = -1;
//	int r_size = -1;
	uint8_t read_buf_in[1] = { 0 };
	uint8_t read_buf_out[1] = { 0 };
	fd_in = open(POWER_12BRANCH1_GPIO_PIN_OUT_VAL, O_RDONLY);
	if (fd_in == -1) {
		printf("OPEN POWER_BRANCH2_GPIO_PIN_VAL ERR\n");
		close(fd_in);
		return;
	}
	//r_size =
	read(fd_in, read_buf_in, sizeof(read_buf_in));

//	r_size = -1;
	fd_out = open(POWER_12BRANCH2_GPIO_PIN_OUT_VAL, O_RDONLY);
	if (fd_out == -1) {
		printf("OPEN POWER_BRANCH2_GPIO_PIN_OUT_VAL ERR\n");
		close(fd_out);
		return;
	}
//	r_size =
	read(fd_out, read_buf_out, sizeof(read_buf_out));
	//	printf("POWER_BRANCH2:%c\n",read_buf[0]);
	if ((read_buf_in[0] == '1' && read_buf_out[0] == '1')
			|| (read_buf_in[0] == '0' && read_buf_out[0] == '0')) {
		pw_board_st.bits.power2_err = 0;
		if (sys_status_cnt.power_branch2_normal_save_flag == 0) {
//			sprintf(log_status,"ok");
//			sprintf(log_kind,"power branch1");
//			sprintf(log_detail,"power branch1 ok");
//			write_log(log_status,log_kind,log_detail,log_file_name);
			sys_status_cnt.power_branch2_normal_save_flag = 1;
			sys_status_cnt.power_branch2_err_save_flag = 0;
		}

	} else if (read_buf_in[0] == '1' && read_buf_out[0] == '0') {
		pw_board_st.bits.power2_err = 1;
		if (sys_status_cnt.power_branch2_err_save_flag == 0) {
//			sprintf(log_status,"ok");
//			sprintf(log_kind,"power branch1");
//			sprintf(log_detail,"power branch1 ok");
//			write_log(log_status,log_kind,log_detail,log_file_name);
			sys_status_cnt.power_branch2_normal_save_flag = 0;
			sys_status_cnt.power_branch2_err_save_flag = 1;
		}
	}
	close(fd_in);
	close(fd_out);
}

void read_power_stuatus(void) {
//	check_power_branch1();
//	check_power_branch2();



//	int fd1 = -1;
//	int fd2 = -1;
//	int ret = -1;
//	char readbuf1,readbuf2;
//	fd1 = open(POWER_BRANCH1_GPIO_PIN_VAL,O_RDWR);
//	if(fd1 == -1)
//	{
//		printf("open enable_gpio_failed\n");
//		return ;
//	}
//	ret = read(fd1,&readbuf1,1);
//	if(ret == -1)
//	{
//		printf("read enable_gpio_failed\n");
//		return ;
//	}
//	close(fd1);
//	if(readbuf1=='1')
//	{
//
//		pw_board_st.bits.power1_err=0;
//		//printf("power_v1_test ok \n");
//	}
//	else
//	{
//		pw_board_st.bits.power1_err=1;
//		//printf("power_v1_test ERR \n");
//	}
//
//	fd2 = open(POWER_BRANCH2_GPIO_PIN_VAL,O_RDWR);
//	if(fd2 == -1)
//	{
//		printf("open enable_gpio_failed\n");
//		return ;
//	}
//	ret = read(fd2,&readbuf2,1);
//	if(ret == -1)
//	{
//		printf("read enable_gpio_failed\n");
//		return ;
//	}
//	close(fd2);
//	if(readbuf2=='1')
//	{
//		//printf("power_v2_test ok \n");
//		pw_board_st.bits.power2_err=0;
//	}
//	else
//	{
//		//printf("power_v2_test ERR \n");
//		pw_board_st.bits.power2_err=1;
//	}

}

#ifdef AD_GPIO_ERR_REC
void check_power_ad()
{
	int fd1 = -1;
	int fd2 = -1;
	uint8_t read_buf1[1] = {0};
	uint8_t read_buf2[1] = {0};
	fd1 = open(POWER_AD_GPIO_1_PIN_VAL,O_RDONLY);
	if(fd1 == -1)
	{
		printf("OPEN POWER_AD_GPIO_1_PIN_VAL ERR\n");
		close(fd1);
		return;
	}
	read(fd1,read_buf1,sizeof(read_buf1));

	fd2 = open(POWER_AD_GPIO_2_PIN_VAL,O_RDONLY);
	if(fd2 == -1)
	{
		printf("OPEN POWER_AD_GPIO_2_PIN_VAL ERR\n");
		close(fd2);
		return;
	}
//	r_size =
	read(fd2,read_buf2,sizeof(read_buf2));

	if(read_buf1[0] == '0' || read_buf2[0] == '0')
	{
		if(sys_status_cnt.power_ad_err_save_flag == 0)
		{
			sys_status_cnt.power_ad_normal_save_flag  = 0;
			sys_status_cnt.power_ad_err_save_flag = 1;
		}
		DEBUG("ad power error \n");

	}else
	{
		if(sys_status_cnt.power_ad_normal_save_flag == 0)
		{
			sys_status_cnt.power_ad_normal_save_flag  = 1;
			sys_status_cnt.power_ad_err_save_flag = 0;
		}
		DEBUG("ad power normal \n");
	}

	close(fd1);
	close(fd2);
}
#endif

#ifdef AD_REF_VOLT_ERR_REBOOT
uint8_t check_ref_volt(uint16_t *psrc,uint32_t size)
{
	uint16_t ad_mean = 0;
	float ad_res = 0.0f;

	ad_mean = uint16_mean(psrc, size);

	ad_res = AD_CONVERT_VOLTAGE(ad_mean);
#ifdef __DEBUG__
	DEBUG("======================\n");
	system("date");
	printf(" %s %d---ad_res:%f V \n",__FUNCTION__,__LINE__,ad_res);
#endif

	if(ad_res>2.7f || ad_res < 2.3f)
	{
#ifdef AD_REBOOT_FLAG
		pw_ref_volt_err_cnt++;
#endif
		DEBUG(" %s %d pw_ref_volt_err_cnt=%d \n",__FUNCTION__,__LINE__,pw_ref_volt_err_cnt);
	}
	else
	{
		pw_ref_volt_err_cnt = 0;
		DEBUG(" %s %d pw_ref_volt_err_cnt=%d \n",__FUNCTION__,__LINE__,pw_ref_volt_err_cnt);
		return 1;
	}
#ifdef AD_REBOOT_FLAG
	if(pw_ref_volt_err_cnt > 5*CHANNEL_NUM)
	{
		DEBUG("REBOOT %s %d\n",__FUNCTION__,__LINE__);
		wirte_err_log_data(ERR,SENSOR_MEASURING_POINT_EMPTY,PW_AD_REF_VOLT_ERR_REBOOT);
		system("sync");
		system("reboot -nf");
	}
#endif
	return 0;
}
#endif

void power_gpio_init(void) {
	open_power_out_branch1();
	open_power_out_branch2();
	open_power_out_12branch1();
	open_power_out_12branch2();

#ifdef AD_GPIO_ERR_REC
	open_power_ad1();
	open_power_ad2();
#endif
//	if(access(POWER_BRANCH1_GPIO_X,F_OK)!=0)
//	{
//		system(POWER_BRANCH1_GPIO_EXPORT);
//		system(POWER_BRANCH1_GPIO_INPUT);
//	}
//
//	if(access(POWER_BRANCH2_GPIO_X,F_OK)!=0)
//	{
//		system(POWER_BRANCH2_GPIO_EXPORT);
//		system(POWER_BRANCH2_GPIO_INPUT);
//	}

//	read_power_stuatus();
}

/*************************************************
 Function:  self_test
 Description:  传感器上电自检程序
 Input:
 Output: 无
 Return: 无
 Others:
 *************************************************/

void sensor_self_test(struct PW_DIAGNOS_PARA *tzdata, uint8_t ch_num) {

	float sum_ad = 0.0f;
	float ave_vol = 0.0f;
	float electric_gap = 0.0f;
	static float self_value[6] = { 0 };
//	uint16_t cnt_i = 0;

//	sum_ad = float_sum(tzdata->data_deal_buff[ch_num].buff, tzdata->deal_num[ch_num]);
//	ave_vol = ((sum_ad/tzdata->deal_num[ch_num])-32768)/65536*20;// 电压均值

#ifdef AD7606_COMPLEMENTARY_TO_ORIGINAL
	sum_ad = self_test_date_deal(tzdata->data_deal_buff[ch_num].station_buff, tzdata->station_deal_num[ch_num]);

	ave_vol = AD_CONVERT_VOLTAGE(sum_ad/(tzdata->station_deal_num[ch_num]-200));// 电压均值
#else
	sum_ad = self_test_date_deal(tzdata->data_deal_buff[ch_num].station_buff, tzdata->station_deal_num[ch_num]);
	ave_vol = ((sum_ad/(tzdata->station_deal_num[ch_num]-200))-32768)/65536*20;// 电压均值
#endif

	DEBUG("sensor_self_test---------------ave_vol:%f V\n", ave_vol);


	//自检数据
	pw_data_save_type = SENSOR_SELF_TEST_DATA;

//	if (self_test_para.sensor_mode == 0 && self_test_para.self_test_num > 1) {
//		self_value[ch_num] = ave_vol;
//
//		/*如果每一个通道*/
//		if (ch_num == pw2_x) {
//			if (self_value[0] >= 1.0f && self_value[1] >= 1.0f
//					&& self_value[2] >= 1.0f && self_value[3] >= 1.0f
//					&& self_value[4] >= 1.0f && self_value[5] >= 1.0f) {
//				self_test_para.self_test_num = 2;
//			}
//
////			printf("self_value[0]:%f\n",self_value[0]);
////			printf("self_value[1]:%f\n",self_value[1]);
////			printf("self_value[2]:%f\n",self_value[2]);
////			printf("self_value[3]:%f\n",self_value[3]);
////			printf("self_value[4]:%f\n",self_value[4]);
////			printf("self_value[5]:%f\n",self_value[5]);
//
//			self_value[0] = 0.0f;
//			self_value[1] = 0.0f;
//			self_value[2] = 0.0f;
//			self_value[3] = 0.0f;
//			self_value[4] = 0.0f;
//			self_value[5] = 0.0f;
//
//		}
//	}

	printf("self_test_para.self_test_num:%d\n", self_test_para.self_test_num);

	if (self_test_para.self_test_num < 8)
	{
		check_ad_value(tzdata->data_deal_buff[ch_num].station_buff, tzdata->station_deal_num[ch_num]);
	}

	tzdata->electric_val_real[ch_num] = vol_to_electric(ave_vol);

	if (self_test_para.sensor_mode == 1) {

		tzdata->electric_val1[ch_num] = vol_to_electric(ave_vol);				//自检电流

		DEBUG("ave_vol:%f,electric_val1[ch_num]:%f,ch_num:%d\n", ave_vol, tzdata->electric_val1[ch_num], ch_num);
	}
	else if (self_test_para.sensor_mode == 0 && self_test_para.self_test_num == 0)
	{
		tzdata->electric_val2[ch_num] = vol_to_electric(ave_vol);			//工作电流
		DEBUG("ave_vol:%f,electric_val2[ch_num]:%f,ch_num:%d\n", ave_vol,tzdata->electric_val2[ch_num], ch_num);
	}

	if (self_test_para.sensor_mode == 0)
	{
		tzdata->sens_para[ch_num].sens_self_test.bits.open_circuit_err_power_on =  SENSOR_OK;			//SENSOR_ERR;

		if (ave_vol < 1.0)
		{
			tzdata->sens_para[ch_num].sens_self_test.bits.open_circuit_err_power_on = SENSOR_ERR;
		}

		if (tzdata->sens_para[ch_num].sens_type == PW_Y)
		{
			if (tzdata->electric_val1[ch_num] > tzdata->electric_val2[ch_num])
			{
				electric_gap = tzdata->electric_val1[ch_num] - tzdata->electric_val2[ch_num];
//				printf("Y:%f\n", electric_gap);
//				if(electric_gap >= 3.5f && electric_gap <= 5.0f)		//4车平稳传感器，偶发自检故障,扩大范围
				if (electric_gap >= 3.0f && electric_gap <= 5.5f)
				{
					tzdata->sens_para[ch_num].sens_self_test.bits.open_circuit_err_power_on = SENSOR_OK;
				}
				else
				{
					//tzdata->sens_para[ch_num].sens_self_test.bits.open_circuit_err_power_on = SENSOR_OK;
				}
			}
			else if (tzdata->electric_val1[ch_num] < tzdata->electric_val2[ch_num])
			{
				electric_gap = tzdata->electric_val2[ch_num] - tzdata->electric_val1[ch_num];
//				printf("Y:%f\n", electric_gap);
				//if(electric_gap >= 0.6f && electric_gap <= 1.5f)
				if (electric_gap >= 0.4f && electric_gap <= 2.0f)
				{
					tzdata->sens_para[ch_num].sens_self_test.bits.open_circuit_err_power_on = SENSOR_OK;
				}
				else
				{
					//tzdata->sens_para[ch_num].sens_self_test.bits.open_circuit_err_power_on = SENSOR_OK;
				}
			}
		}

		else if (tzdata->sens_para[ch_num].sens_type == PW_Z)
		{

			if (tzdata->electric_val1[ch_num] < tzdata->electric_val2[ch_num])
			{
				electric_gap = tzdata->electric_val2[ch_num] - tzdata->electric_val1[ch_num];
				//if((electric_gap >= 0.8f && electric_gap <= 1.8f) || (electric_gap >= 3.5f && electric_gap <= 5.0f))
				if ((electric_gap >= 0.4f && electric_gap <= 2.5f) || (electric_gap >= 3.0f && electric_gap <= 5.5f)) {
					tzdata->sens_para[ch_num].sens_self_test.bits.open_circuit_err_power_on = SENSOR_OK;
				}
				else
				{
					//tzdata->sens_para[ch_num].sens_self_test.bits.open_circuit_err_power_on = SENSOR_OK;
				}
			}
		}

		else if (tzdata->sens_para[ch_num].sens_type == PW_X) {
			if (tzdata->electric_val1[ch_num] > tzdata->electric_val2[ch_num]) {
				electric_gap = tzdata->electric_val1[ch_num] - tzdata->electric_val2[ch_num];
				//if((electric_gap >= 0.6f && electric_gap <= 1.2f) || (electric_gap >= 7.5f && electric_gap <= 8.5f))
				if ((electric_gap >= 0.2f && electric_gap <= 2.0f) || (electric_gap >= 7.0f && electric_gap <= 9.0f))
				{
					tzdata->sens_para[ch_num].sens_self_test.bits.open_circuit_err_power_on = SENSOR_OK;
				}
				else {
					//tzdata->sens_para[ch_num].sens_self_test.bits.open_circuit_err_power_on = SENSOR_OK;
				}
			}
		}
	}

	if (ch_num == pw2_x) {
		for (int i = 0; i < 6; i++) {
			if (tzdata->sens_para[i].sens_self_test.bits.open_circuit_err_power_on)
			{

				DEBUG("ch[*%d*]selftest SENSOR_ERR \n", i);
				pw_self_test_flag = 0;
			}
			else
			{
				pw_self_test_flag = 1;
			}
		}
		sem_post(&self_test_sem);
		DEBUG("send_self_sem\n");
	}
}

void swsensor_self_test(struct SW_DIAGNOS_PARA *tzdata, uint8_t ch_num)
{
#if 1
	float sum_ad = 0.0f;
	float ave_vol = 0.0f;
//	float electric_gap = 0.0f;

	static uint8_t self_test_cnt = 0;
	static uint8_t self_test_result[2][5] = { 0 };
	static float ad_result[2] = { 0 };	//用于存储刚上电时，先在工作状态下检测传感器

	sw_data_save_type = SENSOR_SELF_TEST_DATA;

#ifdef AD7606_COMPLEMENTARY_TO_ORIGINAL
	sum_ad = self_test_date_deal(tzdata->data_deal_buff[ch_num].buff, tzdata->deal_num[ch_num]);
	ave_vol = AD_CONVERT_VOLTAGE(sum_ad/(tzdata->deal_num[ch_num]-200));	// 电压均值
#else
	sum_ad = self_test_date_deal(tzdata->data_deal_buff[ch_num].buff, tzdata->deal_num[ch_num]);
	ave_vol = ((sum_ad/(tzdata->deal_num[ch_num]-200))-32768)/65536*10;	//电压均值
#endif

//	DEBUG("sensor_self_test---------------ave_vol:%f V\n", ave_vol);

	ad_result[ch_num] = ave_vol;

	tzdata->electric_val_real[ch_num] = ave_vol * 4;

	if (ch_num == sw2_y && self_test_para.sensor_mode == 0)
	{
		if (ad_result[0] < 1.0 || ad_result[1] < 1.0 )		//工作状态下，只要有一个传感器状态不对，都需要重新检测
		{
			ad_self_test_end_flag = 0;
		}
		else
		{
			ad_self_test_end_flag = 1;
//			self_test_para.self_test_num = 5;
		}
		//ad_result[0] = 0;
	}

	if (self_test_para.self_test_num < 8)
	{
		check_ad_value(tzdata->data_deal_buff[ch_num].buff, tzdata->deal_num[ch_num]);
	}

//	if (self_test_para.sensor_mode == 0) 、、youbing
//	{
//		tzdata->sens_para[ch_num].sens_self_test.bits.self_test_power_on = SENSOR_OK;
//	}

//	printf("ad_result:%fV,ch_num:%d,self_test_para.self_test_num:%d,ave_vol:%f\n",ad_result[ch_num],ch_num,self_test_para.self_test_num,ave_vol);
	//DEBUG("ave_vol:%fV,ch_num:%d\n",ave_vol,ch_num);

	if (self_test_para.sensor_mode == 1)					//自检状态
	{
		tzdata->electric_val1[ch_num] = ave_vol * 4;		//自检电流
	}

//	printf("ad_result[%d]:%f mA \n",ch_num,ave_vol*4);
	DEBUG("ave_vol:=%fv,electric_val1:=%fmA,ch_num:%d\n", ave_vol,tzdata->electric_val1[ch_num], ch_num);
	if (self_test_para.sensor_mode == 1)
	{
		tzdata->sens_para[ch_num].sens_self_test.bits.self_test_power_on =SENSOR_OK;
		if ((tzdata->electric_val1[ch_num] > 3.2 && tzdata->electric_val1[ch_num] < 4.8) || (tzdata->electric_val1[ch_num] > 12 && tzdata->electric_val1[ch_num] < 14))
		{
			tzdata->sens_para[ch_num].sens_self_test.bits.self_test_power_on = SENSOR_OK;
			DEBUG(
					"-------------------------->selftest SENSOR_OK<-------------------------- CH:%d,%d\n",ch_num,
					tzdata->sens_para[ch_num].sens_self_test.bits.self_test_power_on);
		}
		else
		{
			tzdata->sens_para[ch_num].sens_self_test.bits.self_test_power_on =SENSOR_ERR;
		}
		self_test_result[ch_num][self_test_cnt] = tzdata->sens_para[ch_num].sens_self_test.bits.self_test_power_on;

		DEBUG( "self_test_result[ch_num][self_test_cnt]:%d,ch_num:%d,self_test_cnt:%d\n",
				self_test_result[ch_num][self_test_cnt], ch_num, self_test_cnt);
	}

	if (ch_num == sw2_y && self_test_para.sensor_mode == 1)
	{
		for (int i = 0; i < 2; i++)
		{
			if(tzdata->sens_para[i].sens_self_test.bits.self_test_power_on)
			{
				sw_self_test_flag = 0;
			}
			else if(tzdata->sens_para[i].sens_self_test.bits.self_test_power_on == SENSOR_OK )
			{
				sw_self_test_flag = 1;
			}
		}
//		self_test_cnt++;

//		if (self_test_cnt == 5) {
//			for (int i = 0; i < 2; i++) {
//				tzdata->sens_para[i].sens_self_test.bits.self_test_power_on = self_test_result[i][0] & self_test_result[i][1]
//								& self_test_result[i][2]
//								& self_test_result[i][3]
//								& self_test_result[i][4];
//				if (tzdata->sens_para[i].sens_self_test.bits.self_test_power_on)//传感器上电自检故障
//				{
//					DEBUG("ch[*%d*]selftest SENSOR_ERR \n", i);
//				}
//			}
//		}
//		sem_post(&self_test_sem);
//		DEBUG("send_self_sem\n");
	}
#endif
	if (ch_num == sw2_y) {
		sem_post(&self_test_sw_sem);
		DEBUG("self_test_sw_sem\n");
	}

}

void set_sensor_status(uint8_t status_flag) {
	if (status_flag) {
		control_sensor1_gpio_test(SENSOR_GPIO_RST_VAL_L);//SENSOR_GPIO_RST_VAL_L
		control_sensor2_gpio_test(SENSOR_GPIO_RST_VAL_L);

		control_swsensor1_gpio_test(SENSOR_GPIO_RST_VAL_L);	//SENSOR_GPIO_RST_VAL_L
		control_swsensor2_gpio_test(SENSOR_GPIO_RST_VAL_L);
		DEBUG("sensor status is self_test\n");
	} else {
		control_sensor1_gpio_test(SENSOR_GPIO_RST_VAL_H);//SENSOR_GPIO_RST_VAL_L
		control_sensor2_gpio_test(SENSOR_GPIO_RST_VAL_H);
		control_swsensor1_gpio_test(SENSOR_GPIO_RST_VAL_H);	//SENSOR_GPIO_RST_VAL_L
		control_swsensor2_gpio_test(SENSOR_GPIO_RST_VAL_H);
//		system("echo 1 > /sys/class/gpio/gpio72/value");
//		system("echo 1 > /sys/class/gpio/gpio73/value");
		DEBUG("sensor status is working\n");
	}
}

void set_swsensor_status(uint8_t status_flag) {
	if (status_flag) {
		control_swsensor1_gpio_test(SENSOR_GPIO_RST_VAL_L);	//SENSOR_GPIO_RST_VAL_L
		control_swsensor2_gpio_test(SENSOR_GPIO_RST_VAL_L);
		DEBUG("sensor status is self_test\n");
	} else {
		control_swsensor1_gpio_test(SENSOR_GPIO_RST_VAL_H);	//SENSOR_GPIO_RST_VAL_L
		control_swsensor2_gpio_test(SENSOR_GPIO_RST_VAL_H);
//		system("echo 1 > /sys/class/gpio/gpio72/value");
//		system("echo 1 > /sys/class/gpio/gpio73/value");
		DEBUG("sensor status is working\n");
	}
}

void self_test_control() {
//	uint8_t num = 4;
//	while(num--)
//	{
#ifdef HISTORY_FEED_WATCHDOG
	feed_dog();
#endif
	set_sensor_status(0);
	sleep(5);
//	}

	while (self_test_para.self_test_num--)
//	while(1)
	{
#ifdef HISTORY_FEED_WATCHDOG
		feed_dog();
#endif
//		self_test_para.self_test_num--;

		self_test_para.ad_data_status = 0;
//		self_test_para.ad_data_status = 0;


		if (self_test_para.self_test_num % 2 )
		{
			self_test_para.sensor_mode = 1;
			//传感器模式为自检状态
		}
		else if ((self_test_para.self_test_num % 2 )== 0)
		{
			self_test_para.sensor_mode = 0;
			//传感器模式为工作状态
		}

//		if((self_test_para.self_test_num>=6) && (ad_self_test_end_flag == 0))
//		{
//			self_test_para.sensor_mode = 0;
//			control_swsensor1_gpio_test(SENSOR_GPIO_RST_VAL_H);	//SENSOR_GPIO_RST_VAL_L
//			control_swsensor2_gpio_test(SENSOR_GPIO_RST_VAL_H);
//		}
//	    else if (self_test_para.self_test_num < 6 && self_test_para.self_test_num > 0)
//	    {
//			self_test_para.sensor_mode = 1;
//			//传感器模式为自检状态
//			control_swsensor1_gpio_test(SENSOR_GPIO_RST_VAL_L);	//SENSOR_GPIO_RST_VAL_L
//			control_swsensor2_gpio_test(SENSOR_GPIO_RST_VAL_L);
//		}
//	    else
//	    {
//			self_test_para.sensor_mode = 0;
//			//传感器模式为工作状态
//			control_swsensor1_gpio_test(SENSOR_GPIO_RST_VAL_H);
//			control_swsensor2_gpio_test(SENSOR_GPIO_RST_VAL_H);
//		}


		set_sensor_status(self_test_para.sensor_mode);
		sleep(5);										//等待采集的数据稳定时间

		self_test_para.ad_data_status = 1;
//		self_test_para.ad_data_status = 1;




		sem_wait(&self_test_sem);
		sem_wait(&self_test_sw_sem);

		printf("###pw   %d\n",pw_self_test_flag);
		printf("###sw   %d\n",sw_self_test_flag);
		printf("recv_self_sem-->self_test_para.self_test_num:%d\n", self_test_para.self_test_num);

		if (pw_self_test_flag && sw_self_test_flag)
			break;

//		DEBUG("recv_self_sem-->self_test_para.self_test_num:%d\n", self_test_para.self_test_num);
	}

	self_test_para.self_test_flag = 0;
#ifdef AD7606_ERR_WATCHDOG_RESET
	extern uint8_t self_check_over_flag;
	self_check_over_flag = 1;
#endif
}

void swself_test_control() {
//	self_test_para.self_test_num = 2;
//	self_test_para.sensor_mode = 1;
//	uint8_t self_test_cnt = 0;
// for(self_test_cnt = 0;self_test_cnt < 5;self_test_cnt++)
// {
	//feed_dog();
	self_test_para.sensor_mode = 0;
	set_swsensor_status(0);			//传感器处于工作状态
//	control_sensor1_3_gpio_test(SENSOR_GPIO_RST_VAL_H);
//	control_sensor2_4_gpio_test(SENSOR_GPIO_RST_VAL_H);
	sleep(5);

	while (self_test_para.self_test_num--) {
		feed_dog();
		self_test_para.ad_data_status = 0;
		if ((self_test_para.self_test_num >= 6)
				&& (ad_self_test_end_flag == 0))			//先让传感器处于工作状态下采集AD值
				{
			self_test_para.sensor_mode = 0;
			//传感器模式为工作状态
			//SENSOR_GPIO_RST_VAL_H
			control_swsensor1_gpio_test(SENSOR_GPIO_RST_VAL_H);	//SENSOR_GPIO_RST_VAL_L
			control_swsensor2_gpio_test(SENSOR_GPIO_RST_VAL_H);
		} else if (self_test_para.self_test_num < 6
				&& self_test_para.self_test_num > 0) {
			self_test_para.sensor_mode = 1;
			//传感器模式为自检状态
			control_swsensor1_gpio_test(SENSOR_GPIO_RST_VAL_L);	//SENSOR_GPIO_RST_VAL_L
			control_swsensor2_gpio_test(SENSOR_GPIO_RST_VAL_L);

		} else {
			self_test_para.sensor_mode = 0;
			//传感器模式为工作状态
			control_swsensor1_gpio_test(SENSOR_GPIO_RST_VAL_H);
			control_swsensor2_gpio_test(SENSOR_GPIO_RST_VAL_H);
		}
		set_swsensor_status(self_test_para.sensor_mode);

		if (self_test_para.self_test_num == 0) {
			self_test_para.self_test_flag = 0;
		}
		sleep(5);										//等待采集的数据稳定时间
		self_test_para.ad_data_status = 1;

		if (self_test_para.self_test_num) {
			sem_wait(&self_test_sw_sem);
		}
		DEBUG("recv_self_sem-->self_test_para.self_test_num:%d\n",
				self_test_para.self_test_num);
	}
//	self_test_para.self_test_num = 2;
// }//self_test_control end!
	//sensor status is working
#ifdef AD7606_ERR_WATCHDOG_RESET
	extern uint8_t self_check_over_flag;
	self_check_over_flag = 1;
#endif
}

void Sensor_fault_judge_PW(float *psrc, uint32_t size, struct PW_DIAGNOS_PARA *tz_data, uint8_t ch) {
	float vol_res = 0.0f;
	float electrc_res = 0.0f;
//	float ad_vol_res = 0.0f;
//	float vol_mean = 0.0f;
	static struct sensor_fault_pw sensor_fault_cnt;
//	float pw_sensor_value[6]={0};
//	FAULT_TYPE SENSOR_ERR_PW[6] = {SENSOR_ERR_PW_1,SENSOR_ERR_PW_2,SENSOR_ERR_PW_3,SENSOR_ERR_PW_4,SENSOR_ERR_PW_5,SENSOR_ERR_PW_6};
//	pFault_Type_Cnt_t PW_SENS_ERR_Fault_Type_Cnt;
//	PW_SENS_ERR_Fault_Type_Cnt = &fault_type_cnt[SENSOR_ERR_PW[ch]];		//更新平稳传感器错误计数地址 XZ

#ifdef AD7606_COMPLEMENTARY_TO_ORIGINAL
	vol_res = float_std_rms(psrc, size);

	vol_res = AD_CONVERT_VOLTAGE(vol_res);
#else
	vol_res = float_std_rms(psrc,size);

	vol_res = (vol_res-32768)/65536.0f*10.0f;
#endif
//	vol_mean = float_mean(psrc,size);
//	DEBUG("num:%d,vol_mean1:%f\n",ch,vol_mean);
//	vol_mean = (vol_mean-32768)/65535.0f*20.0f;
	electrc_res = vol_to_electric(vol_res);

	tz_data->electric_val_real[ch] = electrc_res;

	DEBUG("Sensor_fault_judge_PW---ch:%d, vol_res:%f\n", ch, vol_res);

//	printf("Sensor_fault_judge_PW---ch:%d, vol_res:%f\n", ch, vol_res);

	switch (ch) {
	case 0:
		if (electrc_res < 2.0f) {
			sensor_fault_cnt.pw_y_cnt1++;
			if (sensor_fault_cnt.pw_y_cnt1 > SENSOR_REAL_ERR_CNT) {
#ifdef AD_REF_VOLT_ERR_REBOOT
					if(check_ref_volt(tz_data->ad_value, FS_PW))
					{
						tz_data->sens_para[0].sens_self_test.bits.open_circuit_err_real_time=1;
					}
					else
					{
#ifdef AD_RESET
						ad_reset_ioctl();
#endif
						sensor_fault_cnt.pw_y_cnt1 = 0;
						tz_data->sens_para[0].sens_self_test.bits.open_circuit_err_real_time=0;
					}
#else
				tz_data->sens_para[0].sens_self_test.bits.open_circuit_err_real_time =
						1;
#endif
//				printf("sensor1_Y_err\n");
				if (sensor_fault_cnt.pw_y_cnt1 == 15)
					sensor_fault_cnt.pw_y_cnt1 = 4;
			}
		} else {
			sensor_fault_cnt.pw_y_cnt1 = 0;
			tz_data->sens_para[0].sens_self_test.bits.open_circuit_err_real_time =
					0;
		}

		break;

	case 1:
		if (electrc_res < 2.0f) {
			sensor_fault_cnt.pw_z_cnt1++;
			if (sensor_fault_cnt.pw_z_cnt1 > SENSOR_REAL_ERR_CNT) {
#ifdef AD_REF_VOLT_ERR_REBOOT
					if(check_ref_volt(tz_data->ad_value, FS_PW))
					{
						tz_data->sens_para[1].sens_self_test.bits.open_circuit_err_real_time=1;
					}
					else
					{
#ifdef AD_RESET
						ad_reset_ioctl();
#endif
						sensor_fault_cnt.pw_z_cnt1 = 0;
						tz_data->sens_para[1].sens_self_test.bits.open_circuit_err_real_time=0;
					}
#else
				tz_data->sens_para[1].sens_self_test.bits.open_circuit_err_real_time =
						1;
#endif
				//printf("sensor1_Z_err\n");
				if (sensor_fault_cnt.pw_z_cnt1 == 15)
					sensor_fault_cnt.pw_z_cnt1 = 4;
			}
		} else {
			sensor_fault_cnt.pw_z_cnt1 = 0;
			tz_data->sens_para[1].sens_self_test.bits.open_circuit_err_real_time =
					0;
		}

		break;

	case 2:
		if (electrc_res < 2.0f) {
			sensor_fault_cnt.pw_x_cnt1++;
			if (sensor_fault_cnt.pw_x_cnt1 > SENSOR_REAL_ERR_CNT) {
#ifdef AD_REF_VOLT_ERR_REBOOT
					if(check_ref_volt(tz_data->ad_value, FS_PW))
					{
						tz_data->sens_para[2].sens_self_test.bits.open_circuit_err_real_time=1;
					}
					else
					{
#ifdef AD_RESET
						ad_reset_ioctl();
#endif
						sensor_fault_cnt.pw_x_cnt1 = 0;
						tz_data->sens_para[2].sens_self_test.bits.open_circuit_err_real_time=0;
					}
#else
				tz_data->sens_para[2].sens_self_test.bits.open_circuit_err_real_time =
						1;
#endif
				//printf("sensor1_X_err\n");
				if (sensor_fault_cnt.pw_x_cnt1 == 15)
					sensor_fault_cnt.pw_x_cnt1 = 4;
			}
		} else {
			sensor_fault_cnt.pw_x_cnt1 = 0;
			tz_data->sens_para[2].sens_self_test.bits.open_circuit_err_real_time =
					0;
		}

		break;

	case 3:
		if (electrc_res < 2.0f) {
			sensor_fault_cnt.pw_y_cnt2++;

			//rt_kprintf ("test_err_one:%d,status:%d\r\n",Sensor_fault_cnt.PW_Y_cnt1,*status);
			if (sensor_fault_cnt.pw_y_cnt2 > SENSOR_REAL_ERR_CNT) {
#ifdef AD_REF_VOLT_ERR_REBOOT
					if(check_ref_volt(tz_data->ad_value, FS_PW))
					{
						tz_data->sens_para[3].sens_self_test.bits.open_circuit_err_real_time=1;
					}
					else
					{
#ifdef AD_RESET
						ad_reset_ioctl();
#endif
						sensor_fault_cnt.pw_y_cnt2 = 0;
						tz_data->sens_para[3].sens_self_test.bits.open_circuit_err_real_time=0;
					}
#else
				tz_data->sens_para[3].sens_self_test.bits.open_circuit_err_real_time =
						1;
#endif
				//printf("sensor2_Y_err\n");
				if (sensor_fault_cnt.pw_y_cnt2 == 15)
					sensor_fault_cnt.pw_y_cnt2 = 4;
			}
		} else {
			sensor_fault_cnt.pw_y_cnt2 = 0;
			tz_data->sens_para[3].sens_self_test.bits.open_circuit_err_real_time =
					0;
		}
		break;

	case 4:
		if (electrc_res < 2.0f) {
			sensor_fault_cnt.pw_z_cnt2++;
			if (sensor_fault_cnt.pw_z_cnt2 > SENSOR_REAL_ERR_CNT) {
#ifdef AD_REF_VOLT_ERR_REBOOT
					if(check_ref_volt(tz_data->ad_value, FS_PW))
					{
						tz_data->sens_para[4].sens_self_test.bits.open_circuit_err_real_time=1;
					}
					else
					{
#ifdef AD_RESET
						ad_reset_ioctl();
#endif
						sensor_fault_cnt.pw_z_cnt2 = 0;
						tz_data->sens_para[4].sens_self_test.bits.open_circuit_err_real_time=0;
					}
#else
				tz_data->sens_para[4].sens_self_test.bits.open_circuit_err_real_time =
						1;
#endif
				//printf("sensor2_Z_err\n");
				if (sensor_fault_cnt.pw_z_cnt2 == 15)
					sensor_fault_cnt.pw_z_cnt2 = 4;
			}
		}

		else {
//				AD_ERR.pw_jkb = 0;
//				sys_err_flag.Bits .pw_jkb_falut = 0;
			sensor_fault_cnt.pw_z_cnt2 = 0;
			tz_data->sens_para[4].sens_self_test.bits.open_circuit_err_real_time =
					0;
		}

		break;

	case 5:
		if (electrc_res < 2.0f) {
			sensor_fault_cnt.pw_x_cnt2++;
			if (sensor_fault_cnt.pw_x_cnt2 > SENSOR_REAL_ERR_CNT) {
#ifdef AD_REF_VOLT_ERR_REBOOT
					if(check_ref_volt(tz_data->ad_value, FS_PW))
					{
						tz_data->sens_para[5].sens_self_test.bits.open_circuit_err_real_time=1;
					}
					else
					{
#ifdef AD_RESET
						ad_reset_ioctl();
#endif
						sensor_fault_cnt.pw_x_cnt2 = 0;
						tz_data->sens_para[5].sens_self_test.bits.open_circuit_err_real_time=0;
					}
#else
				tz_data->sens_para[5].sens_self_test.bits.open_circuit_err_real_time =
						1;
#endif

				//printf("sensor2_X_err\n");
				if (sensor_fault_cnt.pw_x_cnt2 == 15)
					sensor_fault_cnt.pw_x_cnt2 = 4;
			}
		}

		else {
//				AD_ERR.pw_jkb = 0;
//				sys_err_flag.Bits .pw_jkb_falut = 0;
			sensor_fault_cnt.pw_x_cnt2 = 0;
			tz_data->sens_para[5].sens_self_test.bits.open_circuit_err_real_time =
					0;
		}

		break;

	default:
		break;
	}

}

#ifdef AD7606_COMPLEMENTARY_TO_ORIGINAL
void judge_pw_board_err(uint16_t *psrc, uint32_t size, uint8_t *status) {
	uint16_t ad_mean = 0;
	float ad_res = 0.0f;

	ad_mean = uint16_mean(psrc, size);

	ad_res = AD_CONVERT_VOLTAGE(ad_mean);
#ifdef __DEBUG__
	DEBUG("======================\n");
	system("date");
	printf(" %s %d---ad_res:%f V status: %d\n", __FUNCTION__, __LINE__, ad_res,
			*status);
#endif

//	printf("judge_pw_board_err---ad_res:%f V, status:%d\n",ad_res, *status);
#ifdef AD_REF_VOLT_ERR_REBOOT
	if(ad_res>2.7f || ad_res < 2.3f)
#else
	if (ad_res > 2.1f || ad_res < 1.9f)
#endif
			{
		sensor_fault_cnt.pw_jkb++;
		DEBUG(" %s %d \n", __FUNCTION__, __LINE__);
	} else {
		*status = 0;
		pw_board_st.bits.sample_err = 0;
		sensor_fault_cnt.pw_jkb = 0;
		//printf("pw_jkb2\n");
	}
	if (sensor_fault_cnt.pw_jkb > 5) {
		//*status = 1;					//板卡故障
		//printf("pw_AD ERR\n");
		//pw_board_st.bits.sample_err=1;
#ifdef AD_REBOOT_FLAG
		DEBUG("REBOOT %s %d\n",__FUNCTION__,__LINE__);

			wirte_err_log_data(ERR,SENSOR_MEASURING_POINT_EMPTY,PW_AD_REF_VOLT_ERR_REBOOT);
			system("sync");
			system("reboot -nf");

#endif
	}
}

#else

void judge_pw_board_err(uint16_t *psrc,uint32_t size,uint8_t *status)
{
	uint16_t ad_vol_res = 0;
	float ad_res = 0.0f;

	ad_vol_res = uint16_mean(psrc,size);

	ad_res = ad_vol_res;
	ad_res = AD_CONVERT_ACCELERATION(ad_res);//(ad_res-32768.0)/65536.0f*10.0f;

	printf("BOARD  ad_vol_res:%f,%d,status_one:%d\n",ad_res,ad_vol_res,*status);

	if(ad_res>2.1f || ad_res < 1.9f)
	{
		sensor_fault_cnt.pw_jkb++;

	}
	else
	{
		*status = 0;
		pw_board_st.bits.sample_err=0;
		//printf("pw_jkb2\n");
	}
	if(sensor_fault_cnt.pw_jkb > 5)
	{
		//*status = 1;					//板卡故障
		//printf("pw_AD ERR\n");
		//pw_board_st.bits.sample_err=1;
	}
	//printf("BOARD  ad_vol_res:%f,%d,status_two:%d\n",ad_res,ad_vol_res,*status);
}
#endif

void ADG_A2(uint8_t num) {
	int fd;
	int ret = -1;
	char val = '1';
	if (num == 1)
		val = '1';
	else
		val = '0';
	errno = 0;
	fd = open("/sys/class/gpio/gpio74/value", O_RDWR | O_CREAT);
	if (fd == -1) {
		printf("open clock_gpio_failed\n");
		return;
	}

	ret = write(fd, &val, 1);

	if (ret == -1) {
		printf("write clock_gpio_failed\n");
		printf("Error no.%d: %s\n", errno, strerror(errno));
		return;
	}
	close(fd);
}

void ADG_A1(uint8_t num) {
	int fd;
	int ret = -1;
	char val = '1';
	if (num == 1)
		val = '1';
	else
		val = '0';
	errno = 0;
	fd = open("/sys/class/gpio/gpio75/value", O_RDWR | O_CREAT);
	if (fd == -1) {
		printf("open clock_gpio_failed\n");
		return;
	}

	ret = write(fd, &val, 1);

	if (ret == -1) {
		printf("write clock_gpio_failed\n");
		printf("Error no.%d: %s\n", errno, strerror(errno));
		return;
	}
	close(fd);
}

void ADG_A0(uint8_t num) {
	int fd;
	int ret = -1;
	char val = '1';
	if (num == 1)
		val = '1';
	else
		val = '0';
	errno = 0;
	fd = open("/sys/class/gpio/gpio76/value", O_RDWR | O_CREAT);
	if (fd == -1) {
		printf("open clock_gpio_failed\n");
		return;
	}

	ret = write(fd, &val, 1);

	if (ret == -1) {
		printf("write clock_gpio_failed\n");
		printf("Error no.%d: %s\n", errno, strerror(errno));
		return;
	}
	close(fd);
}

void ADG_EN(uint8_t num) {
	int fd;
	int ret = -1;
	char val = '1';
	if (num == 1)
		val = '1';
	else
		val = '0';
	errno = 0;
	fd = open("/sys/class/gpio/gpio77/value", O_RDWR | O_CREAT);
	if (fd == -1) {
		printf("open clock_gpio_failed\n");
		return;
	}

	ret = write(fd, &val, 1);

	if (ret == -1) {
		printf("write clock_gpio_failed\n");
		printf("Error no.%d: %s\n", errno, strerror(errno));
		return;
	}
	close(fd);
}

/*******************************************************************************
 * Function Name  : adg_408_gpio_init
 * Description    : adg408引脚初始化
 * Input          :
 * Output         : None
 * Return         : None
 *******************************************************************************/
void adg_408_gpio_init(void) {
	if (access("/sys/class/gpio/gpio74", F_OK) != 0) {
		system("echo 74 > /sys/class/gpio/export");
		system("echo out > /sys/class/gpio/gpio74/direction");
	}		//A2

	if (access("/sys/class/gpio/gpio75", F_OK) != 0) {
		system("echo 75 > /sys/class/gpio/export");
		system("echo out > /sys/class/gpio/gpio75/direction");
	}		//A1

	if (access("/sys/class/gpio/gpio76", F_OK) != 0) {
		system("echo 76 > /sys/class/gpio/export");
		system("echo out > /sys/class/gpio/gpio76/direction");
	}		//A0

	if (access("/sys/class/gpio/gpio77", F_OK) != 0) {
		system("echo 77 > /sys/class/gpio/export");
		system("echo out > /sys/class/gpio/gpio77/direction");
	}		//EN

	//使能ADG408
	ADG_EN(1);

}

/*******************************************************************************
 * Function Name  : temp_out
 * Description    : 温度通道切换
 * Input          : 需要切换的通道号，0~6,0代表关闭
 * Output         : None
 * Return         : None
 *******************************************************************************/
void temp_ctrl_ch(uint8_t temp_ch) {
	//temp_ch = channel_exchange_func(temp_ch);           //切换为实际通道

	if (temp_ch > 6)
		return;

	if (temp_ch == 0) {
		ADG_EN(0);
	} else {
		ADG_EN(1);
		if ((temp_ch - 1) & 0x01)
			ADG_A0(1);
		else
			ADG_A0(0);

		if ((((temp_ch - 1) >> 1) & 0x01))
			ADG_A1(1);
		else
			ADG_A1(0);

		if ((((temp_ch - 1) >> 2) & 0x01))
			ADG_A2(1);
		else
			ADG_A2(0);
	}

}

void sensor_fault_judge_sw(struct SW_DIAGNOS_PARA *tzdata, uint8_t ch) {
	float vol_res = 0.0f;
	float electrc_res = 0.0f;
//	float ad_vol_res = 0.0f;
	static struct sensor_fault_sw sensor_fault_cnt;

//	FAULT_TYPE SENSOR_ERR_PW[6] = {SENSOR_ERR_PW_1,SENSOR_ERR_PW_2,SENSOR_ERR_PW_3,SENSOR_ERR_PW_4,SENSOR_ERR_PW_5,SENSOR_ERR_PW_6};
//	pFault_Type_Cnt_t PW_SENS_ERR_Fault_Type_Cnt;
//	PW_SENS_ERR_Fault_Type_Cnt = &fault_type_cnt[SENSOR_ERR_PW[ch]];		//更新平稳传感器错误计数地址 XZ

#ifdef AD7606_COMPLEMENTARY_TO_ORIGINAL
	vol_res = float_std_rms(tzdata->data_deal_buff[ch].buff, tzdata->data_deal_buff[ch].size);

	vol_res = AD_CONVERT_VOLTAGE(vol_res);
#else
	vol_res = float_std_rms(tzdata->data_deal_buff[ch].buff,tzdata->data_deal_buff[ch].size);
	//得到电压有效值
	vol_res = (vol_res-32768)/65535.0f*10.0f;
#endif

	electrc_res = vol_to_electric(vol_res);

	tzdata->electric_val_real[ch] = electrc_res;

	DEBUG(" sensor judge vol_res:%f,electrc_res:%f,ch:%d\n", vol_res, electrc_res, ch);

	switch (ch) {
	case 0:
		if (electrc_res < 1.0f) {
			sensor_fault_cnt.sw_y_cnt1++;
			if (sensor_fault_cnt.sw_y_cnt1 > 2) {
				tzdata->sens_para[ch].sens_self_test.bits.self_test_real_time = SENSOR_ERR;
				//DEBUG("sensor1_err\n");
//				DEBUG("sensor1_err\n");
				if (sensor_fault_cnt.sw_y_cnt1 == 15)
					sensor_fault_cnt.sw_y_cnt1 = 3;
				//printf("sensor");
//					AD_ERR.pw_jkb = 0;
//					sys_err_flag.Bits .pw_jkb_falut = 0;
//					PW_SENS_ERR_Fault_Type_Cnt->Cnt_num ++;
			}
		} else {
			sensor_fault_cnt.sw_y_cnt1 = 0;
			tzdata->sens_para[ch].sens_self_test.bits.self_test_real_time = SENSOR_OK;
		}

		break;

	case 1:
//		printf("##### electrc_res = %f\n",electrc_res);
		if (electrc_res < 1.0f) {
			sensor_fault_cnt.sw_y_cnt2++;
			if (sensor_fault_cnt.sw_y_cnt2 > 2) {
//				printf("#####  sensor1_err\n");
				tzdata->sens_para[ch].sens_self_test.bits.self_test_real_time = SENSOR_ERR;
				if (sensor_fault_cnt.sw_y_cnt2 == 15)
					sensor_fault_cnt.sw_y_cnt2 = 3;
			}
		} else {
			sensor_fault_cnt.sw_y_cnt2 = 0;
			tzdata->sens_para[ch].sens_self_test.bits.self_test_real_time = SENSOR_OK;
		}

		break;

	case 2:
		if (electrc_res < 1.0f) {
			sensor_fault_cnt.sw_y_cnt3++;
			if (sensor_fault_cnt.sw_y_cnt3 > 2) {
				tzdata->sens_para[ch].sens_self_test.bits.self_test_real_time =
						SENSOR_ERR;
				if (sensor_fault_cnt.sw_y_cnt3 == 15)
					sensor_fault_cnt.sw_y_cnt3 = 3;
			}
		} else {
			sensor_fault_cnt.sw_y_cnt3 = 0;
			tzdata->sens_para[ch].sens_self_test.bits.self_test_real_time =
					SENSOR_OK;
		}

		break;

	case 3:
		if (electrc_res < 1.0f) {
			sensor_fault_cnt.sw_y_cnt4++;

			//rt_kprintf ("test_err_one:%d,status:%d\r\n",Sensor_fault_cnt.PW_Y_cnt1,*status);
			if (sensor_fault_cnt.sw_y_cnt4 > 2) {
				tzdata->sens_para[ch].sens_self_test.bits.self_test_real_time =
						SENSOR_ERR;					//传感器故障
				if (sensor_fault_cnt.sw_y_cnt4 == 15)
					sensor_fault_cnt.sw_y_cnt4 = 3;
			}
		} else {
			sensor_fault_cnt.sw_y_cnt4 = 0;
			tzdata->sens_para[ch].sens_self_test.bits.self_test_real_time =
					SENSOR_OK;
		}
		break;

	default:
		break;
	}
}
