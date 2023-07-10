#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include "hdd_save.h"
#include "board.h"
#include "ptu_app.h"
#include <sys/vfs.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "self_test.h"
#include "dictionary.h"
#include "iniparser.h"
#include "pswb_record_prot.h"
#include "sw_diagnos.h"
//sem_t hdd_file_del_sem;
sem_t delete_data_dir_sem; 			//数据删除信号量

#define PW_ORIGNAL_DATA_DIR "/media/pw_data/orignal_data/"

#ifdef ADD_DIAG_TZZ_DATA_FILE
	#define PW_DIAG_TZZ_DATA_NAME "pw_diag_tzz_"
#endif

#ifdef ADD_TZ_DATA_FILE
	#define PW_TZ_DATA_NAME "pw_tz_"
#endif

#define PW_ORIGNAL_DATA_NAME "pw_data_"

#define PW_LOG_DATA_DIR "/media/pw_data/log_data/"
#define PW_LOG_DATA_NAME "log_"

#define PW_ERR_LOG_DATA_DIR "/media/local_config/"
#define PW_ERR_LOG_DATA_NAME "/media/local_config/err_log.dat"

#ifdef CAN_ERR_REBOOT_TWO_TIMES
#define LOCAL_SYSTEM_TEST_LOG		"/media/local_config/test_log.ini"				//记录系统运行硬件、软件复位次数　h0s2
#endif

#define PW_TEST_DATA_DIR "/media/pw_data/test_data/"
#define PW_TEST_DATA_NAME "test"
#define PW_TEST_HZ_NAME "check"

#define PW_DELETE_DIR "rm -rf /media/pw_data/orignal_data/"
//#define PW_DELETE_DIR "/media/pw_data/orignal_data/"
#define PW_OPEN_DIR "/media/pw_data/orignal_data"
#define PW_SDK_DIR "/media/pw_data"
#define PW_DEFAULT_DIR "/media/pw_data/orignal_data/20180808"
#define PW_INITTIME_DIR "20180808"

#define SW_DELETE_DIR "rm -rf /media/pw_data/sw_data/"
#define SW_OPEN_DIR "/media/pw_data/sw_data/"
#define SW_DEFAULT_DIR "/media/pw_data/sw_data/20180808"
#define SW_SDK_DIR "/media/pw_data/sw_data"
#define SW_INITTIME_DIR "20180808"


#if defined(HDD_DEV_MMCBLK0P3_OR_TF)
    #define HDD_DEV1_REF_SIZE 8//GB
    #define HDD_DEV2_REF_SIZE 2//GB

	#define HDD_DEV1_FREESIZE 23552//(23*1024)MB
	#define HDD_DEV2_FREESIZE 1024//1024MB
#elif defined(HDD_DEV_MMCBLK0P3_REMOVE_TF)
    //#define HDD_DEV_REF_SIZE  2//GB
	#define HDD_DEV_FREESIZE (1*1024)//MB 剩余空间小于1个G删数据 1*1024
#else
    //#define HDD_DEV_REF_SIZE  8//GB
	#define HDD_DEV_FREESIZE (23*1024)//MB 剩余空间小于23个G删数据 23*1024
#endif

#ifdef SOFTWARE_CHMOD
	#define PW_MEDIA_FILE_CHMOD  my_system("chmod 755 -R /media")
	#define PW_PW_DATA_FILE_CHMOD  my_system("chmod 777 -R /media/pw_data")
	#define PW_ORIGNAL_DATA_FILE_CHMOD  my_system("chmod 777 -R /media/pw_data/orignal_data/")
	#define PW_LOG_DATA_CHMOD  my_system("chmod 777 -R /media/pw_data/log_data/")
	#define PW_TEST_DATA_CHMOD  my_system("chmod 777 -R /media/pw_data/test_data/")

	#define PW_LOCAL_CONFIG_CHMOD  my_system("chmod 777 -R /media/local_config")
	#define PW_LOCAL_CONFIG_APP_CHMOD  my_system("chmod 777 -R /media/local_config/app/")
	#define PW_LOCAL_CONFIG_CONFIG_CHMOD  my_system("chmod 777 -R /media/local_config/config/")
#endif


//#define SW_ORIGNAL_DATA_DIR "/media/sw_data/orignal_data/"
#define SW_ORIGNAL_DATA_DIR "/media/pw_data/sw_data/"

#ifdef ADD_TZ_DATA_FILE
	#define SW_TZ_DATA_NAME "sw_tz_"
#endif

#define SW_ORIGNAL_DATA_NAME "sw_data_"

#define SW_LOG_DATA_DIR "/media/sw_data/log_data/"
#define SW_LOG_DATA_NAME "log_"

#define SW_ERR_LOG_DATA_DIR "/media/local_config/"
#define SW_ERR_LOG_DATA_NAME "/media/local_config/err_log.dat"

#ifdef CAN_ERR_REBOOT_TWO_TIMES
#define LOCAL_SYSTEM_TEST_LOG		"/media/local_config/test_log.ini"				//记录系统运行硬件、软件复位次数　h0s2
#endif

#define SW_TEST_DATA_DIR "/media/sw_data/test_data/"
#define SW_TEST_DATA_NAME1 "test1"
#define SW_TEST_DATA_NAME2 "test2"

//#define SW_DELETE_DIR "rm -rf /media/sw_data/orignal_data/"
//#define SW_OPEN_DIR "/media/sw_data/orignal_data"
//#define SW_DEFAULT_DIR "/media/sw_data/orignal_data/20180808"
//#define SW_SDK_DIR "/media/sw_data"
//#define SW_INITTIME_DIR "20180808"
/****test1_sw*****/
//#define PW_ORIGNAL_DATA_DIR "/media/sw_data/orignal_data/"
//#define PW_ORIGNAL_DATA_NAME "pw_data_"
//
//#define PW_LOG_DATA_DIR "/media/sw_data/log_data/"
//#define PW_LOG_DATA_NAME "log"
//
//#define PW_TEST_DATA_DIR "/media/sw_data/test_data/"
//#define PW_TEST_DATA_NAME "test"
/****test2_sw*****/

#ifdef CAN_ERR_REBOOT_TWO_TIMES
	#define SOFTWARE_REBOOT_TIMES   2
	uint8_t software_reboot_enable = 0;
	void cteate_reboot_log_file();
	void update_reboot_log_file();
	int first_software_reboot_times();
#endif



psw_info_data_t psw_info_data;

struct PW_FILE pw_file;
struct STORE_FLAG store_flag;
struct SW_FILE sw_file;

#ifdef ADD_DIAG_TZZ_DATA_FILE
	extern struct DIAG_TZZ_DATA diag_tzz_data;
#endif
extern struct PW_RAW_DATA pw_raw_data;
extern struct SW_RAW_DATA sw_raw_data;
extern struct RECV_PUBLIC_PARA recv_public_para;
extern struct ETH_STATUS eth_status[16];
extern struct ETH_STATUS can_status[16];
extern struct COMM_DATA_CNT comm_data_cnt;
//extern struct PTU_SIMULATION_DATA ptu_simulation_data;
//extern struct PW_TZ_DATA pw_tz_data;
//extern struct COMMUNICATE_TYPE communicate_type;

//extern struct SYS_STATUS_CNT sys_status_cnt;
extern struct UPDATE_ERR_LOG_FLAG	update_err_log_flag;
sem_t pw_save_sem;
struct PTU_SIMULATION_DATA ptu_simulation_data;
struct SYS_STATUS_CNT sys_status_cnt;
extern uint8_t sw_data_save_type;
#ifdef NEW_DAY_RECTEATE_DIR
struct LOCAL_TIME old_dir_time;
#endif
extern struct SELF_TEST_PARA self_test_para;

extern union SWBOARD_ERR sw_boarderr;
extern struct SELF_TEST_PARA self_test_para1;


extern txb_MVB_public_t app_save_public;
extern struct PW_CLB_CONFIG_PARA *pw_clb_config_para;

extern struct S1PW_TZ_DATA s1pw_tz_data;
extern struct S1SW_TZ_DATA s1sw_tz_data; //S1 use
/*************************************************
Function:  check_sd_exits
Description: 检测SD卡是否挂载成功
Input:  无
Output: 成功usesd_flag=1,失败usesd_flag=0
Return: 无
Others:
*************************************************/
void check_hdd_test(void)
{
	int test_fd=-1;
	unsigned long long int      totalBytes = 0;
   	struct statfs statFS;
	char name[64]={'\0'};
	uint8_t test_buff[]={0x55,0xaa,0x66,0xbb};
	uint8_t read_buff[4];
	int ret = -1;

//	printf("check_hdd_test-------->check_hdd_test<------\n");
	ret = access(HDD_DEV, F_OK);//SDK 0:存在
#if defined(HDD_DEV_MMCBLK0P3_OR_TF)
	uint16_t RefSizeGB = 0;

    if(ret == 0)
    {
    	ret = access(HDD_DEV1, F_OK);
    	RefSizeGB = HDD_DEV1_REF_SIZE;
    }
    else
    {
    	ret = access(HDD_DEV2, F_OK);
    	RefSizeGB = HDD_DEV2_REF_SIZE;
    }
#endif
	if (statfs(MOUNT_DEV, &statFS) == -1){
       	printf("statfs failed for path->[%s]\n", MOUNT_DEV);
   	}

 	totalBytes = (uint64_t)statFS.f_blocks * (uint64_t)statFS.f_bsize;
 	totalBytes = totalBytes >> 30;

	printf("check_hdd_test---ret:%d, totalBytes:%lldGB\n",ret, totalBytes);

#if defined(HDD_DEV_MMCBLK0P3_OR_TF)
	if (ret == 0 && totalBytes >= RefSizeGB)
#elif defined(HDD_DEV_MMCBLK0P3_REMOVE_TF)
	if (ret == 0 && totalBytes >= 2)
#else
	if (ret == 0 && totalBytes >= 8)
#endif
	{
		store_flag.hdd_exist_flag = TRUE;
		DEBUG("check_hdd_test------>hdd_exist!<------\n");

//		store_flag->hdd_exist_flag = TRUE;
//		printf("sd exist!\n");

		//20200810
#ifdef SD_CARD_SOFTWARE_MOUNT //以下在profile中已执行，此处可省去，为避免与老板冲突，暂保留
//	#if defined(HDD_DEV_MMCBLK0P3_REMOVE_SD)
//		system("umount /run/media/mmcblk0p3");
//		system("mkdir /media/pw_data");
//		system("mount -t ext4 /dev/mmcblk0p3 /media/pw_data");//注：此用于去SD卡槽板子，若不插SD卡是/dev/mmcblk0p3，插SD卡是/dev/mmcblk1p3
//	#else
//		system("umount /run/media/mmcblk0p1");
//		system("mkdir /media/pw_data");
//		system("mount -t ext4 /dev/mmcblk0p1 /media/pw_data");
//	#endif
#endif
//		check_sdk_file();//test

		//20200810
		strcpy(name,PW_DATA_DIR);

		//
		if(creat_dir(name)==0)
		{
			strcat(name,LOCAL_TEST_FILE_NAME);
			test_fd=open(name, O_RDWR | O_CREAT | O_APPEND| O_SYNC);
			if(test_fd>=0)
			{
				//DEBUG("sd  exist----------------->1!\n");
				ret = write(test_fd, test_buff, sizeof(test_buff));
				close(test_fd);
				if(ret==4)
				{
					test_fd=open(name, O_RDWR | O_CREAT | O_APPEND| O_SYNC);
					int size=read(test_fd,read_buff,4);
					close(test_fd);
					if(size==4)
					{
						//DEBUG("sd  exist----------------->2!\n");
						if(memcmp(test_buff,read_buff,4)==0)
						{
							DEBUG("check_hdd_test------>hdd_ok!<------\n");
							store_flag.hdd_err_flag=FALSE;
							store_flag.hdd_save_flag = TRUE;
						}
						else
						{
							DEBUG("check_hdd_test------>hdd_err!<------1\n");
							store_flag.hdd_err_flag=TRUE;
						}
					}
					else
					{
						DEBUG("check_hdd_test------>hdd_err!<------2\n");
						store_flag.hdd_err_flag=TRUE;
					}

				}
				else
				{
					DEBUG("check_hdd_test------>hdd_err!<------3\n");
					store_flag.hdd_err_flag=TRUE;
				}
			}
			else
			{
				DEBUG("check_hdd_test------>hdd_err!<------4\n");
				store_flag.hdd_err_flag=TRUE;
			}
		}
		else
		{
			DEBUG("check_hdd_test------>hdd_err!<------5\n");
			store_flag.hdd_err_flag=TRUE;
		}
	}
	else
	{
		DEBUG("check_hdd_test------>hdd_not_exist & hdd_err!<------\n");
		store_flag.hdd_exist_flag = FALSE;				//SD卡不存在
		store_flag.hdd_err_flag=TRUE;
		//system("reboot");
	}
}

/**
 *SD卡热插拔检测信号
 * */
void sd_exist_test(void)
{
	int ret = -1;
	unsigned long long int      totalBytes = 0;
   	struct statfs statFS;

	ret = access(HDD_DEV, F_OK);
#if defined(HDD_DEV_MMCBLK0P3_OR_TF)
	uint16_t RefSizeGB = 0;

    if(ret == 0)
    {
    	ret = access(HDD_DEV1, F_OK);
    	RefSizeGB = HDD_DEV1_REF_SIZE;
    }
    else
    {
    	ret = access(HDD_DEV2, F_OK);
    	RefSizeGB = HDD_DEV2_REF_SIZE;
    }
#endif
//#if defined(HDD_DEV_MMCBLK0P3_REMOVE_SD)
//	totalBytes = get_size_emmc(HDD_DEV);
//	totalBytes = totalBytes >> 21;//12602812/1024/1024/2=6G
//#else
	if (statfs(PW_SDK_DIR, &statFS) == -1){
       	printf("statfs failed for path->[%s]\n", PW_SDK_DIR);
   	}
  	 totalBytes = (uint64_t)statFS.f_blocks * (uint64_t)statFS.f_bsize;
  	 totalBytes = totalBytes >> 30;
//#endif
  	//printf("sd_exist_test---ret:%d, totalBytes:%lldGB\n",ret, totalBytes);

#if defined(HDD_DEV_MMCBLK0P3_OR_TF)
	if (ret == 0 && totalBytes >= RefSizeGB)
#elif defined(HDD_DEV_MMCBLK0P3_REMOVE_TF)
  	if (ret == 0 && totalBytes >= 2)
#else
	if (ret == 0 && totalBytes >= 8)
#endif
	{
		store_flag.hdd_exist_flag = TRUE;

		store_flag.hdd_err_flag = FALSE;
		store_flag.hdd_save_flag = TRUE;
//		printf("sd exist!\n");
	}
	else
	{
		store_flag.hdd_exist_flag = FALSE;
		store_flag.hdd_err_flag=TRUE;
		store_flag.hdd_save_flag=FALSE;
//		store_flag->speed_save_flag=0;
	    printf("sd do not exist!\n");
		//system("reboot");
	}

#if 0//def AD7606_ERR_WATCHDOG_RESET
	extern uint8_t self_check_over_flag;
	extern uint8_t ad_err_flag;
	extern uint8_t ad_err_cnt;

	if(self_check_over_flag)
	{
		if(!ad_err_flag)
		{
			ad_err_cnt++;

			if(ad_err_cnt >= 2)
			{
				extern void ctrl_hw_watchdog(uint8_t ctrl_type);

				printf("sd_exist_test---reboot\n");
				ctrl_hw_watchdog(0);
				system("reboot -nf");
			}
		}
		else
		{
			ad_err_flag = 0;
			ad_err_cnt = 0;
		}
	}
#endif
}


int my_system(const char * cmd)
{
	FILE * fp;
	int res;// char buf[1024];

	if (cmd == NULL)
	{
//		printf("my_system cmd is NULL!\n");
		return -1;
	}

	if ((fp = popen(cmd, "r") ) == NULL)
	{
		printf("popen error: %s/n", strerror(errno));
		return -1;
	}
	else
	{
//		printf("%s\n", cmd);

		if ( (res = pclose(fp)) == -1)
		{
			printf("close popen file pointer fp error!\n");
			return res;
		}
		else
		{
			printf("popen res is :%d\n", res);
			return res;
		}
	}
}


/**
 * 创建文件目录名 ymd/hms/
 */
void create_ymd_hms_dir_name(char *name)
{
	struct LOCAL_TIME time_now;
	get_local_time(&time_now);							//创建目录前获取当前时间
	sprintf(name,"%04d%02d%02d/%02d%02d%02d/",time_now.year,time_now.mon,time_now.day,time_now.hour,time_now.min,time_now.sec);
}

/**
 * 创建年月日时分秒
 * */
void create_ymdhms_str(char *name)
{
	struct LOCAL_TIME time_now;
	get_local_time(&time_now);

#ifdef NEW_DAY_RECTEATE_DIR
	memcpy(&old_dir_time, &time_now, sizeof(struct LOCAL_TIME));
#endif

	sprintf(name,"%04d%02d%02d%02d%02d%02d",time_now.year,time_now.mon,time_now.day,time_now.hour,time_now.min,time_now.sec);

}
/*************************************************
Function:  init_pw_file_para
Description:  初始化平稳参数
Input: 　文件信息 file_para
Output: 无
Return: 无
Others:
*************************************************/
void init_pw_file_para(struct FILE_INF *file_para)
{
	file_para->fp = NULL;
	file_para->size = 0;
	file_para->fd = -1;
	memset(file_para->name,'\0',sizeof(file_para->name));
}

void init_sw_file_para(struct FILE_INF *file_para)
{
	file_para->fp = NULL;
	file_para->size = 0;
	file_para->fd = -1;
	memset(file_para->name,'\0',sizeof(file_para->name));
}

int creat_dir(char *file_dir)
{
	char dirname[256];
	strcpy(dirname, file_dir);
	int i=0, len = strlen(dirname);
	if (dirname[len - 1] != '/')
	{
		strcat(dirname, "/");
	}
	len = strlen(dirname);
	for (i = 0; i < len; i++)
	{
		if (dirname[i] == '/' && i != 0) //i!=0排除根目录
		{
			dirname[i] = 0;
			if (access(dirname, F_OK) != 0)
			{
				if (mkdir(dirname, 0777) == -1)
				{
					perror("mkdir error!\n");
					return -1;
				}
			}
			dirname[i] = '/';
		}
	}
	return 0;
}

void init_file_sys_para()
{
	DEBUG("init_file_sys_one\n");
	init_pw_file_para(&pw_file.pw_original_data);
	init_sw_file_para(&sw_file.sw_original_data);
#ifdef ADD_TZ_DATA_FILE
	init_pw_file_para(&pw_file.pw_tz_data);
	init_sw_file_para(&sw_file.sw_tz_data);
#endif
	DEBUG("init_file_sys_two\n");

//	if(access(SW_DATA_DIR,F_OK) != 0)
//	{
//		if(creat_dir(SW_DATA_DIR) < 0)
//		{
//			return;
//		}
//	}
}


/*
 *
 * 	printf("chmod jxds_pw original data 1\n");

	system("chmod 777 -R /media/pw_data/orignal_data/");

	printf("chmod jxds_pw original data 2\n");
 *
 * */


void open_pw_original_data_file()
{

	char dir_name[256];
#ifdef ADD_TZ_DATA_FILE
	char dir_tz_name[256];
#endif
#ifdef ADD_DIAG_TZZ_DATA_FILE
	char dir_diag_tzz_name[256];
#endif
	char file_type[10];
	char file_name[32];
	char date_dir[32];
	char date_name[32];


	if((store_flag.hdd_exist_flag == FALSE) && (store_flag.hdd_err_flag == TRUE)) //SD卡不存在或者SD卡错误,就不再建立数据文件
		return;

	if(access(PW_ORIGNAL_DATA_DIR,F_OK)!= 0)
	{
		if(creat_dir(PW_ORIGNAL_DATA_DIR) < 0)
		{
			return;
		}
	}
	//补全文件路径名
	strcpy(dir_name,PW_ORIGNAL_DATA_DIR);
	printf("pw_orignal_dir:%s\n",dir_name);
	create_ymd_hms_dir_name(date_dir);
	strcat(dir_name,date_dir);
	if(creat_dir(dir_name) < 0)
	{
		return;
	}

#ifdef ADD_TZ_DATA_FILE
	strcpy(dir_tz_name,dir_name);
#endif
#ifdef ADD_DIAG_TZZ_DATA_FILE
	strcpy(dir_diag_tzz_name,dir_name);
#endif

	//创建平稳原始数据文件
	if(pw_file.pw_original_data.fp != NULL)
	{
		fclose(pw_file.pw_original_data.fp);
		pw_file.pw_original_data.fp = NULL;
	}

	//补全文件路径及文件名
	strcpy(file_name,PW_ORIGNAL_DATA_NAME);
	create_ymdhms_str(date_name);
	strcat(file_name,date_name);
	strcpy(file_type,".dat");
	strcat(file_name,file_type);
	strcat(dir_name,file_name);
	DEBUG("pw_orignal_data_dir:%s\n",dir_name);
	pw_file.pw_original_data.fp = fopen(dir_name,"w+");
	if(pw_file.pw_original_data.fp == NULL)
	{
		DEBUG("pw_file.pw_original_data file open fail\n");
		return;
	}
//	printf("before change original fp:%d\n",pw_file.pw_original_data.fp->_fileno);
	pw_file.pw_original_data.fd = fileno(pw_file.pw_original_data.fp);
//	printf("after change original fp:%d\n",pw_file.pw_original_data.fd);

#ifdef ADD_TZ_DATA_FILE
	if(pw_file.pw_tz_data.fp != NULL)
	{
		fclose(pw_file.pw_tz_data.fp);
		pw_file.pw_tz_data.fp = NULL;
	}

	memset(file_name, 0, sizeof(file_name));
	memset(date_name, 0, sizeof(date_name));
	memset(file_type, 0, sizeof(file_type));

	//补全文件路径及文件名
	strcpy(file_name,PW_TZ_DATA_NAME);
	create_ymdhms_str(date_name);
	strcat(file_name,date_name);
	strcpy(file_type,".dat");
	strcat(file_name,file_type);
	strcat(dir_tz_name,file_name);
	DEBUG("pw_tz_data_dir:%s\n",dir_tz_name);
	pw_file.pw_tz_data.fp = fopen(dir_tz_name,"w+");
	if(pw_file.pw_tz_data.fp == NULL)
	{
		DEBUG("pw_file.pw_tz_data file open fail\n");
		return;
	}
//	printf("before change tz fp:%d\n",pw_file.pw_tz_data.fp->_fileno);
	pw_file.pw_tz_data.fd = fileno(pw_file.pw_tz_data.fp);
//	printf("after change tz fp:%d\n",pw_file.pw_tz_data.fd);
#endif
}


void open_sw_original_data_file()
{
	char dir_name[256];
#ifdef ADD_TZ_DATA_FILE
	char dir_tz_name[256];
#endif
	char file_type[10];
	char file_name[32];
	char date_dir[32];
	char date_name[32];

	if((store_flag.hdd_exist_flag == FALSE) || (store_flag.hdd_err_flag == TRUE))		//SD卡不存在或者SD卡错误,就不再建立数据文件
		return;

	if(access(SW_ORIGNAL_DATA_DIR,F_OK)!= 0)
	{
		if(creat_dir(SW_ORIGNAL_DATA_DIR) < 0)
		{
			return;
		}
	}
	//补全文件路径名
	strcpy(dir_name,SW_ORIGNAL_DATA_DIR);
	printf("sw_orignal_dir_one:%s\n",dir_name);
	create_ymd_hms_dir_name(date_dir);
	strcat(dir_name,date_dir);
	printf("sw_orignal_dir_two:%s\n",dir_name);
	if(creat_dir(dir_name) < 0)
	{
		return;
	}

#ifdef ADD_TZ_DATA_FILE
	strcpy(dir_tz_name,dir_name);
#endif

	//创建平稳原始数据文件
	if(sw_file.sw_original_data.fp != NULL)
	{
		fclose(sw_file.sw_original_data.fp);
		sw_file.sw_original_data.fp = NULL;
	}

	//printf("sw_orignal_dir_three:%s\n",dir_name);
	//补全文件路径及文件名
	strcpy(file_name,SW_ORIGNAL_DATA_NAME);
	create_ymdhms_str(date_name);
	//printf("sw_orignal_date:%s\n",date_name);
	strcat(file_name,date_name);
	strcpy(file_type,".dat");
	strcat(file_name,file_type);

	strcat(dir_name,file_name);
	DEBUG("sw_orignal_dir_four:%s\n",dir_name);
	sw_file.sw_original_data.fp = fopen(dir_name,"w+");

	if(sw_file.sw_original_data.fp == NULL)
	{
		DEBUG("sw_file.sw_original_data file open fail\n");
		return;
	}
	sw_file.sw_original_data.fd = fileno(sw_file.sw_original_data.fp);

#ifdef ADD_TZ_DATA_FILE
	if(sw_file.sw_tz_data.fp != NULL)
	{
		fclose(sw_file.sw_tz_data.fp);
		sw_file.sw_tz_data.fp = NULL;
	}

	memset(file_name, 0, sizeof(file_name));
	memset(date_name, 0, sizeof(date_name));
	memset(file_type, 0, sizeof(file_type));

	//补全文件路径及文件名
	strcpy(file_name,SW_TZ_DATA_NAME);
	create_ymdhms_str(date_name);
	strcat(file_name,date_name);
	strcpy(file_type,".dat");
	strcat(file_name,file_type);
	strcat(dir_tz_name,file_name);
	DEBUG("sw_tz_data_dir:%s\n",dir_tz_name);
	sw_file.sw_tz_data.fp = fopen(dir_tz_name,"w+");
	if(sw_file.sw_tz_data.fp == NULL)
	{
		DEBUG("sw_file.sw_tz_data file open fail\n");
		return;
	}

	sw_file.sw_tz_data.fd = fileno(sw_file.sw_tz_data.fp);
#endif
}

#ifdef ADD_LOG_DATA
void open_log_data_file()				//每天建1个log
{
		char dir_name[256];
		char file_type[10];
		char file_name[32];
		char date_dir[32];
		char date_name[32];

		if((store_flag.hdd_exist_flag == FALSE) || (store_flag.hdd_err_flag == TRUE))		//SD卡不存在或者SD卡错误,就不再建立数据文件
			return;
		if(access(PW_LOG_DATA_DIR,F_OK)!= 0)
		{
			if(creat_dir(PW_LOG_DATA_DIR) < 0)
			{
				return;
			}
		}
		//补全文件路径名
		strcpy(dir_name,PW_LOG_DATA_DIR);
		DEBUG("PW_LOG_DATA_DIR_one:%s\n",dir_name);
		create_ymd_hms_dir_name(date_dir);
		strcat(dir_name,date_dir);
		DEBUG("PW_LOG_DATA_DIR_two:%s\n",dir_name);

		if(creat_dir(dir_name) < 0)
		{
			return;
		}


		//	//创建平稳日志文件
		if(pw_file.log_data.fp != NULL)
		{
			fclose(pw_file.log_data.fp);
			pw_file.log_data.fp = NULL;
		}
		//补全文件路径及文件名
		strcpy(file_name,PW_LOG_DATA_NAME);
		create_ymdhms_str(date_name);
		strcat(file_name,date_name);
		strcpy(file_type,".dat");
		strcat(file_name,file_type);

		strcat(dir_name,file_name);
		DEBUG("PW_LOG_DATA_DIR_three:%s\n",dir_name);

		//pw_file.pw_original_data.fd = fopen(dir_name,"w+");
		pw_file.log_data.fp = fopen(dir_name,"w+");
		if(pw_file.log_data.fp == NULL)
		{
			DEBUG("pw_file.pw_log_data file open fail\n");
			return;
		}
		pw_file.log_data.fd = fileno(pw_file.log_data.fp);
}
#endif

