#ifndef _HDD_SAVE_H_
#define _HDD_SAVE_H_


#include <stdio.h>
#include "user_data.h"
#include "global_macro.h"


#if defined(HDD_DEV_MMCBLK0P3_OR_TF)
    #define HDD_DEV 		"/dev/mmcblk1"  //核心板EMMC:8G或4G
	#define HDD_DEV1 		"/dev/mmcblk0p1"//TF卡:32G
    #define HDD_DEV2 		"/dev/mmcblk0p3"//核心板EMMC第三分区:6G或2.4G
	#define MOUNT_DEV		"/media/pw_data"//"/run/media/mmcblk0p3"
#elif defined(HDD_DEV_MMCBLK0P3_REMOVE_TF)
	#define HDD_DEV 		"/dev/mmcblk0p3"  //核心板EMMC第三分区:6G
	#define MOUNT_DEV		"/media/pw_data"//"/run/media/mmcblk0p3"
#else
	#define HDD_DEV 		"/dev/mmcblk0p1" //TF卡设备:32G
	#define MOUNT_DEV		"/media/pw_data"//"/run/media/mmcblk0p1"
#endif

#define PW_DATA_DIR "/media/pw_data/"

#define SW_DATA_DIR "/media/sw_data/"
//#define PW_DATA_DIR "/media/sw_data/"
#define LOCAL_TEST_FILE_NAME "sd_test.txt"

#ifdef DIAG_DATA_SAVE_FOR_TEST
//	#define DIAG_DATA_CSV_DIR 		"/media/pw_data/"

	#ifdef PW_DIAG
		#define DIAG_DATA_PWTZZ_FILE 	"/media/pw_data/pwtzz_diag.csv"
	#endif
	#ifdef JT_DIAG
		#define DIAG_DATA_JTTZZ_FILE 	"/media/pw_data/jttzz_diag.csv"
	#endif
	#ifdef DC_DIAG
		#define DIAG_DATA_DCTZZ_FILE 	"/media/pw_data/dctzz_diag.csv"
	#endif
	#ifdef HC_DIAG
		#define DIAG_DATA_HCTZZ_FILE 	"/media/pw_data/hctzz_diag.csv"
	#endif
	#ifdef CT_10S_DIAG
		#define DIAG_DATA_TENTZZ_FILE 	"/media/pw_data/tentzz_diag.csv"
	#endif
#endif