void open_err_log_data_file()
{
		char dir_name[256] = {0};
		char file_type[10] = {0};
		char file_name[32] = {0};


		if((store_flag.hdd_exist_flag == FALSE) || store_flag.hdd_save_flag == FALSE)
			return;

		if(access(PW_ERR_LOG_DATA_DIR,F_OK)!= 0)
		{
			if(creat_dir(PW_ERR_LOG_DATA_DIR) < 0)
			{
				return;
			}
		}
		//补全文件路径名
		strcpy(dir_name,PW_ERR_LOG_DATA_DIR);
		DEBUG("PW_ERR_LOG_DATA_DIR_one:%s\n",dir_name);

		//文件存在,则返回
		if(access(PW_ERR_LOG_DATA_NAME,F_OK) == 0)
		{
			printf("err_log.dat is exist\n");
			return;
		}


		printf("err_log.dat is not exist\n");
		//	//创建平稳日志文件
		if(pw_file.err_log_data.fp != NULL)
		{
			fclose(pw_file.err_log_data.fp);
			pw_file.err_log_data.fp = NULL;
		}
		//补全文件路径及文件名
		strcpy(file_name,"err_log");
		strcpy(file_type,".dat");
		strcat(file_name,file_type);

		strcat(dir_name,file_name);
		DEBUG("PW_ERR_LOG_DATA_DIR_three:%s\n",dir_name);

		pw_file.err_log_data.fp = fopen(dir_name,"wb+");					//打开或者新建一个二进制文件，可以读，只允许在末尾追加写
		if(pw_file.err_log_data.fp == NULL)
		{
			DEBUG("pw_file.pw_err_log_data1 file open fail\n");
			return;
		}
		pw_file.err_log_data.fd = fileno(pw_file.err_log_data.fp);

		fclose(pw_file.err_log_data.fp);			//关闭故障日志文件

		pw_file.err_log_data.fp = NULL;
		pw_file.err_log_data.fd = -1;
}

#ifdef DIAG_DATA_SAVE_FOR_TEST

#define DEL_FILE_SIZE  (1.5*1024) //1.5G

//仅获取2G以内文件大小．2^31/1024/1024/1024=2G
uint32_t get_one_file_size(const char *path)
{
    unsigned long filesize = -1;
    struct stat statbuff;

    if(stat(path, &statbuff) < 0){
        return filesize;
    }else{
        filesize = statbuff.st_size/1024/1024;//单位：MB
    }

    return filesize;
}

#ifdef PW_DIAG
int create_pwtzz_diag_file()
{
	static uint8_t called_one_time_flag = 0;

	if((store_flag.hdd_exist_flag == FALSE) || store_flag.hdd_save_flag == FALSE)
		return -1;

	if(access(DIAG_DATA_PWTZZ_FILE, F_OK)==0)//文件存在
	{
		if(called_one_time_flag)//已调用一次create_pwtzz_diag_file,就不再重建
		{
			return 0;
		}
		else//第一次调用,删除存在文件
		{
			system("chmod 777 -R /media/pw_data/");
			system("rm -rf /media/pw_data/pwtzz_diag.csv");

			printf("create_pwtzz_diag_file---delete file!\n");
		}
	}

	pw_file.pwtzz_diag_data.fp = fopen(DIAG_DATA_PWTZZ_FILE, "a+");
	if (pw_file.pwtzz_diag_data.fp == NULL)
	{
		return -1;
	}

//	vibr_file.pwtzz_diag_data.fd = fileno(vibr_file.pwtzz_diag_data.fp);

//	save_gearbear_diag_file(BEAR_DATA_Y_TYPE, DIAG_DATA_NUMBER_TITLE, NULL, 8192);//BEARING_DIAG_STEP:8192

	fclose(pw_file.pwtzz_diag_data.fp);
	pw_file.pwtzz_diag_data.fp = NULL;

	called_one_time_flag = 1;

	printf("create_pwtzz_diag_file\n");

	return 0;
}

void save_pwtzz_diag_file(SAVE_DATA_TYPE type, DIAG_DETAIL_DATA_E detail_type, void *buff, uint32_t size)
{
	int i=0;

	if(pw_file.pwtzz_diag_data.fp == NULL)
	{
		if(get_one_file_size((char *)&DIAG_DATA_PWTZZ_FILE) > DEL_FILE_SIZE)
		{
			system("chmod 777 -R /media/pw_data/");
			system("rm -rf /media/pw_data/pwtzz_diag.csv");

			printf("create_pwtzz_diag_file---delete file!\n");
		}

		printf("save_pwtzz_diag_file\n");
		pw_file.pwtzz_diag_data.fp = fopen(DIAG_DATA_PWTZZ_FILE, "a+");
		fseek(pw_file.pwtzz_diag_data.fp, 0, SEEK_END);
//			pw_file.pwtzz_diag_data.fd = fileno(pw_file.pwtzz_diag_data.fp);
	}

	switch(detail_type)
	{		//TITLE
		case DIAG_DATA_NUMBER_TITLE:
			fprintf(pw_file.pwtzz_diag_data.fp, "\n");
//				rewind(pw_file.pwtzz_diag_data.fp);
			fprintf(pw_file.pwtzz_diag_data.fp, "Number,");
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.pwtzz_diag_data.fp, "%d,", i);
			}
			break;

		case DIAG_DATA_CHANNEL_TITLE:
			fprintf(pw_file.pwtzz_diag_data.fp, "\n");
//				rewind(pw_file.pwtzz_diag_data.fp);
			fprintf(pw_file.pwtzz_diag_data.fp, "CHANNEL,");
			break;

		case DIAG_DATA_SPEED_TITLE:
			fprintf(pw_file.pwtzz_diag_data.fp, "\n");
//				rewind(pw_file.pwtzz_diag_data.fp);
			fprintf(pw_file.pwtzz_diag_data.fp, "SPEED,");
			break;

		case DIAG_DATA_ACC_TITLE:
			fprintf(pw_file.pwtzz_diag_data.fp, "\n");
//				rewind(pw_file.pwtzz_diag_data.fp);
			fprintf(pw_file.pwtzz_diag_data.fp, "ACC,");
			break;

		case DIAG_DATA_AD_TITLE:
			fprintf(pw_file.pwtzz_diag_data.fp, "\n");
//				rewind(pw_file.pwtzz_diag_data.fp);
			fprintf(pw_file.pwtzz_diag_data.fp, "AD,");
			break;

		case DIAG_DATA_FFT_AMP_TITLE:
			fprintf(pw_file.pwtzz_diag_data.fp, "\n");
//				rewind(pw_file.pwtzz_diag_data.fp);
			fprintf(pw_file.pwtzz_diag_data.fp, "FFT_AMP,");
			break;

		case DIAG_DATA_40HZ_FFT_AMP_TITLE:
			fprintf(pw_file.pwtzz_diag_data.fp, "\n");
//				rewind(pw_file.pwtzz_diag_data.fp);
			fprintf(pw_file.pwtzz_diag_data.fp, "40HZ_FFT_AMP,");
			break;

		case DIAG_DATA_1S_FFT_AMP_TITLE:
			fprintf(pw_file.pwtzz_diag_data.fp, "\n");
//				rewind(pw_file.pwtzz_diag_data.fp);
			fprintf(pw_file.pwtzz_diag_data.fp, "1S_FFT_AMP,");
			break;

		case DIAG_DATA_5S_MEAN_FFT_AMP_TITLE:
			fprintf(pw_file.pwtzz_diag_data.fp, "\n");
//				rewind(pw_file.pwtzz_diag_data.fp);
			fprintf(pw_file.pwtzz_diag_data.fp, "5S_MEAN_FFT_AMP,");
			break;

		case DIAG_DATA_PW_TZZ_SUM_TITLE:
			fprintf(pw_file.pwtzz_diag_data.fp, "\n");
//				rewind(pw_file.pwtzz_diag_data.fp);
			fprintf(pw_file.pwtzz_diag_data.fp, "PW_TZZ_SUM,");
			break;

		case DIAG_DATA_PW_TZZ_TITLE:
			fprintf(pw_file.pwtzz_diag_data.fp, "\n");
//				rewind(pw_file.pwtzz_diag_data.fp);
			fprintf(pw_file.pwtzz_diag_data.fp, "PW_TZZ,");
			break;

		case DIAG_DATA_BEFORE_FILTER_TITLE:
			fprintf(pw_file.pwtzz_diag_data.fp, "\n");
//				rewind(pw_file.pwtzz_diag_data.fp);
			fprintf(pw_file.pwtzz_diag_data.fp, "BEFORE_FILTER,");
			break;

		case DIAG_DATA_AFTER_FILTER_TITLE:
			fprintf(pw_file.pwtzz_diag_data.fp, "\n");
//				rewind(pw_file.pwtzz_diag_data.fp);
			fprintf(pw_file.pwtzz_diag_data.fp, "AFTER_FILTER,");
			break;

			//DATA
		case DIAG_DATA_CHANNEL:
			fprintf(pw_file.pwtzz_diag_data.fp, "%d,", *((uint8_t*)buff));
			break;

		case DIAG_DATA_SPEED:
			fprintf(pw_file.pwtzz_diag_data.fp, "%6.2f,", *((float *)buff));
			break;

		case DIAG_DATA_ACC:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.pwtzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_AD:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.pwtzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_FFT_AMP:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.pwtzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_40HZ_FFT_AMP:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.pwtzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_1S_FFT_AMP:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.pwtzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_5S_MEAN_FFT_AMP:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.pwtzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_PW_TZZ_SUM:
			fprintf(pw_file.pwtzz_diag_data.fp, "%.7f,", *((float*)buff));
			break;

		case DIAG_DATA_PW_TZZ:
			fprintf(pw_file.pwtzz_diag_data.fp, "%.7f,", *((float*)buff));
			break;

		case DIAG_DATA_BEFORE_FILTER:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.pwtzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_AFTER_FILTER:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.pwtzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;
	}

	return;
}

void close_pwtzz_diag_file(SAVE_DATA_TYPE type)
{
	printf("close_pwtzz_diag_file\n");

//		fsync(pw_file.pwtzz_diag_data.fd);
	fclose(pw_file.pwtzz_diag_data.fp);
	pw_file.pwtzz_diag_data.fp = NULL;
//		pw_file.pwtzz_diag_data.fd = -1;

	return;
}
#endif
#ifdef JT_DIAG
int create_jttzz_diag_file()
{
	static uint8_t called_one_time_flag = 0;

	if((store_flag.hdd_exist_flag == FALSE) || store_flag.hdd_save_flag == FALSE)
		return -1;

	if(access(DIAG_DATA_JTTZZ_FILE, F_OK)==0)//文件存在
	{
		if(called_one_time_flag)//已调用一次create_jttzz_diag_file,就不再重建
		{
			return 0;
		}
		else//第一次调用,删除存在文件
		{
			system("chmod 777 -R /media/pw_data/");
			system("rm -rf /media/pw_data/jttzz_diag.csv");

			printf("create_jttzz_diag_file---delete file!\n");
		}
	}

	pw_file.jttzz_diag_data.fp = fopen(DIAG_DATA_JTTZZ_FILE, "a+");
	if (pw_file.jttzz_diag_data.fp == NULL)
	{
		return -1;
	}

//	vibr_file.jttzz_diag_data.fd = fileno(vibr_file.jttzz_diag_data.fp);

//	save_gearbear_diag_file(BEAR_DATA_Y_TYPE, DIAG_DATA_NUMBER_TITLE, NULL, 8192);//BEARING_DIAG_STEP:8192

	fclose(pw_file.jttzz_diag_data.fp);
	pw_file.jttzz_diag_data.fp = NULL;

	called_one_time_flag = 1;

	printf("create_jttzz_diag_file\n");

	return 0;
}

void save_jttzz_diag_file(SAVE_DATA_TYPE type, DIAG_DETAIL_DATA_E detail_type, void *buff, uint32_t size)
{
	int i=0;

	if(pw_file.jttzz_diag_data.fp == NULL)
	{
		if(get_one_file_size((char *)&DIAG_DATA_JTTZZ_FILE) > DEL_FILE_SIZE)
		{
			system("chmod 777 -R /media/pw_data/");
			system("rm -rf /media/pw_data/jttzz_diag.csv");

			printf("create_jttzz_diag_file---delete file!\n");
		}

		printf("save_jttzz_diag_file\n");
		pw_file.jttzz_diag_data.fp = fopen(DIAG_DATA_JTTZZ_FILE, "a+");
		fseek(pw_file.jttzz_diag_data.fp, 0, SEEK_END);
//			pw_file.jttzz_diag_data.fd = fileno(pw_file.jttzz_diag_data.fp);
	}

	switch(detail_type)
	{		//TITLE
		case DIAG_DATA_NUMBER_TITLE:
			fprintf(pw_file.jttzz_diag_data.fp, "\n");
//				rewind(pw_file.jttzz_diag_data.fp);
			fprintf(pw_file.jttzz_diag_data.fp, "Number,");
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.jttzz_diag_data.fp, "%d,", i);
			}
			break;

		case DIAG_DATA_CHANNEL_TITLE:
			fprintf(pw_file.jttzz_diag_data.fp, "\n");
//				rewind(pw_file.jttzz_diag_data.fp);
			fprintf(pw_file.jttzz_diag_data.fp, "CHANNEL,");
			break;

		case DIAG_DATA_SPEED_TITLE:
			fprintf(pw_file.jttzz_diag_data.fp, "\n");
//				rewind(pw_file.jttzz_diag_data.fp);
			fprintf(pw_file.jttzz_diag_data.fp, "SPEED,");
			break;

		case DIAG_DATA_ACC_TITLE:
			fprintf(pw_file.jttzz_diag_data.fp, "\n");
//				rewind(pw_file.jttzz_diag_data.fp);
			fprintf(pw_file.jttzz_diag_data.fp, "ACC,");
			break;

		case DIAG_DATA_AD_TITLE:
			fprintf(pw_file.jttzz_diag_data.fp, "\n");
//				rewind(pw_file.jttzz_diag_data.fp);
			fprintf(pw_file.jttzz_diag_data.fp, "AD,");
			break;

		case DIAG_DATA_BEFORE_FILTER_TITLE:
			fprintf(pw_file.jttzz_diag_data.fp, "\n");
//				rewind(pw_file.jttzz_diag_data.fp);
			fprintf(pw_file.jttzz_diag_data.fp, "BEFORE_FILTER,");
			break;

		case DIAG_DATA_AFTER_FILTER_TITLE:
			fprintf(pw_file.jttzz_diag_data.fp, "\n");
//				rewind(pw_file.jttzz_diag_data.fp);
			fprintf(pw_file.jttzz_diag_data.fp, "AFTER_FILTER,");
			break;

		case DIAG_DATA_AVG_SPEED_TITLE:
			fprintf(pw_file.jttzz_diag_data.fp, "\n");
//				rewind(pw_file.pwtzz_diag_data.fp);
			fprintf(pw_file.jttzz_diag_data.fp, "AVG_SPEED,");
			break;

		case DIAG_DATA_JT_MAX_FRE_TITLE:
			fprintf(pw_file.jttzz_diag_data.fp, "\n");
//				rewind(pw_file.pwtzz_diag_data.fp);
			fprintf(pw_file.jttzz_diag_data.fp, "JT_MAX_FRE,");
			break;

		case DIAG_DATA_JT_TZZ_TITLE:
			fprintf(pw_file.jttzz_diag_data.fp, "\n");
//				rewind(pw_file.pwtzz_diag_data.fp);
			fprintf(pw_file.jttzz_diag_data.fp, "JT_TZZ,");
			break;

			//DATA
		case DIAG_DATA_CHANNEL:
			fprintf(pw_file.jttzz_diag_data.fp, "%d,", *((uint8_t*)buff));
			break;

		case DIAG_DATA_SPEED:
			fprintf(pw_file.jttzz_diag_data.fp, "%6.2f,", *((float *)buff));
			break;

		case DIAG_DATA_ACC:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.jttzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_AD:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.jttzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_BEFORE_FILTER:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.jttzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_AFTER_FILTER:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.jttzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_AVG_SPEED:
			fprintf(pw_file.jttzz_diag_data.fp, "%.7f,", *((float*)buff));
			break;

		case DIAG_DATA_JT_MAX_FRE:
			fprintf(pw_file.jttzz_diag_data.fp, "%.7f,", *((float*)buff));
			break;

		case DIAG_DATA_JT_TZZ:
			fprintf(pw_file.jttzz_diag_data.fp, "%.7f,", *((float*)buff));
			break;
	}

	return;
}

void close_jttzz_diag_file(SAVE_DATA_TYPE type)
{
	printf("close_jttzz_diag_file\n");

//		fsync(pw_file.jttzz_diag_data.fd);
	fclose(pw_file.jttzz_diag_data.fp);
	pw_file.jttzz_diag_data.fp = NULL;
//		pw_file.jttzz_diag_data.fd = -1;

	return;
}
#endif
#ifdef DC_DIAG
int create_dctzz_diag_file()
{
	static uint8_t called_one_time_flag = 0;

	if((store_flag.hdd_exist_flag == FALSE) || store_flag.hdd_save_flag == FALSE)
		return -1;

	if(access(DIAG_DATA_DCTZZ_FILE, F_OK)==0)//文件存在
	{
		if(called_one_time_flag)//已调用一次create_dctzz_diag_file,就不再重建
		{
			return 0;
		}
		else//第一次调用,删除存在文件
		{
			system("chmod 777 -R /media/pw_data/");
			system("rm -rf /media/pw_data/dctzz_diag.csv");

			printf("create_dctzz_diag_file---delete file!\n");
		}
	}

	pw_file.dctzz_diag_data.fp = fopen(DIAG_DATA_DCTZZ_FILE, "a+");
	if (pw_file.dctzz_diag_data.fp == NULL)
	{
		return -1;
	}

//	vibr_file.dctzz_diag_data.fd = fileno(vibr_file.dctzz_diag_data.fp);

//	save_gearbear_diag_file(BEAR_DATA_Y_TYPE, DIAG_DATA_NUMBER_TITLE, NULL, 8192);//BEARING_DIAG_STEP:8192

	fclose(pw_file.dctzz_diag_data.fp);
	pw_file.dctzz_diag_data.fp = NULL;

	called_one_time_flag = 1;

	printf("create_dctzz_diag_file\n");

	return 0;
}

void save_dctzz_diag_file(SAVE_DATA_TYPE type, DIAG_DETAIL_DATA_E detail_type, void *buff, uint32_t size)
{
	int i=0;

	if(pw_file.dctzz_diag_data.fp == NULL)
	{
		if(get_one_file_size((char *)&DIAG_DATA_DCTZZ_FILE) > DEL_FILE_SIZE)
		{
			system("chmod 777 -R /media/pw_data/");
			system("rm -rf /media/pw_data/dctzz_diag.csv");

			printf("create_dctzz_diag_file---delete file!\n");
		}

		printf("save_dctzz_diag_file\n");
		pw_file.dctzz_diag_data.fp = fopen(DIAG_DATA_DCTZZ_FILE, "a+");
		fseek(pw_file.dctzz_diag_data.fp, 0, SEEK_END);
//			pw_file.dctzz_diag_data.fd = fileno(pw_file.dctzz_diag_data.fp);
	}

	switch(detail_type)
	{		//TITLE
		case DIAG_DATA_NUMBER_TITLE:
			fprintf(pw_file.dctzz_diag_data.fp, "\n");
//				rewind(pw_file.dctzz_diag_data.fp);
			fprintf(pw_file.dctzz_diag_data.fp, "Number,");
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.dctzz_diag_data.fp, "%d,", i);
			}
			break;

		case DIAG_DATA_CHANNEL_TITLE:
			fprintf(pw_file.dctzz_diag_data.fp, "\n");
//				rewind(pw_file.dctzz_diag_data.fp);
			fprintf(pw_file.dctzz_diag_data.fp, "CHANNEL,");
			break;

		case DIAG_DATA_SPEED_TITLE:
			fprintf(pw_file.dctzz_diag_data.fp, "\n");
//				rewind(pw_file.dctzz_diag_data.fp);
			fprintf(pw_file.dctzz_diag_data.fp, "SPEED,");
			break;

		case DIAG_DATA_ACC_TITLE:
			fprintf(pw_file.dctzz_diag_data.fp, "\n");
//				rewind(pw_file.dctzz_diag_data.fp);
			fprintf(pw_file.dctzz_diag_data.fp, "ACC,");
			break;

		case DIAG_DATA_AD_TITLE:
			fprintf(pw_file.dctzz_diag_data.fp, "\n");
//				rewind(pw_file.dctzz_diag_data.fp);
			fprintf(pw_file.dctzz_diag_data.fp, "AD,");
			break;

		case DIAG_DATA_BEFORE_FILTER_TITLE:
			fprintf(pw_file.dctzz_diag_data.fp, "\n");
//				rewind(pw_file.dctzz_diag_data.fp);
			fprintf(pw_file.dctzz_diag_data.fp, "BEFORE_FILTER,");
			break;

		case DIAG_DATA_AFTER_FILTER_TITLE:
			fprintf(pw_file.dctzz_diag_data.fp, "\n");
//				rewind(pw_file.dctzz_diag_data.fp);
			fprintf(pw_file.dctzz_diag_data.fp, "AFTER_FILTER,");
			break;

		case DIAG_DATA_5S_AFTER_FILTER_TITLE:
			fprintf(pw_file.dctzz_diag_data.fp, "\n");
//				rewind(pw_file.dctzz_diag_data.fp);
			fprintf(pw_file.dctzz_diag_data.fp, "5S_AFTER_FILTER,");
			break;

		case DIAG_DATA_DC_TZZ_TITLE:
			fprintf(pw_file.dctzz_diag_data.fp, "\n");
//				rewind(pw_file.dctzz_diag_data.fp);
			fprintf(pw_file.dctzz_diag_data.fp, "ROOT_MEAN_SQUARE,");
			break;


			//DATA
		case DIAG_DATA_CHANNEL:
			fprintf(pw_file.dctzz_diag_data.fp, "%d,", *((uint8_t*)buff));
			break;

		case DIAG_DATA_SPEED:
			fprintf(pw_file.dctzz_diag_data.fp, "%6.2f,", *((float *)buff));
			break;

		case DIAG_DATA_ACC:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.dctzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_AD:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.dctzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_BEFORE_FILTER:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.dctzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_AFTER_FILTER:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.dctzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_5S_AFTER_FILTER:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.dctzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_DC_TZZ:
			fprintf(pw_file.dctzz_diag_data.fp, "%.7f,", *((float*)buff));//root_mean_square
			break;
	}

	return;
}

void close_dctzz_diag_file(SAVE_DATA_TYPE type)
{
	printf("close_dctzz_diag_file\n");

//		fsync(pw_file.dctzz_diag_data.fd);
	fclose(pw_file.dctzz_diag_data.fp);
	pw_file.dctzz_diag_data.fp = NULL;
//		pw_file.dctzz_diag_data.fd = -1;

	return;
}
#endif
#ifdef HC_DIAG
int create_hctzz_diag_file()
{
	static uint8_t called_one_time_flag = 0;

	if((store_flag.hdd_exist_flag == FALSE) || store_flag.hdd_save_flag == FALSE)
		return -1;

	if(access(DIAG_DATA_HCTZZ_FILE, F_OK)==0)//文件存在
	{
		if(called_one_time_flag)//已调用一次create_hctzz_diag_file,就不再重建
		{
			return 0;
		}
		else//第一次调用,删除存在文件
		{
			system("chmod 777 -R /media/pw_data/");
			system("rm -rf /media/pw_data/hctzz_diag.csv");

			printf("create_hctzz_diag_file---delete file!\n");
		}
	}

	pw_file.hctzz_diag_data.fp = fopen(DIAG_DATA_HCTZZ_FILE, "a+");
	if (pw_file.hctzz_diag_data.fp == NULL)
	{
		return -1;
	}

//	vibr_file.hctzz_diag_data.fd = fileno(vibr_file.hctzz_diag_data.fp);

//	save_gearbear_diag_file(BEAR_DATA_Y_TYPE, DIAG_DATA_NUMBER_TITLE, NULL, 8192);//BEARING_DIAG_STEP:8192

	fclose(pw_file.hctzz_diag_data.fp);
	pw_file.hctzz_diag_data.fp = NULL;

	called_one_time_flag = 1;

	printf("create_hctzz_diag_file\n");

	return 0;
}

void save_hctzz_diag_file(SAVE_DATA_TYPE type, DIAG_DETAIL_DATA_E detail_type, void *buff, uint32_t size)
{
	int i=0;

	if(pw_file.hctzz_diag_data.fp == NULL)
	{
		if(get_one_file_size((char *)&DIAG_DATA_HCTZZ_FILE) > DEL_FILE_SIZE)
		{
			system("chmod 777 -R /media/pw_data/");
			system("rm -rf /media/pw_data/hctzz_diag.csv");

			printf("create_hctzz_diag_file---delete file!\n");
		}

		printf("save_hctzz_diag_file\n");
		pw_file.hctzz_diag_data.fp = fopen(DIAG_DATA_HCTZZ_FILE, "a+");
		fseek(pw_file.hctzz_diag_data.fp, 0, SEEK_END);
//			pw_file.hctzz_diag_data.fd = fileno(pw_file.hctzz_diag_data.fp);
	}

	switch(detail_type)
	{		//TITLE
		case DIAG_DATA_NUMBER_TITLE:
			fprintf(pw_file.hctzz_diag_data.fp, "\n");
//				rewind(pw_file.hctzz_diag_data.fp);
			fprintf(pw_file.hctzz_diag_data.fp, "Number,");
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.hctzz_diag_data.fp, "%d,", i);
			}
			break;

		case DIAG_DATA_CHANNEL_TITLE:
			fprintf(pw_file.hctzz_diag_data.fp, "\n");
//				rewind(pw_file.hctzz_diag_data.fp);
			fprintf(pw_file.hctzz_diag_data.fp, "CHANNEL,");
			break;

		case DIAG_DATA_SPEED_TITLE:
			fprintf(pw_file.hctzz_diag_data.fp, "\n");
//				rewind(pw_file.hctzz_diag_data.fp);
			fprintf(pw_file.hctzz_diag_data.fp, "SPEED,");
			break;

		case DIAG_DATA_ACC_TITLE:
			fprintf(pw_file.hctzz_diag_data.fp, "\n");
//				rewind(pw_file.hctzz_diag_data.fp);
			fprintf(pw_file.hctzz_diag_data.fp, "ACC,");
			break;

		case DIAG_DATA_AD_TITLE:
			fprintf(pw_file.hctzz_diag_data.fp, "\n");
//				rewind(pw_file.hctzz_diag_data.fp);
			fprintf(pw_file.hctzz_diag_data.fp, "AD,");
			break;

		case DIAG_DATA_BEFORE_FILTER_TITLE:
			fprintf(pw_file.hctzz_diag_data.fp, "\n");
//				rewind(pw_file.hctzz_diag_data.fp);
			fprintf(pw_file.hctzz_diag_data.fp, "BEFORE_FILTER,");
			break;

		case DIAG_DATA_AFTER_FILTER_TITLE:
			fprintf(pw_file.hctzz_diag_data.fp, "\n");
//				rewind(pw_file.hctzz_diag_data.fp);
			fprintf(pw_file.hctzz_diag_data.fp, "AFTER_FILTER,");
			break;

		case DIAG_DATA_FFZ_TITLE:
			fprintf(pw_file.hctzz_diag_data.fp, "\n");
//				rewind(pw_file.hctzz_diag_data.fp);
			fprintf(pw_file.hctzz_diag_data.fp, "FFZ,");
			break;

		case DIAG_DATA_FZ_TITLE:
			fprintf(pw_file.hctzz_diag_data.fp, "\n");
//				rewind(pw_file.hctzz_diag_data.fp);
			fprintf(pw_file.hctzz_diag_data.fp, "FZ,");
			break;

			//DATA
		case DIAG_DATA_CHANNEL:
			fprintf(pw_file.hctzz_diag_data.fp, "%d,", *((uint8_t*)buff));
			break;

		case DIAG_DATA_SPEED:
			fprintf(pw_file.hctzz_diag_data.fp, "%6.2f,", *((float *)buff));
			break;

		case DIAG_DATA_ACC:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.hctzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_AD:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.hctzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_BEFORE_FILTER:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.hctzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_AFTER_FILTER:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.hctzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_FFZ:
			fprintf(pw_file.hctzz_diag_data.fp, "%.7f,", *((float*)buff));
			break;

		case DIAG_DATA_FZ:
			fprintf(pw_file.hctzz_diag_data.fp, "%.7f,", *((float*)buff));
			break;
	}

	return;
}

void close_hctzz_diag_file(SAVE_DATA_TYPE type)
{
	printf("close_hctzz_diag_file\n");

//		fsync(pw_file.hctzz_diag_data.fd);
	fclose(pw_file.hctzz_diag_data.fp);
	pw_file.hctzz_diag_data.fp = NULL;
//		pw_file.hctzz_diag_data.fd = -1;

	return;
}
#endif
#ifdef CT_10S_DIAG
int create_tentzz_diag_file()
{
	static uint8_t called_one_time_flag = 0;

	if((store_flag.hdd_exist_flag == FALSE) || store_flag.hdd_save_flag == FALSE)
		return -1;

	if(access(DIAG_DATA_TENTZZ_FILE, F_OK)==0)//文件存在
	{
		if(called_one_time_flag)//已调用一次create_tentzz_diag_file,就不再重建
		{
			return 0;
		}
		else//第一次调用,删除存在文件
		{
			system("chmod 777 -R /media/pw_data/");
			system("rm -rf /media/pw_data/tentzz_diag.csv");

			printf("create_tentzz_diag_file---delete file!\n");
		}
	}

	pw_file.tentzz_diag_data.fp = fopen(DIAG_DATA_TENTZZ_FILE, "a+");
	if (pw_file.tentzz_diag_data.fp == NULL)
	{
		return -1;
	}

//	vibr_file.tentzz_diag_data.fd = fileno(vibr_file.tentzz_diag_data.fp);

//	save_gearbear_diag_file(BEAR_DATA_Y_TYPE, DIAG_DATA_NUMBER_TITLE, NULL, 8192);//BEARING_DIAG_STEP:8192

	fclose(pw_file.tentzz_diag_data.fp);
	pw_file.tentzz_diag_data.fp = NULL;

	called_one_time_flag = 1;

	printf("create_tentzz_diag_file\n");

	return 0;
}

void save_tentzz_diag_file(SAVE_DATA_TYPE type, DIAG_DETAIL_DATA_E detail_type, void *buff, uint32_t size)
{
	int i=0;

	if(pw_file.tentzz_diag_data.fp == NULL)
	{
		if(get_one_file_size((char *)&DIAG_DATA_TENTZZ_FILE) > DEL_FILE_SIZE)
		{
			system("chmod 777 -R /media/pw_data/");
			system("rm -rf /media/pw_data/tentzz_diag.csv");

			printf("create_tentzz_diag_file---delete file!\n");
		}

		printf("save_tentzz_diag_file\n");
		pw_file.tentzz_diag_data.fp = fopen(DIAG_DATA_TENTZZ_FILE, "a+");
		fseek(pw_file.tentzz_diag_data.fp, 0, SEEK_END);
//			pw_file.tentzz_diag_data.fd = fileno(pw_file.tentzz_diag_data.fp);
	}

	switch(detail_type)
	{		//TITLE
		case DIAG_DATA_NUMBER_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "Number,");
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.tentzz_diag_data.fp, "%d,", i);
			}
			break;

		case DIAG_DATA_CHANNEL_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "CHANNEL,");
			break;

		case DIAG_DATA_SPEED_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "SPEED,");
			break;

		case DIAG_DATA_ACC_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "ACC,");
			break;

		case DIAG_DATA_AD_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "AD,");
			break;

		case DIAG_DATA_BEFORE_FILTER_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "BEFORE_FILTER,");
			break;

		case DIAG_DATA_AFTER_FILTER_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "AFTER_FILTER,");
			break;

		case DIAG_DATA_RESAMPLE_AD_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "RESAMPLE_AD,");
			break;

		case DIAG_DATA_FFT_AMP_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "FFT_AMP,");
			break;
//0.2-3Hz
		case DIAG_DATA_MAX_FFT_AMP1_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "MAX_FFT_AMP1(0.2-3Hz),");
			break;

		case DIAG_DATA_PERCENT_MAIN_FREQ1_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "PERCENT_MAIN_FREQ1(0.2-3Hz),");
			break;

		case DIAG_DATA_MAIN_FREQ1_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "MAIN_FREQ1(0.2-3Hz),");
			break;
//5-13Hz
		case DIAG_DATA_MAX_FFT_AMP2_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "MAX_FFT_AMP2(5-13Hz),");
			break;

		case DIAG_DATA_PERCENT_MAIN_FREQ2_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "PERCENT_MAIN_FREQ2(5-13Hz),");
			break;

		case DIAG_DATA_MAIN_FREQ2_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "MAIN_FREQ2(5-13Hz),");
			break;

		case DIAG_DATA_MAX_FABS_AMP3_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "MAX_FABS_AMP3,");
			break;
//1-3Hz
		case DIAG_DATA_MAX_FFT_AMP3_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "MAX_FFT_AMP3(1-3Hz),");
			break;

		case DIAG_DATA_PERCENT_MAIN_FREQ3_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "PERCENT_MAIN_FREQ3(1-3Hz),");
			break;

		case DIAG_DATA_MAIN_FREQ3_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "MAIN_FREQ3(1-3Hz),");
			break;

		case DIAG_DATA_AVERAGE_SPEED_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "AVERAGE_SPEED(1-3Hz),");
			break;

		case DIAG_DATA_BRIDGE_LEN_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "BRIDGE_LEN(1-3Hz),");
			break;

		case DIAG_DATA_AVERAGE_BRIDGE_LEN_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "AVERAGE_BRIDGE_LEN(1-3Hz),");
			break;

		case DIAG_DATA_BRIDGE_FLAG_TITLE:
			fprintf(pw_file.tentzz_diag_data.fp, "\n");
//				rewind(pw_file.tentzz_diag_data.fp);
			fprintf(pw_file.tentzz_diag_data.fp, "BRIDGE_FLAG(1-3Hz),");
			break;

			//DATA
		case DIAG_DATA_CHANNEL:
			fprintf(pw_file.tentzz_diag_data.fp, "%d,", *((uint8_t*)buff));
			break;

		case DIAG_DATA_SPEED:
			fprintf(pw_file.tentzz_diag_data.fp, "%6.2f,", *((float *)buff));
			break;

		case DIAG_DATA_ACC:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.tentzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_AD:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.tentzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_BEFORE_FILTER:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.tentzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_AFTER_FILTER:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.tentzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_RESAMPLE_AD:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.tentzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_FFT_AMP:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.tentzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_MAX_FFT_AMP1:
			fprintf(pw_file.tentzz_diag_data.fp, "%.7f,", *((float*)buff));
			break;

		case DIAG_DATA_PERCENT_MAIN_FREQ1:
			fprintf(pw_file.tentzz_diag_data.fp, "%.7f,", *((float*)buff));
			break;

		case DIAG_DATA_MAIN_FREQ1:
			fprintf(pw_file.tentzz_diag_data.fp, "%.7f,", *((float*)buff));
			break;

		case DIAG_DATA_MAX_FFT_AMP2:
			fprintf(pw_file.tentzz_diag_data.fp, "%.7f,", *((float*)buff));
			break;

		case DIAG_DATA_PERCENT_MAIN_FREQ2:
			fprintf(pw_file.tentzz_diag_data.fp, "%.7f,", *((float*)buff));
			break;

		case DIAG_DATA_MAIN_FREQ2:
			fprintf(pw_file.tentzz_diag_data.fp, "%.7f,", *((float*)buff));
			break;

		case DIAG_DATA_MAX_FABS_AMP3:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.tentzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_MAX_FFT_AMP3:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.tentzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_PERCENT_MAIN_FREQ3:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.tentzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_MAIN_FREQ3:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.tentzz_diag_data.fp, "%.7f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_AVERAGE_SPEED:
			fprintf(pw_file.tentzz_diag_data.fp, "%6.2f,", *((float*)buff));
			break;

		case DIAG_DATA_BRIDGE_LEN:
			for(i=0; i<size; i++)
			{
				fprintf(pw_file.tentzz_diag_data.fp, "%.1f,", *((float*)buff+i));
			}
			break;

		case DIAG_DATA_AVERAGE_BRIDGE_LEN:
			fprintf(pw_file.tentzz_diag_data.fp, "%.1f,", *((float*)buff));
			break;

		case DIAG_DATA_BRIDGE_FLAG:
			fprintf(pw_file.tentzz_diag_data.fp, "%d,", *((uint8_t*)buff));
			break;
	}

	return;
}

void close_tentzz_diag_file(SAVE_DATA_TYPE type)
{
	printf("close_tentzz_diag_file\n");

//		fsync(pw_file.tentzz_diag_data.fd);
	fclose(pw_file.tentzz_diag_data.fp);
	pw_file.tentzz_diag_data.fp = NULL;
//		pw_file.tentzz_diag_data.fd = -1;

	return;
}
#endif
#endif//end  DIAG_DATA_SAVE_FOR_TEST

#ifdef CAN_ERR_REBOOT_TWO_TIMES
//void create_test_ini_file(void)
//{
//    FILE    *   ini ;
//    struct SYSTEM_REBOOT_LOG out_data;
//
////	if(access(LOCAL_LOG_DIR,F_OK)!= 0)
////	{
////		create_dir(LOCAL_LOG_DIR);
////	}
//
//    ini = fopen(LOCAL_SYSTEM_TEST_LOG, "w");
//
//    fprintf(ini,
//    "[REBOOT]\n"
//	"Mode  = 0 ;\n"
//	"Count = 0 ;\n"
//    "\n");
//    fclose(ini);
//
////    read_test_ini_file(LOCAL_SYSTEM_TEST_LOG, &out_data);
////
////	printf("update_reboot_log_file---file open---read---m:%d,c:%d\n",
////					out_data.system_reboot_mode, out_data.software_reboot_cnt);
//
//}

int read_test_ini_file(char * ini_name, struct SYSTEM_REBOOT_LOG *reboot)
{
    dictionary  *   ini ;

    ini = iniparser_load(ini_name);
    if (ini==NULL) {
        fprintf(stderr, "cannot parse file: %s\n", ini_name);
        return -1 ;
    }

    iniparser_dump(ini, stderr);

    reboot->system_reboot_mode = iniparser_getint(ini, "REBOOT:Mode", -1);

    reboot->software_reboot_cnt = iniparser_getint(ini, "REBOOT:Count", -1);

    iniparser_freedict(ini);

    return 0 ;
}

int write_test_ini_file(char * ini_name, struct SYSTEM_REBOOT_LOG *reboot)
{
    FILE    *   ini ;

    ini = fopen(LOCAL_SYSTEM_TEST_LOG, "w");
    if (ini==NULL) {
		fprintf(stderr, "cannot write file: %s\n", ini_name);
		return -1;
	}
//	fprintf(ini,
//	"[REBOOT]\n"
//	"Mode  = %d ;\n"
//	"Count = %d ;\n"
//	"\n", reboot->system_reboot_mode, reboot->software_reboot_cnt);

    if(reboot->system_reboot_mode == 0 && reboot->software_reboot_cnt == 0 )
    {
		fprintf(ini,
		"[REBOOT]\n"
		"Mode  = 0 ;\n"
		"Count = 0 ;\n"
		"\n");
    }
    else if(reboot->system_reboot_mode == 0 && reboot->software_reboot_cnt == 1 )
    {
		fprintf(ini,
		"[REBOOT]\n"
		"Mode  = 0 ;\n"
		"Count = 1 ;\n"
		"\n");
    }
    else if(reboot->system_reboot_mode == 0 && reboot->software_reboot_cnt == 2 )
	{
		fprintf(ini,
		"[REBOOT]\n"
		"Mode  = 0 ;\n"
		"Count = 2 ;\n"
		"\n");
	}
    else if(reboot->system_reboot_mode == 1 && reboot->software_reboot_cnt == 0 )
    {
		fprintf(ini,
		"[REBOOT]\n"
		"Mode  = 1 ;\n"
		"Count = 0 ;\n"
		"\n");
    }
    else if(reboot->system_reboot_mode == 1 && reboot->software_reboot_cnt == 1 )
	{
		fprintf(ini,
		"[REBOOT]\n"
		"Mode  = 1 ;\n"
		"Count = 1 ;\n"
		"\n");
	}
    else if(reboot->system_reboot_mode == 1 && reboot->software_reboot_cnt == 2 )
    {
		fprintf(ini,
		"[REBOOT]\n"
		"Mode  = 1 ;\n"
		"Count = 2 ;\n"
		"\n");
    }
    else if(reboot->system_reboot_mode == 0 && reboot->software_reboot_cnt == 3 )
	{
		fprintf(ini,
		"[REBOOT]\n"
		"Mode  = 0 ;\n"
		"Count = 3 ;\n"
		"\n");
	}
    else if(reboot->system_reboot_mode == 1 && reboot->software_reboot_cnt == 3 )
    {
		fprintf(ini,
		"[REBOOT]\n"
		"Mode  = 1 ;\n"
		"Count = 3 ;\n"
		"\n");
    }


    fclose(ini);
}

void cteate_reboot_log_file()
{
    FILE    *   ini ;
    struct SYSTEM_REBOOT_LOG out_data;
    struct SYSTEM_REBOOT_LOG in_data;

	if((store_flag.hdd_exist_flag == FALSE) || store_flag.hdd_save_flag == FALSE)
		return -1;

    software_reboot_enable = 1;

	if(access(LOCAL_SYSTEM_TEST_LOG,F_OK)!= 0)
	{
		printf("cteate_reboot_log_file---file not exist---cteate file!\n");

		ini = fopen(LOCAL_SYSTEM_TEST_LOG, "w");
	    if (ini==NULL) {
	        fprintf(stderr, "cannot parse file: %s\n", LOCAL_SYSTEM_TEST_LOG);
	        return;
	    }

	    fprintf(ini,
	    "[REBOOT]\n"
		"Mode  = 0 ;\n"
		"Count = 0 ;\n"
	    "\n");
	    fclose(ini);
	}
	else
	{
//		printf("cteate_reboot_log_file---file exist!\n");

		if(read_test_ini_file(LOCAL_SYSTEM_TEST_LOG, &out_data) < 0)
			return;

		if(!out_data.system_reboot_mode)
		{
//			printf("cteate_reboot_log_file---file exist---read---m0\n");
			in_data.software_reboot_cnt = 0;

			write_test_ini_file(LOCAL_SYSTEM_TEST_LOG, &in_data);
			return;
		}
		else if(out_data.system_reboot_mode > 1)
		{
			in_data.system_reboot_mode = 0;
			in_data.software_reboot_cnt = 0;
			write_test_ini_file(LOCAL_SYSTEM_TEST_LOG, &in_data);
			return;
		}
		else
		{
			printf("cteate_reboot_log_file---file exist---read---m:%d,c:%d,f:%d\n",
					out_data.system_reboot_mode, out_data.software_reboot_cnt, software_reboot_enable);

			in_data.system_reboot_mode = 0;
			in_data.software_reboot_cnt = out_data.software_reboot_cnt;
			in_data.software_reboot_cnt++;

			if(in_data.software_reboot_cnt >= 2)
			{
//				in_data.software_reboot_cnt = 0;
				software_reboot_enable = 0;
			}
			else
				software_reboot_enable = 1;


			printf("cteate_reboot_log_file---file exist---write---m:%d,c:%d,f:%d\n",
					in_data.system_reboot_mode, in_data.software_reboot_cnt, software_reboot_enable);

			if(write_test_ini_file(LOCAL_SYSTEM_TEST_LOG, &in_data) < 0)
				return;
		}
	}
}

void update_reboot_log_file()//reboot
{
	 struct SYSTEM_REBOOT_LOG out_data;
	 struct SYSTEM_REBOOT_LOG in_data;

//	 printf("update_reboot_log_file---entry\n",software_reboot_enable);

//	 if(access(LOCAL_SYSTEM_TEST_LOG,F_OK)== 0)
//	 {
//		printf("update_reboot_log_file---entry---f:%d\n", software_reboot_enable);
		if(software_reboot_enable)
		{
			if(read_test_ini_file(LOCAL_SYSTEM_TEST_LOG, &out_data) < 0)
				return;

			printf("update_reboot_log_file---file open---read---m:%d,c:%d,f:%d\n",
					out_data.system_reboot_mode, out_data.software_reboot_cnt, software_reboot_enable);

			in_data.system_reboot_mode = 1;
//			if(out_data.software_reboot_cnt > 2)
//				in_data.software_reboot_cnt = 0;
//			else
			in_data.software_reboot_cnt = out_data.software_reboot_cnt;

			printf("update_reboot_log_file---file open---write---m:%d,c:%d,f:%d\n",
					in_data.system_reboot_mode, in_data.software_reboot_cnt, software_reboot_enable);

			if(write_test_ini_file(LOCAL_SYSTEM_TEST_LOG, &in_data) < 0)
				return;

			return;
		}
//	 }

	 return;
}

int first_software_reboot_times()
{
	struct SYSTEM_REBOOT_LOG out_data;

	if(access(LOCAL_SYSTEM_TEST_LOG,F_OK) == 0)
	{
		if(read_test_ini_file(LOCAL_SYSTEM_TEST_LOG, &out_data) >= 0)
		{
			if(out_data.system_reboot_mode == 0 && out_data.software_reboot_cnt == 1)//1th software reboot
				return 0;
		}
		else
			return -1;
	}
	return -1;
}
#endif

//void open_err_log_data_file_second()				//每天建1个log
//{
//		char dir_name[256];
//		char file_type[10];
//		char file_name[32];
//		char date_dir[10];
//		char date_name[10];
//
//		if(access(PW_ERR_LOG_DATA_DIR,F_OK)!= 0)
//		{
//			if(creat_dir(PW_LOG_DATA_DIR) < 0)
//			{
//				return;
//			}
//		}
//		//补全文件路径名
//		strcpy(dir_name,PW_ERR_LOG_DATA_DIR);
//		DEBUG("PW_LOG_DATA_DIR_one:%s\n",dir_name);
////		create_ymd_hms_dir_name(date_dir);
////		strcat(dir_name,date_dir);
////		DEBUG("PW_LOG_DATA_DIR_two:%s\n",dir_name);
////
////		if(creat_dir(dir_name) < 0)
////		{
////			return;
////		}
//
//
//		//	//创建平稳日志文件
//		if(pw_file.err_log_data.fp != NULL)
//		{
//			fclose(pw_file.err_log_data.fp);
//			pw_file.err_log_data.fp = NULL;
//		}
//		//补全文件路径及文件名
//		strcpy(file_name,"errr_log2");
////		create_ymdhms_str(date_name);
////		strcat(file_name,date_name);
//		strcpy(file_type,".dat");
//		strcat(file_name,file_type);
//
//		strcat(dir_name,file_name);
//		DEBUG("PW_ERR_LOG_DATA_DIR_three:%s\n",dir_name);
//
//		//pw_file.pw_original_data.fd = fopen(dir_name,"w+");
//		pw_file.err_log_data.fp = fopen(dir_name,"ab+");
//		if(pw_file.err_log_data.fp == NULL)
//		{
//			DEBUG("pw_file.pw_err_log_data2 file open fail\n");
//			return;
//		}
//		pw_file.err_log_data.fd = fileno(pw_file.err_log_data.fp);
//}


void open_test_data_file()				//test 验证AD数据使用
{
		char dir_name[256];
		char file_type[10];
		char file_name[32];
		char date_dir[32];
		if(access(PW_TEST_DATA_DIR,F_OK)!= 0)
		{
			if(creat_dir(PW_TEST_DATA_DIR) < 0)
			{
				return;
			}
		}
		//补全文件路径名
		strcpy(dir_name,PW_TEST_DATA_DIR);
		DEBUG("PW_TEST_DATA_DIR_one:%s\n",dir_name);
		create_ymd_hms_dir_name(date_dir);
		strcat(dir_name,date_dir);
		DEBUG("PW_TEST_DATA_DIR_two:%s\n",dir_name);

		if(creat_dir(dir_name) < 0)
		{
			return;
		}

		if(pw_file.test_data.fp != NULL)
		{
			fclose(pw_file.test_data.fp);
			pw_file.test_data.fp = NULL;
		}
		//补全文件路径及文件名
		strcpy(file_name,PW_TEST_DATA_NAME);
		//create_ymdhms_str(file_name);
		strcpy(file_type,".txt");
		strcat(file_name,file_type);

		strcat(dir_name,file_name);
		DEBUG("PW_TEST_DATA_DIR_three:%s\n",dir_name);

		//pw_file.pw_original_data.fd = fopen(dir_name,"w+");
		pw_file.test_data.fp = fopen(dir_name,"w+");

		if(pw_file.test_data.fp == NULL)
		{
			DEBUG("pw_file.pw_test_data file open fail\n");
			return;
		}
		pw_file.test_data.fd = fileno(pw_file.test_data.fp);
}

void open_check_data_file()				//HZ 验证FFT数据使用
{
		char dir_name[256];
		char file_type[10];
		char file_name[32];
		char date_dir[32];
		if(access(PW_TEST_DATA_DIR,F_OK)!= 0)
		{
			if(creat_dir(PW_TEST_DATA_DIR) < 0)
			{
				return;
			}
		}
		//补全文件路径名
		strcpy(dir_name,PW_TEST_DATA_DIR);
		DEBUG("PW_TEST_DATA_DIR_one:%s\n",dir_name);
		create_ymd_hms_dir_name(date_dir);
		strcat(dir_name,date_dir);
		DEBUG("PW_TEST_DATA_DIR_two:%s\n",dir_name);

		if(creat_dir(dir_name) < 0)
		{
			return;
		}


		if(pw_file.check_data.fp != NULL)
		{
			fclose(pw_file.check_data.fp);
			pw_file.check_data.fp = NULL;
		}
		//补全文件路径及文件名
		strcpy(file_name,PW_TEST_HZ_NAME);
		//create_ymdhms_str(file_name);
		strcpy(file_type,".txt");
		strcat(file_name,file_type);

		strcat(dir_name,file_name);
		DEBUG("PW_TEST_DATA_DIR_three:%s\n",dir_name);

		//pw_file.pw_original_data.fd = fopen(dir_name,"w+");
		pw_file.check_data.fp = fopen(dir_name,"w+");

		DEBUG("PW_TEST_DATA_DIR_three-1:%s\n",dir_name);
		if(pw_file.check_data.fp == NULL)
		{
			DEBUG("pw_file.check_data file open fail\n");
			return;
		}
		DEBUG("PW_TEST_DATA_DIR_three-2:%s\n",dir_name);
		pw_file.check_data.fd = fileno(pw_file.check_data.fp);
		DEBUG("PW_TEST_DATA_DIR_three-3:%s\n",dir_name);
}



void pw_save_thread_entry()
{
	int sem_res = -1;
//	uint16_t save_num = 0;
	sem_res = sem_init(&pw_save_sem,0,0);
	if(sem_res == -1)
	{
		DEBUG ("init pw_save_sem error!\n");
	}

	while(1)
	{
		//
		sem_wait(&pw_save_sem);
	}
}

int init_pw_save_thread()
{
	pthread_t pw_save_thread_id;
	int ret = -1;
	ret=pthread_create(&pw_save_thread_id,NULL,(void *)pw_save_thread_entry,NULL);
	if(ret!=0)
	 {
	 	DEBUG ("Create pw save thread error!\n");
	 }
	 	return ret;
}


/*************************************************
Function:  wirte_log_data
Description: 记录日志数据
Input: 	err_event_log　故障事件，err_des_log　故障事件描述
Output:
Return: 无
Others:
*************************************************/