#ifdef DIAG_DATA_SAVE_FOR_TEST
typedef enum DIAG_DETAIL_DATA_TYPE{
	DIAG_DATA_NUMBER_TITLE,

	DIAG_DATA_CHANNEL_TITLE,
	DIAG_DATA_CHANNEL,

	DIAG_DATA_SPEED_TITLE,
	DIAG_DATA_SPEED,

	DIAG_DATA_ACC_TITLE,
	DIAG_DATA_ACC,

	DIAG_DATA_AD_TITLE,
	DIAG_DATA_AD,

	DIAG_DATA_FFT_AMP_TITLE,
	DIAG_DATA_FFT_AMP,

	DIAG_DATA_BEFORE_FILTER_TITLE,
	DIAG_DATA_BEFORE_FILTER,

	DIAG_DATA_AFTER_FILTER_TITLE,
	DIAG_DATA_AFTER_FILTER,

	DIAG_DATA_40HZ_FFT_AMP_TITLE,
	DIAG_DATA_40HZ_FFT_AMP,

	DIAG_DATA_1S_FFT_AMP_TITLE,
	DIAG_DATA_1S_FFT_AMP,

	DIAG_DATA_5S_MEAN_FFT_AMP_TITLE,
	DIAG_DATA_5S_MEAN_FFT_AMP,

	DIAG_DATA_PW_TZZ_SUM_TITLE,
	DIAG_DATA_PW_TZZ_SUM,

	DIAG_DATA_PW_TZZ_TITLE,
	DIAG_DATA_PW_TZZ

#ifdef CT_10S_DIAG
	,
	DIAG_DATA_MAX_FFT_AMP_TITLE,
	DIAG_DATA_MAX_FFT_AMP,

	DIAG_DATA_PERCENT_MAIN_FREQ_TITLE,
	DIAG_DATA_PERCENT_MAIN_FREQ,

	DIAG_DATA_MAIN_FREQ_TITLE,
	DIAG_DATA_MAIN_FREQ,

	//0.2-3Hz
	DIAG_DATA_MAX_FFT_AMP1_TITLE,
	DIAG_DATA_MAX_FFT_AMP1,

	DIAG_DATA_PERCENT_MAIN_FREQ1_TITLE,
	DIAG_DATA_PERCENT_MAIN_FREQ1,

	DIAG_DATA_MAIN_FREQ1_TITLE,
	DIAG_DATA_MAIN_FREQ1,

	//5-13Hz
	DIAG_DATA_MAX_FFT_AMP2_TITLE,
	DIAG_DATA_MAX_FFT_AMP2,

	DIAG_DATA_PERCENT_MAIN_FREQ2_TITLE,
	DIAG_DATA_PERCENT_MAIN_FREQ2,

	DIAG_DATA_MAIN_FREQ2_TITLE,
	DIAG_DATA_MAIN_FREQ2,

	DIAG_DATA_MAX_FABS_AMP3_TITLE,
	DIAG_DATA_MAX_FABS_AMP3,

	//1-3Hz  10s
	DIAG_DATA_MAX_FFT_AMP3_TITLE,
	DIAG_DATA_MAX_FFT_AMP3,

	DIAG_DATA_PERCENT_MAIN_FREQ3_TITLE,
	DIAG_DATA_PERCENT_MAIN_FREQ3,

	DIAG_DATA_MAIN_FREQ3_TITLE,
	DIAG_DATA_MAIN_FREQ3,

	DIAG_DATA_AVERAGE_SPEED_TITLE,
	DIAG_DATA_AVERAGE_SPEED,

	DIAG_DATA_BRIDGE_LEN_TITLE,
	DIAG_DATA_BRIDGE_LEN,

	DIAG_DATA_AVERAGE_BRIDGE_LEN_TITLE,
	DIAG_DATA_AVERAGE_BRIDGE_LEN,

	DIAG_DATA_BRIDGE_FLAG_TITLE,
	DIAG_DATA_BRIDGE_FLAG,

	DIAG_DATA_RESAMPLE_AD_TITLE,
	DIAG_DATA_RESAMPLE_AD
#endif
#ifdef HC_DIAG
	,
	DIAG_DATA_FFZ_TITLE,
	DIAG_DATA_FFZ,

	DIAG_DATA_FZ_TITLE,
	DIAG_DATA_FZ,

	DIAG_DATA_HC_TZZ_TITLE,
	DIAG_DATA_HC_TZZ
#endif
#ifdef JT_DIAG
	,
	DIAG_DATA_AVG_SPEED_TITLE,
	DIAG_DATA_AVG_SPEED,

	DIAG_DATA_JT_MAX_FRE_TITLE,
	DIAG_DATA_JT_MAX_FRE,

	DIAG_DATA_JT_TZZ_TITLE,
	DIAG_DATA_JT_TZZ
#endif
#ifdef DC_DIAG
	,
	DIAG_DATA_5S_AFTER_FILTER_TITLE,
	DIAG_DATA_5S_AFTER_FILTER,

	DIAG_DATA_DC_TZZ_TITLE,
	DIAG_DATA_DC_TZZ
#endif
}DIAG_DETAIL_DATA_E;
#endif

/**
 * 存储数据类型
 */
typedef enum SAVE_DATA_TYPE_EN
{
	PW_DATA_TYPE,
	PWTZZ_TYPE,
	JTTZZ_TYPE,
	DCTZZ_TYPE,
	HCTZZ_TYPE,
	TENTZZ_TYPE
}SAVE_DATA_TYPE;


struct FILE_INF
{
	char name[256];
	FILE *fp;
	int fd;
	uint32_t size;
};


struct PW_FILE
{
	struct FILE_INF pw_original_data;
#ifdef ADD_TZ_DATA_FILE
	struct FILE_INF pw_tz_data;
#endif
#ifdef ADD_DIAG_TZZ_DATA_FILE
	struct FILE_INF pw_diag_tzz_data;
#endif
#ifdef ADD_LOG_DATA
	struct FILE_INF log_data;
#endif
	struct FILE_INF test_data;
	struct FILE_INF check_data;
	struct FILE_INF err_log_data;
#ifdef DIAG_DATA_SAVE_FOR_TEST
	#ifdef PW_DIAG
		struct FILE_INF pwtzz_diag_data;
	#endif
	#ifdef JT_DIAG
		struct FILE_INF jttzz_diag_data;
	#endif
	#ifdef DC_DIAG
		struct FILE_INF dctzz_diag_data;
	#endif
	#ifdef HC_DIAG
		struct FILE_INF hctzz_diag_data;
	#endif
	#ifdef CT_10S_DIAG
		struct FILE_INF tentzz_diag_data;
	#endif
#endif
#ifdef CAN_ERR_REBOOT_TWO_TIMES
	struct FILE_INF reboot;
#endif
};