void wirte_err_log_data(enum ERR_EVENT err_event_log,enum SENSOR_MEASURING_POINT sensor_measuring_point,enum ERR_DES err_des_log)
{
	struct LOCAL_TIME time_now;
	struct stat file_info;
	uint16_t total_err_data_num = 1000;
	uint8_t cnt_i = 0;
	uint32_t sum_temp = 0;
	uint8_t data_buff[16] = {0};
	uint16_t err_data_num = 0;
	uint8_t read_nmemb = 1;	//读取故障日志条数的元素个数

	data_buff[0] = 0x55;
	data_buff[1] = 0xAA;
	*(uint16_t *)&data_buff[2] = htons(sizeof(data_buff));		//将长度数据转化为大端

	get_local_time(&time_now);

	data_buff[4] = time_now.year %2000;
	data_buff[5] = (uint8_t)time_now.mon;
	data_buff[6] = (uint8_t)time_now.day;
	data_buff[7] = (uint8_t)time_now.hour;
	data_buff[8] = (uint8_t)time_now.min;
	data_buff[9] = (uint8_t)time_now.sec;
	data_buff[10] = err_event_log;
	data_buff[11] = err_des_log;
	data_buff[12] = 0;				//固定为0
	data_buff[13] = sensor_measuring_point;

	for(cnt_i = 0;cnt_i < 14;cnt_i++)
	{
		sum_temp += data_buff[cnt_i];
	}

	*(uint32_t *)&data_buff[14] = htons(sum_temp);

	//printf("-------before pw_file.err_log_data.fp fopen---------\n");
	pw_file.err_log_data.fp = fopen(PW_ERR_LOG_DATA_NAME,"rb+");		//打开一个二进制文件,文件必需存在,允许读写
	if(pw_file.err_log_data.fp  == NULL)
	{
		return;
	}
	pw_file.err_log_data.fd = fileno(pw_file.err_log_data.fp);

	//获取文件当前大小
	if(stat(PW_ERR_LOG_DATA_NAME,&file_info) == -1)
	{

		fclose(pw_file.err_log_data.fp);
		pw_file.err_log_data.fp  = NULL;
		pw_file.err_log_data.fd = -1;
		return;
	}
//------>ctrl_A_ETH<-------
//	printf("------------>file_info.st_size<------:%d\n",file_info.st_size);
	if(file_info.st_size > 0)
	{
		//偏移到文件末尾的两个字节
		//文件末尾的两个字节存储了故障日志的条数
//		printf("-------before pw_file.err_log_data.fp fseek1---------\n");
		if(fseek(pw_file.err_log_data.fp,-2L,SEEK_END)!=0)
		{
			fclose(pw_file.err_log_data.fp);
			pw_file.err_log_data.fp  = NULL;
			pw_file.err_log_data.fd = -1;
			return;
		}
		//读取故障日志条数
//		printf("-------before pw_file.err_log_data.fp fread---------\n");
		if(fread(&err_data_num,sizeof(uint16_t),read_nmemb,pw_file.err_log_data.fp)!=read_nmemb)
		{
			fclose(pw_file.err_log_data.fp);
			pw_file.err_log_data.fp  = NULL;
			pw_file.err_log_data.fd = -1;
			return;
		}
//		printf("err_data_num:%d\n",err_data_num);

		if(err_data_num >= 65535)
		{
			err_data_num = 0;
		}
	}


	//从文件的起始位置偏移到已存储的故障条数末尾
//	printf("-------before pw_file.err_log_data.fp fseek---------\n");
	if(fseek(pw_file.err_log_data.fp,(err_data_num%total_err_data_num)<<4,SEEK_SET)!=0)
	{
		fclose(pw_file.err_log_data.fp);
		pw_file.err_log_data.fp  = NULL;
		pw_file.err_log_data.fd = -1;
		return;
	}

	//写入故障数据
//	printf("-------before pw_file.err_log_data.fp fwrite---------1\n");
	if(fwrite(data_buff,sizeof(data_buff[0]),sizeof(data_buff)/sizeof(data_buff[0]),pw_file.err_log_data.fp) != (sizeof(data_buff)/sizeof(data_buff[0])))
	{
		fclose(pw_file.err_log_data.fp);
		pw_file.err_log_data.fp  = NULL;
		pw_file.err_log_data.fd = -1;
		return;
	}

	//偏移到文件末尾的前２个字节
//	printf("-------before pw_file.err_log_data.fp fseek---------\n");

	if(err_data_num < total_err_data_num)
	{
		if(fseek(pw_file.err_log_data.fp,0L,SEEK_END)!=0)
		{
			fclose(pw_file.err_log_data.fp);
			pw_file.err_log_data.fp  = NULL;
			pw_file.err_log_data.fd = -1;
			return;
		}
	}
	else
	{
		//存储满total_err_data_num 条故障记录后,数据文件的字节数不再增加
		//每次在固定位置更新存储条数记录
		if(fseek(pw_file.err_log_data.fp,-2L,SEEK_END)!=0)
		{
			fclose(pw_file.err_log_data.fp);
			pw_file.err_log_data.fp  = NULL;
			pw_file.err_log_data.fd = -1;
			return;
		}
	}
	//before pw_file.err_log_data.fp
	//存储故障日志条数计数
	err_data_num++;
//	printf("-------before pw_file.err_log_data.fp fwrite---------2\n");
	if(fwrite(&err_data_num,sizeof(uint16_t),read_nmemb,pw_file.err_log_data.fp)!=read_nmemb)
	{
		printf("fwrite   err_data_num  err\n");
		fclose(pw_file.err_log_data.fp);
		pw_file.err_log_data.fp  = NULL;
		pw_file.err_log_data.fd = -1;
		return;
	}

	fsync(pw_file.err_log_data.fd);
	fclose(pw_file.err_log_data.fp);
	pw_file.err_log_data.fp  = NULL;
	pw_file.err_log_data.fd = -1;

	//
}



/*
 *更新故障日志
 * */
void update_err_log_data(struct SYS_STATUS_CNT sys_status)
{
//	static struct UPDATE_ERR_LOG_FLAG update_err_log_flag;
	uint8_t sensor_ch = 0;
	uint8_t sensor_measuring_temp[6] = {
			PW_SENSOR_ONE_BIT_Y,PW_SENSOR_ONE_BIT_Z,PW_SENSOR_ONE_BIT_X,PW_SENSOR_TWO_BIT_Y,PW_SENSOR_TWO_BIT_Z,PW_SENSOR_TWO_BIT_X
	};


	//板卡状态日志记录
	if(sys_status.board_err_save_flag == 1 && update_err_log_flag.board_err_update_log_flag == 0)
	{
		update_err_log_flag.board_err_update_log_flag = 1;
		update_err_log_flag.board_normal_update_log_flag = 0;
		wirte_err_log_data(ERR,SENSOR_MEASURING_POINT_EMPTY,BOARD_ERR);
	}
	if(sys_status.board_normal_save_flag == 1 && update_err_log_flag.board_normal_update_log_flag == 0)
	{
		if(update_err_log_flag.board_err_update_log_flag)
		{
			wirte_err_log_data(OK,SENSOR_MEASURING_POINT_EMPTY,BOARD_ERR_REMOVE);
		}
		update_err_log_flag.board_err_update_log_flag = 0;
		update_err_log_flag.board_normal_update_log_flag = 1;
	}

	//存储状态日志记录
	if(sys_status.save_err_save_flag == 1 && update_err_log_flag.save_err_update_log_flag == 0)
	{
		update_err_log_flag.save_err_update_log_flag = 1;
		update_err_log_flag.save_normal_update_log_flag = 0;
		wirte_err_log_data(ERR,SENSOR_MEASURING_POINT_EMPTY,SAVE_ERR);
	}
	if(sys_status.save_normal_save_flag == 1 && update_err_log_flag.save_normal_update_log_flag == 0)
	{
		if(update_err_log_flag.save_err_update_log_flag)
		{
			wirte_err_log_data(OK,SENSOR_MEASURING_POINT_EMPTY,SAVE_ERR_REMOVE);
		}
		update_err_log_flag.save_err_update_log_flag = 0;
		update_err_log_flag.save_normal_update_log_flag = 1;
	}

	//电源支路１状态日志记录
	if(sys_status.power_branch1_err_save_flag == 1 && update_err_log_flag.power_branch1_err_update_log_flag == 0)
	{
		update_err_log_flag.power_branch1_err_update_log_flag = 1;
		update_err_log_flag.power_branch1_normal_update_log_flag = 0;
		wirte_err_log_data(ERR,SENSOR_MEASURING_POINT_EMPTY,POWER_BRANCH1_ERR);
	}
	if(sys_status.power_branch1_normal_save_flag == 1 && update_err_log_flag.power_branch1_normal_update_log_flag == 0)
	{
		if(update_err_log_flag.power_branch1_err_update_log_flag)
		{
			wirte_err_log_data(OK,SENSOR_MEASURING_POINT_EMPTY,POWER_BRANCH1_ERR_REMOVE);
		}
		update_err_log_flag.power_branch1_err_update_log_flag = 0;
		update_err_log_flag.power_branch1_normal_update_log_flag = 1;
	}

	//电源支路2状态日志记录
	if(sys_status.power_branch2_err_save_flag == 1 && update_err_log_flag.power_branch2_err_update_log_flag == 0)
	{
		update_err_log_flag.power_branch2_err_update_log_flag = 1;
		update_err_log_flag.power_branch2_normal_update_log_flag = 0;
		wirte_err_log_data(ERR,SENSOR_MEASURING_POINT_EMPTY,POWER_BRANCH2_ERR);
	}
	if(sys_status.power_branch2_normal_save_flag == 1 && update_err_log_flag.power_branch2_normal_update_log_flag == 0)
	{
		if(update_err_log_flag.power_branch2_err_update_log_flag)
		{
			wirte_err_log_data(OK,SENSOR_MEASURING_POINT_EMPTY,POWER_BRANCH2_ERR_REMOVE);
		}
		update_err_log_flag.power_branch2_err_update_log_flag = 0;
		update_err_log_flag.power_branch2_normal_update_log_flag = 1;
	}

#ifdef AD_REF_VOLT_ERR_REBOOT
	//AD电源日志记录
	if(sys_status.power_ad_err_save_flag == 1 && update_err_log_flag.power_ad_err_update_log_flag == 0)
	{
		update_err_log_flag.power_ad_err_update_log_flag = 1;
		update_err_log_flag.power_ad_normal_update_log_flag = 0;
		wirte_err_log_data(ERR,SENSOR_MEASURING_POINT_EMPTY,PW_AD_POWER_ERR);
	}
	if(sys_status.power_ad_normal_save_flag == 1 && update_err_log_flag.power_ad_normal_update_log_flag == 0)
	{
		if(update_err_log_flag.power_ad_err_update_log_flag)
		{
			wirte_err_log_data(OK,SENSOR_MEASURING_POINT_EMPTY,PW_AD_POWER_ERR_REMOVE);
		}
		update_err_log_flag.power_ad_err_update_log_flag = 0;
		update_err_log_flag.power_ad_normal_update_log_flag = 1;
	}
#endif

	//与控制Ａ的以太网状态日志记录
	if(sys_status.ctrla_eth_err_save_flag == 1 && update_err_log_flag.ctrla_eth_err_update_log_flag == 0)
	{
		update_err_log_flag.ctrla_eth_err_update_log_flag = 1;
		update_err_log_flag.ctrla_eth_normal_update_log_flag = 0;
		wirte_err_log_data(ERR,SENSOR_MEASURING_POINT_EMPTY,CTRLA_ETH_ERR);
	}
	if(sys_status.ctrla_eth_normal_save_flag == 1 && update_err_log_flag.ctrla_eth_normal_update_log_flag == 0)
	{
		if(update_err_log_flag.ctrla_eth_err_update_log_flag)
		{
			wirte_err_log_data(OK,SENSOR_MEASURING_POINT_EMPTY,CTRLA_ETH_ERR_REMOVE);
		}
		update_err_log_flag.ctrla_eth_err_update_log_flag = 0;
		update_err_log_flag.ctrla_eth_normal_update_log_flag = 1;
	}

	//与控制Ａ的CAN状态日志记录
	if(sys_status.ctrla_can_err_save_flag == 1 && update_err_log_flag.ctrla_can_err_update_log_flag == 0)
	{
		update_err_log_flag.ctrla_can_err_update_log_flag = 1;
		update_err_log_flag.ctrla_can_normal_update_log_flag = 0;
		wirte_err_log_data(ERR,SENSOR_MEASURING_POINT_EMPTY,CTRLA_CAN_ERR);
	}
	if(sys_status.ctrla_can_normal_save_flag == 1 && update_err_log_flag.ctrla_can_normal_update_log_flag == 0)
	{
		if(update_err_log_flag.ctrla_can_err_update_log_flag)
		{
			wirte_err_log_data(OK,SENSOR_MEASURING_POINT_EMPTY,CTRLA_CAN_ERR_REMOVE);
		}
		update_err_log_flag.ctrla_can_err_update_log_flag = 0;
		update_err_log_flag.ctrla_can_normal_update_log_flag = 1;
	}

	//与控制B的以太网状态日志记录
	if(sys_status.ctrlb_eth_err_save_flag == 1 && update_err_log_flag.ctrlb_eth_err_update_log_flag == 0)
	{
		update_err_log_flag.ctrlb_eth_err_update_log_flag = 1;
		update_err_log_flag.ctrlb_eth_normal_update_log_flag = 0;
		wirte_err_log_data(ERR,SENSOR_MEASURING_POINT_EMPTY,CTRLB_ETH_ERR);
	}
	if(sys_status.ctrlb_eth_normal_save_flag == 1 && update_err_log_flag.ctrlb_eth_normal_update_log_flag == 0)
	{
		if(update_err_log_flag.ctrlb_eth_err_update_log_flag)
		{
			wirte_err_log_data(OK,SENSOR_MEASURING_POINT_EMPTY,CTRLB_ETH_ERR_REMOVE);
		}
		update_err_log_flag.ctrlb_eth_err_update_log_flag = 0;
		update_err_log_flag.ctrlb_eth_normal_update_log_flag = 1;
	}

	//与控制B的CAN状态日志记录
	if(sys_status.ctrlb_can_err_save_flag == 1 && update_err_log_flag.ctrlb_can_err_update_log_flag == 0)
	{
		update_err_log_flag.ctrlb_can_err_update_log_flag = 1;
		update_err_log_flag.ctrlb_can_normal_update_log_flag = 0;
		wirte_err_log_data(ERR,SENSOR_MEASURING_POINT_EMPTY,CTRLB_CAN_ERR);
	}
	if(sys_status.ctrlb_can_normal_save_flag == 1 && update_err_log_flag.ctrlb_can_normal_update_log_flag == 0)
	{
		if(update_err_log_flag.ctrlb_can_err_update_log_flag)
		{
			wirte_err_log_data(OK,SENSOR_MEASURING_POINT_EMPTY,CTRLB_CAN_ERR_REMOVE);
		}
		update_err_log_flag.ctrlb_can_err_update_log_flag = 0;
		update_err_log_flag.ctrlb_can_normal_update_log_flag = 1;
	}

	//与记录板的以太网状态日志记录
	if(sys_status.save_eth_err_save_flag == 1 && update_err_log_flag.save_eth_err_update_log_flag == 0)
	{
		update_err_log_flag.save_eth_err_update_log_flag = 1;
		update_err_log_flag.save_eth_normal_update_log_flag = 0;
		wirte_err_log_data(ERR,SENSOR_MEASURING_POINT_EMPTY,SAVE_ETH_ERR);
	}
	if(sys_status.save_eth_normal_save_flag == 1 && update_err_log_flag.save_eth_normal_update_log_flag == 0)
	{
		if(update_err_log_flag.save_eth_err_update_log_flag)
		{
			wirte_err_log_data(OK,SENSOR_MEASURING_POINT_EMPTY,SAVE_ETH_ERR_REMOVE);
		}
		update_err_log_flag.save_eth_err_update_log_flag = 0;
		update_err_log_flag.save_eth_normal_update_log_flag = 1;
	}

	//与记录板的CAN状态日志记录
	if(sys_status.save_can_err_save_flag == 1 && update_err_log_flag.save_can_err_update_log_flag == 0)
	{
		update_err_log_flag.save_can_err_update_log_flag = 1;
		update_err_log_flag.save_can_normal_update_log_flag = 0;
		wirte_err_log_data(ERR,SENSOR_MEASURING_POINT_EMPTY,SAVE_CAN_ERR);
	}
	if(sys_status.save_can_normal_save_flag == 1 && update_err_log_flag.save_can_normal_update_log_flag == 0)
	{
		if(update_err_log_flag.save_can_err_update_log_flag)
		{
			wirte_err_log_data(OK,SENSOR_MEASURING_POINT_EMPTY,SAVE_CAN_ERR_REMOVE);
		}
		update_err_log_flag.save_can_err_update_log_flag = 0;
		update_err_log_flag.save_can_normal_update_log_flag = 1;
	}

	for(sensor_ch = 0;sensor_ch < 6;sensor_ch++)
	{
		//传感器振动状态日志记录
		if(sys_status.err_type[sensor_ch].sensor_err_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].sensor_err_update_log_flag == 0)
		{
			update_err_log_flag.update_err_type_log[sensor_ch].sensor_err_update_log_flag = 1;
			update_err_log_flag.update_err_type_log[sensor_ch].sensor_normal_update_log_flag = 0;
			wirte_err_log_data(ERR,sensor_measuring_temp[sensor_ch],ACC_SENSOR_ERR);
		}
		if(sys_status.err_type[sensor_ch].sensor_normal_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].sensor_normal_update_log_flag == 0)
		{
			if(update_err_log_flag.update_err_type_log[sensor_ch].sensor_err_update_log_flag)
			{
				wirte_err_log_data(OK,sensor_measuring_temp[sensor_ch],ACC_SENSOR_ERR_REMOVE);
			}
			update_err_log_flag.update_err_type_log[sensor_ch].sensor_err_update_log_flag = 0;
			update_err_log_flag.update_err_type_log[sensor_ch].sensor_normal_update_log_flag = 1;
		}



		//平稳性算法诊断正常日志记录
		if(sys_status.err_type[sensor_ch].diag_normal_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].diag_normal_update_log_flag == 0)
		{
			if(update_err_log_flag.update_err_type_log[sensor_ch].diag_warn_update_log_flag || update_err_log_flag.update_err_type_log[sensor_ch].diag_alarm_update_log_flag)
			{
				wirte_err_log_data(OK,sensor_measuring_temp[sensor_ch],PW_DIAG_OK);
			}
			update_err_log_flag.update_err_type_log[sensor_ch].diag_normal_update_log_flag = 1;
//			update_err_log_flag.update_err_type_log[sensor_ch].diag_predict_update_log_flag = 0;
			update_err_log_flag.update_err_type_log[sensor_ch].diag_warn_update_log_flag = 0;
			update_err_log_flag.update_err_type_log[sensor_ch].diag_alarm_update_log_flag = 0;
		}
		//平稳性算法诊断预判日志记录
//		if(sys_status.err_type[sensor_ch].diag_predict_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].diag_predict_update_log_flag == 0)
//		{
//			update_err_log_flag.update_err_type_log[sensor_ch].diag_normal_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].diag_predict_update_log_flag = 1;
//			wirte_err_log_data(PREDICT,sensor_measuring_temp[sensor_ch],PW_DIAG_PREDICT);
//		}
		//平稳性算法诊断预警日志记录
		if(sys_status.err_type[sensor_ch].diag_warn_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].diag_warn_update_log_flag == 0)
		{
			update_err_log_flag.update_err_type_log[sensor_ch].diag_normal_update_log_flag = 0;
			update_err_log_flag.update_err_type_log[sensor_ch].diag_warn_update_log_flag = 1;
			wirte_err_log_data(WARN,sensor_measuring_temp[sensor_ch],PW_DIAG_WARN);
		}

		//平稳性算法诊断报警日志记录
		if(sys_status.err_type[sensor_ch].diag_alarm_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].diag_alarm_update_log_flag == 0)
		{
			update_err_log_flag.update_err_type_log[sensor_ch].diag_normal_update_log_flag = 0;
			update_err_log_flag.update_err_type_log[sensor_ch].diag_alarm_update_log_flag = 1;
			wirte_err_log_data(ALARM,sensor_measuring_temp[sensor_ch],PW_DIAG_ALARM);
		}

		//晃车算法诊断正常日志记录
		if(sys_status.err_type[sensor_ch].shack_diag_normal_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].shack_diag_normal_update_log_flag == 0)
		{
			if(update_err_log_flag.update_err_type_log[sensor_ch].shack_diag_warn_update_log_flag || update_err_log_flag.update_err_type_log[sensor_ch].shack_diag_alarm_update_log_flag)
			{
				wirte_err_log_data(OK,sensor_measuring_temp[sensor_ch],HC_DIAG_OK);
			}
			update_err_log_flag.update_err_type_log[sensor_ch].shack_diag_normal_update_log_flag = 1;
			update_err_log_flag.update_err_type_log[sensor_ch].shack_diag_warn_update_log_flag = 0;
			update_err_log_flag.update_err_type_log[sensor_ch].shack_diag_alarm_update_log_flag = 0;
		}

		//晃车算法诊断预警日志记录
		if(sys_status.err_type[sensor_ch].shack_diag_warn_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].shack_diag_warn_update_log_flag == 0)
		{
			update_err_log_flag.update_err_type_log[sensor_ch].shack_diag_normal_update_log_flag = 0;
			update_err_log_flag.update_err_type_log[sensor_ch].shack_diag_warn_update_log_flag = 1;
			wirte_err_log_data(WARN,sensor_measuring_temp[sensor_ch],HC_DIAG_WARN);
		}

		//晃车算法诊断报警日志记录
		if(sys_status.err_type[sensor_ch].shack_diag_alarm_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].shack_diag_alarm_update_log_flag == 0)
		{
			update_err_log_flag.update_err_type_log[sensor_ch].shack_diag_normal_update_log_flag = 0;
			update_err_log_flag.update_err_type_log[sensor_ch].shack_diag_alarm_update_log_flag = 1;
			wirte_err_log_data(ALARM,sensor_measuring_temp[sensor_ch],HC_DIAG_ALARM);
		}

		//抖车算法诊断正常日志记录
		if(sys_status.err_type[sensor_ch].jitter_diag_normal_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].jitter_diag_normal_update_log_flag == 0)
		{
			if(update_err_log_flag.update_err_type_log[sensor_ch].jitter_diag_warn_update_log_flag || update_err_log_flag.update_err_type_log[sensor_ch].jitter_diag_alarm_update_log_flag)
			{
				wirte_err_log_data(OK,sensor_measuring_temp[sensor_ch],DC_DIAG_OK);
			}
			update_err_log_flag.update_err_type_log[sensor_ch].jitter_diag_normal_update_log_flag = 1;
			update_err_log_flag.update_err_type_log[sensor_ch].jitter_diag_warn_update_log_flag = 0;
			update_err_log_flag.update_err_type_log[sensor_ch].jitter_diag_alarm_update_log_flag = 0;
		}

		//抖车算法诊断预警日志记录
		if(sys_status.err_type[sensor_ch].jitter_diag_warn_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].jitter_diag_warn_update_log_flag == 0)
		{
			update_err_log_flag.update_err_type_log[sensor_ch].jitter_diag_normal_update_log_flag = 0;
			update_err_log_flag.update_err_type_log[sensor_ch].jitter_diag_warn_update_log_flag = 1;
			wirte_err_log_data(WARN,sensor_measuring_temp[sensor_ch],DC_DIAG_WARN);
		}

		//抖车算法诊断报警日志记录
		if(sys_status.err_type[sensor_ch].jitter_diag_alarm_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].jitter_diag_alarm_update_log_flag == 0)
		{
			update_err_log_flag.update_err_type_log[sensor_ch].jitter_diag_normal_update_log_flag = 0;
			update_err_log_flag.update_err_type_log[sensor_ch].jitter_diag_alarm_update_log_flag = 1;
			wirte_err_log_data(ALARM,sensor_measuring_temp[sensor_ch],DC_DIAG_ALARM);
		}

//		//内环诊断正常日志记录
//		if(sys_status.err_type[sensor_ch].inner_normal_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].inner_normal_update_log_flag == 0)
//		{
//			update_err_log_flag.update_err_type_log[sensor_ch].inner_normal_update_log_flag = 1;
//			update_err_log_flag.update_err_type_log[sensor_ch].inner_predict_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].inner_warn_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].inner_alarm_update_log_flag = 0;
//			wirte_err_log_data(OK,sensor_measuring_temp[sensor_ch],INNER_DIAG_OK);
//		}
//		//内环诊断预判日志记录
//		if(sys_status.err_type[sensor_ch].inner_predict_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].inner_predict_update_log_flag == 0)
//		{
//			update_err_log_flag.update_err_type_log[sensor_ch].inner_normal_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].inner_predict_update_log_flag = 1;
//			wirte_err_log_data(PREDICT,sensor_measuring_temp[sensor_ch],INNER_DIAG_PREDICT);
//		}
//		//内环诊断预警日志记录
//		if(sys_status.err_type[sensor_ch].inner_warn_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].inner_predict_update_log_flag == 0)
//		{
//			update_err_log_flag.update_err_type_log[sensor_ch].inner_normal_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].inner_warn_update_log_flag = 1;
//			wirte_err_log_data(WARN,sensor_measuring_temp[sensor_ch],INNER_DIAG_WARN);
//		}
//
//		//内环诊断报警日志记录
//		if(sys_status.err_type[sensor_ch].inner_alarm_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].inner_alarm_update_log_flag == 0)
//		{
//			update_err_log_flag.update_err_type_log[sensor_ch].inner_normal_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].inner_alarm_update_log_flag = 1;
//			wirte_err_log_data(ALARM,sensor_measuring_temp[sensor_ch],INNER_DIAG_ALARM);
//		}
//
//		//外环诊断正常日志记录
//		if(sys_status.err_type[sensor_ch].outter_normal_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].outter_normal_update_log_flag == 0)
//		{
//			update_err_log_flag.update_err_type_log[sensor_ch].outter_normal_update_log_flag = 1;
//			update_err_log_flag.update_err_type_log[sensor_ch].outter_predict_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].outter_warn_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].outter_alarm_update_log_flag = 0;
//			wirte_err_log_data(OK,sensor_measuring_temp[sensor_ch],OUTTER_DIAG_OK);
//		}
//		//外环诊断预判日志记录
//		if(sys_status.err_type[sensor_ch].outter_predict_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].outter_predict_update_log_flag == 0)
//		{
//			update_err_log_flag.update_err_type_log[sensor_ch].outter_normal_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].outter_predict_update_log_flag = 1;
//			wirte_err_log_data(PREDICT,sensor_measuring_temp[sensor_ch],OUTTER_DIAG_PREDICT);
//		}
//		//外环诊断预警日志记录
//		if(sys_status.err_type[sensor_ch].outter_warn_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].outter_predict_update_log_flag == 0)
//		{
//			update_err_log_flag.update_err_type_log[sensor_ch].outter_normal_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].outter_warn_update_log_flag = 1;
//			wirte_err_log_data(WARN,sensor_measuring_temp[sensor_ch],OUTTER_DIAG_WARN);
//		}
//
//		//外环诊断报警日志记录
//		if(sys_status.err_type[sensor_ch].outter_alarm_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].outter_alarm_update_log_flag == 0)
//		{
//			update_err_log_flag.update_err_type_log[sensor_ch].outter_normal_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].outter_alarm_update_log_flag = 1;
//			wirte_err_log_data(ALARM,sensor_measuring_temp[sensor_ch],OUTTER_DIAG_ALARM);
//		}
//
//		//滚动体诊断正常日志记录
//		if(sys_status.err_type[sensor_ch].roller_normal_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].roller_normal_update_log_flag == 0)
//		{
//			update_err_log_flag.update_err_type_log[sensor_ch].roller_normal_update_log_flag = 1;
//			update_err_log_flag.update_err_type_log[sensor_ch].roller_predict_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].roller_warn_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].roller_alarm_update_log_flag = 0;
//			wirte_err_log_data(OK,sensor_measuring_temp[sensor_ch],ROLLER_DIAG_OK);
//		}
//		//滚动体诊断预判日志记录
//		if(sys_status.err_type[sensor_ch].roller_predict_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].roller_predict_update_log_flag == 0)
//		{
//			update_err_log_flag.update_err_type_log[sensor_ch].roller_normal_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].roller_predict_update_log_flag = 1;
//			wirte_err_log_data(PREDICT,sensor_measuring_temp[sensor_ch],ROLLER_DIAG_PREDICT);
//		}
//		//滚动体诊断预警日志记录
//		if(sys_status.err_type[sensor_ch].roller_warn_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].roller_predict_update_log_flag == 0)
//		{
//			update_err_log_flag.update_err_type_log[sensor_ch].roller_normal_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].roller_warn_update_log_flag = 1;
//			wirte_err_log_data(WARN,sensor_measuring_temp[sensor_ch],ROLLER_DIAG_WARN);
//		}
//
//		//滚动体诊断报警日志记录
//		if(sys_status.err_type[sensor_ch].roller_alarm_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].roller_alarm_update_log_flag == 0)
//		{
//			update_err_log_flag.update_err_type_log[sensor_ch].roller_normal_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].roller_alarm_update_log_flag = 1;
//			wirte_err_log_data(ALARM,sensor_measuring_temp[sensor_ch],ROLLER_DIAG_ALARM);
//		}
//
//		//保持架诊断正常日志记录
//		if(sys_status.err_type[sensor_ch].holder_normal_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].holder_normal_update_log_flag == 0)
//		{
//			update_err_log_flag.update_err_type_log[sensor_ch].holder_normal_update_log_flag = 1;
//			update_err_log_flag.update_err_type_log[sensor_ch].holder_predict_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].holder_warn_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].holder_alarm_update_log_flag = 0;
//			wirte_err_log_data(OK,sensor_measuring_temp[sensor_ch],HOLDER_DIAG_OK);
//		}
//		//保持架诊断预判日志记录
//		if(sys_status.err_type[sensor_ch].holder_predict_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].holder_predict_update_log_flag == 0)
//		{
//			update_err_log_flag.update_err_type_log[sensor_ch].holder_normal_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].holder_predict_update_log_flag = 1;
//			wirte_err_log_data(PREDICT,sensor_measuring_temp[sensor_ch],HOLDER_DIAG_PREDICT);
//		}
//		//保持架诊断预警日志记录
//		if(sys_status.err_type[sensor_ch].holder_warn_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].holder_predict_update_log_flag == 0)
//		{
//			update_err_log_flag.update_err_type_log[sensor_ch].holder_normal_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].holder_warn_update_log_flag = 1;
//			wirte_err_log_data(WARN,sensor_measuring_temp[sensor_ch],HOLDER_DIAG_WARN);
//		}
//
//		//保持架诊断报警日志记录
//		if(sys_status.err_type[sensor_ch].holder_alarm_save_flag == 1 && update_err_log_flag.update_err_type_log[sensor_ch].holder_alarm_update_log_flag == 0)
//		{
//			update_err_log_flag.update_err_type_log[sensor_ch].holder_normal_update_log_flag = 0;
//			update_err_log_flag.update_err_type_log[sensor_ch].holder_alarm_update_log_flag = 1;
//			wirte_err_log_data(ALARM,sensor_measuring_temp[sensor_ch],HOLDER_DIAG_ALARM);
//		}
	}
}


/*************************************************
Function:  fsync_sys_fd
Description: 更新刷新系统的文件
Input: 	无
Output:
Return: 无
Others:
*************************************************/
void fsync_sys_fd(void)
{
	fsync(pw_file.pw_original_data.fd);
	fsync(sw_file.sw_original_data.fd);
#ifdef ADD_TZ_DATA_FILE
	fsync(pw_file.pw_tz_data.fd);
	fsync(sw_file.sw_tz_data.fd);
#endif
#ifdef ADD_DIAG_TZZ_DATA_FILE
	fsync(pw_file.pw_diag_tzz_data.fd);
#endif
#ifdef ADD_LOG_DATA
	fsync(pw_file.log_data.fd);
#endif
#ifdef CAN_ERR_REBOOT_TWO_TIMES
	fsync(pw_file.reboot.fd);
#endif
}

/*************************************************
Function:  close_sys_fd
Description: 更新刷新系统的文件
Input: 	无
Output:
Return: 无
Others:
*************************************************/
void close_sys_fd(void)
{
	close(pw_file.pw_original_data.fd);
#ifdef ADD_TZ_DATA_FILE
	close(pw_file.pw_tz_data.fd);
#endif
#ifdef ADD_DIAG_TZZ_DATA_FILE
	close(pw_file.pw_diag_tzz_data.fd);
#endif
#ifdef ADD_LOG_DATA
	close(pw_file.log_data.fd);
#endif
#ifdef CAN_ERR_REBOOT_TWO_TIMES
	close(pw_file.reboot.fd);
#endif
}


/*************************************************
Function:  getFsSize
Description:
Input: 	无
Output:
Return: 无
Others:
*************************************************/
int getFsSize(char *path)
{
	uint32_t freesize,totalsize;
	struct statfs myStatfs;

#if defined(HDD_DEV_MMCBLK0P3_OR_TF)
	uint8_t ret=-1;
	uint16_t FreeSizeMB = 0;

	ret = access(HDD_DEV, F_OK);
    if(ret == 0)
    {
    	//ret = access(HDD_DEV1, F_OK);
    	FreeSizeMB = HDD_DEV1_FREESIZE;//(23*1024)MB
    }
    else
    {
    	//ret = access(HDD_DEV2, F_OK);
    	FreeSizeMB = HDD_DEV2_FREESIZE;//MB
    }
#endif

	if (statfs(path, &myStatfs) == -1) {
		return -1;
	}
	printf("******SIZE f_blocks=%ld f_bfree=%ld******\n",myStatfs.f_blocks,myStatfs.f_bfree);
	freesize = (4 * myStatfs.f_bfree/1024);//MB
	totalsize = (4 * myStatfs.f_blocks/1024 );//MB
	printf("******SDK totalsize=%d MB  freesize=%d MB******\n",totalsize,freesize);

#if defined(HDD_DEV_MMCBLK0P3_OR_TF)
	if(freesize<=FreeSizeMB)
#else
	if(freesize<=HDD_DEV_FREESIZE)
#endif
		return 0;
	else
		return -1;
}

/*************************************************
Function:  delete_sdk_file
Description:
Input: 	无
Output:
Return: 0:ok  -1:err
Others:
*************************************************/
int delete_sdk_file(void)
{
	DIR * dir_t;
	struct dirent * ptr=NULL;
	char min_names[64];
	char dir_file[128]=PW_DELETE_DIR;
	uint8_t first_use_flag=0;

	dir_t = opendir(PW_OPEN_DIR);
	if(dir_t==NULL)
		return -1;

	while((ptr = readdir(dir_t)) != NULL)
	{
		if(strstr(ptr->d_name,"."))
			continue;
		if(strstr(ptr->d_name,PW_INITTIME_DIR))
			continue;

		if(!first_use_flag)
		{
			strcpy(min_names,ptr->d_name);

			first_use_flag=1;
		}
		else
		{
			if(strcmp(min_names,ptr->d_name)>0)				//找出时间最早的文件
			{
				strcpy(min_names,ptr->d_name);
			}
		}
//		printf("******%s******\n",ptr->d_name);
	}

	strcat(dir_file,min_names);
	printf("******%s******\n",dir_file);
	closedir(dir_t);

//	return remove_dir(dir_file);

	if(my_system(dir_file)==0)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}


int delete_sdk_file_sw(void)
{
	DIR *dir_t;
	struct dirent * ptr=NULL;
	char min_names[64];
	char dir_file[128]=SW_DELETE_DIR;
	uint8_t first_use_flag=0;

	dir_t = opendir(SW_OPEN_DIR);
	if(dir_t==NULL)
		return -1;

	while((ptr = readdir(dir_t)) != NULL)
	{
		if(strstr(ptr->d_name,"."))
			continue;
		if(strstr(ptr->d_name,SW_INITTIME_DIR))
			continue;

		if(!first_use_flag)
		{
			strcpy(min_names,ptr->d_name);
			first_use_flag=1;
		}
		else
		{
			if(strcmp(min_names,ptr->d_name)>0)
			{
				strcpy(min_names,ptr->d_name);
			}
		}
		printf("******%s******\n",ptr->d_name);
	}

	strcat(dir_file,min_names);
	printf("******%s******\n",dir_file);
	closedir(dir_t);

	if(system(dir_file)==0)
		return 0;
	else
		return -1;

}

void init_pw_save()
{
//	FILE *fp=NULL;

#ifdef SD_CARD_SOFTWARE_MOUNT
	check_hdd_test();
#endif

	init_file_sys_para();
	//初始化故障数据记录
	init_pw_file_para(&pw_file.err_log_data);
	init_sw_file_para(&sw_file.err_log_data);
//	init_local_time();
	open_pw_original_data_file();
	open_sw_original_data_file();
	open_err_log_data_file();

#ifdef SOFTWARE_CHMOD

	printf("init_pw_save---PW_ORIGNAL_DATA_FILE_CHMOD\n");
	PW_ORIGNAL_DATA_FILE_CHMOD;
	printf("init_pw_save---PW_LOG_DATA_CHMOD\n");
	PW_LOG_DATA_CHMOD;

	system("chmod 777 -R /media/sw_data/");
	system("chmod 777 -R /media/sw_data/orignal_data/");

#endif
	printf("------->init_pw_save<------\n");
}


//建立平稳存储数据文件夹

void creat_pw_save_dir()
{

	if((store_flag.hdd_exist_flag == FALSE) && (store_flag.hdd_err_flag == TRUE)) //SD卡不存在或者SD卡错误,就不再建立数据文件
		return;




	open_pw_original_data_file();

#ifdef ADD_LOG_DATA
	open_log_data_file();
#endif
#if 0//20210410_lch00
	open_test_data_file();
	open_check_data_file();
#endif
//
//	if(access(PW_ORIGNAL_DATA_DIR,F_OK)!= 0)
//	{
//		if(creat_dir(PW_ORIGNAL_DATA_DIR) < 0)
//		{
//			return;
//		}
//	}
//	//补全文件路径名
//	strcpy(dir_name,PW_ORIGNAL_DATA_DIR);
////	printf("pw_orignal_dir_one:%s\n",dir_name);
//	create_ymd_hms_dir_name(date_dir);
//	strcat(dir_name,date_dir);
////	printf("pw_orignal_dir_two:%s\n",dir_name);
//	if(creat_dir(dir_name) < 0)			//创建orignal_data 文件夹下面的日期文件夹
//	{
//		return;
//	}
//
//	system("chmod 777 -R /media/pw_data/orignal_data/");
//
//
//	//创建平稳原始数据文件
//	if(pw_file.pw_original_data.fp != NULL)
//	{
//		fclose(pw_file.pw_original_data.fp);
//		pw_file.pw_original_data.fp = NULL;
//	}
//
//	strcpy(file_name,PW_ORIGNAL_DATA_NAME);
//	create_ymdhms_str(date_name);
////	printf("pw_orignal_date:%s\n",date_name);
//	strcat(file_name,date_name);
//	strcpy(file_type,".dat");
//	strcat(file_name,file_type);
//
//	strcat(dir_name,file_name);
//	DEBUG("pw_orignal_dir_four:%s\n",dir_name);
//	pw_file.pw_original_data.fp = fopen(dir_name,"w+");
//
//	if(pw_file.pw_original_data.fp == NULL)
//	{
//		DEBUG("pw_file.pw_original_data file open fail\n");
//		return;
//	}
//
//	printf("before fp change:%d\n",pw_file.pw_original_data.fp->_fileno);
//
//	pw_file.pw_original_data.fd = fileno(pw_file.pw_original_data.fp);
//
//	printf("after fp change:%d\n",pw_file.pw_original_data.fd);


}

void reset_pw_file()
{
	fsync_sys_fd();
	close_sys_fd();

	init_file_sys_para();
	open_pw_original_data_file();
	open_sw_original_data_file();

	printf("reset_pw_file---PW_ORIGNAL_DATA_FILE_CHMOD\n");
	PW_ORIGNAL_DATA_FILE_CHMOD;
	printf("reset_pw_file---PW_LOG_DATA_CHMOD\n");
	PW_LOG_DATA_CHMOD;

	system("chmod 777 -R /media/sw_data/");
	system("chmod 777 -R /media/sw_data/orignal_data/");

	printf("------->reset_psw_file<------\n");
}

/*************************************************
Function:    get_memoccupy
Description: 获取当前系统可用内存大小
Input:
Output:
Return:　可用内存大小
Others:
*************************************************/
uint32_t get_memoccupy(void)
{
	FILE *fd;
//	int n;
	char buff[256];
	char name[20];
	uint32_t total;
	char name2[20];
	uint32_t free;
	fd = fopen("/proc/meminfo", "r");
	fgets(buff, sizeof(buff), fd);
	sscanf(buff, "%s %d %s", name, &total, name2);
	fgets(buff, sizeof(buff), fd); //从fd文件中读取长度为buff的字符串再存到起始地址为buff这个空间里
	sscanf(buff, "%s %d %s", name, &free, name2);
//	DEBUG("free_size:\n");
//	DEBUG("%s \r\n", name);
//	DEBUG("%d \r\n", free);
	fclose(fd); //关闭文件fd
	return free;
}

uint16_t little_to_big_16bit(uint16_t value)
{
	uint16_t res_value = 0;
	res_value |= (value&0xff)<<8;
	res_value |= (value&0xff00)>>8;
	return res_value;
}

uint32_t little_to_big_32bit(uint32_t value)
{
	uint32_t res_value = 0;
	res_value |=(value&0xff)<<24;
	res_value |=(value&0xff00)<<8;
	res_value |=(value&0xff0000)>>8;
	res_value |=(value&0xff000000)>>24;
	return res_value;
}

//struct SW_DIAGNOS_PARA
void update_pw_tz_data(struct PW_TZ_DATA *save_tz_data,void *tzdata_temp,uint8_t update_tz_flag)
{
	struct PW_DIAGNOS_PARA *tzdata;
//	struct LOCAL_TIME time_now;
//	static uint32_t time_cnt=0;
#ifdef ADD_DIAG_TZZ_DATA_FILE
	uint8_t ch=0;
#endif

	switch(update_tz_flag)
	{
		case UPDATE_DIAG_DATE:
			tzdata = (struct PW_DIAGNOS_PARA*)tzdata_temp;
			save_tz_data->sensor_status.bits.side1_sensor_err_power_on = tzdata->sens_para[0].sens_self_test.bits.open_circuit_err_power_on| \
					tzdata->sens_para[1].sens_self_test.bits.open_circuit_err_power_on | \
					tzdata->sens_para[2].sens_self_test.bits.open_circuit_err_power_on;



			save_tz_data->sensor_status.bits.side1_sensor_err_real_time = tzdata->sens_para[0].sens_self_test.bits.open_circuit_err_real_time| \
					tzdata->sens_para[1].sens_self_test.bits.open_circuit_err_real_time | \
					tzdata->sens_para[2].sens_self_test.bits.open_circuit_err_real_time;

//			printf("## side1_sensor_err_power_on %d\n",save_tz_data->sensor_status.bits.side1_sensor_err_power_on);
//			printf("## side1_sensor_err_real_time %d\n",save_tz_data->sensor_status.bits.side1_sensor_err_real_time);

			psw_info_data.pwb_err.bits.pw_sensor1_self_err = save_tz_data->sensor_status.bits.side1_sensor_err_power_on;
			psw_info_data.pwb_err.bits.pw_sensor1_err      = save_tz_data->sensor_status.bits.side1_sensor_err_real_time;

			s1pw_tz_data.pwb_err.bits.pw_sensor1_self_err = save_tz_data->sensor_status.bits.side1_sensor_err_power_on;
			s1pw_tz_data.pwb_err.bits.pw_sensor1_err      = save_tz_data->sensor_status.bits.side1_sensor_err_real_time;

			//save_tz_data->sensor12_err.bits.sensor1_err=save_tz_data->sensor_status.bits.side1_sensor_err_real_time|save_tz_data->sensor_status.bits.side1_sensor_err_power_on;
			save_tz_data->sensor12_err.bits.sensor1_err=save_tz_data->sensor_status.bits.side1_sensor_err_real_time;

//			printf("--------------------before wirte_err_log_data-------------\n");
//			if(save_tz_data->sensor12_err.bits.sensor1_err)
//			{
//				wirte_err_log_data(ERR,PW_SENSOR1_ERR);
//				//记录传感器故障
//			}
//			printf("--------------------after wirte_err_log_data-------------\n");

			save_tz_data->sensor_status.bits.side2_sensor_err_power_on = tzdata->sens_para[3].sens_self_test.bits.open_circuit_err_power_on| \
					tzdata->sens_para[4].sens_self_test.bits.open_circuit_err_power_on | \
					tzdata->sens_para[5].sens_self_test.bits.open_circuit_err_power_on;

			save_tz_data->sensor_status.bits.side2_sensor_err_real_time = tzdata->sens_para[3].sens_self_test.bits.open_circuit_err_real_time| \
					tzdata->sens_para[4].sens_self_test.bits.open_circuit_err_real_time | \
					tzdata->sens_para[5].sens_self_test.bits.open_circuit_err_real_time;
			//save_tz_data->sensor12_err.bits.sensor2_err=save_tz_data->sensor_status.bits.side2_sensor_err_real_time|save_tz_data->sensor_status.bits.side2_sensor_err_power_on;
			save_tz_data->sensor12_err.bits.sensor2_err=save_tz_data->sensor_status.bits.side2_sensor_err_real_time;
//			if(save_tz_data->sensor12_err.bits.sensor2_err)
//			{
//				wirte_err_log_data(ERR,PW_SENSOR2_ERR);
//				//记录传感器故障
//			}
//			printf("## side2_sensor_err_power_on %d\n",save_tz_data->sensor_status.bits.side2_sensor_err_power_on);
//			printf("## side2_sensor_err_real_time %d\n",save_tz_data->sensor_status.bits.side2_sensor_err_real_time);
			psw_info_data.pwb_err.bits.pw_sensor2_self_err = save_tz_data->sensor_status.bits.side2_sensor_err_power_on;
			psw_info_data.pwb_err.bits.pw_sensor2_err      = save_tz_data->sensor_status.bits.side2_sensor_err_real_time;

			s1pw_tz_data.pwb_err.bits.pw_sensor2_self_err = save_tz_data->sensor_status.bits.side2_sensor_err_power_on;
			s1pw_tz_data.pwb_err.bits.pw_sensor2_err = save_tz_data->sensor_status.bits.side2_sensor_err_real_time;


#ifdef INTERNAL_PROTOCOL_20210725
			save_tz_data->side1_y_quota.wh = ((uint16_t)(tzdata->diag_res[pw1_y].value * 100) >> 0x8) & 0xff;//范围0～5.12,1=0.01
			save_tz_data->side1_y_quota.wl = (uint16_t)(tzdata->diag_res[pw1_y].value * 100) & 0xff;
			save_tz_data->side2_y_quota.wh = ((uint16_t)(tzdata->diag_res[pw2_y].value * 100) >> 0x8) & 0xff;
			save_tz_data->side2_y_quota.wl = (uint16_t)(tzdata->diag_res[pw2_y].value * 100) & 0xff;
			save_tz_data->side1_z_quota.wh = ((uint16_t)(tzdata->diag_res[pw1_z].value * 100) >> 0x8) & 0xff;
			save_tz_data->side1_z_quota.wl = (uint16_t)(tzdata->diag_res[pw1_z].value * 100) & 0xff;
			save_tz_data->side2_z_quota.wh = ((uint16_t)(tzdata->diag_res[pw2_z].value * 100) >> 0x8) & 0xff;
			save_tz_data->side2_z_quota.wl = (uint16_t)(tzdata->diag_res[pw2_z].value * 100) & 0xff;
			save_tz_data->side1_x_quota.wh = ((uint16_t)(tzdata->diag_res[pw1_x].value * 100) >> 0x8) & 0xff;
			save_tz_data->side1_x_quota.wl = (uint16_t)(tzdata->diag_res[pw1_x].value * 100) & 0xff;
			save_tz_data->side2_x_quota.wh = ((uint16_t)(tzdata->diag_res[pw2_x].value * 100) >> 0x8) & 0xff;
			save_tz_data->side2_x_quota.wl = (uint16_t)(tzdata->diag_res[pw2_x].value * 100) & 0xff;

			psw_info_data.side1_y_quota.wh = save_tz_data->side1_y_quota.wh;
			psw_info_data.side1_y_quota.wl = save_tz_data->side1_y_quota.wl;
			psw_info_data.side1_z_quota.wh = save_tz_data->side1_z_quota.wh;
			psw_info_data.side1_z_quota.wl = save_tz_data->side1_z_quota.wl;
			psw_info_data.side2_y_quota.wh = save_tz_data->side2_y_quota.wh;
			psw_info_data.side2_y_quota.wl = save_tz_data->side2_y_quota.wl;
			psw_info_data.side2_z_quota.wh = save_tz_data->side2_z_quota.wh;
			psw_info_data.side2_z_quota.wl = save_tz_data->side2_z_quota.wl;

			s1pw_tz_data.side1_y_quota.wh = save_tz_data->side1_y_quota.wh;
			s1pw_tz_data.side1_y_quota.wl = save_tz_data->side1_y_quota.wl;
			s1pw_tz_data.side1_z_quota.wh = save_tz_data->side1_z_quota.wh;
			s1pw_tz_data.side1_z_quota.wl = save_tz_data->side1_z_quota.wl;
			s1pw_tz_data.side2_y_quota.wh = save_tz_data->side2_y_quota.wh;
			s1pw_tz_data.side2_y_quota.wl = save_tz_data->side2_y_quota.wl;
			s1pw_tz_data.side2_z_quota.wh = save_tz_data->side2_z_quota.wh;
			s1pw_tz_data.side2_z_quota.wl = save_tz_data->side2_z_quota.wl;



//			printf("## save_tz_data->side1_y_quota.wh %x\n",save_tz_data->side1_y_quota.wh);
//			printf("## save_tz_data->side1_y_quota.wl %x\n",save_tz_data->side1_y_quota.wl);
//			printf("## save_tz_data->side1_z_quota.wh %x\n",save_tz_data->side1_z_quota.wh);
//			printf("## save_tz_data->side1_z_quota.wl %x\n",save_tz_data->side1_z_quota.wl);
//			printf("## save_tz_data->side2_y_quota.wh %x\n",save_tz_data->side2_y_quota.wh);
//			printf("## save_tz_data->side2_y_quota.wl %x\n",save_tz_data->side2_y_quota.wl);
//			printf("## save_tz_data->side2_z_quota.wh %x\n",save_tz_data->side2_z_quota.wh);
//			printf("## save_tz_data->side2_z_quota.wl %x\n",save_tz_data->side2_z_quota.wl);

#else
			save_tz_data->side1_y_quota = tzdata->diag_res[pw1_y].value * 10;//范围0～25.6,1=0.1
			save_tz_data->side2_y_quota = tzdata->diag_res[pw2_y].value * 10;
			save_tz_data->side1_z_quota = tzdata->diag_res[pw1_z].value * 10;
			save_tz_data->side2_z_quota = tzdata->diag_res[pw2_z].value * 10;
			save_tz_data->side1_x_quota = tzdata->diag_res[pw1_x].value * 10;
			save_tz_data->side2_x_quota = tzdata->diag_res[pw2_x].value * 10;
#endif

			save_tz_data->borad_err.bits.ctrla_can_err = can_status[BIT_CTRLA].connect_flag;
			save_tz_data->borad_err.bits.ctrlb_can_err = can_status[BIT_CTRLB].connect_flag;
			save_tz_data->borad_err.bits.ctrla_eth_err = eth_status[BIT_CTRLA].connect_flag;
			save_tz_data->borad_err.bits.ctrlb_eth_err = eth_status[BIT_CTRLB].connect_flag;

			save_tz_data->borad_err.bits.power1_err = pw_board_st.bits.power1_err;
			save_tz_data->borad_err.bits.power2_err = pw_board_st.bits.power2_err;
			save_tz_data->borad_err.bits.sample_err=pw_board_st.bits.sample_err;

			save_tz_data->borad_err.bits.pw_board_err=0;
//			printf("sensor_status.byte= %x\n",save_tz_data->sensor_status.byte);

#ifdef INTERNAL_PROTOCOL_20210416
			save_tz_data->dc_hc_status.bits.dc_warn_w1 = tzdata->jitter_diag_res[pw1_y].warn_status_new | tzdata->jitter_diag_res[pw1_z].warn_status_new;
			save_tz_data->dc_hc_status.bits.dc_warn_w2 = tzdata->jitter_diag_res[pw2_y].warn_status_new | tzdata->jitter_diag_res[pw2_z].warn_status_new;
			save_tz_data->dc_hc_status.bits.dc_alarm_w1 = (tzdata->jitter_diag_res[pw1_y].alarm_status_new | tzdata->jitter_diag_res[pw1_z].alarm_status_new)>>0x1;
			save_tz_data->dc_hc_status.bits.dc_alarm_w2 = (tzdata->jitter_diag_res[pw2_y].alarm_status_new | tzdata->jitter_diag_res[pw2_z].alarm_status_new)>>0x1;
			save_tz_data->dc_hc_status.bits.hc_warn_w1 = tzdata->shack_diag_res[pw1_y].warn_status_new;
			save_tz_data->dc_hc_status.bits.hc_warn_w2 = tzdata->shack_diag_res[pw2_y].warn_status_new;
			save_tz_data->dc_hc_status.bits.hc_alarm_w1 = tzdata->shack_diag_res[pw1_y].alarm_status_new>>0x1;
			save_tz_data->dc_hc_status.bits.hc_alarm_w2 = tzdata->shack_diag_res[pw2_y].alarm_status_new>>0x1;

	#ifdef INTERNAL_PROTOCOL_20210725
			save_tz_data->diag_alarm.bits.dc_warn = save_tz_data->dc_hc_status.bits.dc_warn_w1 | save_tz_data->dc_hc_status.bits.dc_warn_w2;
			save_tz_data->diag_alarm.bits.dc_alarm = save_tz_data->dc_hc_status.bits.dc_alarm_w1 | save_tz_data->dc_hc_status.bits.dc_alarm_w2;
			save_tz_data->diag_alarm.bits.hc_warn = save_tz_data->dc_hc_status.bits.hc_warn_w1 | save_tz_data->dc_hc_status.bits.hc_warn_w2;
		    save_tz_data->diag_alarm.bits.hc_alarm = save_tz_data->dc_hc_status.bits.hc_alarm_w1 | save_tz_data->dc_hc_status.bits.hc_alarm_w2;
	#endif

#ifdef PW_DIAG_NO_WARN_ALARM
			save_tz_data->warn_status.bits.z_warn_w1 = 0;
			save_tz_data->warn_status.bits.z_warn_w2 = 0;
			save_tz_data->warn_status.bits.y_warn_w1 = 0;
			save_tz_data->warn_status.bits.y_warn_w2 = 0;

			save_tz_data->warn_status.bits.y_warn = 0;
			save_tz_data->warn_status.bits.z_warn = 0;

			save_tz_data->alarm_status.bits.side1_y_alarm = 0;
			save_tz_data->alarm_status.bits.side1_z_alarm = 0;
			save_tz_data->alarm_status.bits.side1_x_alarm = 0;

			save_tz_data->alarm_status.bits.side2_y_alarm = 0;
			save_tz_data->alarm_status.bits.side2_z_alarm = 0;
			save_tz_data->alarm_status.bits.side2_x_alarm = 0;

			save_tz_data->alarm_status.bits.y_alarm = 0;
			save_tz_data->alarm_status.bits.z_alarm = 0;
			save_tz_data->alarm_status.bits.x_alarm = 0;
#else
			save_tz_data->warn_status.bits.z_warn_w1 = tzdata->diag_res[pw1_z].warn_status_new;
			save_tz_data->warn_status.bits.z_warn_w2 = tzdata->diag_res[pw2_z].warn_status_new;
			save_tz_data->warn_status.bits.y_warn_w1 = tzdata->diag_res[pw1_y].warn_status_new;
			save_tz_data->warn_status.bits.y_warn_w2 = tzdata->diag_res[pw2_y].warn_status_new;

			save_tz_data->warn_status.bits.y_warn = tzdata->diag_res[pw1_y].warn_status_new | tzdata->diag_res[pw2_y].warn_status_new;
			save_tz_data->warn_status.bits.z_warn = tzdata->diag_res[pw1_z].warn_status_new | tzdata->diag_res[pw2_z].warn_status_new;

			save_tz_data->alarm_status.bits.side1_z_alarm = tzdata->diag_res[pw1_z].alarm_status_new>>0x1;
			save_tz_data->alarm_status.bits.side2_z_alarm = tzdata->diag_res[pw2_z].alarm_status_new>>0x1;
			save_tz_data->alarm_status.bits.side1_y_alarm = tzdata->diag_res[pw1_y].alarm_status_new>>0x1;
			save_tz_data->alarm_status.bits.side2_y_alarm = tzdata->diag_res[pw2_y].alarm_status_new>>0x1;
			save_tz_data->alarm_status.bits.side1_x_alarm = tzdata->diag_res[pw1_x].alarm_status_new>>0x1;
			save_tz_data->alarm_status.bits.side2_x_alarm = tzdata->diag_res[pw2_x].alarm_status_new>>0x1;

			save_tz_data->alarm_status.bits.y_alarm = (tzdata->diag_res[pw1_y].alarm_status_new | tzdata->diag_res[pw2_y].alarm_status_new)>>0x1;
			save_tz_data->alarm_status.bits.z_alarm = (tzdata->diag_res[pw1_z].alarm_status_new | tzdata->diag_res[pw2_z].alarm_status_new)>>0x1;
			save_tz_data->alarm_status.bits.x_alarm = (tzdata->diag_res[pw1_x].alarm_status_new | tzdata->diag_res[pw2_x].alarm_status_new)>>0x1;

			psw_info_data.pw_alarm.bits.pw1_y_warning = save_tz_data->warn_status.bits.y_warn_w1;
			psw_info_data.pw_alarm.bits.pw2_y_warning = save_tz_data->warn_status.bits.y_warn_w2;
			psw_info_data.pw_alarm.bits.pw1_z_warning = save_tz_data->warn_status.bits.z_warn_w1;
			psw_info_data.pw_alarm.bits.pw2_z_warning = save_tz_data->warn_status.bits.z_warn_w2;
			psw_info_data.pw_alarm.bits.pw1_y_alarm = save_tz_data->alarm_status.bits.side1_y_alarm;
			psw_info_data.pw_alarm.bits.pw2_y_alarm = save_tz_data->alarm_status.bits.side2_y_alarm;
			psw_info_data.pw_alarm.bits.pw1_z_alarm = save_tz_data->alarm_status.bits.side1_z_alarm;
			psw_info_data.pw_alarm.bits.pw2_z_alarm = save_tz_data->alarm_status.bits.side2_z_alarm;



			s1pw_tz_data.pw_alarm.bits.pw1_y_warning = save_tz_data->warn_status.bits.y_warn_w1;
			s1pw_tz_data.pw_alarm.bits.pw2_y_warning = save_tz_data->warn_status.bits.y_warn_w2;
			s1pw_tz_data.pw_alarm.bits.pw1_z_warning = save_tz_data->warn_status.bits.z_warn_w1;
			s1pw_tz_data.pw_alarm.bits.pw2_z_warning = save_tz_data->warn_status.bits.z_warn_w2;
			s1pw_tz_data.pw_alarm.bits.pw1_y_alarm = save_tz_data->alarm_status.bits.side1_y_alarm;
			s1pw_tz_data.pw_alarm.bits.pw2_y_alarm = save_tz_data->alarm_status.bits.side2_y_alarm;
			s1pw_tz_data.pw_alarm.bits.pw1_z_alarm = save_tz_data->alarm_status.bits.side1_z_alarm;
			s1pw_tz_data.pw_alarm.bits.pw2_z_alarm = save_tz_data->alarm_status.bits.side2_z_alarm;


			psw_info_data.side1_y_adval.wh = ((uint16_t)(tzdata->electric_val_real[pw1_y] * 100) >> 0x8) & 0xff;
			psw_info_data.side1_y_adval.wl = (uint16_t)(tzdata->electric_val_real[pw1_y] * 100) & 0xff;
			psw_info_data.side1_z_adval.wh = ((uint16_t)(tzdata->electric_val_real[pw1_z] * 100) >> 0x8) & 0xff;
			psw_info_data.side1_z_adval.wl = (uint16_t)(tzdata->electric_val_real[pw1_z] * 100) & 0xff;
			psw_info_data.side2_y_adval.wh = ((uint16_t)(tzdata->electric_val_real[pw2_y] * 100) >> 0x8) & 0xff;
			psw_info_data.side2_y_adval.wl = (uint16_t)(tzdata->electric_val_real[pw2_y] * 100) & 0xff;
			psw_info_data.side2_z_adval.wh = ((uint16_t)(tzdata->electric_val_real[pw2_z] * 100) >> 0x8) & 0xff;
			psw_info_data.side2_z_adval.wl = (uint16_t)(tzdata->electric_val_real[pw2_z] * 100) & 0xff;


//			printf("## save_tz_data->warn_status.bits.y_warn_w1 %d\n",save_tz_data->warn_status.bits.y_warn_w1);
//			printf("## save_tz_data->warn_status.bits.y_warn_w2 %d\n",save_tz_data->warn_status.bits.y_warn_w2);
//			printf("## save_tz_data->warn_status.bits.z_warn_w1 %d\n",save_tz_data->warn_status.bits.z_warn_w1);
//			printf("## save_tz_data->warn_status.bits.z_warn_w2 %d\n",save_tz_data->warn_status.bits.z_warn_w2);
//			printf("## save_tz_data->alarm_status.bits.side1_y_alarm %d\n",save_tz_data->alarm_status.bits.side1_y_alarm);
//			printf("## save_tz_data->alarm_status.bits.side2_y_alarm %d\n",save_tz_data->alarm_status.bits.side2_y_alarm);
//			printf("## save_tz_data->alarm_status.bits.side1_z_alarm %d\n",save_tz_data->alarm_status.bits.side1_z_alarm);
//			printf("## save_tz_data->alarm_status.bits.side2_z_alarm %d\n",save_tz_data->alarm_status.bits.side2_z_alarm);
//
//
//			printf("## psw_info_data.side1_y_adval.wh %x\n",psw_info_data.side1_y_adval.wh);
//			printf("## psw_info_data.side1_y_adval.wl %x\n",psw_info_data.side1_y_adval.wl);
//			printf("## psw_info_data.side1_z_adval.wh %x\n",psw_info_data.side1_z_adval.wh);
//			printf("## psw_info_data.side1_z_adval.wl %x\n",psw_info_data.side1_z_adval.wl);
//			printf("## psw_info_data.side2_y_adval.wh %x\n",psw_info_data.side2_y_adval.wh);
//			printf("## psw_info_data.side2_y_adval.wl %x\n",psw_info_data.side2_y_adval.wl);
//			printf("## psw_info_data.side2_z_adval.wh %x\n",psw_info_data.side2_z_adval.wh);
//			printf("## psw_info_data.side2_z_adval.wl %x\n",psw_info_data.side2_z_adval.wl);

#endif

			save_tz_data->acc_index.y_peak_w1 = tzdata->shack_diag_res[pw1_y].value * 1000;//范围0～0.256g,1=0.001g
			save_tz_data->acc_index.y_peak_w2 = tzdata->shack_diag_res[pw2_y].value * 1000;

			save_tz_data->acc_index.y_root_w1 = tzdata->jitter_diag_res[pw1_y].value * 1000;
			save_tz_data->acc_index.y_root_w2 = tzdata->jitter_diag_res[pw2_y].value * 1000;
			save_tz_data->acc_index.z_root_w1 = tzdata->jitter_diag_res[pw1_z].value * 1000;
			save_tz_data->acc_index.z_root_w2 = tzdata->jitter_diag_res[pw2_z].value * 1000;

			save_tz_data->acc_index.z_freq_amp_w1 = tzdata->wheel_diag_res[pw1_z].value * 1000;
			save_tz_data->acc_index.z_freq_amp_w2 = tzdata->wheel_diag_res[pw2_z].value * 1000;

		#ifdef INTERNAL_PROTOCOL_20210725
			save_tz_data->acc_index.cl_z_freq_w1.wh = ((uint16_t)(tzdata->wheel_diag_res[pw1_z].max_fre * 10)>>0x8) & 0xff;
			save_tz_data->acc_index.cl_z_freq_w1.wl = (uint16_t)(tzdata->wheel_diag_res[pw1_z].max_fre * 10) & 0xff;
			save_tz_data->acc_index.cl_z_freq_w2.wh = ((uint16_t)(tzdata->wheel_diag_res[pw2_z].max_fre * 10)>>0x8) & 0xff;
			save_tz_data->acc_index.cl_z_freq_w2.wl = (uint16_t)(tzdata->wheel_diag_res[pw2_z].max_fre * 10) & 0xff;
		#else

		#endif
#else//2214

#endif


		break;

		case UPDATE_OTHER__DATE:
			save_tz_data->data_head.company_id = LUHANG;
			save_tz_data->data_head.board_id = PW_BOARD;
			save_tz_data->data_head.packet_num =0x07;
			memset(save_tz_data->data_head.res,0,sizeof(save_tz_data->data_head.res));
			*(uint16_t *)save_tz_data->soft_version = little_to_big_16bit(SOFT_VERSION_VAL);

			memmove(&save_tz_data->train_public_info.year,&recv_public_para.recv_ctrl_board_data.train_public_info.year,sizeof(struct TRAIN_PUBLIC_INFO));
#if defined(INTERNAL_PROTOCOL_20210416) && !defined(PUB_INFO_REMOVE_KM_FLAG)
			memmove(&save_tz_data->km_flag_ll, &recv_public_para.recv_ctrl_board_data.train_public_info.km_flag_ll, 4);
#endif

#if defined(INTERNAL_PROTOCOL_20210416)
		#ifndef PUB_INFO_REMOVE_KM_FLAG

		#endif
			memset(save_tz_data->company_res,0,sizeof(save_tz_data->company_res));

			save_tz_data->company_res[125] = pw_data_save_type;

			memset(save_tz_data->pw_res,0,sizeof(save_tz_data->pw_res));

			*(uint16_t *)&save_tz_data->pw_res[0] = little_to_big_16bit(comm_data_cnt.ctrla_eth_recv_all_cnt);		//

#else

#endif

		break;

		case UPDATE_SIMULATION_DATE:

			break;

		default:
			break;
	}
}