struct SW_FILE
{
	struct FILE_INF sw_original_data;
#ifdef ADD_TZ_DATA_FILE
	struct FILE_INF sw_tz_data;
#endif
	struct FILE_INF log_data;
	struct FILE_INF test_data1;
	struct FILE_INF test_data2;
	struct FILE_INF err_log_data;
#ifdef DIAG_DATA_SAVE_FOR_TEST
	struct FILE_INF diag_swtzz_data;
#endif
#ifdef CAN_ERR_REBOOT_TWO_TIMES
	struct FILE_INF reboot;
#endif
};

/*获取ＳＤ卡大小类型*/
enum GET_SD_SIZE_TYPE
{
	SD_TOTAL_SIZE,
	SD_USED_SIZE,
	SD_FREE_SIZE,
};

void test_pw_save();
int creat_dir(char *file_dir);
void init_pw_save();

void save_sw_original_data(uint16_t *save_buf,uint16_t singal_num,uint8_t ch);
void save_pw_original_data(uint16_t *save_buf,uint16_t singal_num,uint8_t ch);
//#ifdef ORIGINAL_DATA_SAVE_ACC
//	void save_pw_original_data(int16_t *save_buf,uint16_t save_head,uint16_t singal_num,uint16_t target_addr,uint8_t resend_flag,uint8_t ack_flag);
//#else
//	void save_pw_original_data(uint16_t *save_buf,uint16_t save_head,uint16_t singal_num,uint16_t target_addr,uint8_t resend_flag,uint8_t ack_flag);
//#endif
uint32_t get_memoccupy(void);
int init_file_del_thread();
void file_del_thread_entry();
uint8_t check_sdk_file(void);
unsigned long long int get_sd_size(char *dev_path,enum GET_SD_SIZE_TYPE type,uint8_t length);
void wirte_err_log_data(enum ERR_EVENT err_event_log,enum SENSOR_MEASURING_POINT sensor_measuring_point,enum ERR_DES err_des_log);
void update_err_log_data(struct SYS_STATUS_CNT sys_status);
void open_err_log_data_file();
void reset_pw_file();
void sd_exist_test(void);
int my_system(const char * cmd);
int remove_dir(const char *dir);
void update_pw_tz_data(struct PW_TZ_DATA *save_tz_data,void *tzdata_temp,uint8_t update_tz_flag);

#ifdef DIAG_DATA_SAVE_FOR_TEST
	uint32_t get_one_file_size(const char *path);
	#ifdef PW_DIAG
		int create_pwtzz_diag_file();
		void save_pwtzz_diag_file(SAVE_DATA_TYPE type, DIAG_DETAIL_DATA_E detail_type, void *buff, uint32_t size);
		void close_pwtzz_diag_file(SAVE_DATA_TYPE type);
	#endif
	#ifdef JT_DIAG
		int create_jttzz_diag_file();
		void save_jttzz_diag_file(SAVE_DATA_TYPE type, DIAG_DETAIL_DATA_E detail_type, void *buff, uint32_t size);
		void close_jttzz_diag_file(SAVE_DATA_TYPE type);
	#endif
	#ifdef DC_DIAG
		int create_dctzz_diag_file();
		void save_dctzz_diag_file(SAVE_DATA_TYPE type, DIAG_DETAIL_DATA_E detail_type, void *buff, uint32_t size);
		void close_dctzz_diag_file(SAVE_DATA_TYPE type);
	#endif
	#ifdef HC_DIAG
		int create_hctzz_diag_file();
		void save_hctzz_diag_file(SAVE_DATA_TYPE type, DIAG_DETAIL_DATA_E detail_type, void *buff, uint32_t size);
		void close_hctzz_diag_file(SAVE_DATA_TYPE type);
	#endif
	#ifdef CT_10S_DIAG
		int create_tentzz_diag_file();
		void save_tentzz_diag_file(SAVE_DATA_TYPE type, DIAG_DETAIL_DATA_E detail_type, void *buff, uint32_t size);
		void close_tentzz_diag_file(SAVE_DATA_TYPE type);
	#endif
#endif

#endif