#ifdef ADD_DIAG_TZZ_DATA_FILE
void save_pw_diag_tzz_data(struct DIAG_TZZ_DATA *save_tz_data, uint16_t singal_num)
{
	uint16_t ret = 0;

	*(uint16_t *)save_tz_data->head = little_to_big_16bit(0xBB66);					//算法特征数据的数据头
	save_tz_data->packet_len = little_to_big_16bit(sizeof(struct DIAG_TZZ_DATA));	//算法特征数据的长度
	save_tz_data->packet_num = little_to_big_16bit(singal_num);//算法特征数据
	save_tz_data->data_type = 0x60;//pw板(0x66取前6为首，0x6N)算法特征数据:0x60
	*(uint16_t *)save_tz_data->check_sum = little_to_big_16bit(check_sum((uint8_t *)save_tz_data,sizeof(struct DIAG_TZZ_DATA)-2));

//printf("save_pw_diag_tzz_data--hdd_exist_flag:%x, hdd_err_flag:%x, hdd_save_flag:%x\n",
//		store_flag.hdd_exist_flag, store_flag.hdd_err_flag, store_flag.hdd_save_flag);

	if((store_flag.hdd_exist_flag == FALSE) || (store_flag.hdd_err_flag == TRUE) || store_flag.hdd_save_flag == FALSE)		//SD卡不存在或者SD卡错误,就不再存数据
	{
		//DEBUG("not save_pw_diag_tzz_data\n");
		return;
	}

	ret = fwrite(save_tz_data,sizeof(struct DIAG_TZZ_DATA),1,pw_file.pw_diag_tzz_data.fp);

	if(ret < 1)
	{
		DEBUG("write tz_data failed\n");
		store_flag.hdd_err_flag = TRUE;
	}
	else
	{
		store_flag.hdd_err_flag = FALSE;
	}

}
#endif

void save_pw_tz_data(struct PW_TZ_DATA *save_tz_data,uint16_t singal_num,uint16_t target_addr,uint8_t resend_flag,uint8_t ack_flag)
{
	/*以上测试屏蔽*/
	uint16_t write_res = 0;
//	static uint16_t pack_cnt2 = 0;

	s1pw_tz_data.tzsave_data_head.save_data_head_h = 0x66;
	s1pw_tz_data.tzsave_data_head.save_data_head_l = 0xbb;
	*(uint16_t *)s1pw_tz_data.tzsave_data_head.data_len = little_to_big_16bit(sizeof(struct S1PW_TZ_DATA));
	s1pw_tz_data.tzsave_data_head.save_data_type = 0x0c; //数据类型 byte4

	struct LOCAL_TIME time_now;
	get_local_time(&time_now);

	s1pw_tz_data.tzsave_data_head.year = time_now.year -2000;
	s1pw_tz_data.tzsave_data_head.month = time_now.mon;
	s1pw_tz_data.tzsave_data_head.day = time_now.day;
	s1pw_tz_data.tzsave_data_head.hour = time_now.hour;
	s1pw_tz_data.tzsave_data_head.min = time_now.min;
	s1pw_tz_data.tzsave_data_head.sec = time_now.sec;

	s1pw_tz_data.tzsave_data_head.pack_count_h = (singal_num >> 8) & 0xff;
	s1pw_tz_data.tzsave_data_head.pack_count_l =  singal_num & 0xff;
//	pack_cnt2++;

	s1pw_tz_data.tzsave_data_head.current_id_h = app_save_public.curr_id[0];
	s1pw_tz_data.tzsave_data_head.current_id_h = app_save_public.curr_id[1];
	s1pw_tz_data.tzsave_data_head.next_id_h = app_save_public.next_id[0];
	s1pw_tz_data.tzsave_data_head.next_id_h = app_save_public.next_id[1];

	s1pw_tz_data.tzsave_data_head.speed_h = app_save_public.speed[0];
	s1pw_tz_data.tzsave_data_head.speed_l = app_save_public.speed[1];

	s1pw_tz_data.tzsave_data_head.wheel_diameter_h = 0;
	s1pw_tz_data.tzsave_data_head.wheel_diameter_l = 0;

	s1pw_tz_data.tzsave_data_head.channel = 0;
	s1pw_tz_data.tzsave_data_head.host_slave_flag = 0;
	s1pw_tz_data.tzsave_data_head.train_id = pw_clb_config_para->trainnum;


	for(int i = 0;i < 38 ;i++)
		s1pw_tz_data.tzsave_data_head.reserve[i]  = 0;


	*(uint16_t *)s1pw_tz_data.check_sum = little_to_big_16bit(check_sum((uint8_t *)&s1pw_tz_data,sizeof(struct S1PW_TZ_DATA)-2));


	if((store_flag.hdd_exist_flag == FALSE) || (store_flag.hdd_err_flag == TRUE) || store_flag.hdd_save_flag == FALSE)		//SD卡不存在或者SD卡错误,就不再存数据
	{
		//DEBUG("not save_pw_tz_data\n");
		return;
	}

	write_res = fwrite(&s1pw_tz_data,sizeof(struct S1PW_TZ_DATA),1,pw_file.pw_tz_data.fp);

	if(write_res < 1)
	{
		DEBUG("write tz_data failed\n");
//		store_flag.hdd_err_flag = TRUE;
	}
	else
	{
		store_flag.hdd_err_flag = FALSE;
	}

}

//uint8_t head[2];               //"0xAA50：过程数据包0xAA51-0xAA54：原始数据包（根据实际增减）
//uint8_t len[2];                //振动过程数据为360，失稳数据为256 平稳为256 原始为1024 轴温为256
//uint8_t company_id;            //板卡供应商编号 参考供应商定义
//uint8_t board_id;              //本身板卡编号 参考宏定义LOCAL_BOARD
//uint8_t life_signal[2];        //生命信号，每秒加1
//uint8_t target_board_group[2]; //目标板卡的位集合
//uint8_t resend_flag;           //"0x55：表示首次发送该包数据，0xAA：表示重发该包数据；重发时的数据与首次发送的数据需全部一样，且仅对未应答的单板重发数据（通过Byte8-9来选型），超时时间为300ms，最多重发3次。"
//uint8_t ack_flag;              //"0x5A:目标板需要返回给请求板收到一包数据的应答帧   0x00:无需应答其它无效"
//uint8_t packet_num;            //当前数据类型发送的总包数
//uint8_t res[11];               //预留

void save_pw_original_data(uint16_t *save_buf,uint16_t singal_num,uint8_t ch)
{
	uint16_t write_res = 0;
	struct LOCAL_TIME time_now;
	get_local_time(&time_now);

	pw_raw_data.send_data_head.save_data_head_h = 0x66;
	pw_raw_data.send_data_head.save_data_head_l = 0xbb;
	*(uint16_t *)pw_raw_data.send_data_head.data_len = little_to_big_16bit(sizeof(struct PW_RAW_DATA));
	pw_raw_data.send_data_head.save_data_type = 0x0a; //数据类型 byte4

	pw_raw_data.send_data_head.year = time_now.year - 2000;
	pw_raw_data.send_data_head.month = time_now.mon;
	pw_raw_data.send_data_head.day = time_now.day;
	pw_raw_data.send_data_head.hour = time_now.hour;
	pw_raw_data.send_data_head.min = time_now.min;
	pw_raw_data.send_data_head.sec = time_now.sec;

	pw_raw_data.send_data_head.pack_count_h = (singal_num >> 8) & 0xff;
	pw_raw_data.send_data_head.pack_count_l =  singal_num & 0xff;
	pw_raw_data.send_data_head.current_id_h = app_save_public.curr_id[0];
	pw_raw_data.send_data_head.current_id_l = app_save_public.curr_id[1];
	pw_raw_data.send_data_head.next_id_h = app_save_public.next_id[0];
	pw_raw_data.send_data_head.next_id_l = app_save_public.next_id[1];

	pw_raw_data.send_data_head.speed_h = app_save_public.speed[0];;
	pw_raw_data.send_data_head.speed_l = app_save_public.speed[1];;

	pw_raw_data.send_data_head.wheel_diameter_h = 0;
	pw_raw_data.send_data_head.wheel_diameter_l = 0;

	pw_raw_data.send_data_head.channel = ch + 1;
	pw_raw_data.send_data_head.host_slave_flag = 0;
	pw_raw_data.send_data_head.train_id = pw_clb_config_para->trainnum;
	for(int i = 0;i < 38 ;i++)
		pw_raw_data.send_data_head.reserve[i]  = 0;

	memmove(pw_raw_data.ad_acc,save_buf,512*2);

	*(uint16_t *)pw_raw_data.check_sum = little_to_big_16bit(check_sum((uint8_t *)&pw_raw_data,sizeof(struct PW_RAW_DATA)-2));

//	printf("store_flag.hdd_exist_flag = %d\n",store_flag.hdd_exist_flag);
//	printf("store_flag.hdd_err_flag = %d\n",store_flag.hdd_err_flag);
//	printf("store_flag.hdd_save_flag = %d\n",store_flag.hdd_save_flag);
	if((store_flag.hdd_exist_flag == FALSE) || (store_flag.hdd_err_flag == TRUE) || store_flag.hdd_save_flag == FALSE)		//SD卡不存在或者SD卡错误,就不再存数据
	{
		DEBUG("not save_pw_original_data\n");
		printf("not save_pw_original_data\n");
		return;
	}

	write_res = fwrite(&pw_raw_data,sizeof(struct PW_RAW_DATA),1,pw_file.pw_original_data.fp);

	if(write_res < 1)
	{
		printf("write original_data failed\n");
		DEBUG("write original_data failed\n");
//		store_flag.hdd_err_flag = TRUE;
	}
	else
	{
		store_flag.hdd_err_flag = FALSE;
	}
}

/*************************************************
Function:  remove_dir
Description:删除指定路径中的文件及该路径
Input: 	需要删除的路径
Output:
Return: 0:ok  -1:err
Others:
*************************************************/
int remove_dir(const char *dir)
{
    char cur_dir[] = ".";
    char up_dir[] = "..";
    char dir_name[128];
    DIR *dirp;
    struct dirent *dp;
    struct stat dir_stat;

    // 参数传递进来的目录不存在，直接返回
    if ( 0 != access(dir, F_OK) )
    {
        return 0;
    }

    // 获取目录属性失败，返回错误
    if ( 0 > stat(dir, &dir_stat) )
    {
        perror("get directory stat error");
        return -1;
    }

    if ( S_ISREG(dir_stat.st_mode) )
    {  // 普通文件直接删除
    	printf("remove dir:%s\n",dir);
        remove(dir);
        return 0;
    }
    else if ( S_ISDIR(dir_stat.st_mode) )
    {   // 目录文件，递归删除目录中内容
        dirp = opendir(dir);
        while ( (dp=readdir(dirp)) != NULL )
        {
            // 忽略 . 和 ..
            if ( (0 == strcmp(cur_dir, dp->d_name)) || (0 == strcmp(up_dir, dp->d_name)) )
            {
                continue;
            }

            sprintf(dir_name, "%s/%s", dir, dp->d_name);
            remove_dir(dir_name);   // 递归调用

            usleep(200000);
        }
        closedir(dirp);

        rmdir(dir);     //删除空目录
        return 0;
    }
    else
    {
        perror("unknow file type!");
        return -1;
    }
}


/*************************************************
Function:  delete_default_file
Description:
Input: 	无
Output:
Return: 无
Others:
*************************************************/
void delete_default_file()
{
	if(remove_dir((char*)&PW_DEFAULT_DIR) == 0)
	{
		printf("delete PW_DEFAULT_DIR ok\n");
	}

	if(remove_dir((char *)&SW_DEFAULT_DIR) == 0)
	{
		printf("delete SW_DEFAULT_DIR ok\n");
	}
}


unsigned long long int get_sd_size(char *dev_path,enum GET_SD_SIZE_TYPE type,uint8_t length)
{

    struct statfs statFS;
    char dev_path_temp[50] = {0};
    uint8_t cnt_i = 0;
    unsigned long long int 		capacity;
    unsigned long long int      usedBytes = 0;
    unsigned long long int      freeBytes = 0;
    unsigned long long int      totalBytes = 0;
    unsigned long long int      endSpace = 0;

    for(cnt_i = 0;cnt_i < length;cnt_i++)
    {
    	*(dev_path_temp + cnt_i) = *(dev_path + cnt_i);			//将路径取出
    }

    printf("dev_path:%s\n",dev_path_temp);
    if (statfs(dev_path_temp, &statFS) == -1){
        printf("statfs failed for path->[%s]\n", dev_path_temp);
        return(-1);
    }

    //printf("");
    /***得到字节数据****//*块数乘以每一块的字节数*/
    totalBytes = (uint64_t)statFS.f_blocks * (uint64_t)statFS.f_bsize;
    freeBytes = (uint64_t)statFS.f_bfree * (uint64_t)statFS.f_bsize;

    usedBytes = totalBytes - freeBytes;

//    printf("statFS.f_blocks1:%ld\n",statFS.f_blocks);
//    printf("statFS.f_bsize:%d\n",statFS.f_bsize);
//    printf("statFS.f_bfree1:%ld\n",statFS.f_bfree);
    printf("totalBytes:%lld B\n",totalBytes);
    printf("freeBytes:%lld B\n",freeBytes);


    switch(type)
    {
        case SD_TOTAL_SIZE:
            endSpace = totalBytes/1024/1024;			//单位MB
//            printf("statFS.f_frsize1:%d\n",statFS.f_frsize);
//            printf("statFS.f_frsize2:%lld\n",(uint64_t)statFS.f_frsize);
//            printf("statFS.f_bsize:%d\n",statFS.f_bsize);
//            printf("statFS.f_bsize2:%lld\n",(uint64_t)statFS.f_bsize);
//            printf("statFS.f_blocks1:%d\n",statFS.f_blocks);
//            printf("statFS.f_blocks2:%11d\n",statFS.f_blocks);
//            printf("statFS.f_bfree1:%d\n",statFS.f_bfree);
//            printf("statFS.f_bfree2:%11d\n",statFS.f_bfree);
//            printf("totalbyte1:%lld\n",totalBytes);
//           // printf("totalbyte:%lld,%lld,%lld,%lld,%lld\n",totalBytes,statFS.f_blocks,statFS.f_blocks * statFS.f_frsize,statFS.f_frsize,statFS.f_bfree);
//            printf("endspace:%lld\n",endSpace);
            break;

        case SD_USED_SIZE:
            endSpace = usedBytes/1024/1024;					//单位MB
            break;

        case SD_FREE_SIZE:
            endSpace = freeBytes/1024/1024;					//单位MB

            printf("sd_free_size:%lld\n",endSpace);
            break;

        default:
            return ( -1 );
    }
   // capacity = usedBytes;

    capacity = endSpace;

    return capacity;
}


uint8_t check_sdk_file(void)
{

	uint8_t del_file_flag=0;
	unsigned long long int free_capacity = 0;
	unsigned long long int total_capacity = 0;

	unsigned long long int free_capacity_sw = 0;
	unsigned long long int total_capacity_sw = 0;

#if defined(HDD_DEV_MMCBLK0P3_OR_TF)
	uint8_t ret=-1;
	uint16_t FreeSizeMB = 0;

	ret = access(HDD_DEV, F_OK);
    if(ret == 0)
    {
    	//ret = access(HDD_DEV1, F_OK);
    	FreeSizeMB = HDD_DEV1_FREESIZE;//(23*1024)MB
    }
    else
    {
    	//ret = access(HDD_DEV2, F_OK);
    	FreeSizeMB = HDD_DEV2_FREESIZE;//1024MB
    }
#endif

	//获取ＴＦ卡剩余空间
	free_capacity = get_sd_size((char *)&PW_SDK_DIR,SD_FREE_SIZE,sizeof(PW_SDK_DIR));
	printf("free_capacity_pw:%lld MB\n",free_capacity);

	//获取ＴＦ卡总大小空间
	total_capacity = get_sd_size((char *)&PW_SDK_DIR,SD_TOTAL_SIZE,sizeof(PW_SDK_DIR));
	printf("total_capacity_pw:%lld MB\n",total_capacity);

	//获取ＴＦ卡剩余空间
//	free_capacity_sw = get_sd_size((char *)&SW_SDK_DIR,SD_FREE_SIZE,sizeof(SW_SDK_DIR));
//	printf("free_capacity_sw:%lld MB\n",free_capacity);
//
//	//获取ＴＦ卡总大小空间
//	total_capacity_sw = get_sd_size((char *)&SW_SDK_DIR,SD_TOTAL_SIZE,sizeof(SW_SDK_DIR));
//	printf("total_capacity_sw:%lld MB\n",total_capacity);


#if defined(HDD_DEV_MMCBLK0P3_OR_TF)
	if(free_capacity < FreeSizeMB && total_capacity > FreeSizeMB)
//	if(free_capacity < FreeSizeMB && total_capacity > FreeSizeMB)
#else
	//剩余空间小于1个Ｇ并且总容量需要大于1个Ｇ
	if(free_capacity < HDD_DEV_FREESIZE && total_capacity > HDD_DEV_FREESIZE)
#endif
	{
		del_file_flag=1;
	}
	else
	{
		del_file_flag=0;
	}
//	if(getFsSize(VIBR_SDK_DIR)==0)
//	{
//		del_file_flag=1;
//	}
//	else
//	{
//		del_file_flag=0;
//	}
	return del_file_flag;

//	uint8_t del_file_flag=0;
//	if(getFsSize(GEAR_SDK_DIR)==0)
//	{
//		del_file_flag=1;
//	}
//	else
//	{
//		del_file_flag=0;
//	}
//	return del_file_flag;

}


//sock successful
void file_del_thread_entry()
{
	while(1)
	{
		//对时完成后,可以执行数据删除线程
		sem_wait(&delete_data_dir_sem);
		printf("\n***file_del_thread_entry---start!***\n");

		if(!store_flag.hdd_exist_flag)
		{
			continue;
		}

//		if(file_del_deal==0)
//			break;
		delete_default_file();

		if(check_sdk_file() == 0)
		{
			printf("\n***file_del_thread_entry---no need DEL!***\n");
			continue;
		}

		printf("file_del_thread_entry---DEL start!\n");

		if(delete_sdk_file()==0)										//目前只删除１天数据
		{
			printf("DEL pw_orignal_file success!\n");
		}
		else
			printf("DEL pw_orignal_file faild!\n");

		if(delete_sdk_file_sw()==0)										//目前只删除１天数据
		{
			printf("DEL Sw_orignal_file success!\n");
		}
		else
			printf("DEL Sw_orignal_file faild!\n");

		printf("\n***file_del_thread_entry---end!***\n");
	}

//	file_10min_st.del_file_finish_flag=1;
//	printf("******file_del_thread_entry exit!******\n");
//
//	pthread_detach(pthread_self());
//	pthread_exit(0);
}

/*************************************************
 Function:    init_file_del_thread
 Description: 数据删除线程
 Input:
 Output:
 Return:
 Others:
 *************************************************/
int init_file_del_thread()
{
	pthread_t file_del_thread_id;
	int ret = -1;

	printf("init delete_data_dir_sem\n");
	sem_init(&delete_data_dir_sem, 0, 0); //信号量初始化

	ret = pthread_create(&file_del_thread_id, NULL, (void *)file_del_thread_entry, NULL);
	if (ret != 0)
	{
		DEBUG("net file_del thread error!\n");
		return ret;
	}
	return 0;
}

void init_sw_save()
{

//	check_hdd_test();
//	if(store_flag.hdd_save_flag)
	{
//		if(getFsSize(SW_SDK_DIR)==0)
//		{
//			if(delete_sdk_file()==0)
//			{
//				printf("******delete_sdk_file success!******\n");
//			}
//			else
//				printf("******delete_sdk_file faild!******\n");
//		}
		init_file_sys_para();
		init_sw_file_para(&sw_file.err_log_data);

		init_local_time();
		printf("----------init_sw_save\n");
		open_sw_original_data_file();
//		open_log_data_file();
		open_err_log_data_file();

#ifdef DIAG_DATA_SAVE_FOR_TEST
		init_sw_file_para(&sw_file.diag_swtzz_data);
		create_swtzz_diag_file();
#endif

#ifdef CAN_ERR_REBOOT_TWO_TIMES
		init_sw_file_para(&sw_file.reboot);
		cteate_reboot_log_file();
#endif
		system("chmod 777 -R /media/sw_data/");
		system("chmod 777 -R /media/sw_data/orignal_data/");
	}

	/*测试AD数据使用，测试完成后屏蔽*/
//	open_test_data_file1();
//	open_test_data_file2();
	/*测试AD数据使用，测试完成后屏蔽*/
}

void update_sw_tz_data(struct SW_TZ_DATA *save_tz_data,void *tzdata_temp,uint8_t update_tz_flag)
{
	struct SW_DIAGNOS_PARA *tzdata;
//	struct LOCAL_TIME time_now,time_new;
//	static uint32_t time_cnt=0;
	switch(update_tz_flag)
	{
		case UPDATE_DIAG_DATE:									//更新特征数据
			tzdata = (struct SW_DIAGNOS_PARA*)tzdata_temp;
			/***************************ceshipingbi********/
			//DEBUG("......................................%d.............................\n",tzdata->sens_para[2].sens_self_test.bits.self_test_power_on);
			//传感器故障
			save_tz_data->sensor_status.bits.bogie_axis1_self_test_err = tzdata->sens_para[0].sens_self_test.bits.self_test_power_on;
			save_tz_data->sensor_status.bits.bogie_axis2_self_test_err = tzdata->sens_para[1].sens_self_test.bits.self_test_power_on;
//			save_tz_data->sensor_status.bits.bogie_axis3_self_test_err = tzdata->sens_para[2].sens_self_test.bits.self_test_power_on;
//			save_tz_data->sensor_status.bits.bogie_axis4_self_test_err = tzdata->sens_para[3].sens_self_test.bits.self_test_power_on;
			save_tz_data->sensor_status.bits.bogie_axis1_real_time_err = tzdata->sens_para[0].sens_self_test.bits.self_test_real_time;
			save_tz_data->sensor_status.bits.bogie_axis2_real_time_err = tzdata->sens_para[1].sens_self_test.bits.self_test_real_time;
//			save_tz_data->sensor_status.bits.bogie_axis3_real_time_err = tzdata->sens_para[2].sens_self_test.bits.self_test_real_time;
//			save_tz_data->sensor_status.bits.bogie_axis4_real_time_err = tzdata->sens_para[3].sens_self_test.bits.self_test_real_time;

//			save_tz_data->sensor_err.bits.axis1_sensor_err=tzdata->sens_para[0].sens_self_test.bits.self_test_real_time|tzdata->sens_para[0].sens_self_test.bits.self_test_power_on;
			save_tz_data->sensor_err.bits.axis1_sensor_err=tzdata->sens_para[0].sens_self_test.bits.self_test_real_time;


			psw_info_data.swb_err.bits.sw_sensor1_err = save_tz_data->sensor_status.bits.bogie_axis1_real_time_err;
			psw_info_data.swb_err.bits.sw_sensor2_err = save_tz_data->sensor_status.bits.bogie_axis2_real_time_err;
			psw_info_data.swb_err.bits.sw_sensor1_self_err = save_tz_data->sensor_status.bits.bogie_axis1_self_test_err;
			psw_info_data.swb_err.bits.sw_sensor2_self_err = save_tz_data->sensor_status.bits.bogie_axis2_self_test_err;

			s1sw_tz_data.swb_err.bits.sw_sensor1_err = save_tz_data->sensor_status.bits.bogie_axis1_real_time_err;
			s1sw_tz_data.swb_err.bits.sw_sensor2_err = save_tz_data->sensor_status.bits.bogie_axis2_real_time_err;
			s1sw_tz_data.swb_err.bits.sw_sensor1_self_err = save_tz_data->sensor_status.bits.bogie_axis1_self_test_err;
			s1sw_tz_data.swb_err.bits.sw_sensor2_self_err = save_tz_data->sensor_status.bits.bogie_axis2_self_test_err;

//			printf("psw_info_data.swb_err.bits.sw_sensor1_err = %d\n",psw_info_data.swb_err.bits.sw_sensor1_err);
//			printf("psw_info_data.swb_err.bits.sw_sensor2_err = %d\n",psw_info_data.swb_err.bits.sw_sensor2_err);
//			printf("psw_info_data.swb_err.bits.sw_sensor1_self_err = %d\n",psw_info_data.swb_err.bits.sw_sensor1_self_err);
//			printf("psw_info_data.swb_err.bits.sw_sensor2_self_err = %d\n",psw_info_data.swb_err.bits.sw_sensor2_self_err);

//			printf("s1sw_tz_data.swb_err.bits.sw_sensor1_err = %d\n",s1sw_tz_data.swb_err.bits.sw_sensor1_err);
//			printf("s1sw_tz_data.swb_err.bits.sw_sensor2_err = %d\n",s1sw_tz_data.swb_err.bits.sw_sensor2_err);
//			printf("s1sw_tz_data.swb_err.bits.sw_sensor1_self_err = %d\n",s1sw_tz_data.swb_err.bits.sw_sensor1_self_err);
//			printf("s1sw_tz_data.swb_err.bits.sw_sensor2_self_err = %d\n",s1sw_tz_data.swb_err.bits.sw_sensor2_self_err);

			if(save_tz_data->sensor_err.bits.axis1_sensor_err)
			{
				if(sys_status_cnt.err_type[0].sensor_err_save_flag == 0)
				{
					//if 条件 sys_status_cnt.ctrla_can_err_save_flag == 1 目的是为了在can通信正常情况下只存储一次
		//			sprintf(log_status,"ok");
		//			sprintf(log_kind,"ctrla can");
		//			sprintf(log_detail,"ctrla can ok");
		//			write_log(log_status,log_kind,log_detail,log_file_name);
					sys_status_cnt.err_type[0].sensor_err_save_flag = 1;
					sys_status_cnt.err_type[0].sensor_normal_save_flag = 0;
				}
				//wirte_err_log_data(ERR,SW_SENSOR1_ERR);
			}
			else
			{
				if(sys_status_cnt.err_type[0].sensor_normal_save_flag == 0)
				{
				//if 条件 sys_status_cnt.ctrla_can_err_save_flag == 1 目的是为了在can通信正常情况下只存储一次
				//			sprintf(log_status,"ok");
				//			sprintf(log_kind,"ctrla can");
				//			sprintf(log_detail,"ctrla can ok");
				//			write_log(log_status,log_kind,log_detail,log_file_name);
				sys_status_cnt.err_type[0].sensor_err_save_flag = 0;
				sys_status_cnt.err_type[0].sensor_normal_save_flag = 1;
				}
			}
			//save_tz_data->sensor_err.bits.axis2_sensor_err=tzdata->sens_para[1].sens_self_test.bits.self_test_real_time|tzdata->sens_para[1].sens_self_test.bits.self_test_power_on;
			save_tz_data->sensor_err.bits.axis2_sensor_err=tzdata->sens_para[1].sens_self_test.bits.self_test_real_time;
			if(save_tz_data->sensor_err.bits.axis2_sensor_err)
			{
				//wirte_err_log_data(ERR,SW_SENSOR2_ERR);
				if(save_tz_data->sensor_err.bits.axis2_sensor_err)
				{
					if(sys_status_cnt.err_type[1].sensor_err_save_flag == 0)
					{
						//if 条件 sys_status_cnt.ctrla_can_err_save_flag == 1 目的是为了在can通信正常情况下只存储一次
			//			sprintf(log_status,"ok");
			//			sprintf(log_kind,"ctrla can");
			//			sprintf(log_detail,"ctrla can ok");
			//			write_log(log_status,log_kind,log_detail,log_file_name);
						sys_status_cnt.err_type[1].sensor_err_save_flag = 1;
						sys_status_cnt.err_type[1].sensor_normal_save_flag = 0;
					}
					//wirte_err_log_data(ERR,SW_SENSOR1_ERR);
				}
				else
				{
					if(sys_status_cnt.err_type[1].sensor_normal_save_flag == 0)
					{
					//if 条件 sys_status_cnt.ctrla_can_err_save_flag == 1 目的是为了在can通信正常情况下只存储一次
					//			sprintf(log_status,"ok");
					//			sprintf(log_kind,"ctrla can");
					//			sprintf(log_detail,"ctrla can ok");
					//			write_log(log_status,log_kind,log_detail,log_file_name);
					sys_status_cnt.err_type[1].sensor_err_save_flag = 0;
					sys_status_cnt.err_type[1].sensor_normal_save_flag = 1;
					}
				}
			}

			//继电器 暂不需要（默认填0）
			save_tz_data->relay_status.byte=0;

			//转向架 异常  预判
			save_tz_data->alarm_status.bits.bogie_axis1_alarm = tzdata->diag_res[0].diagnos_alarm_status.bits.alarm_status_new;
			save_tz_data->alarm_status.bits.bogie_axis2_alarm = tzdata->diag_res[1].diagnos_alarm_status.bits.alarm_status_new;
//			save_tz_data->alarm_status.bits.bogie_axis3_alarm = tzdata->diag_res[2].diagnos_alarm_status.bits.alarm_status_new;
//			save_tz_data->alarm_status.bits.bogie_axis4_alarm = tzdata->diag_res[3].diagnos_alarm_status.bits.alarm_status_new;
			save_tz_data->alarm_status.bits.bogie_alarm = save_tz_data->alarm_status.bits.bogie_axis1_alarm |save_tz_data->alarm_status.bits.bogie_axis2_alarm | save_tz_data->alarm_status.bits.bogie_axis3_alarm | save_tz_data->alarm_status.bits.bogie_axis4_alarm;
			save_tz_data->warn_status.bits.bogie_axis1_warn = tzdata->diag_res[0].diagnos_alarm_status.bits.warn_status_new;
			save_tz_data->warn_status.bits.bogie_axis2_warn = tzdata->diag_res[1].diagnos_alarm_status.bits.warn_status_new;
//			save_tz_data->warn_status.bits.bogie_axis3_warn = tzdata->diag_res[2].diagnos_alarm_status.bits.warn_status_new;
//			save_tz_data->warn_status.bits.bogie_axis4_warn = tzdata->diag_res[3].diagnos_alarm_status.bits.warn_status_new;
			save_tz_data->warn_status.bits.bogie_warn = save_tz_data->warn_status.bits.bogie_axis1_warn | save_tz_data->warn_status.bits.bogie_axis2_warn | save_tz_data->warn_status.bits.bogie_axis3_warn | save_tz_data->warn_status.bits.bogie_axis4_warn;


			psw_info_data.swb_err.bits.sw1_y_judge = save_tz_data->warn_status.bits.bogie_axis1_warn;
			psw_info_data.swb_err.bits.sw2_y_judge = save_tz_data->warn_status.bits.bogie_axis2_warn;
			psw_info_data.swb_err.bits.sw1_y_alarm = save_tz_data->alarm_status.bits.bogie_axis1_alarm;
			psw_info_data.swb_err.bits.sw2_y_alarm = save_tz_data->alarm_status.bits.bogie_axis2_alarm;

			s1sw_tz_data.swb_err.bits.sw1_y_judge = save_tz_data->warn_status.bits.bogie_axis1_warn;
			s1sw_tz_data.swb_err.bits.sw2_y_judge = save_tz_data->warn_status.bits.bogie_axis2_warn;
			s1sw_tz_data.swb_err.bits.sw1_y_alarm = save_tz_data->alarm_status.bits.bogie_axis1_alarm;
			s1sw_tz_data.swb_err.bits.sw2_y_alarm = save_tz_data->alarm_status.bits.bogie_axis2_alarm;


//			printf("psw_info_data.swb_err.bits.sw1_y_judge = %d\n",psw_info_data.swb_err.bits.sw1_y_judge);
//			printf("psw_info_data.swb_err.bits.sw1_y_alarm = %d\n",psw_info_data.swb_err.bits.sw1_y_alarm);
//			printf("psw_info_data.swb_err.bits.sw2_y_judge = %d\n",psw_info_data.swb_err.bits.sw2_y_judge);
//			printf("psw_info_data.swb_err.bits.sw2_y_alarm = %d\n",psw_info_data.swb_err.bits.sw2_y_alarm);


//			printf("s1sw_tz_data.swb_err.bits.sw1_y_judge = %d\n",s1sw_tz_data.swb_err.bits.sw1_y_judge);
//			printf("s1sw_tz_data.swb_err.bits.sw1_y_alarm = %d\n",s1sw_tz_data.swb_err.bits.sw1_y_alarm);
//			printf("s1sw_tz_data.swb_err.bits.sw2_y_judge = %d\n",s1sw_tz_data.swb_err.bits.sw2_y_judge);
//			printf("s1sw_tz_data.swb_err.bits.sw2_y_alarm = %d\n",s1sw_tz_data.swb_err.bits.sw2_y_alarm);

			psw_info_data.sw1_y_adval.wh = ((uint16_t)(tzdata->electric_val_real[sw1_y] * 100) >> 0x8) & 0xff;
			psw_info_data.sw1_y_adval.wl = (uint16_t)(tzdata->electric_val_real[sw1_y] * 100) & 0xff;
			psw_info_data.sw2_y_adval.wh = ((uint16_t)(tzdata->electric_val_real[sw2_y] * 100) >> 0x8) & 0xff;
			psw_info_data.sw2_y_adval.wl = (uint16_t)(tzdata->electric_val_real[sw2_y] * 100) & 0xff;

//			printf("psw_info_data.sw1_y_adval.wh = %x\n",psw_info_data.sw1_y_adval.wh);
//			printf("psw_info_data.sw1_y_adval.wl = %x\n",psw_info_data.sw1_y_adval.wl);
//			printf("psw_info_data.sw2_y_adval.wh = %x\n",psw_info_data.sw2_y_adval.wh);
//			printf("psw_info_data.sw2_y_adval.wl = %x\n",psw_info_data.sw2_y_adval.wl);


			save_tz_data->total_bogie_err_cnt = tzdata->sw_alarm_cnt;													//转向架累计发生异常计数
			save_tz_data->bogie_err_para.bits.para1_grade = 2;										//参数1 转向架异常检测参数等级(失稳报警阈值)
			save_tz_data->bogie_err_para.bits.para2_grade = 2;										//参数2 转向架异常检测参数等级(失稳报警波头数)

			save_tz_data->bogie_err_para.bits.over_amp_cycle_ge = tzdata->alarm_h_num%10;
			save_tz_data->bogie_err_para.bits.over_amp_cycle_shi = tzdata->alarm_h_num/10;//加速度超过加速度阈值的连续周期数
			save_tz_data->bogie_err_para.bits.max_amp_ge=(tzdata->max_gvalue%1000%100%10);
			save_tz_data->bogie_err_para.bits.max_amp_shi=(tzdata->max_gvalue%1000%100/10);
			save_tz_data->bogie_err_para.bits.max_amp_bai=(tzdata->max_gvalue%1000/100);
			save_tz_data->bogie_err_para.bits.max_amp_qian=(tzdata->max_gvalue/1000);

			sw_boarderr.bits.save_err = store_flag.hdd_err_flag;						//存储故障

			save_tz_data->borad_err.byte=sw_boarderr.byte;


//			save_tz_data->com_err.bits.ctrla_can_err = comm_type.ctrlA_CAN.st_flag.err_flag;
//			save_tz_data->com_err.bits.ctrlb_can_err = comm_type.ctrlB_CAN.st_flag.err_flag;
//			save_tz_data->com_err.bits.ctrla_eth_err = comm_type.ctrlA_ETH.st_flag.err_flag;
//			save_tz_data->com_err.bits.ctrlb_eth_err = comm_type.ctrlB_ETH.st_flag.err_flag;


			save_tz_data->sys_cur_work_on_time[0] = 0;
			save_tz_data->sys_cur_work_on_time[1] = 0;
			save_tz_data->sys_cur_work_on_time[2] = 0;
			save_tz_data->sys_cur_work_on_time[3] = 0;
			save_tz_data->total_work_on_time[0] = 0;
			save_tz_data->total_work_on_time[1] = 0;
			save_tz_data->total_work_on_time[2] = 0;
			save_tz_data->total_work_on_time[3] = 0;

#if defined(PP_TZZ_DIAG_20210420)||defined(PP_TZZ_DIAG_1S_FILLTER)
//			save_tz_data->tz_min.bogie_axis1_value = 9.8 * tzdata->ffz_min_tzz[0] * 10;//*GRAVITY_PARA,范围0～256，1=0.1m/s2-->注：wtd数据落地要求改1＝0.01m/s2
//			save_tz_data->tz_min.bogie_axis2_value = 9.8 * tzdata->ffz_min_tzz[1] * 10;
//			save_tz_data->tz_min.bogie_axis3_value = 9.8 * tzdata->ffz_min_tzz[2] * 10;
//			save_tz_data->tz_min.bogie_axis4_value = 9.8 * tzdata->ffz_min_tzz[3] * 10;
//
//			save_tz_data->tz_mean.bogie_axis1_value = 9.8 * tzdata->ffz_mean_tzz[0] * 10;
//			save_tz_data->tz_mean.bogie_axis2_value = 9.8 * tzdata->ffz_mean_tzz[1] * 10;
//			save_tz_data->tz_mean.bogie_axis3_value = 9.8 * tzdata->ffz_mean_tzz[2] * 10;
//			save_tz_data->tz_mean.bogie_axis4_value = 9.8 * tzdata->ffz_mean_tzz[3] * 10;
			save_tz_data->tz_min.bogie_axis1_value = 9.8 * tzdata->ffz_min_tzz[0] * 100;//*GRAVITY_PARA,范围0～256，1=0.1m/s2-->注：wtd数据落地要求改1＝0.01m/s2
			save_tz_data->tz_min.bogie_axis2_value = 9.8 * tzdata->ffz_min_tzz[1] * 100;
//			save_tz_data->tz_min.bogie_axis3_value = 9.8 * tzdata->ffz_min_tzz[2] * 100;
//			save_tz_data->tz_min.bogie_axis4_value = 9.8 * tzdata->ffz_min_tzz[3] * 100;

			save_tz_data->tz_mean.bogie_axis1_value = 9.8 * tzdata->ffz_mean_tzz[0] * 100;
			save_tz_data->tz_mean.bogie_axis2_value = 9.8 * tzdata->ffz_mean_tzz[1] * 100;
//			save_tz_data->tz_mean.bogie_axis3_value = 9.8 * tzdata->ffz_mean_tzz[2] * 100;
//			save_tz_data->tz_mean.bogie_axis4_value = 9.8 * tzdata->ffz_mean_tzz[3] * 100;

			psw_info_data.sw1_min_tzz = save_tz_data->tz_min.bogie_axis1_value;
			psw_info_data.sw1_mean_tzz = save_tz_data->tz_mean.bogie_axis1_value;
			psw_info_data.sw2_min_tzz = save_tz_data->tz_min.bogie_axis2_value;
			psw_info_data.sw2_mean_tzz = save_tz_data->tz_mean.bogie_axis2_value;

			s1sw_tz_data.sw1_min_tzz = save_tz_data->tz_min.bogie_axis1_value;
			s1sw_tz_data.sw1_mean_tzz = save_tz_data->tz_mean.bogie_axis1_value;
			s1sw_tz_data.sw2_min_tzz = save_tz_data->tz_min.bogie_axis2_value;
			s1sw_tz_data.sw2_mean_tzz = save_tz_data->tz_mean.bogie_axis2_value;

#endif

#if 0

			save_tz_data->sensor_status.bits.bogie_axis1_self_test_err = 1;
			save_tz_data->sensor_status.bits.bogie_axis2_self_test_err =1;
			save_tz_data->sensor_status.bits.bogie_axis3_self_test_err = 1;
			save_tz_data->sensor_status.bits.bogie_axis4_self_test_err = 1;
			save_tz_data->sensor_status.bits.bogie_axis1_real_time_err =1;
			save_tz_data->sensor_status.bits.bogie_axis2_real_time_err = 1;
			save_tz_data->sensor_status.bits.bogie_axis3_real_time_err = 1;
			save_tz_data->sensor_status.bits.bogie_axis4_real_time_err = 1;

			save_tz_data->sensor_err.bits.axis1_sensor_err=1;
			save_tz_data->sensor_err.bits.axis2_sensor_err=1;
			save_tz_data->sensor_err.bits.axis3_sensor_err=1;
			save_tz_data->sensor_err.bits.axis4_sensor_err=1;
			//继电器 暂不需要（默认填0）
			save_tz_data->relay_status.bits.bogie_out_relay_err=1;
			save_tz_data->relay_status.bits.device_out_relay_err=1;
			save_tz_data->relay_status.bits.high_speed_in_relay_err=0;
			save_tz_data->relay_status.bits.low_speed_in_relay_err=1;
			save_tz_data->relay_status.bits.sensor_test_out_relay_err=1;
			//转向架 异常  预判
			save_tz_data->alarm_status.bits.bogie_axis1_alarm = 1;
			save_tz_data->alarm_status.bits.bogie_axis2_alarm = 1;
			save_tz_data->alarm_status.bits.bogie_axis3_alarm = 1;
			save_tz_data->alarm_status.bits.bogie_axis4_alarm = 1;
			save_tz_data->alarm_status.bits.bogie_alarm = 1;
			save_tz_data->warn_status.bits.bogie_axis1_warn = 1;
			save_tz_data->warn_status.bits.bogie_axis2_warn = 1;
			save_tz_data->warn_status.bits.bogie_axis3_warn = 1;
			save_tz_data->warn_status.bits.bogie_axis4_warn = 1;
			save_tz_data->warn_status.bits.bogie_warn = 1;

			save_tz_data->total_bogie_err_cnt = 99;													//转向架累计发生异常计数
			save_tz_data->bogie_err_para.bits.para1_grade = 1;										//参数1 转向架异常检测参数等级(失稳报警阈值)
			save_tz_data->bogie_err_para.bits.para2_grade = 2;										//参数2 转向架异常检测参数等级(失稳报警波头数)
			save_tz_data->bogie_err_para.bits.max_amp_ge=1;
			save_tz_data->bogie_err_para.bits.max_amp_shi=2;
			save_tz_data->bogie_err_para.bits.max_amp_bai=3;
			save_tz_data->bogie_err_para.bits.max_amp_qian=4;
			save_tz_data->bogie_err_para.bits.over_amp_cycle_ge=1;
			save_tz_data->bogie_err_para.bits.over_amp_cycle_shi=2;



#endif


		break;

		case UPDATE_OTHER__DATE:									//更新公共数据
			save_tz_data->data_head.packet_num =0x05;
			memset(save_tz_data->data_head.res,0,sizeof(save_tz_data->data_head.res));

			*(uint16_t *)save_tz_data->soft_version = little_to_big_16bit(SOFT_VERSION_VAL);
//			update_use_public_info();
			memmove(&save_tz_data->train_public_info,&recv_public_para.recv_ctrl_board_data.train_public_info,sizeof(struct TRAIN_PUBLIC_INFO));

//			if(recv_public_para.board_data.recv_data_head.valid_info.BITS.time_valid)												//校准系统时间
//			{
//
//				if(time_cnt==0)
//				{
//					printf("###Rcev tcms time check start###\n");
//					time_now.year = 2000 + save_tz_data->train_public_info.year;
//					time_now.mon = save_tz_data->train_public_info.mon;
//					time_now.day = save_tz_data->train_public_info.day;
//					time_now.hour = save_tz_data->train_public_info.hour;
//					time_now.min = save_tz_data->train_public_info.min;
//					time_now.sec = save_tz_data->train_public_info.sec;
//					set_local_time(&time_now);
//					reset_sw_file();
//				}
//				else if(time_cnt%30==0)
//				{
//					//printf("###time check start###\n");
//					get_local_time(&time_now);
//					time_new.year = 2000 + save_tz_data->train_public_info.year;
//					time_new.mon = save_tz_data->train_public_info.mon;
//					time_new.day = save_tz_data->train_public_info.day;
//					time_new.hour = save_tz_data->train_public_info.hour;
//					time_new.min = save_tz_data->train_public_info.min;
//					time_new.sec = save_tz_data->train_public_info.sec;
//					int cmpcnt=memcmp(&time_now,&time_new,16);
//
//					if(cmpcnt>0)
//					{
//						printf("time reset $$$$$$\n");
//						set_local_time(&time_new);
//						reset_sw_file();
//					}
//				}
//
//				time_cnt++;
//
//			}




#if 0
			save_tz_data->train_public_info.year=19;
			save_tz_data->train_public_info.mon=8;
			save_tz_data->train_public_info.day=29;
			save_tz_data->train_public_info.hour=19;
			save_tz_data->train_public_info.min=19;
			save_tz_data->train_public_info.sec=19;

			save_tz_data->train_public_info.gps_data.valid.byte=0x1f;
			save_tz_data->train_public_info.gps_data.air_spring_pressure1=0x0a;
			save_tz_data->train_public_info.gps_data.air_spring_pressure2=0x0b;

			save_tz_data->train_public_info.gps_data.longitude_dir=69;
			save_tz_data->train_public_info.gps_data.latitude_dir=78;//E:69 W:87 N:78 S:83
			save_tz_data->train_public_info.gps_data.latitude_down=0x11;
			save_tz_data->train_public_info.gps_data.latitude_mid_down=0x22;
			save_tz_data->train_public_info.gps_data.latitude_mid_up=0x33;
			save_tz_data->train_public_info.gps_data.latitude_up=0x44;
			save_tz_data->train_public_info.gps_data.longitude_down=0x55;
			save_tz_data->train_public_info.gps_data.longitude_mid_down=0x66;
			save_tz_data->train_public_info.gps_data.longitude_mid_up=0x77;
			save_tz_data->train_public_info.gps_data.longitude_up=0x88;
			save_tz_data->train_public_info.marshalling.byte[0]=0x12;
			save_tz_data->train_public_info.marshalling.byte[1]=0x34;
			save_tz_data->train_public_info.gps_data.wheel1_value[0]=0;
			save_tz_data->train_public_info.gps_data.wheel1_value[1]=250;
			save_tz_data->train_public_info.gps_data.wheel2_value[0]=0;
			save_tz_data->train_public_info.gps_data.wheel2_value[1]=250;

			save_tz_data->train_public_info.train_style=1;
			save_tz_data->train_public_info.carriage_number=0;
			save_tz_data->train_public_info.minitor_trailer_flag=0x55;
			save_tz_data->train_public_info.train_outer_temp=32;
			save_tz_data->train_public_info.ctrl_train_mode=0xaa;
			save_tz_data->train_public_info.speed_30km_flag=0x1;
			save_tz_data->train_public_info.speed[0]=251;
#endif

			//memmove(save_tz_data->train_public_info.company_rev,recv_public_para.recv_ctrl_board_data.train_public_info.company_rev,sizeof(recv_public_para.recv_ctrl_board_data.train_public_info.company_rev));
#if defined(INTERNAL_PROTOCOL_20210416)
			memset(save_tz_data->sw_res,0,sizeof(save_tz_data->sw_res));
			memset(save_tz_data->company_res,0,sizeof(save_tz_data->company_res));
			//用来设置数据类型　01:自检过程数据　　02:工作过程数据  03:报警过程数据
			save_tz_data->company_res[25] = sw_data_save_type;

	#ifdef WTD_DATA_TRANSLATE_PROTOCOL
			memset(save_tz_data->data_head.res,0,sizeof(save_tz_data->data_head.res));
	#else
			*(uint16_t *)&save_tz_data->data_head.res[0] = little_to_big_16bit(comm_data_cnt.ctrla_eth_recv_valid_cnt);//控制板Ａ以太网正确包数
			*(uint16_t *)&save_tz_data->data_head.res[2] = little_to_big_16bit(comm_data_cnt.ctrla_can_recv_valid_cnt);//控制板ＡＣＡＮ正确包数
			*(uint16_t *)&save_tz_data->data_head.res[4] = little_to_big_16bit(comm_data_cnt.ctrlb_eth_recv_valid_cnt);//控制板Ｂ以太网正确包数
			*(uint16_t *)&save_tz_data->data_head.res[6] = little_to_big_16bit(comm_data_cnt.ctrlb_can_recv_valid_cnt);
	#endif

			*(uint16_t *)&save_tz_data->sw_res[0] = little_to_big_16bit(comm_data_cnt.ctrla_eth_recv_all_cnt);		//
			*(uint16_t *)&save_tz_data->sw_res[2] = little_to_big_16bit(comm_data_cnt.ctrla_can_recv_all_cnt);
			*(uint16_t *)&save_tz_data->sw_res[4] = little_to_big_16bit(comm_data_cnt.ctrlb_eth_recv_all_cnt);
			*(uint16_t *)&save_tz_data->sw_res[6] = little_to_big_16bit(comm_data_cnt.ctrlb_can_recv_all_cnt);
			*(uint16_t *)&save_tz_data->sw_res[8] = little_to_big_16bit(comm_data_cnt.send_eth_all_cnt);
			*(uint16_t *)&save_tz_data->sw_res[10] = little_to_big_16bit(comm_data_cnt.send_can_all_cnt);
#else
			memset(save_tz_data->km_scale,0,sizeof(save_tz_data->km_scale));
			memset(save_tz_data->resv,0,sizeof(save_tz_data->resv));
			//用来设置数据类型　01:自检过程数据　　02:工作过程数据  03:报警过程数据
			save_tz_data->resv[113] = sw_data_save_type;


			*(uint16_t *)&save_tz_data->km_scale[0] = little_to_big_16bit(comm_data_cnt.ctrla_eth_recv_all_cnt);		//
			*(uint16_t *)&save_tz_data->km_scale[2] = little_to_big_16bit(comm_data_cnt.ctrla_can_recv_all_cnt);
			*(uint16_t *)&save_tz_data->km_scale[4] = little_to_big_16bit(comm_data_cnt.ctrlb_eth_recv_all_cnt);
			*(uint16_t *)&save_tz_data->km_scale[6] = little_to_big_16bit(comm_data_cnt.ctrlb_can_recv_all_cnt);
			*(uint16_t *)&save_tz_data->km_scale[8] = little_to_big_16bit(comm_data_cnt.send_eth_all_cnt);
			*(uint16_t *)&save_tz_data->km_scale[10] = little_to_big_16bit(comm_data_cnt.send_can_all_cnt);
#endif
			save_tz_data->data_head.packet_num =0x05;
			*(uint16_t *)&save_tz_data->data_head.res[0] = little_to_big_16bit(comm_data_cnt.ctrla_eth_recv_valid_cnt);//控制板Ａ以太网正确包数
			*(uint16_t *)&save_tz_data->data_head.res[2] = little_to_big_16bit(comm_data_cnt.ctrla_can_recv_valid_cnt);//控制板ＡＣＡＮ正确包数
			*(uint16_t *)&save_tz_data->data_head.res[4] = little_to_big_16bit(comm_data_cnt.ctrlb_eth_recv_valid_cnt);//控制板Ｂ以太网正确包数
			*(uint16_t *)&save_tz_data->data_head.res[6] = little_to_big_16bit(comm_data_cnt.ctrlb_can_recv_valid_cnt);//控制板ＢＣＡＮ正确包数

		break;

		case UPDATE_SIMULATION_DATE:

			save_tz_data->sensor_status.bits.bogie_axis1_self_test_err = ptu_simulation_data.simulation_sw_st.bogie_one_sensor_self;
			save_tz_data->sensor_status.bits.bogie_axis2_self_test_err = 0;
			save_tz_data->sensor_status.bits.bogie_axis3_self_test_err =0;
			save_tz_data->sensor_status.bits.bogie_axis4_self_test_err = 0;
			save_tz_data->sensor_status.bits.bogie_axis1_real_time_err = ptu_simulation_data.simulation_sw_st.bogie_one_sensor_online;
			save_tz_data->sensor_status.bits.bogie_axis2_real_time_err = 0;
			save_tz_data->sensor_status.bits.bogie_axis3_real_time_err = 0;
			save_tz_data->sensor_status.bits.bogie_axis4_real_time_err = 0;

			save_tz_data->sensor_err.bits.axis1_sensor_err=ptu_simulation_data.simulation_sw_st.bogie_one_sensor_state;
			save_tz_data->sensor_err.bits.axis2_sensor_err=save_tz_data->sensor_status.bits.bogie_axis2_self_test_err|save_tz_data->sensor_status.bits.bogie_axis2_real_time_err;
			save_tz_data->sensor_err.bits.axis3_sensor_err=save_tz_data->sensor_status.bits.bogie_axis3_self_test_err|save_tz_data->sensor_status.bits.bogie_axis3_real_time_err;
			save_tz_data->sensor_err.bits.axis4_sensor_err=save_tz_data->sensor_status.bits.bogie_axis4_self_test_err|save_tz_data->sensor_status.bits.bogie_axis4_real_time_err;
			//继电器 暂不需要（默认填0）
			save_tz_data->relay_status.byte=0;

			//转向架 异常  预判
			save_tz_data->alarm_status.bits.bogie_axis1_alarm = ptu_simulation_data.simulation_sw_st.bogie_one_err;
			save_tz_data->alarm_status.bits.bogie_axis2_alarm = 0;
			save_tz_data->alarm_status.bits.bogie_axis3_alarm = 0;
			save_tz_data->alarm_status.bits.bogie_axis4_alarm = 0;
			save_tz_data->alarm_status.bits.bogie_alarm = ptu_simulation_data.simulation_sw_st.bogie_err|ptu_simulation_data.simulation_sw_st.bogie_one_err;
			save_tz_data->warn_status.bits.bogie_axis1_warn = ptu_simulation_data.simulation_sw_st.bogie_one_warning;
			save_tz_data->warn_status.bits.bogie_axis2_warn = 0;
			save_tz_data->warn_status.bits.bogie_axis3_warn = 0;
			save_tz_data->warn_status.bits.bogie_axis4_warn = 0;
			save_tz_data->warn_status.bits.bogie_warn = ptu_simulation_data.simulation_sw_st.bogie_warning;
			sw_boarderr.bits.save_err = ptu_simulation_data.simulation_sw_st.save_err;						//存储故障
			save_tz_data->borad_err.byte=sw_boarderr.byte;
			//ptu_simulation_data.simulation_sw_st.watchdog_reset=1;
			save_tz_data->borad_err.bits.sw_board_err=ptu_simulation_data.simulation_sw_st.board_err;
			if(sw_boarderr.bits.watchdog)
				save_tz_data->borad_err.bits.sw_board_err=1;
			save_tz_data->com_err.bits.ctrla_eth_err = ptu_simulation_data.simulation_sw_st.eth_A_err;
			save_tz_data->com_err.bits.ctrla_can_err = ptu_simulation_data.simulation_sw_st.can_A_err;
			save_tz_data->com_err.bits.ctrlb_eth_err =ptu_simulation_data.simulation_sw_st.eth_B_err;
			save_tz_data->com_err.bits.ctrlb_can_err = ptu_simulation_data.simulation_sw_st.can_B_err;

			break;


		default:
			break;
	}
}

void save_sw_tz_data(struct SW_TZ_DATA *save_tz_data,uint16_t singal_num,uint16_t target_addr,uint8_t resend_flag,uint8_t ack_flag)
{

	uint16_t write_res = 0;
	//	static uint16_t pack_cnt2 = 0;



	s1sw_tz_data.tzsave_data_head.save_data_head_h = 0x66;
	s1sw_tz_data.tzsave_data_head.save_data_head_l = 0xbb;
	*(uint16_t *)s1sw_tz_data.tzsave_data_head.data_len = little_to_big_16bit(sizeof(struct S1SW_TZ_DATA));
	s1sw_tz_data.tzsave_data_head.save_data_type = 0x09; //数据类型 byte4

	struct LOCAL_TIME time_now;
	get_local_time(&time_now);

	s1sw_tz_data.tzsave_data_head.year = time_now.year -2000;
	s1sw_tz_data.tzsave_data_head.month = time_now.mon;
	s1sw_tz_data.tzsave_data_head.day = time_now.day;
	s1sw_tz_data.tzsave_data_head.hour = time_now.hour;
	s1sw_tz_data.tzsave_data_head.min = time_now.min;
	s1sw_tz_data.tzsave_data_head.sec = time_now.sec;

	s1sw_tz_data.tzsave_data_head.pack_count_h = (singal_num >> 8) & 0xff;
	s1sw_tz_data.tzsave_data_head.pack_count_l =  singal_num & 0xff;
//	pack_cnt2++;

	s1sw_tz_data.tzsave_data_head.current_id_h = app_save_public.curr_id[0];
	s1sw_tz_data.tzsave_data_head.current_id_l = app_save_public.curr_id[1];
	s1sw_tz_data.tzsave_data_head.next_id_h = app_save_public.next_id[0];
	s1sw_tz_data.tzsave_data_head.next_id_l = app_save_public.next_id[1];

	s1sw_tz_data.tzsave_data_head.speed_h = app_save_public.speed[0];
	s1sw_tz_data.tzsave_data_head.speed_l = app_save_public.speed[1];

	s1sw_tz_data.tzsave_data_head.wheel_diameter_h = 0;
	s1sw_tz_data.tzsave_data_head.wheel_diameter_l = 0;

	s1sw_tz_data.tzsave_data_head.channel = 0;
	s1sw_tz_data.tzsave_data_head.host_slave_flag = 0;
	s1sw_tz_data.tzsave_data_head.train_id = pw_clb_config_para->trainnum;


	for(int i = 0;i < 38 ;i++)
		s1sw_tz_data.tzsave_data_head.reserve[i]  = 0;


	*(uint16_t *)s1sw_tz_data.check_sum = little_to_big_16bit(check_sum((uint8_t *)&s1sw_tz_data,sizeof(struct S1SW_TZ_DATA)-2));


	if((store_flag.hdd_exist_flag == FALSE) || (store_flag.hdd_err_flag == TRUE) || store_flag.hdd_save_flag == FALSE)		//SD卡不存在或者SD卡错误,就不再存数据
	{
		//DEBUG("not save_pw_tz_data\n");
		return;
	}

	write_res = fwrite(&s1sw_tz_data,sizeof(struct S1SW_TZ_DATA),1,sw_file.sw_tz_data.fp);

	if(write_res < 1)
	{
		DEBUG("write sw tz_data failed\n");
//		store_flag.hdd_err_flag = TRUE;
	}
	else
	{
		store_flag.hdd_err_flag = FALSE;
	}
#if 0
	//包头固定
	//数据包长度固定
	//厂家代码固定
	//设备代码固定

	/*以上测试屏蔽*/
	/*测试使用*/
	uint16_t write_res = 0;

//	static uint16_t pack_cnt3 = 0;

	s1sw_tz_data.tzsave_data_head.data_head_h = 0x66;
	s1sw_tz_data.tzsave_data_head.data_head_l = 0xbb;
	*(uint16_t *)s1sw_tz_data.tzsave_data_head.data_len = little_to_big_16bit(sizeof(struct S1SW_TZ_DATA));
	s1sw_tz_data.tzsave_data_head.data_type = 0x09; //数据类型 byte4

	struct LOCAL_TIME time_now;
	get_local_time(&time_now);

	s1sw_tz_data.tzsave_data_head.year = time_now.year - 2000;
	s1sw_tz_data.tzsave_data_head.month = time_now.mon;
	s1sw_tz_data.tzsave_data_head.day = time_now.day;
	s1sw_tz_data.tzsave_data_head.hour = time_now.hour;
	s1sw_tz_data.tzsave_data_head.min = time_now.min;
	s1sw_tz_data.tzsave_data_head.sec = time_now.sec;

	s1sw_tz_data.tzsave_data_head.pack_count_h = (singal_num >> 8) & 0xff;
	s1sw_tz_data.tzsave_data_head.pack_count_l =  singal_num & 0xff;
//	pack_cnt3++;

	s1sw_tz_data.tzsave_data_head.curr_station_id_h = app_save_public.curr_id[0];
	s1sw_tz_data.tzsave_data_head.curr_station_id_l = app_save_public.curr_id[1];
	s1sw_tz_data.tzsave_data_head.next_station_id_h = app_save_public.next_id[0];
	s1sw_tz_data.tzsave_data_head.next_station_id_l = app_save_public.next_id[1];

	s1sw_tz_data.tzsave_data_head.speed_h = app_save_public.speed[0];
	s1sw_tz_data.tzsave_data_head.speed_l = app_save_public.speed[1];

	s1sw_tz_data.tzsave_data_head.wheel_h = 0;
	s1sw_tz_data.tzsave_data_head.wheel_l = 0;

	s1sw_tz_data.tzsave_data_head.channel = 0;
	s1sw_tz_data.tzsave_data_head.host_flag = 0;
	s1sw_tz_data.tzsave_data_head.train_id = pw_clb_config_para->trainnum;

	s1sw_tz_data.tzsave_data_head.total_km[0] = 0;
	s1sw_tz_data.tzsave_data_head.total_km[1] = 0;
	s1sw_tz_data.tzsave_data_head.total_km[2] = 0;
	s1sw_tz_data.tzsave_data_head.total_km[3] = 0;

	s1sw_tz_data.tzsave_data_head.km_post[0] = 0;
	s1sw_tz_data.tzsave_data_head.km_post[1] = 0;

	s1sw_tz_data.tzsave_data_head.start_station[0] = 0;
	s1sw_tz_data.tzsave_data_head.start_station[1] = 0;

	s1sw_tz_data.tzsave_data_head.end_station[0] = 0;
	s1sw_tz_data.tzsave_data_head.end_station[1] = 0;

	s1sw_tz_data.tzsave_data_head.running_state = 0;
	s1sw_tz_data.tzsave_data_head.sensor_state[0] = 0;
	s1sw_tz_data.tzsave_data_head.sensor_state[1] = 0;
	s1sw_tz_data.tzsave_data_head.sensor_state[2] = 0;
	s1sw_tz_data.tzsave_data_head.train_number[0] = 0;
	s1sw_tz_data.tzsave_data_head.train_number[1] = 0;

	for(int i = 0;i < 22;i++)
		s1sw_tz_data.tzsave_data_head.reserve[22] = 0;
//	if(ack_flag == 1)
//	{
//		ack_flag = 0x5a;
//	}
//	else
//	{
//		ack_flag = 0;
//	}
//	*(uint16_t *)save_tz_data->data_head.head =little_to_big_16bit(0xAA50);
//	*(uint16_t *)save_tz_data->data_head.len = little_to_big_16bit(sizeof(struct SW_TZ_DATA));
//	save_tz_data->data_head.company_id = LUHANG;
//	save_tz_data->data_head.board_id = PW_BOARD;
//	*(uint16_t *)save_tz_data->data_head.life_signal = little_to_big_16bit(singal_num);
//
//	*(uint16_t *)save_tz_data->data_head.target_board_group = little_to_big_16bit(target_addr);
//	save_tz_data->data_head.resend_flag = resend_flag;
//	save_tz_data->data_head.ack_flag = ack_flag;

	//save_tz_data->resv[114] = sw_data_save_type;

	*(uint16_t *)s1sw_tz_data.check_sum = little_to_big_16bit(check_sum((uint8_t *)&s1sw_tz_data,sizeof(struct S1SW_TZ_DATA)-2));


//	printf("save_sw_tz_data--hdd_exist_flag:%x, hdd_err_flag:%x, hdd_save_flag:%x\n",
//			store_flag.hdd_exist_flag, store_flag.hdd_err_flag, store_flag.hdd_save_flag);
	if((store_flag.hdd_exist_flag == FALSE) || (store_flag.hdd_err_flag == TRUE) || store_flag.hdd_save_flag == FALSE)		//SD卡不存在或者SD卡错误,就不再存数据
	{
		//DEBUG("not save_sw_tz_data\n");
		return;
	}

#ifdef ADD_TZ_DATA_FILE
	write_res = fwrite(&s1sw_tz_data,sizeof(struct S1SW_TZ_DATA),1,sw_file.sw_tz_data.fp);
#else
	write_res = fwrite(save_tz_data,sizeof(struct SW_TZ_DATA),1,sw_file.sw_original_data.fp);
#endif
	if(write_res < 1)
	{
		printf("write tz_data failed\n");
		DEBUG("write tz_data failed\n");
		store_flag.hdd_err_flag = TRUE;
	}
#endif
}


void save_sw_original_data(uint16_t *save_buf ,uint16_t singal_num,uint8_t ch)
{
	uint16_t write_res = 0;
	uint16_t pack_cnt4 = 0;

	struct LOCAL_TIME time_now;
	get_local_time(&time_now);

//	*(uint16_t *)sw_raw_data.send_data_head.head = little_to_big_16bit(save_head);
//	*(uint16_t *)sw_raw_data.send_data_head.len = little_to_big_16bit(sizeof(struct SW_RAW_DATA));              //数据头24字节
//	sw_raw_data.send_data_head.company_id = LUHANG;            							  //板卡供应商编号 参考供应商定义
//	sw_raw_data.send_data_head.board_id = PW_BOARD;                                      //本身板卡编号 参考宏定义LOCAL_BOARD
//	*(uint16_t *)sw_raw_data.send_data_head.life_signal =  little_to_big_16bit(singal_num);       			//生命信号，每秒加1
//	*(uint16_t *)sw_raw_data.send_data_head.target_board_group = little_to_big_16bit(target_addr); 			//目标板卡的位集合
//	sw_raw_data.send_data_head.resend_flag = resend_flag;           //"0x55：表示首次发送该包数据，0xAA：表示重发该包数据；重发时的数据与首次发送的数据需全部一样，且仅对未应答的单板重发数据（通过Byte8-9来选型），超时时间为300ms，最多重发3次。"
//	sw_raw_data.send_data_head.ack_flag = ack_flag;              //"0x5A:目标板需要返回给请求板收到一包数据的应答帧   0x00:无需应答其它无效"
//	sw_raw_data.send_data_head.packet_num = 5;            //当前数据类型发送的总包数


	sw_raw_data.send_data_head.save_data_head_h = 0x66;
	sw_raw_data.send_data_head.save_data_head_l = 0xbb;
	*(uint16_t *)sw_raw_data.send_data_head.data_len = little_to_big_16bit(sizeof(struct SW_RAW_DATA));
	sw_raw_data.send_data_head.save_data_type = 0x0b; //数据类型 byte4



	sw_raw_data.send_data_head.year = time_now.year - 2000;
	sw_raw_data.send_data_head.month = time_now.mon;
	sw_raw_data.send_data_head.day = time_now.day;
	sw_raw_data.send_data_head.hour = time_now.hour;
	sw_raw_data.send_data_head.min = time_now.min;
	sw_raw_data.send_data_head.sec = time_now.sec;

	sw_raw_data.send_data_head.pack_count_h = (singal_num >> 8) & 0xff;
	sw_raw_data.send_data_head.pack_count_l =  singal_num & 0xff;
	sw_raw_data.send_data_head.current_id_h = app_save_public.curr_id[0];
	sw_raw_data.send_data_head.current_id_l = app_save_public.curr_id[1];
	sw_raw_data.send_data_head.next_id_h = app_save_public.next_id[0];
	sw_raw_data.send_data_head.next_id_l = app_save_public.next_id[1];

	sw_raw_data.send_data_head.speed_h = app_save_public.speed[0];;
	sw_raw_data.send_data_head.speed_l = app_save_public.speed[1];;

	sw_raw_data.send_data_head.wheel_diameter_h = 0;
	sw_raw_data.send_data_head.wheel_diameter_l = 0;

	sw_raw_data.send_data_head.channel = ch + 1;
	sw_raw_data.send_data_head.host_slave_flag = 0;
	sw_raw_data.send_data_head.train_id = pw_clb_config_para->trainnum;
	for(int i = 0;i < 38 ;i++)
		sw_raw_data.send_data_head.reserve[i]  = 0;

//#ifdef ORIGINAL_SAVE_PUBLIC_INFO
//	extern struct SW_TZ_DATA sw_tz_data;
//	/*原始数据存储日期、编组编号、速度、车箱号, PTU解析对应，以便数据中心数据核对*/
//	memset(sw_raw_data.send_data_head.res,0,sizeof(sw_raw_data.send_data_head.res));
//	memmove(&sw_raw_data.send_data_head.res[0], &sw_tz_data.train_public_info.year, 10);
//	sw_raw_data.send_data_head.res[10] = sw_tz_data.train_public_info.carriage_number;
//#else
//	memset(sw_raw_data.send_data_head.res,0,sizeof(sw_raw_data.send_data_head.res));
//#endif
	//用来设置数据类型　01:自检过程数据　　02:正常工作过程数据  03:报警过程数据
//	sw_raw_data.send_data_head.res[0] = sw_data_save_type;
//
//	printf("sw_data_save_type:%d\n",sw_data_save_type);
//	printf("sw_raw_data.send_data_head.res[0]:%d\n",sw_raw_data.send_data_head.res[0]);


	memmove(sw_raw_data.ad,save_buf,512*2);

	*(uint16_t *)sw_raw_data.check_sum = little_to_big_16bit(check_sum((uint8_t *)&sw_raw_data,sizeof(struct SW_RAW_DATA)-2));


	if((store_flag.hdd_exist_flag == FALSE) || (store_flag.hdd_err_flag == TRUE) || store_flag.hdd_save_flag == FALSE)		//SD卡不存在或者SD卡错误,就不再存数据
	{
		printf("not save_sw_original_data\n");
		DEBUG("not save_sw_original_data\n");
		return;
	}

	write_res = fwrite(&sw_raw_data,sizeof(struct SW_RAW_DATA),1,sw_file.sw_original_data.fp);

	if(write_res < 1)
	{
		printf("write sw_original_data _data failed\n");
		DEBUG("write tz_data failed\n");
	}
}
