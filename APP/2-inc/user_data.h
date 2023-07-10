#ifndef _USER_DATA_H_
#define _USER_DATA_H_
#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>
#include "can_data_deal.h"
//#include "ptu_app.h"
#include "pw_diagnos.h"
#include "global_macro.h"
#include "pswb_record_prot.h"

//#define __DEBUG__
#ifdef __DEBUG__
#define DEBUG(format,...)  printf(format,##__VA_ARGS__)//    printf//
#else
#define DEBUG(format,...)
#endif

#ifdef COMM_CHECK_TIMEOUT_INC
//	#define  CTRLA_CAN_COMM_ERR_TIMES  5//5*0.5s=2.5s内接收数据正确清零
//	#define  CTRLB_CAN_COMM_ERR_TIMES  5
//	#define  CTRLA_ETH_COMM_ERR_TIMES  5
//	#define  CTRLB_ETH_COMM_ERR_TIMES  5
	#define  CAN_FIRST_REBOOT_TIMEOUT_S  5

	#define  CTRLA_CAN_TIMEOUT_S  16//16*0.5s=8s内未接收到
	#define  CTRLB_CAN_TIMEOUT_S  16
//	#define  CTRLA_ETH_TIMEOUT_S  10
//	#define  CTRLB_ETH_TIMEOUT_S  10
//	#define  IO_CAN_TIMEOUT_S 	  5
//	#define  SAVE_CAN_TIMEOUT_S   16
//	#define  ALL_ETH_TIMEOUT_S   4//超时重发,未跑, //实际跑是无超时重发  //客户addr对了connect_flag就清零
#else
//	#define  CTRLA_CAN_COMM_ERR_TIMES  5 //5*0.5s=2.5s内接收数据正确清零
//	#define  CTRLB_CAN_COMM_ERR_TIMES  5
//	#define  CTRLA_ETH_COMM_ERR_TIMES  5
//	#define  CTRLB_ETH_COMM_ERR_TIMES  5
	#define  CAN_FIRST_REBOOT_TIMEOUT_S  5

	#define  CTRLA_CAN_TIMEOUT_S  10//10*0.5s=5s内未接收到
	#define  CTRLB_CAN_TIMEOUT_S  10
//	#define  CTRLA_ETH_TIMEOUT_S  10
//	#define  CTRLB_ETH_TIMEOUT_S  10
//	#define  IO_CAN_TIMEOUT_S 	  5
//	#define  SAVE_CAN_TIMEOUT_S   10
//	#define  ALL_ETH_TIMEOUT_S   4//超时重发,未跑, //实际跑是无超时重发  //客户addr对了connect_flag就清零
#endif


#define CAN_BOARD_NUMB  PW_BOARD_ADDR

#define RECORD_BOARD 0x11
#define CTRLA_BOARD 0x11
#define CTRLB_BOARD 0x22
#define SAVE_BOARD 0x33
#define AXLE_TEMPA_BOARD 0x44
#define AXLE_TEMPB_BOARD 0x55
#define PW_BOARD 0X77
#define SW_BOARD 0x66
#define VIBR_BOARD 0x88
#define IO_BOARD 0X99
#define GEAR_BOARD 0XAA
#define MOTOR_BOARD 0XBB
#define RESEND_CNT 3//表示重发3次

#define LOCAL_BOARD PW_BOARD
#define BOARD_NUMB  LOCAL_BOARD
#define PROT_HEAD_LEN 26

#define SAMPLE_HZ			512											  //平稳采样频率
#define PW_CH_NUM			6											  //平稳通道数量
#define	PW_AD_DATA_TOP		2											  //每包数据头列数
#define PW_AD_DATA_COL		PW_AD_DATA_TOP + SAMPLE_HZ*PW_CH_NUM			//平稳数据列数
#define PW_AD_DATA_ROW		4												//平稳数据行数  //4s data

//#define SAMPLE_HZ			512											  //平稳采样频率
#define SW_CH_NUM			2											  //平稳通道数量
#define	SW_AD_DATA_TOP		2											  //每包数据头列数
#define SW_AD_DATA_COL		SW_AD_DATA_TOP + SAMPLE_HZ*SW_CH_NUM			//平稳数据列数
#define SW_AD_DATA_ROW		4
extern uint8_t pw_data_save_type;

#define FS_SW 512
/*京雄高铁相关定义*/


//    "0x01：时代
//     0x02：华高
//     0x03：纵横
//     0x04：路航
//     0x05：四方所
//     0x06：紫台
//     0x07：柏盛源"
enum COMPANY_ID
{
    SHIDAI = 0x01,
    HUAGAO = 0x02,
    ZONGHENG = 0x03,
    LUHANG = 0x04,
    SIFANGSUO = 0x05,
    ZITAI = 0x06,
    BAISHENGYUAN = 0x07
};

//更新诊断结果标志
enum UPDATE_DIAG_RES
{
	UPDATE_DIAG_DATE,
	UPDATE_OTHER__DATE,
	UPDATE_SIMULATION_DATE
};

struct UINT16_TYPE
{
	uint8_t wh;	//高位字节
	uint8_t wl;	//低位字节
};//大端模式

struct ETH_STATUS
{
	uint32_t not_recv_cnt;
	uint32_t connect_flag;
};

struct CAN_STATUS
{
	uint32_t not_recv_cnt;
	uint32_t connect_flag;
};

struct FILE_OPERTE
{
	uint8_t file_delete_flag;
	uint8_t time_update_flag;
};

// 1	IO	    192.168.1.11	   8000
// 2	控制板A	 192.168.1.12	    8000
// 3	控制板B	 192.168.1.13	    8000
// 4	记录板	 192.168.1.14	    8000
// 5	轴温A	 192.168.1.15	    8000
// 6	轴温B	 192.168.1.16	    8000
// 7	平稳	 192.168.1.17	    8000
// 8	失稳	 192.168.1.18	    8000
// 9	振动	 192.168.1.19	    8000
#define MCAST_CTL_SAVE_GROUP_ADDR "239.255.10.2"
#define MCAST_CTL_SAVE_GROUP_PORT 9002

#define TRAIN_INFO_GROUP_ADDR "239.255.10.1"
#define TRAIN_INFO_GROUP_PORT 9001

#define UCAST_IO_BOARD_ADDR "192.168.40.11"					//"192.168.1.11"
#define UCAST_IO_BOARD_PORT 8000

#define UCAST_CTRLA_BOARD_ADDR "192.168.40.222"
#define UCAST_CTRLA_BOARD_PORT 8000

#define UCAST_CTRLB_BOARD_ADDR "192.168.40.13"
#define UCAST_CTRLB_BOARD_PORT 8000

#define UCAST_SAVE_BOARD_ADDR "192.168.40.14"
#define UCAST_SAVE_BOARD_PORT 8000

#define UCAST_AXLE_TEMPA_BOARD_ADDR "192.168.1.15"
#define UCAST_AXLE_TEMPA_BOARD_PORT 8000

#define UCAST_AXLE_TEMPB_BOARD_ADDR "192.168.1.16"
#define UCAST_AXLE_TEMPB_BOARD_PORT 8000

#define UCAST_ST_BOARD_ADDR "192.168.40.201"					//40.17
#define UCAST_ST_BOARD_PORT 8000

#define UCAST_ST_PTU_ADDR "192.168.40.76"
#define UCAST_ST_PTU_PORT 8000

#define UCAST_AD_BOARD_ADDR "192.168.1.18"
#define UCAST_AD_BOARD_PORT 8000

#define UCAST_VIBR_BOARD_ADDR "192.168.1.19"
#define UCAST_VIBR_BOARD_PORT 8000

#define UCAST_GEAR_BOARD_ADDR "192.168.1.20"
#define UCAST_GEAR_BOARD_PORT 8000

#define UCAST_MOTOR_BOARD_ADDR "192.168.1.21"
#define UCAST_MOTOR_BOARD_PORT 8000
// 0x11：控制板A
// 0x22：控制板B
// 0x33：记录板
// 0x44：轴温A
// 0x55：轴温B
// 0x66：平稳
// 0x77：失稳
// 0x88：振动
// 0x99：IO板
// 其他预留

enum SW_SENSOR_CH
{
	sw1_y,
	sw2_y,
	sw3_y,
	sw4_y
};

struct TRAIN_SOFT_VERSION
{
 uint32_t  version;  //BCD码,V12.34 大版本号
 uint32_t  update_time; //更新时间
 uint32_t  small_version; //小版本号,供内部使用
};

enum PW_DATA_TYPE
{
	SENSOR_SELF_TEST_DATA = 0x01,
	SENSOR_WORKIONG_DATA = 0x02,
	SENSOR_WARN_DATA = 0x03,
	SENSOR_ALARM_DATA = 0x04
};

/*
Bit0：控制板A
Bit1：控制板B
Bit2：记录板
Bit3：轴温A
Bit4：轴温B
Bit5：平稳
Bit6：失稳
Bit7：振动
Bit8：IO板
Bit9-14：预留
该位为“0”：目标板忽略该包数据
该位为“1”：目标板接收该包数据
广播发送时选择多板bit置“1”，单播时仅对目标板bit置“1”。
res[11]:13-23
兼容测试用：
Byte13-14：控制A以太网正确包数；res0 1
Byte15-16：控制A CAN正确包数； res2 3
Byte17-18：控制B以太网正确包数；res4 5
Byte19-20：控制B CAN正确包数；res6 7
Byte21-22：测试总数据包数res8 9
Byte23：测试启动（0xAA）/停止（0x55）标志 res10 11
*/
struct SEND_DATA_HEAD
{
    uint8_t head[2];               //"0xAA50：过程数据包0xAA51-0xAA54：原始数据包（根据实际增减）
    uint8_t len[2];                //振动过程数据为360，失稳数据为256 平稳为256 原始为1024 轴温为256
    uint8_t company_id;            //板卡供应商编号 参考供应商定义
    uint8_t board_id;              //本身板卡编号 参考宏定义LOCAL_BOARD
    uint8_t life_signal[2];        //生命信号，每秒加1
    uint8_t target_board_group[2]; //目标板卡的位集合
    uint8_t resend_flag;           //"0x55：表示首次发送该包数据，0xAA：表示重发该包数据；重发时的数据与首次发送的数据需全部一样，且仅对未应答的单板重发数据（通过Byte8-9来选型），超时时间为300ms，最多重发3次。"
    uint8_t ack_flag;              //"0x5A:目标板需要返回给请求板收到一包数据的应答帧   0x00:无需应答其它无效"
    uint8_t packet_num;            //当前数据类型发送的总包数
    uint8_t res[11];               //预留
};//24


typedef struct save_data_head
{
		uint8_t save_data_head_h; //包头 H  byte0 66 (存储包头：66bb)
		uint8_t save_data_head_l; //包头 L  byte1 bb (存储包头：66bb)
		uint8_t data_len[2]; //数据包长度   byte2-3

		uint8_t save_data_type; //数据类型 byte4
		uint8_t year;  //年 byte5
		uint8_t month; //月 byte6
		uint8_t day;   //日 byte7
		uint8_t hour;  //时 byte8
		uint8_t min;   //分 byte9
		uint8_t sec;   //秒 byte10

		uint8_t pack_count_h; //数据包计数H byte11
		uint8_t pack_count_l; //数据包计数L byte12
		uint8_t current_id_h; //当前站ID H byte13
		uint8_t current_id_l; //当前站ID L byte14
		uint8_t next_id_h; //下一站ID H byte15

		uint8_t next_id_l; //下一站ID L byte16
		uint8_t speed_h; //速度 H byte17
		uint8_t speed_l; //速度 L byte18
		uint8_t wheel_diameter_h; //轮径 H byte19

		uint8_t wheel_diameter_l; //轮径 L byte20
		uint8_t channel; //adg1608片选模拟通道 byte21
		uint8_t host_slave_flag; //主从机标识 byte22 //适合主从机记录板自己记录的机型
		uint8_t train_id; //车厢号ID byte23 //适合主机记录板统一存所有车厢信息的机型
		uint8_t reserve[38];
}psw_save_data_head_t, * psw_save_data_head_p;


/* tz 数据的包头结构 head */
//用于存储特征数据的每个的公共部分
typedef struct //24 byte
{
	uint8_t data_head_h;   //报文头H: 0x66 byte0
	uint8_t data_head_l;   //报文头L: 0xcc byte1
//	uint8_t data_len_h;    //报文总长度H byte2
//	uint8_t data_len_l;    //报文总长度L byte3
	uint8_t data_len[2];
	uint8_t data_type;   //byte4 存储的原始数据类型,类型对应，参看 tz_type_e
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;           //byte 10
	uint8_t pack_count_h;  //包计数H，顺序计数 byte 11
	uint8_t pack_count_l;  //包计数L，顺序计数 byte 12
	uint8_t curr_station_id_h; //byte 13
	uint8_t curr_station_id_l; //byte 14
	uint8_t next_station_id_h; //byte 15
	uint8_t next_station_id_l; //byte 16
	uint8_t speed_h;           //byte 17
	uint8_t speed_l;           //byte 18
	uint8_t wheel_h;           //byte 19
	uint8_t wheel_l;           //byte 20
	uint8_t channel;            //byte 21 振动采集的通道号，1-8
	uint8_t host_flag;         //byte 22 主从机标识（不一定使用）
	uint8_t train_id;          //byte 23 车厢号
	uint8_t total_km[4];       //byte 24-27 总里程
	uint8_t km_post[2];        //byte 28-29 公里标
	uint8_t start_station[2];  //byte 30-31 起点站
	uint8_t end_station[2];    //byte 32-33 终点站
	uint8_t running_state;     //byte 34 从公共信息中取得列车运行状态，包括：上行、下行、TC1司机室激活，TC2司机室激活
	uint8_t sensor_state[3]; //byte 35-37 所有传感器的状态
	uint8_t train_number[2];   //byte 38-39
	uint8_t reserve[22];
}psw_save_tzdata_head_t;

/*
接收控制板发送的公共信息的数据协议头
*/

union VALID_INFO
{
	struct
	{
		uint8_t time_valid:  1;					//时间有效
		uint8_t group_valid: 1;					//编组有效
		uint8_t speed_valid: 1;					//速度有效
		uint8_t battry_off:  1;					//蓄电池off
		uint8_t err_reset:	 1;					//故障复位
		uint8_t ccu_owner:	 2;					//ＣＣＵ配属
		uint8_t nc:			 1;					//预留
	}BITS;
	uint8_t byte;
};

struct RECV_DATA_HEAD
{
	uint8_t ctrl_life[2];
    uint8_t ccu_life[2];             //"0xAA50
    union VALID_INFO valid_info;
};


#ifdef INTERNAL_PROTOCOL_20210416
struct PW_SIMULATION{

  //以下所有状态  0--正常    1--故障

	uint8_t eth_A_err : 1; // 控制板A以太网通信故障
	uint8_t can_A_err : 1; // 控制板B以太网通信故障
	uint8_t eth_B_err : 1;// 控制板A CAN 通信故障
	uint8_t can_B_err : 1;// 控制板B CAN通信故障
	uint8_t board_err : 1;//板卡故障
	uint8_t save_err : 1; //存储故障

	uint8_t one_sensor_self_state: 1;   //1位端加速度传感器自检故障
	uint8_t one_sensor_online_state : 1;//1位端加速度传感器实时故障

	uint8_t one_sensor_state : 1;       //1位端加速度传感器状态
	uint8_t one_warning_z : 1;          //1位端垂向平稳性报警状态
	uint8_t one_warning_y : 1;          //1位端横向平稳性报警状态
	uint8_t one_warning_x : 1;          //1位端纵向平稳性报警状态
	uint8_t collect_state : 1;          //采集接口故障

	uint8_t dc_yj_state : 1;			//1位端抖车预警状态
	uint8_t dc_bj_state : 1;			//1位端抖车报警状态
	uint8_t hc_yj_state : 1;			//1位端晃车预警状态
	uint8_t hc_bj_state : 1;			//1位端晃车报警状态
	uint8_t pw_yj_z_state : 1;		//1位端垂向平稳性预警状态
	uint8_t pw_yj_y_state : 1;		//1位端横向平稳性预警状态

	uint8_t reserve : 5;

 };
#else
struct PW_SIMULATION{

  //以下所有状态  0--正常    1--故障
  uint8_t eth_A_err : 1; // 控制板A以太网通信故障
  uint8_t can_A_err : 1; // 控制板B以太网通信故障
  uint8_t eth_B_err : 1;// 控制板A CAN 通信故障
  uint8_t can_B_err : 1;// 控制板B CAN通信故障
  uint8_t board_err : 1;//板卡故障
  uint8_t save_err : 1;//存储故障

  uint8_t one_sensor_self_state: 1;  //1位端加速度传感器自检故障
  uint8_t one_sensor_online_state : 1;//1位端加速度传感器实时故障

  uint8_t one_sensor_state : 1;      // 1位端加速度传感器状态
  uint8_t one_warning_z : 1;     // 1位端垂向平稳性报警状态
  uint8_t one_warning_y : 1;     //1位端横向平稳性报警状态
  uint8_t one_warning_x : 1;     //1位端纵向平稳性报警状态
  uint8_t collect_state : 1;     //采集接口故障

  uint8_t reserve : 3;

 };
#endif
/**
Byte36	年
Byte37	月
Byte38	日
Byte39	时
Byte40	分
Byte41	秒
Byte42	车型
Byte43	车厢号
Byte44	MVB通讯方式
Byte45	以太网通讯方式
Byte46-53	厂家数据
Byte54:0-3	编组编号（个位）
Byte54:4-7	编组编号（十位）
Byte54:0-3	编组编号（百位）
Byte54:4-7	编组编号（千位）
Byte55-Byte56	全车速度
Byte57	车外温度
Byte58	控车模式选择
Byte59：0	空簧压力有效
Byte59：1	GPS经纬有效
Byte60	空气弹簧压力1
Byte61	空气弹簧压力2
Byte62	经度（中下位）
Byte63	经度（下位）
Byte64	经度（上位）
Byte65	经度（中上位）
Byte66	经度方向ASCII（unsigned char）
Byte67	经度方向ASCII（unsigned char）
Byte68	纬度（中下位）
Byte69	纬度（下位）
Byte70	纬度（上位）
Byte71	纬度（中上位）
 */
struct RECV_TRAIN_PUBLIC_INFO
{
    uint8_t year;
    uint8_t mon;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec; //byte10

    union RECV_MARSHALLING {
        struct
        {
            uint8_t ge : 4;
            uint8_t shi : 4;
            uint8_t bai : 4;
            uint8_t qian : 4;
        } bits;

        uint8_t byte[2];
    } marshalling;

    uint8_t speed[2];
    uint8_t train_outer_temp;
    uint8_t ctrl_train_mode;

    struct RECV_GPS
    {
        union RECV_VALID {
            struct
            {
                uint8_t air_spring1_pressure : 1;
                uint8_t air_spring2_pressure : 1;
                uint8_t gps : 1;
                uint8_t wheel1_valid :1;
                uint8_t wheel2_valid :1;
            } bits;
            uint8_t byte;
        } valid;
        uint8_t air_spring_pressure1;
        uint8_t air_spring_pressure2;
        uint8_t longitude_down;
        uint8_t longitude_mid_down;
        uint8_t longitude_mid_up;
        uint8_t longitude_up;
        uint8_t longitude_dir;			//经度方向
        uint8_t latitude_dir;			//纬度方向
        uint8_t latitude_down;
        uint8_t latitude_mid_down;
        uint8_t latitude_mid_up;
        uint8_t latitude_up;
        uint8_t wheel1_value[2];
        uint8_t wheel2_value[2];
    } gps_data;

        uint8_t train_type;		//车型
        uint8_t carriage_number;//车厢号0~16
        uint8_t minitor_trailer_flag;//动车拖车标识
        uint8_t speed_30km_flag;	 //30公里硬线信号 37
#if defined(INTERNAL_PROTOCOL_20210416) && !defined(PUB_INFO_REMOVE_KM_FLAG)
		uint8_t km_flag_ll;//公里标（低 低位）byte38
		uint8_t km_flag_lh;//公里标（低 高位）byte39
		uint8_t km_flag_hl;//公里标（高 低位）byte40
		uint8_t km_flag_hh;//公里标（高 高位）byte41

	#ifdef TCMS_MSG_ADD_ATP_TIME
		uint8_t atp_year;//42
		uint8_t atp_mon;
		uint8_t atp_day;
		uint8_t atp_hour;
		uint8_t atp_min;
		uint8_t atp_sec;//47
		uint8_t res[6];//48-53
	#else
		uint8_t res[12];   //默认填０ Byte42-53
	#endif

		uint8_t check_sum[2];//和校验 byte54 55
#else
        uint8_t check_sum[2];	//校验和 38 39
#endif
};

struct RECV_CTRL_BOARD_DATA
{
	struct RECV_DATA_HEAD recv_data_head;
	struct RECV_TRAIN_PUBLIC_INFO train_public_info;
};

/*
Byte0-1	包头	与接收到的包头一致
Byte2-3	 帧长度	数据包长度
Byte4 	厂家代码	识别部件厂家（时代、华高、纵横、路航）
Byte5 	设备代码	识别设备类型（失稳）
Byte6-7	生命信号	与接收到的发送板生命信号一致
Byte8	数据接收正确标志	用于返回数据是否正常接收
Byte9-13	预留
Byte14-15	校验和	累加和方式
*/
struct ACK_FRAME
{
    uint8_t head[2];
    uint8_t len[2];
    uint8_t company_id;
    uint8_t board_id;
    uint8_t life_sign[2];
    uint8_t recv_flag;
    uint8_t res[5];
    uint8_t check_sum[2];
};

#if (LOCAL_BOARD == PW_BOARD)
#define FS_PW 512
#define RE_FS_PW 128

/**
Byte36	year
Byte37	月
Byte38	日
Byte39	时
Byte40	分
Byte41	秒
Byte42	车型
Byte43	车厢号
Byte44	MVB通讯方式
Byte45	以太网通讯方式
Byte46-53	厂家数据
Byte54:0-3	编组编号（个位）
Byte54:4-7	编组编号（十位）
Byte54:0-3	编组编号（百位）
Byte54:4-7	编组编号（千位）
Byte55-Byte56	全车速度
Byte57	车外温度
Byte58	控车模式选择
Byte59：0	空簧压力有效
Byte59：1	GPS经纬有效
Byte60	空气弹簧压力1
Byte61	空气弹簧压力2
Byte62	经度（中下位）
Byte63	经度（下位）
Byte64	经度（上位）
Byte65	经度（中上位）
Byte66	经度方向ASCII（unsigned char）
Byte67	经度方向ASCII（unsigned char）
Byte68	纬度（中下位）
Byte69	纬度（下位）
Byte70	纬度（上位）
Byte71	纬度（中上位）
 */
struct TRAIN_PUBLIC_INFO
{
    uint8_t year;
    uint8_t mon;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;

    union RECV_MARSHALLING1 {
        struct
        {
            uint8_t ge : 4;
            uint8_t shi : 4;
            uint8_t bai : 4;
            uint8_t qian : 4;
        } bits;

        uint8_t byte[2];
    } marshalling;

    uint8_t speed[2];
    uint8_t train_outer_temp;
    uint8_t ctrl_train_mode;

    struct RECV_GPS1
    {
        union RECV_VALID1 {
            struct
            {
                uint8_t air_spring1_pressure : 1;
                uint8_t air_spring2_pressure : 1;
                uint8_t gps : 1;
                uint8_t wheel1_valid :1;
                uint8_t wheel2_valid :1;
            } bits;
            uint8_t byte;
        } valid;
        uint8_t air_spring_pressure1;
        uint8_t air_spring_pressure2;
        uint8_t longitude_down;
        uint8_t longitude_mid_down;
        uint8_t longitude_mid_up;
        uint8_t longitude_up;
        uint8_t longitude_dir;			//经度方向
        uint8_t latitude_dir;			//纬度方向
        uint8_t latitude_down;
        uint8_t latitude_mid_down;
        uint8_t latitude_mid_up;
        uint8_t latitude_up;
        uint8_t wheel1_value[2];
        uint8_t wheel2_value[2];
    } gps_data;

	uint8_t train_type;
	uint8_t carriage_number;//车厢号0~16
	uint8_t minitor_trailer_flag;//动车拖车标识
	uint8_t speed_30km_flag;//30公里硬线信号

};//33

/**
Byte26:0	1位端加速度传感器开路故障 0=正常，1=故障
Byte26:1	2位端加速度传感器开路故障
Byte26:2	1位端加速度传感器短路故障
Byte26:3	2位端加速度传感器短路故障
Byte26:4	平稳采集卡故障
 */
union SENSOR_STATUS {
    struct
    {
        uint8_t side1_sensor_err_power_on : 1;
        uint8_t side2_sensor_err_power_on : 1;
        uint8_t side1_sensor_err_real_time : 1;
        uint8_t side2_sensor_err_real_time : 1;
        uint8_t nc : 4;
    } bits;
    uint8_t byte;
};

union SENSOR_ERR {
    struct
    {
        uint8_t sensor1_err : 1;
        uint8_t sensor2_err : 1;
        uint8_t nc : 6;
    } bits;
    uint8_t byte;
};
/*
Byte35:0	控制板A 以太网通信故障	与控制板A 以太网通信故障	0=正常，1=故障
Byte35:1	控制板A CAN通信故障	与控制板A CAN通信故障	0=正常，1=故障
Byte35:2	控制板B 以太网通信故障	与控制板B 以太网通信故障	0=正常，1=故障
Byte35:3	控制板B CAN通信故障	与控制板B CAN通信故障	0=正常，1=故障
Byte35:4	输入电源支路1故障	检查出输入电源支路1故障	0=正常，1=故障
Byte35:5	输入电源支路2故障	检查出输入电源支路2故障	0=正常，1=故障 */
/*
Byte35:0	平稳模块故障	平稳模块故障（该字节bit1-bit7取或逻辑）,用于控车限速，模块的监控功能失效故障	0=正常，1=故障
Byte35:1	存储故障	检测出平稳模块存储故障	0=正常，1=故障
Byte35:2	输入电源支路1状态	检查出输入电源支路1故障	0=正常，1=故障
Byte35:3	输入电源支路2状态	检查出输入电源支路2故障	0=正常，1=故障
Byte35:4	采集接口故障	采集接口功能故障	0=正常，1=故障
Byte35:5	平稳采集板卡故障	默认填0	0=正常，1=故障
Byte35:6	预留	默认填0	0=正常，1=故障
Byte35:7	预留	默认填0	0=正常，1=故障

Byte36:0	控制板A 以太网通信状态	与控制板A 以太网通信故障	0=正常，1=故障
Byte36:1	控制板A CAN通信状态	与控制板A CAN通信故障	0=正常，1=故障
Byte36:2	控制板B 以太网通信状态	与控制板B 以太网通信故障	0=正常，1=故障
Byte36:3	控制板B CAN通信状态	与控制板B CAN通信故障	0=正常，1=故障
 */
union BOARD_ERR {
    struct
    {
    	uint8_t pw_board_err  : 1;
    	uint8_t save_err      : 1;
        uint8_t power1_err : 1;
        uint8_t power2_err : 1;
        uint8_t sample_err : 1;
#ifdef INTERNAL_PROTOCOL_20210416
        uint8_t ad_board_err : 1;
        uint8_t res :		 2;
#else
        uint8_t res :		 3;
#endif
        uint8_t ctrla_eth_err : 1; //与控制板A 以太网通信状态 0:正常 1：故障
        uint8_t ctrla_can_err : 1; //与控制板A CAN通信状态
        uint8_t ctrlb_eth_err : 1; //与控制板B 以太网通信状态
        uint8_t ctrlb_can_err : 1; //与控制板B CAN通信状态
        uint8_t nc:			 4;
    } bits;
    uint8_t byte[2];
};
extern union BOARD_ERR pw_board_st;
/* Byte33:0	横向平稳性报警状态	检测出平稳性报警	 0：正常，1：报警
Byte33:1	垂向平稳性报警状态	检测出平稳性报警	 0：正常，1：报警
Byte33:2	纵向平稳性报警状态	检测出平稳性报警	 0：正常，1：报警

//新增：
//Byte33:3  平稳性减速1报警		检出平稳性减速1报警	 0：正常，1：报警（脉冲持续4s）
//Byte33:4  平稳性减速2报警		检出平稳性减速2报警	 0：正常，1：报警（脉冲持续4s）
//Byte33:5  平稳性减速3报警		检出平稳性减速3报警	 0：正常，1：报警（脉冲持续4s）

Byte34:0	1位端横向平稳性报警状态	检测出平稳性报警	 0：正常，1：报警
Byte34:1	2位端横向平稳性报警状态	检测出平稳性报警	 0：正常，1：报警
Byte34:2	1位端垂向平稳性报警状态	检测出平稳性报警	 0：正常，1：报警
Byte34:3	2位端垂向平稳性报警状态	检测出平稳性报警	 0：正常，1：报警
Byte34:4	1位端纵向平稳性报警状态	检测出平稳性报警	 0：正常，1：报警
Byte34:5	2位端纵向平稳性报警状态	检测出平稳性报警	 0：正常，1：报警 */
union ALARM_STATUS {
    struct
    {
        uint8_t y_alarm : 1;
        uint8_t z_alarm : 1;
        uint8_t x_alarm : 1;
#ifdef INTERNAL_PROTOCOL_20210416
        uint8_t dec1_alarm : 1;
		uint8_t dec2_alarm : 1;
		uint8_t dec3_alarm : 1;
        uint8_t res : 2;
#else
        uint8_t res : 5;
#endif
        uint8_t side1_y_alarm : 1;
        uint8_t side2_y_alarm : 1;
        uint8_t side1_z_alarm : 1;
        uint8_t side2_z_alarm : 1;
        uint8_t side1_x_alarm : 1;
        uint8_t side2_x_alarm : 1;
        uint8_t res1 :2;
    } bits;

    uint8_t byte[2];//33 34
};

#ifdef INTERNAL_PROTOCOL_20210416
/*
Byte101：0	1位端垂向平稳性预警状态	检测出1位端垂向平稳性预警	0=正常，1=预警
Byte101：1	2位端垂向平稳性预警状态	检测出2位端垂向平稳性预警	0=正常，1=预警
Byte101：2	1位端横向平稳性预警状态	检测出1位端横向平稳性预警	0=正常，1=预警
Byte101：3	2位端横向平稳性预警状态	检测出2位端横向平稳性预警	0=正常，1=预警
Byte101：4	横向平稳性预警状态	检测出平稳性报警（任一位端横向平稳性预警置1）	0=正常，1=预警
Byte101：5	垂向平稳性预警状态	检测出平稳性报警（任一位端垂向平稳性预警置1）	0=正常，1=预警
 */
union ZY_WARNING_STATUS {
    struct
    {
        uint8_t z_warn_w1 : 1;
        uint8_t z_warn_w2 : 1;
        uint8_t y_warn_w1 : 1;
        uint8_t y_warn_w2 : 1;
        uint8_t y_warn : 1;
        uint8_t z_warn : 1;
        uint8_t res : 2;
    } bits;

    uint8_t byte;
};


/*
Byte102	1位端横向加速度0.2-3Hz峰值	范围0～0.256g,1=0.001g
Byte103	2位端横向加速度0.2-3Hz峰值	范围0～0.256g,1=0.001g
Byte104	1位端横向加速度5-13Hz均方根值	范围0～0.256g,1=0.001g
Byte105	2位端横向加速度5-13Hz均方根值	范围0～0.256g,1=0.001g
Byte106	1位端垂向加速度5-13Hz均方根值	范围0～0.256g,1=0.001g
Byte107	2位端垂向加速度5-13Hz均方根值	范围0～0.256g,1=0.001g
Byte108	1位端垂向加速度车轮转频主频幅值	范围0～0.256g,1=0.001g
Byte109	2位端垂向加速度车轮转频主频幅值	范围0～0.256g,1=0.001g
Byte110:0-6	1位端垂向加速度车轮转频主频	范围0～127Hz,1=1Hz
Byte110:7	0	默认0
Byte111:0-6	2位端垂向加速度车轮转频主频	范围0～127Hz,1=1Hz
Byte111:7	0	默认0
 */
struct ZY_ACCELERATION_INDEX
{
    uint8_t y_peak_w1;
    uint8_t y_peak_w2;
    uint8_t y_root_w1;
    uint8_t y_root_w2;
    uint8_t z_root_w1;
    uint8_t z_root_w2;
    uint8_t z_freq_amp_w1;
    uint8_t z_freq_amp_w2;

#ifdef INTERNAL_PROTOCOL_20210725
    struct UINT16_TYPE cl_z_freq_w1;//范围0～51.2Hz,1=0.1Hz
    struct UINT16_TYPE cl_z_freq_w2;//范围0～51.2Hz,1=0.1Hz
#else
    union cl_frequency_amp {
        struct
        {
            uint8_t z_freq_w1 : 7;
            uint8_t res1 : 1;
            uint8_t z_freq_w2 : 7;
            uint8_t res2 : 1;
        } bits;

        uint8_t byte[2];
    } cl_freq_amp;
#endif
};
#endif

/*
Byte100：0	1位端抖车预警状态	检测出1位端抖车预警	0=正常，1=预警
Byte100：1	2位端抖车预警状态	检测出2位端抖车预警	0=正常，1=预警
Byte100：2	1位端抖车报警状态	检测出1位端抖车报警	0=正常，1=报警
Byte100：3	2位端抖车报警状态	检测出2位端抖车报警	0=正常，1=报警
Byte100：4	1位端晃车预警状态	检测出1位端晃车预警	0=正常，1=预警
Byte100：5	2位端晃车预警状态	检测出2位端晃车预警	0=正常，1=预警
Byte100：6	1位端晃车报警状态	检测出1位端晃车报警	0=正常，1=报警
Byte100：7	2位端晃车报警状态	检测出2位端晃车报警	0=正常，1=报警
 */
union DC_HC_ALARM_STATUS {
    struct
    {
        uint8_t dc_warn_w1 : 1;//1轴1位
        uint8_t dc_warn_w2 : 1;
        uint8_t dc_alarm_w1 : 1;
        uint8_t dc_alarm_w2 : 1;
        uint8_t hc_warn_w1 : 1;
        uint8_t hc_warn_w2 : 1;
        uint8_t hc_alarm_w1 : 1;
        uint8_t hc_alarm_w2 : 1;
    } bits;

    uint8_t byte;
};

/*
Byte38:0	抖车预警状态	任一端检测出抖车预警置1
Byte38:1	抖车报警状态	任一端检测出抖车报警置1
Byte38:2	晃车预警状态	任一端检测出晃车预警置1
Byte38:3	晃车报警状态	任一端检测出晃车报警置1
 */
union DC_HC_DIAG_ALARM{
    struct
    {
		uint8_t dc_warn : 1;
		uint8_t dc_alarm : 1;
		uint8_t hc_warn : 1;
		uint8_t hc_alarm : 1;
		uint8_t nc : 4;
    } bits;

    uint8_t byte;
};

struct S1PW_TZ_DATA
{
//	psw_save_tzdata_head_t tzsave_data_head;
	psw_save_data_head_t tzsave_data_head;
	struct UINT16_TYPE side1_y_quota;//范围0～25.6,1=0.1  114 115
	struct UINT16_TYPE side1_z_quota;//118 119
	struct UINT16_TYPE side2_y_quota;//116 117
	struct UINT16_TYPE side2_z_quota;//120 121
	pwb_alarm_t pw_alarm;
	pwb_err_t pwb_err;
	uint8_t check_sum[2];
};

struct S1SW_TZ_DATA
{
	psw_save_data_head_t tzsave_data_head;
	swb_err_alarm_t swb_err;
	uint8_t sw1_min_tzz;
	uint8_t sw2_min_tzz;
	uint8_t sw1_mean_tzz;
	uint8_t sw2_mean_tzz;
	uint8_t check_sum[2];
};

struct PW_TZ_DATA
{
    struct SEND_DATA_HEAD data_head;//0-23
    uint8_t soft_version[2];//24 25
    union SENSOR_STATUS sensor_status;//26

#ifdef INTERNAL_PROTOCOL_20210725
    uint8_t res[6];//27-32
#elif defined(INTERNAL_PROTOCOL_20210416)
    uint8_t side2_y_quota;//2位端横向平稳性指标 0~25.6  1=0.1
	uint8_t side1_y_quota;
	uint8_t side2_z_quota;//2位端垂向平稳性指标
	uint8_t side1_z_quota;
	uint8_t side2_x_quota;//2位端纵向平稳性指标
	uint8_t side1_x_quota;      //32
#else
    uint8_t side1_y_quota;//范围0～25.6,1=0.1
    uint8_t side2_y_quota;
    uint8_t side1_z_quota;
    uint8_t side2_z_quota;
    uint8_t side1_x_quota;
    uint8_t side2_x_quota;//32
#endif

    union ALARM_STATUS alarm_status;//33 34

    union BOARD_ERR borad_err;	  //平稳模块故障 35 36
    union SENSOR_ERR sensor12_err;//1-2位端加速度传感器故障 37

#ifdef INTERNAL_PROTOCOL_20210725
    union DC_HC_DIAG_ALARM diag_alarm;//38
    uint8_t rc;//39
#else
    uint8_t rc[2]; //38 39
#endif
    uint8_t output_data;//数字量输出预留 40
    uint8_t company_data[8]; //41 48

    struct TRAIN_PUBLIC_INFO train_public_info;//49 81

#if defined(INTERNAL_PROTOCOL_20210416) && !defined(PUB_INFO_REMOVE_KM_FLAG)
    uint8_t km_flag_ll;//公里标（低 低位）82
    uint8_t km_flag_lh;//公里标（低 高位）83
    uint8_t km_flag_hl;//公里标（高 低位）84
    uint8_t km_flag_hh;//公里标（高 高位）85
    uint8_t nc[4];	  //公共数据后统一预留4Bytes 默认填０ 86 89
#else
	uint8_t nc[8];//82 89
#endif

    uint8_t total_power_on_times[2];//90 91
    uint8_t total_work_on_time[4];//92 95
    uint8_t sys_cur_work_on_time[4];//-Byte96 99

#ifdef INTERNAL_PROTOCOL_20210725
    union DC_HC_ALARM_STATUS dc_hc_status;//Byte100
    union ZY_WARNING_STATUS warn_status;//Byte101
    struct ZY_ACCELERATION_INDEX acc_index;//102 113   20210725
    struct UINT16_TYPE side2_y_quota;//范围0～25.6,1=0.1  114 115
    struct UINT16_TYPE side1_y_quota;//116 117
    struct UINT16_TYPE side2_z_quota;//118 119
    struct UINT16_TYPE side1_z_quota;//120 121
    struct UINT16_TYPE side2_x_quota;//122 123
    struct UINT16_TYPE side1_x_quota;//124 125
    uint8_t pw_res[2];//126 127
    uint8_t company_res[126];//128 253
    uint8_t check_sum[2];//Byte254-Byte255
#elif defined(INTERNAL_PROTOCOL_20210416)
    union DC_HC_ALARM_STATUS dc_hc_status;//Byte100
    union ZY_WARNING_STATUS warn_status;//Byte101
    struct ZY_ACCELERATION_INDEX acc_index;//102 111   20210416
    uint8_t pw_res[16];//112 127
    uint8_t company_res[126];//128 253
    uint8_t check_sum[2];//Byte254-Byte255
#else
    uint8_t km_scale[40];//使用预留 100 139
    uint8_t resv[114];//各个厂家自行使用预留 140 253
    uint8_t check_sum[2]; //254 255
#endif
};

struct PW_DIAGNOS_THRESHOLD_PARA
{
	struct THRESHOLD_PARA side1_y_para;
	struct THRESHOLD_PARA side1_z_para;
	struct THRESHOLD_PARA side1_x_para;

	struct THRESHOLD_PARA side2_y_para;
	struct THRESHOLD_PARA side2_z_para;
	struct THRESHOLD_PARA side2_x_para;

	/*1位端抖车阈值*/
	struct JITTER_THRESHOLD_PARA jitter_side1_y_para;
	struct JITTER_THRESHOLD_PARA jitter_side1_z_para;

	/*2位端抖车阈值*/
	struct JITTER_THRESHOLD_PARA jitter_side2_y_para;
	struct JITTER_THRESHOLD_PARA jitter_side2_z_para;

	/*1位端晃车阈值*/
	struct SHAKE_THRESHOLD_PARA shake_side1_y_para;

	/*2位端晃车阈值*/
	struct SHAKE_THRESHOLD_PARA shake_side2_y_para;

	/*径跳阈值*/
	struct WHEEL_THRESHOLD_PARA wheel_side1_z_para;

	struct WHEEL_THRESHOLD_PARA wheel_side2_z_para;

};

struct MCAST_ADDR
{
	int mcast_port;
	uint8_t mcast_addr[4];
	uint32_t is_need_ack;
};

struct UCAST_ADDR
{
	int ucast_port;
	uint8_t ucast_addr[4];
	uint32_t is_need_ack;
};

struct COMM_DATA_CNT
{
	uint16_t ctrla_eth_recv_all_cnt;			//控制板Ａ以太网接收总包数计数
	uint16_t ctrla_eth_recv_valid_cnt;			//控制板Ａ以太网接收有效包数计数
	uint16_t ctrla_eth_recv_signal;				//控制板Ａ以太网生命信号
	uint16_t ctrla_eth_lost_cnt;				//控制板Ａ以太网丢失计数

	uint16_t ctrla_can_recv_all_cnt;			//控制板Ａ CAN接收总包数计数
	uint16_t ctrla_can_recv_valid_cnt;			//控制板Ａ CAN接收有效包数计数
	uint16_t ctrla_can_recv_signal;				//控制板Ａ CAN生命信号
	uint16_t ctrla_can_lost_cnt;				//控制板Ａ CAN丢失计数

	uint16_t ctrlb_eth_recv_all_cnt;			//控制板B以太网接收总包数计数
	uint16_t ctrlb_eth_recv_valid_cnt;			//控制板B以太网接收有效包数计数
	uint16_t ctrlb_eth_recv_signal;				//控制板B以太网生命信号
	uint16_t ctrlb_eth_lost_cnt;				//控制板B以太网丢失计数

	uint16_t ctrlb_can_recv_all_cnt;			//控制板B CAN接收总包数计数
	uint16_t ctrlb_can_recv_valid_cnt;			//控制板B CAN接收有效包数计数
	uint16_t ctrlb_can_recv_signal;				//控制板B　CAN生命信号
	uint16_t ctrlb_can_lost_cnt;				//控制板B CAN丢失计数

	uint16_t send_eth_all_cnt;					//以太网发送总包数
	uint16_t send_eth_signal;					//以太网发送生命信号
	uint16_t send_can_all_cnt;					//CAN 发送总包数
	uint16_t send_can_signal;					//CAN 发送生命信号
};

//本地网络参数
/* 网络端口好
 * IP
 * 网关
 * 子网掩码
 */
//struct LOCAL_NET_ADDR
//{
//	int net_port;
//	uint8_t self_ip[4];
//	uint8_t self_gwaddr[4];
//	uint8_t self_maskaddr[4];
//
//};

//PTU网络参数
struct NET_ADDR
{
	uint32_t net_port;
	uint8_t self_ip[4];
	uint8_t self_gwaddr[4];
	uint8_t self_maskaddr[4];
};

#ifdef WTD_DATA_TRANSLATE_PROTOCOL
struct WTD_NET_ADDR
{
	uint32_t self_port;
//	uint8_t self_ip[4];//同PTU
//	uint8_t self_gwaddr[4];//同PTU
	uint32_t wtd_tz_port;
	uint32_t wtd_alarm_port;
	uint8_t wtd_ip[4];
};
#endif





//recv carriage_number
//struct PW_RAW_DATA
//{
//    struct SEND_DATA_HEAD send_data_head;
//    short ad[512];
//    uint8_t check_sum[2];
//};

struct PW_RAW_DATA
{
//    struct SEND_DATA_HEAD send_data_head;
	psw_save_data_head_t send_data_head;
    uint16_t ad_acc[512];//ad_acc=100*acc
    uint8_t check_sum[2];
};

struct PW_AD_DATA
{
	uint16_t **buff;//AD缓存的二级指针
	uint16_t row_size;//AD缓存的行容量
	uint16_t row_index;//AD缓存的写的行下标，即写到多少行
	uint16_t column_size;//AD缓存的列容量
	uint16_t column_index;//AD缓存的写的列下标，即写到多少列
	uint16_t packet_id;//AD缓存的已经写过的总行号，除非重新采集，否则不会清零
	uint16_t ad_chnnel7[512];			//存放参考电压
};
//数据行数循环参数
struct RINGBUFF_PARA
{
	uint16_t size;		//需要初始化的总行数
	uint16_t index;		//当前处理的行数
	uint16_t num;		//当前累积的行数
};

enum PW_SENSOR_CH
{
	pw1_y,
	pw1_z,
	pw1_x,
	pw2_y,
	pw2_z,
	pw2_x
};

enum BOOL
{
	FALSE,
	TRUE
};

//存储相关数据
struct STORE_FLAG
{
	enum BOOL hdd_exist_flag;  		//判断SD卡是否存在的标志
    enum BOOL hdd_save_flag;		//硬盘存储标志位，表示是否存储数据
    enum BOOL hdd_err_flag;			//表示硬盘故障

};


struct COMMUNICATE_STA_FLAG
{
	uint16_t life_singal_judge[3];						//生命信号判断依据
	uint16_t check_sum_judge[3];						//校验和判断依据
	uint8_t judge_index;								//判断索引
	uint8_t err_flag;									//错误标志
	uint8_t not_recv_cnt;
	uint8_t current_type;								//当前的通信标志
};

struct RECV_PUBLIC_PARA
{
	struct RECV_CTRL_BOARD_DATA recv_ctrl_board_data;
	struct COMMUNICATE_STA_FLAG communicate_sta_flag;
};

struct COMMUNICATE_TYPE
{
	struct RECV_PUBLIC_PARA ctrl_A_ETH;
	struct RECV_PUBLIC_PARA ctrl_A_CAN;
	struct RECV_PUBLIC_PARA ctrl_B_ETH;
	struct RECV_PUBLIC_PARA ctrl_B_CAN;
};

struct TEMP_LIFE_NUM
{
	uint16_t ctrl_a_eth_life_num;
	uint16_t ctrl_a_can_life_num;
	uint16_t ctrl_b_eth_life_num;
	uint16_t ctrl_b_can_life_num;
};

enum PUBLIC_INFO_UPDATE_FLAG
{
	CTRL_A_ETH = 0x01,
	CTRL_A_CAN,
	CTRL_B_ETH,
	CTRL_B_CAN
};

struct ERR_LOG
{
	uint8_t year;					//年
	uint8_t mon;					//月
	uint8_t day;					//日
	uint8_t hour;					//时
	uint8_t min;					//分
	uint8_t sec;					//秒
	uint8_t err_event;				//故障事件
	uint8_t err_des;				//故障描述
};

enum ERR_EVENT
{
  ERR_EVENT_EMPTY,
  OK,								//正常
  ERR,								//故障
  PREDICT,							//预判
  WARN,								//预警
  ALARM,							//报警
  PREDICT_REMOVE,
  WARN_REMOVE,						//预警解除
  ALARM_REMOVE						//报警解除
};



enum ERR_DES
{
	ERR_DES_EMPTY,
	DEV_POWER_ON = 1,
	BOARD_ERR,
	BOARD_ERR_REMOVE,
	SAVE_ERR,
	SAVE_ERR_REMOVE,
	ACC_SENSOR_ERR,
	ACC_SENSOR_ERR_REMOVE = 7,

	PW_DIAG_OK = 82,
	PW_DIAG_PREDICT = 83,
	PW_DIAG_WARN = 84,
	PW_DIAG_ALARM = 85,

	HC_DIAG_OK = 86,
	HC_DIAG_WARN = 87,
	HC_DIAG_ALARM = 88,

	DC_DIAG_OK = 89,
	DC_DIAG_WARN = 90,
	DC_DIAG_ALARM = 91,

	POWER_BRANCH1_ERR = 38,
	POWER_BRANCH1_ERR_REMOVE,
	POWER_BRANCH2_ERR,
	POWER_BRANCH2_ERR_REMOVE,
	CTRLA_ETH_ERR,
	CTRLA_ETH_ERR_REMOVE,
	CTRLA_CAN_ERR,
	CTRLA_CAN_ERR_REMOVE,
	CTRLB_ETH_ERR,
	CTRLB_ETH_ERR_REMOVE,
	CTRLB_CAN_ERR,
	CTRLB_CAN_ERR_REMOVE ,
	SAVE_ETH_ERR,
	SAVE_ETH_ERR_REMOVE,
	SAVE_CAN_ERR,
	SAVE_CAN_ERR_REMOVE = 53,
#ifdef AD_REF_VOLT_ERR_REBOOT
	PW_AD_POWER_ERR=97,
	PW_AD_POWER_ERR_REMOVE,
	PW_AD_REF_VOLT_ERR_REBOOT = 99,
#endif
};
//VIBR:10~37,GEAR:54~81,PW:82~85,SW:86~89

enum SENSOR_MEASURING_POINT
{
	SENSOR_MEASURING_POINT_EMPTY,
	PW_SENSOR_ONE_BIT_Y = 17,					//1位端平稳传感器Y向
	PW_SENSOR_ONE_BIT_Z ,
	PW_SENSOR_ONE_BIT_X ,
	PW_SENSOR_TWO_BIT_Y ,
	PW_SENSOR_TWO_BIT_Z ,
	PW_SENSOR_TWO_BIT_X = 22,
};
//VIBR:1~8,GEAR:9~16,PW:17~22,SW:23~26

//设备状态计数
struct ERR_TYPE
{
	uint16_t sensor_normal_save_flag: 1;		//振动传感器正常记录标志
	uint16_t sensor_err_save_flag:	1;			//振动传感器故障记录标志

	uint16_t diag_normal_save_flag:	1;			//算法诊断正常记录标志
//	uint16_t diag_predict_save_flag:	1;		//多边形预判记录标志
	uint16_t diag_warn_save_flag:	1;			//平稳性指标预警记录标志
	uint16_t diag_alarm_save_flag:	1;			//平稳性指标报警记录标志

	uint16_t shack_diag_normal_save_flag:	1;		//晃车正常记录标志
	uint16_t shack_diag_warn_save_flag:		1;		//晃车预警存储标志
	uint16_t shack_diag_alarm_save_flag:	1;		//晃车报警存储标志

	uint16_t jitter_diag_normal_save_flag:	1;		//晃车正常记录标志
	uint16_t jitter_diag_warn_save_flag:	1;		//晃车预警存储标志
	uint16_t jitter_diag_alarm_save_flag:	1;		//晃车报警存储标志

	uint16_t nc:							4;

//	uint32_t nc1:	1;
//	uint32_t nc2:	1;

};

struct SWERR_TYPE
{
	uint8_t sensor_normal_save_flag: 1;				//振动传感器正常记录标志
	uint8_t sensor_err_save_flag:	1;				//振动传感器故障记录标志

	uint8_t diag_normal_save_flag:	1;			//算法诊断正常记录标志
	uint8_t diag_predict_save_flag:	1;			//多边形预判记录标志
	uint8_t diag_warn_save_flag:	1;			//多边形预警记录标志
	uint8_t diag_alarm_save_flag:	1;			//多边形报警记录标志
	uint8_t nc1:					1;
	uint8_t nc2:					1;


//	uint32_t nc1:	1;
//	uint32_t nc2:	1;

};


struct SYS_STATUS_CNT		//设备状态计数
{

		struct ERR_TYPE err_type[6];					//参数6表示６个传感器通道
		struct SWERR_TYPE swerr_type[2];					//参数6表示６个传感器通道
	//
		uint8_t board_normal_save_flag:	1;					//振动模块正常
		uint8_t board_err_save_flag:	1;					//振动模块故障
		uint8_t save_normal_save_flag:	1;					//存储正常
		uint8_t save_err_save_flag:	1;						//存储故障

	//
		uint8_t power_branch1_normal_save_flag:	1;
		uint8_t power_branch1_err_save_flag:	1;
		uint8_t power_branch2_normal_save_flag:	1;
		uint8_t power_branch2_err_save_flag:	1;

	//
		uint8_t ctrla_eth_normal_save_flag:	1;
		uint8_t ctrla_eth_err_save_flag:	1;
		uint8_t ctrla_can_normal_save_flag:	1;
		uint8_t ctrla_can_err_save_flag:	1;

		uint8_t ctrlb_eth_normal_save_flag:	1;
		uint8_t ctrlb_eth_err_save_flag:	1;
		uint8_t ctrlb_can_normal_save_flag:	1;
		uint8_t ctrlb_can_err_save_flag:	1;

		uint8_t save_eth_normal_save_flag:	1;
		uint8_t save_eth_err_save_flag:	1;
		uint8_t save_can_normal_save_flag:	1;
		uint8_t save_can_err_save_flag:	1;

#ifdef AD_REF_VOLT_ERR_REBOOT
		uint8_t power_ad_normal_save_flag:	1;
		uint8_t power_ad_err_save_flag:	1;
#endif

};


struct UPDATE_ERR_TYPE_LOG										//故障日志更新标志(算法相关)
{
	uint16_t sensor_normal_update_log_flag: 1;				//振动传感器正常记录标志
	uint16_t sensor_err_update_log_flag:	1;				//振动传感器故障记录标志

	uint16_t diag_normal_update_log_flag:	1;			//平稳性指标正常记录标志
//	uint16_t diag_predict_update_log_flag:	1;			//多边形预判记录标志
	uint16_t diag_warn_update_log_flag:		1;			//平稳性指标预警记录标志
	uint16_t diag_alarm_update_log_flag:	1;			//平稳性指标报警记录标志

	uint16_t shack_diag_normal_update_log_flag:	1;		//晃车诊断正常记录标志
	uint16_t shack_diag_warn_update_log_flag:	1;		//晃车诊断预警记录标志
	uint16_t shack_diag_alarm_update_log_flag:	1;		//晃车诊断报警记录标志

	uint16_t jitter_diag_normal_update_log_flag:	1;	//抖车诊断正常记录标志
	uint16_t jitter_diag_warn_update_log_flag:	1;		//抖车诊断预警记录标志
	uint16_t jitter_diag_alarm_update_log_flag:	1;		//抖车诊断报警记录标志

	uint16_t nc: 4;

};

struct POWER_CNT
{
	uint8_t power_on_cnt;
	uint8_t power_on_type;
	uint16_t power_on_sum;
};

struct UPDATE_ERR_LOG_FLAG				//故障日志更新标志
{

		struct UPDATE_ERR_TYPE_LOG update_err_type_log[6];

	//
		uint8_t board_normal_update_log_flag:	1;					//振动模块正常
		uint8_t board_err_update_log_flag:	1;						//振动模块故障
		uint8_t save_normal_update_log_flag:	1;					//存储正常
		uint8_t save_err_update_log_flag:	1;						//存储故障

	//
		uint8_t power_branch1_normal_update_log_flag:	1;			//电源支路１故障
		uint8_t power_branch1_err_update_log_flag:	1;
		uint8_t power_branch2_normal_update_log_flag:	1;
		uint8_t power_branch2_err_update_log_flag:	1;

	//
		uint8_t ctrla_eth_normal_update_log_flag:	1;
		uint8_t ctrla_eth_err_update_log_flag:	1;
		uint8_t ctrla_can_normal_update_log_flag:	1;
		uint8_t ctrla_can_err_update_log_flag:	1;

		uint8_t ctrlb_eth_normal_update_log_flag:	1;
		uint8_t ctrlb_eth_err_update_log_flag:	1;
		uint8_t ctrlb_can_normal_update_log_flag:	1;
		uint8_t ctrlb_can_err_update_log_flag:	1;

		uint8_t save_eth_normal_update_log_flag:	1;
		uint8_t save_eth_err_update_log_flag:	1;
		uint8_t save_can_normal_update_log_flag:	1;
		uint8_t save_can_err_update_log_flag:	1;

#ifdef AD_REF_VOLT_ERR_REBOOT
		uint8_t power_ad_normal_update_log_flag:	1;
		uint8_t power_ad_err_update_log_flag:	1;
#endif

};


void init_system_default_para(void);
void get_para_from_local(void);
void update_system_version(void);
void malloc_two_dim_pointer_buff(uint16_t **pointer_buff,uint16_t row_size,uint16_t col_size);
void init_pw_ad_data(void);
void init_pw_tz_data(void);
void init_pw_raw_data(void);
void init_ringbuff_para(struct RINGBUFF_PARA *ringbuff_para,uint16_t size);
void init_pw_diagnos_para();
void init_pw_data_para(void);
void init_user_data(void);
void init_eth_status(void);
void init_power_on_para(void);
void printf_power_on_para(void);
void set_power_on_para(void);
void get_power_on_para(void);

#elif (LOCAL_BOARD == SW_BOARD)

//#define CHANNEL_NUM 4
#define FS_SW 512

/**
 * 列车公共信息
 */
struct TRAIN_PUBLIC_INFO
{
    uint8_t year;
    uint8_t mon;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t train_style;
    uint8_t carriage_number;
    uint8_t motor_or_trailer_flag;
    uint8_t rev[2];
    uint8_t digital_input;
    uint8_t digital_output;
    uint8_t company_rev[8];

    union MARSHALLING {
        struct
        {
            uint8_t ge : 4;
            uint8_t shi : 4;
            uint8_t bai : 4;
            uint8_t qian : 4;
        } bits;

        uint8_t byte[2];
    } marshalling;

    uint8_t speed[2];
    uint8_t train_outer_temp;
    uint8_t ctrl_train_mode;

    struct GPS
    {
        union VALID {
            struct
            {
                uint8_t air_spring_pressure : 1;
                uint8_t gps : 1;
            } bits;
            uint8_t byte;
        } valid;
        uint8_t air_spring_pressure1;
        uint8_t air_spring_pressure2;
        uint8_t longitude_mid_down;
        uint8_t longitude_down;
        uint8_t longitude_up;
        uint8_t longitude_mid_up;
        uint8_t longitude_dir;
        uint8_t latitude_dir;
        uint8_t latitude_mid_down;
        uint8_t latitude_down;
        uint8_t latitude_up;
        uint8_t latitude_mid_up;
    } gps_data;
};

/*
Byte26:0	转向架 1 轴加速度传感器自检故障	上电时检测加速度传感器故障	0=正常，1=故障
Byte26:1	转向架 2 轴加速度传感器自检故障	上电时检测加速度传感器故障	0=正常，1=故障
Byte26:2	转向架 3 轴加速度传感器自检故障	上电时检测加速度传感器故障	0=正常，1=故障
Byte26:3	转向架 4 轴加速度传感器自检故障	上电时检测加速度传感器故障	0=正常，1=故障
Byte26:4	转向架 1 轴加速度传感器实时故障	实时检测加速度传感器故障	0=正常，1=故障
Byte26:5	转向架 2 轴加速度传感器实时故障	实时检测加速度传感器故障	0=正常，1=故障
Byte26:6	转向架 3 轴加速度传感器实时故障	实时检测加速度传感器故障	0=正常，1=故障
Byte26:7	转向架 4 轴加速度传感器实时故障	实时检测加速度传感器故障	0=正常，1=故障 */
union SENSOR_STATUS {
    struct
    {
        uint8_t bogie_axis1_self_test_err : 1;
        uint8_t bogie_axis2_self_test_err : 1;
        uint8_t bogie_axis3_self_test_err : 1;
        uint8_t bogie_axis4_self_test_err : 1;
        uint8_t bogie_axis1_real_time_err : 1;
        uint8_t bogie_axis2_real_time_err : 1;
        uint8_t bogie_axis3_real_time_err : 1;
        uint8_t bogie_axis4_real_time_err : 1;

    } bits;
    uint8_t byte;
};

/*
Byte27:0	速度输入（低速） 继电器故障
Byte27:1	速度输入（中速） 继电器故障
Byte27:2	速度输入（高速） 继电器故障
Byte27:4	转向架异常输出继电器故障
Byte27:5	装置故障输出继电器故障
Byte27:6	传感器试验输出继电器故障 */
union RELAY_STATUS {
    struct
    {
        uint8_t low_speed_in_relay_err : 1;
        uint8_t mid_speed_in_relay_err : 1;
        uint8_t high_speed_in_relay_err : 1;
        uint8_t res : 1;
        uint8_t bogie_out_relay_err : 1;
        uint8_t device_out_relay_err : 1;
        uint8_t sensor_test_out_relay_err : 1;
    } bits;
    uint8_t byte;
};

/*
Byte28:0	转向架 1 轴异常	检测出转向架 1 轴失稳故障	0=正常，1=故障
Byte28:1	转向架 2 轴异常	检测出转向架 2 轴失稳故障	0=正常，1=故障
Byte28:2	转向架 3 轴异常	检测出转向架 3 轴失稳故障	0=正常，1=故障
Byte28:3	转向架 4 轴异常	检测出转向架 4 轴失稳故障	0=正常，1=故障
Byte28:4	转向架故障	发生转向架失稳故障（任一轴失稳则置位）	0=正常，1=故障 */
union ALARM_STATUS {
    struct
    {
        uint8_t bogie_axis1_alarm : 1;
        uint8_t bogie_axis2_alarm : 1;
        uint8_t bogie_axis3_alarm : 1;
        uint8_t bogie_axis4_alarm : 1;
        uint8_t bogie_alarm : 1;
    } bits;
    uint8_t byte;
};

/*
Byte29:0	转向架 1 轴预判	检测出转向架 1 轴失稳预判	0=正常，1=故障
Byte29:1	转向架 2 轴预判	检测出转向架 2 轴失稳预判	0=正常，1=故障
Byte29:2	转向架 3 轴预判	检测出转向架 3 轴失稳预判	0=正常，1=故障
Byte29:3	转向架 4 轴预判	检测出转向架 4 轴失稳预判	0=正常，1=故障
Byte29:4	转向架预判	发生转向架失稳预判（任一轴预判则置位）	0=正常，1=故障 */
union WARN_STATUS {
    struct
    {
        uint8_t bogie_axis1_warn : 1;
        uint8_t bogie_axis2_warn : 1;
        uint8_t bogie_axis3_warn : 1;
        uint8_t bogie_axis4_warn : 1;
        uint8_t bogie_warn : 1;
    } bits;
    uint8_t byte;
};

/*
Byte31:0-3	参数1 转向架异常检测参数等级	拨码开关设定的失稳报警判据中加速度阈值
Byte31:4-7	参数2 转向架异常检测参数等级	"拨码开关设定的失稳报警判据中波头数（波谷和波峰）"
Byte32:0-3	转向架异常检测参数1(百位)	1 个全波内波峰与波谷间转向架加速度最大值，用于监视记录
Byte32:4-7	转向架异常检测参数1(千位)
Byte33:0-3	转向架异常检测参数1（个位）
Byte33:4-7	转向架异常检测参数1（十位）
Byte34:0-3	转向架异常检测参数1(个位)	加速度超过加速度阈值的连续周期数，用于监视记录
Byte34:4-7	转向架异常检测参数1(十位)	 */
union BOGIE_ERR_PARA {
    struct
    {
        uint8_t para1_grade : 4;
        uint8_t para2_grade : 4;
        uint8_t max_amp_bai : 4;
        uint8_t max_amp_qian : 4;
        uint8_t max_amp_ge : 4;
        uint8_t max_amp_shi : 4;
        uint8_t over_amp_cycle_ge : 4;
        uint8_t over_amp_cycle_shi : 4;
    } bits;
    uint8_t byte[4];
};

union BOARD_ERR {
    struct
    {
        uint8_t err : 1;
        uint8_t save_err : 1;
        uint8_t power1_err : 1;
        uint8_t power2_err : 1;
        uint8_t ad_err : 1;
        uint8_t res : 3;
    } bits;
    uint8_t byte;
};

union COMM_ERR {
    struct
    {
        uint8_t ctrla_eth_err : 1; //与控制板A 以太网通信状态 0:正常 1：故障
        uint8_t ctrla_can_err : 1; //与控制板A CAN通信状态
        uint8_t ctrlb_eth_err : 1; //与控制板B 以太网通信状态
        uint8_t ctrlb_can_err : 1; //与控制板B CAN通信状态
        uint8_t res : 4;
    } bits;
    uint8_t byte;
};

struct SW_TZ_DATA
{
    struct SEND_DATA_HEAD data_head;
    uint8_t soft_version[2];
    union SENSOR_STATUS sensor_status;
    union RELAY_STATUS relay_status;
    union ALARM_STATUS alarm_status;
    union WARN_STATUS warn_status;

    uint8_t total_bogie_err_cnt;
    union BOGIE_ERR_PARA bogie_err_para;

    union BOARD_ERR borad_err;
    union COMM_ERR com_err;
    struct TRAIN_PUBLIC_INFO train_public_info;

    uint8_t res;
    uint8_t total_power_on_times[2];
    uint8_t total_work_on_time[4];
    uint8_t sys_cur_work_on_time[4];
    uint8_t km_scale[40];

    uint8_t resv[126];
    uint8_t check_sum[2];
};

struct SW_RAW_DATA
{
    struct SEND_DATA_HEAD send_data_head;
    uint16_t ad[512];
    uint8_t check_sum[2];
};

//数据行数循环参数
struct RINGBUFF_PARA
{
	uint16_t size;		//需要初始化的总行数
	uint16_t index;		//当前处理的行数
	uint16_t num;		//当前累积的行数
};
#endif

/*
 * 板卡号
 */
enum ETH_BOARD_TYPE
{
	PTU,

	TRAIN1_MCU,
	TRAIN1_MVB,
	TRAIN1_PROC1,
	TRAIN1_PROC2,

	TRAIN2_MCU,
	TRAIN2_PROC1,
	TRAIN2_PROC2,

	TRAIN3_MCU,
	TRAIN3_PROC1,
	TRAIN3_PROC2,

	TRAIN4_MCU,
	TRAIN4_PROC1,
	TRAIN4_PROC2,

	TRAIN5_MCU,
	TRAIN5_PROC1,
	TRAIN5_PROC2,

	TRAIN6_MCU,
	TRAIN6_PROC1,
	TRAIN6_PROC2,

	TRAIN7_MCU,
	TRAIN7_PROC1,
	TRAIN7_PROC2,

	TRAIN8_MCU,
	TRAIN8_MVB,
	TRAIN8_PROC1,
	TRAIN8_PROC2
};
/*
 * 通信消息类型
 */
enum NET_MSG_TYPE
{

    RESET_TYPE,							//复位
    LIFE_TYPE,							//生命信号
    CONFIG_TYPE,						//配置
    TEMP_RAW_DATA_TYPE,					//温度原始数据
    TEMP_TZ_DATA_TYPE,					//温度特征数据，暂时不用
    BEARING_RAW_DATA_TYPE,				//轴承原始数据
    BEARING_TZ_DATA_TYPE,
    POLYGON_RAW_DATA_TYPE,				//轮对原始数据
    POLYGON_TZ_DATA_TYPE,
    SPEED_TIME_TYPE,					//速度时间数据
    MVB_TZ_TYPE,						//MVB特征数据
    UPDATE_SOFT_TYPE = 0x0b,			//版本升级
    GET_SOFT_VERSION_TYPE,				//软件版本
	NET_RESET_TYPE = 0x0d,
    DEV_STAT_TYPE,
    WHEEL_MESG_TYPE,					//轮径信息
    GET_CONFIG_TYPE,
	MONITOR_DATA_TYPE,
};


#ifdef CAN_ERR_REBOOT_TWO_TIMES
struct SYSTEM_REBOOT_LOG
{
	uint32_t system_reboot_mode;//0:硬件复位  1:软件复位
	uint32_t software_reboot_cnt;//软件复位次数 max:2
};
#endif


struct _SOFT_VERSION
{
	uint32_t  pw_soft_version;  //BCD码,V1.0.1
	uint32_t  pw_update_time;
};

//自定义时间结构体
struct LOCAL_TIME
{
	uint32_t year;
	uint32_t mon;
	uint32_t day;
	uint32_t hour;
	uint32_t min;
	uint32_t sec;
};

//MVB获取相关参数结构体
struct _DATA_FROM_MVB_ST
{
	enum BOOL speed_valid_flag;  	//速度有效标志
	enum BOOL time_valid_flag;  	//时间有效标志
	enum BOOL wheel_vaild_flag;		//轮径有效标志
	enum BOOL train_id_valid_flag;	//列车号有效标志
	uint16_t train_id;
	uint8_t time_buf[6];
	uint16_t wheel_value;
	uint16_t speed_buf[15];
	uint16_t station_next;
	uint16_t station_cur;
	uint16_t train_line_num;
};





/*************************************************
Function:  check_dir_exits
Description:  检查一个目录是否存在
Input: 　目录绝对路径 dir_path
Output: 无
Return: 存在0，不存在-１
Others:
*************************************************/
int check_dir_exits(const char* dir_path);
void self_test_control();



/*************************************************
Function:  get_para_from_local
Description:  从本地emmc获取配置参数
Input: 　无
Output: 无
Return: 无
Others:
*************************************************/
void get_para_from_local(void);



/*************************************************
Function:  create_filedir
Description:  创建数据存储文件夹
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void create_filedir(void);


/*************************************************
Function:  get_local_time
Description: 获取本机当前时间
Input:  时间结构体指针
Output: 无
Return: 无
Others:
*************************************************/
void get_local_time(struct LOCAL_TIME *timel);

void init_local_time(void);
/*************************************************
	Function:  open_tzdata_file
	Description:  创建本地特征数据存储文件
	Input:  无
	Output: 无
	Return: 无
	Others:
*************************************************/
void open_tzdata_file(void);



/*************************************************
Function:  open_rawdata_file
Description:  创建SD卡原始数据存储文件
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void open_rawdata_file(void);


uint8_t read_para_from_config(void);


/*************************************************
Function:  init_data
Description: 读取配置参数，创建存储文件
Input:  无
Output: 无
Return: 无
Others:
*************************************************/
void init_user_data(void);

/*************************************************
Function:  print_system_para
Description: 打印系统配置的相关参数
Input:
Output:
Return:
Others:
*************************************************/
void print_system_para(void);


/*************************************************
Function:  set_local_time
Description: 设置本机当前时间
Input:  时间结构体指针
Output: 无
Return: 无
Others:
*************************************************/
void set_local_time(struct LOCAL_TIME *timel);

void printf_recv_msg(uint8_t *temp_data,uint16_t len);


void init_can_status(void);

void set_para_to_local(void);
//void malloc_two_dim_pointer_buff(uint16_t **pointer_buff,uint16_t row_size,uint16_t col_size);

/**
 *1车传感器的报警状态
 * */
/**
 * 向MVB发送的传感器报警信息
*/
union MVB_SEND_ALARM_STATUS
{
    struct
    {
        // 2车1位轴端轴承一级报警	Bit	1=故障 0=正常	0	0
        // 2车1位轴端轴承二级报警	Bit		0	1
        // 2车1位轴端踏面一级报警	Bit		0	2
        // 2车1位轴端踏面二级报警	Bit		0	3
        // 2车1位轴端温度预警	    Bit		0	4
        // 2车1位轴端温度报警	    Bit		0	5
        unsigned char side1_shaft_end_bearing_first_alarm : 1;
        unsigned char side1_shaft_end_bearing_second_alarm : 1;
        unsigned char side1_shaft_end_tread_first_alarm : 1;
        unsigned char side1_shaft_end_tread_second_alarm : 1;
        unsigned char side1_shaft_end_temp_warn : 1;
        unsigned char side1_shaft_end_temp_alarm : 1;
        unsigned char side2_shaft_end_bearing_first_alarm : 1;
        unsigned char side2_shaft_end_bearing_second_alarm : 1;
        unsigned char side2_shaft_end_tread_first_alarm : 1;
        unsigned char side2_shaft_end_tread_second_alarm : 1;
        unsigned char side2_shaft_end_temp_warn : 1;
        unsigned char side2_shaft_end_temp_alarm : 1;
        unsigned char side3_shaft_end_bearing_first_alarm : 1;
        unsigned char side3_shaft_end_bearing_second_alarm : 1;
        unsigned char side3_shaft_end_tread_first_alarm : 1;
        unsigned char side3_shaft_end_tread_second_alarm : 1;
        unsigned char side3_shaft_end_temp_warn : 1;
        unsigned char side3_shaft_end_temp_alarm : 1;
        unsigned char side4_shaft_end_bearing_first_alarm : 1;
        unsigned char side4_shaft_end_bearing_second_alarm : 1;
        unsigned char side4_shaft_end_tread_first_alarm : 1;
        unsigned char side4_shaft_end_tread_second_alarm : 1;
        unsigned char side4_shaft_end_temp_warn : 1;
        unsigned char side4_shaft_end_temp_alarm : 1;
        unsigned char side5_shaft_end_bearing_first_alarm : 1;
        unsigned char side5_shaft_end_bearing_second_alarm : 1;
        unsigned char side5_shaft_end_tread_first_alarm : 1;
        unsigned char side5_shaft_end_tread_second_alarm : 1;
        unsigned char side5_shaft_end_temp_warn : 1;
        unsigned char side5_shaft_end_temp_alarm : 1;
        unsigned char side6_shaft_end_bearing_first_alarm : 1;
        unsigned char side6_shaft_end_bearing_second_alarm : 1;
        unsigned char side6_shaft_end_tread_first_alarm : 1;
        unsigned char side6_shaft_end_tread_second_alarm : 1;
        unsigned char side6_shaft_end_temp_warn : 1;
        unsigned char side6_shaft_end_temp_alarm : 1;
        unsigned char side7_shaft_end_bearing_first_alarm : 1;
        unsigned char side7_shaft_end_bearing_second_alarm : 1;
        unsigned char side7_shaft_end_tread_first_alarm : 1;
        unsigned char side7_shaft_end_tread_second_alarm : 1;
        unsigned char side7_shaft_end_temp_warn : 1;
        unsigned char side7_shaft_end_temp_alarm : 1;
        unsigned char side8_shaft_end_bearing_first_alarm : 1;
        unsigned char side8_shaft_end_bearing_second_alarm : 1;
        unsigned char side8_shaft_end_tread_first_alarm : 1;
        unsigned char side8_shaft_end_tread_second_alarm : 1;
        unsigned char side8_shaft_end_temp_warn : 1;
        unsigned char side8_shaft_end_temp_alarm : 1;
    } bits;
    unsigned char bytes[6];
};

#ifdef WTD_DATA_TRANSLATE_PROTOCOL

#ifdef WTD_DATA_PROTOCOL_20220822

	#ifdef WTD_DATA_TEST
		#define QUEUE_CACHE_BEFORE_ALARM_TIME   2//报警前30秒
		#define QUEUE_CACHE_AFTER_ALARM_TIME	2//30s //报警解除后30秒
		#define QUEUE_CACHE_LIMIT_TIME	    (30*2)//报警中最长1分钟缓存  6*(30*2*1024)=0.35MB
	#else
		#define QUEUE_CACHE_BEFORE_ALARM_TIME   60//报警前60秒
		#define QUEUE_CACHE_AFTER_ALARM_TIME	60//报警解除后60秒
		#define QUEUE_CACHE_LIMIT_TIME	    (30*60)//报警中最长30分钟缓存  6*(30*60*1024)=10.5MB
	#endif

	#define QUEUE_CACHE_BEFORE_ALARM_NUM 	QUEUE_CACHE_BEFORE_ALARM_TIME
	#define QUEUE_CACHE_AFTER_ALARM_NUM 	QUEUE_CACHE_AFTER_ALARM_TIME
	#define QUEUE_QNODE_LIMIT_NUM			QUEUE_CACHE_LIMIT_TIME

	#define MSG_ALARM_DATA_SIZE			968

	#define WTD_TZ_MEMSAGE_LENGTH     400//从帧头到第一个校验和的所有字节
	#define WTD_ALARM_MEMSAGE_LENGTH 1000//从帧头到第一个校验和的所有字节

	#define ONE_PACKAGE_RESEND_NUM  1
#else

	#ifdef WTD_DATA_TEST
		#define QUEUE_CACHE_BEFORE_ALARM_TIME   2//报警前30秒
		#define QUEUE_CACHE_AFTER_ALARM_TIME	2//30s //报警解除后30秒
	#else
		#define QUEUE_CACHE_BEFORE_ALARM_TIME   30//报警前30秒
		#define QUEUE_CACHE_AFTER_ALARM_TIME	30//30s //报警解除后30秒
	#endif
	#define QUEUE_CACHE_LIMIT_TIME	    (30*60)//报警中最长30分钟缓存  6*(30*60*1024)=10.5MB

	#define QUEUE_CACHE_BEFORE_ALARM_NUM 	QUEUE_CACHE_BEFORE_ALARM_TIME
	#define QUEUE_CACHE_AFTER_ALARM_NUM 	QUEUE_CACHE_AFTER_ALARM_TIME
	#define QUEUE_QNODE_LIMIT_NUM			QUEUE_CACHE_LIMIT_TIME

	#ifdef WTD_ALARM_DATA_AS_STANDARD
		#define MSG_ALARM_DATA_SIZE			974
		#define MSG_ALARM_RES_DATA_SIZE		25 //预留25个字节，0

		#define TZ_PART_CHECK_SUM_START_INDEX 	26
		#define TZ_PART_CHECK_SUM_OUT_SIZE 		(2+624+2+26)
		#define TZ_CHECK_SUM_START_INDEX        2
		#define TZ_CHECK_SUM_OUT_SIZE 			(2+2)

		#define ALARM_PART_CHECK_SUM_START_INDEX  26
		#define ALARM_PART_CHECK_SUM_OUT_SIZE 	  (2+24+2+26)
		#define ALARM_CHECK_SUM_START_INDEX       2
		#define ALARM_CHECK_SUM_OUT_SIZE 		  (2+2)

		#define TZ_COPY_START_INDEX 	24
		#define TZ_COPY_OUT_SIZE 		(2+624+24)
		#define ALARM_COPY_START_INDEX  24
		#define ALARM_COPY_OUT_SIZE 	(2+24+24)

		#define WTD_TZ_MEMSAGE_LENGTH   (423-23)//从帧头到第一个校验和的所有字节
		#define WTD_ALARM_MEMSAGE_LENGTH (1023-23)//从帧头到第一个校验和的所有字节
	#else
		#define MSG_ALARM_DATA_SIZE			949
		#define MSG_ALARM_RES_DATA_SIZE		25 //预留25个字节，0

		#define TZ_PART_CHECK_SUM_START_INDEX 	26
		#define TZ_PART_CHECK_SUM_OUT_SIZE 		(2+624+2+26)
		#define TZ_CHECK_SUM_START_INDEX        2
		#define TZ_CHECK_SUM_OUT_SIZE 			(2+2)

		#define ALARM_PART_CHECK_SUM_START_INDEX  26
		#define ALARM_PART_CHECK_SUM_OUT_SIZE 	  (2+49+2+26)
		#define ALARM_CHECK_SUM_START_INDEX       2
		#define ALARM_CHECK_SUM_OUT_SIZE 		  (2+2)

		#define TZ_COPY_START_INDEX 	24
		#define TZ_COPY_OUT_SIZE 		(2+624+24)
		#define ALARM_COPY_START_INDEX  24
		#define ALARM_COPY_OUT_SIZE 	(2+49+24)

		#define WTD_TZ_MEMSAGE_LENGTH   (423-23)//从帧头到第一个校验和的所有字节
		#define WTD_ALARM_MEMSAGE_LENGTH (998-23)//从帧头到第一个校验和的所有字节
	#endif

	#define ONE_PACKAGE_RESEND_NUM  10
#endif

enum ALARM_TRIG_EVENT
{
  TRIG_OK,					//正常
  TRIG_ALARM,				//报警
  TRIG_ALARM_REMOVE,		//报警解除,//直到解除后30s结束，才恢复TRIG_OK
  TRIG_ALARM_WAIT           //超最上限等待解除数据发完
};

enum FAULT_SENSOR_POS
{
	FAULT_SENSOR_POS_ONE = 0x00,
	FAULT_SENSOR_POS_FOUR = 0x01,
	FAULT_SENSOR_POS_FIVE = 0x02,
	FAULT_SENSOR_POS_EIGHT = 0x03
};

enum ALARM_VALID_FLAG
{
	ALARM_INVALID = 0x0,
	ALARM_VALID = 0x01//报警解除30s后才无效
};

enum ALARM_SEND_CTRL
{
	ALARM_SEND_DISABLE = 0x0,
	ALARM_SEND_ENABLE = 0x01
};

enum ALARM_TZ_TYPE
{
	NO_ALARM_TYPE = 0x0,

	//CH0
	PW1_Y_PWX_ALARM_TYPE,
	PW1_Y_HC_ALARM_TYPE,
	PW1_Y_DC_ALARM_TYPE,

	//CH1
	PW1_Z_PWX_ALARM_TYPE,
	PW1_Z_DC_ALARM_TYPE,

	//CH3
	PW2_Y_PWX_ALARM_TYPE,
	PW2_Y_HC_ALARM_TYPE,
	PW2_Y_DC_ALARM_TYPE,

	//CH4
	PW2_Z_PWX_ALARM_TYPE,
	PW2_Z_DC_ALARM_TYPE
};

struct MSG_ALARM_TRIG
{
	uint8_t alarm_trig_status;
	uint8_t alarm_valid_flag;
	uint8_t alarm_tz_type;//一个通道数据可触发多个特征报警，仅当一个报警数据存完后才触发保存下一个特征警数据
};

struct DEQUEUE_DATA
{
	int8_t *deq_buf;//出队deq_buf //保存剩余数deq_buf，先从上秒deq_buf取，不够968，再出队放deq_buf,从deq_buf中取deq_buf_front_len, 取剩deq_buf_rear_len前移
	uint16_t deq_buf_len;//deq_buf，remain_buf,  =SAMPLE_HZ*sizeof(int16_t)
	uint16_t deq_buf_front_len;
	uint16_t deq_buf_rear_len;//save_buf中保存deq_buf中取剩个数
};

struct LESS_HEAD_INFO
{
	uint16_t life_signal; //生命信号，每秒加1
	uint16_t board_set;   //目标板卡的位集合
};

struct MESSAGE_COUNT_CTRL
{
	uint16_t update_cnt;//10s计数
	uint16_t packet_num_cnt;//包序号，每10s加1
	uint16_t packet_total;//包总数
	uint8_t  send_ctrl;//enum ALARM_SEND_CTRL
	uint8_t  not_first_flag;//0为首次发送，1为非首次发送
};

union MESSAGE_TZ_TYPE
{
    struct
	{
    	uint8_t res : 6;// res=0
		uint8_t type : 2;//报文类型：0x01  	特征数据
	} bits;

	uint8_t byte;//33 34
};

union MESSAGE_ALARM_TYPE
{
    struct
	{
    	uint8_t fault_data_flag : 1;//报警数据标志位	1 =有效
    	uint8_t fault_sensor_pos : 2;//故障传感器位置	0=1 位，1=4 位，2=5 位，3=8 位
    	uint8_t res : 3;// res=0
		uint8_t type : 2;//报文类型：0x02  	原始数据
	} bits;

	uint8_t byte;//33 34
};

#ifdef WTD_DATA_PROTOCOL_20220822
//自定义时间结构体
struct UINT8_DATE_TIME
{
	uint8_t year;
	uint8_t mon;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
};//6

struct WTD_GPS_POS
{
    uint8_t longitude_down;
    uint8_t longitude_mid_down;
    uint8_t longitude_mid_up;
    uint8_t longitude_up;

    uint8_t latitude_dir;//纬度方向
    uint8_t longitude_dir;//经度方向

    uint8_t latitude_down;
    uint8_t latitude_mid_down;
    uint8_t latitude_mid_up;
    uint8_t latitude_up;
};//10

struct TZ_VALUE_DATA
{
    struct UINT16_TYPE side1_y_quota;
    struct UINT16_TYPE side2_y_quota;
    struct UINT16_TYPE side1_z_quota;
    struct UINT16_TYPE side2_z_quota;
    uint8_t res[8];
    struct UINT16_TYPE y_peak_w1;
    struct UINT16_TYPE y_peak_w2;
    struct UINT16_TYPE y_root_w1;
    struct UINT16_TYPE y_root_w2;
    struct UINT16_TYPE z_root_w1;
    struct UINT16_TYPE z_root_w2;
};//28

struct WTD_PW_TZ_DATA
{
    uint8_t frame_head[2];//0 1  //0xAA 0xAA

    uint8_t res0;//2
    uint8_t carriage_number;//3 //车厢号1~17

    union MESSAGE_TZ_TYPE message_type;//4
    struct UINT16_TYPE packet_num;//5 6   //0-65535，10s 更新一次，WTD 根据跟包序号落地

    struct UINT8_DATE_TIME atp_time;//7 12 //数据开始时间（ATP时间）---填包时间，数据产生的时间

    struct UINT16_TYPE km_flag_h;//公里标（低 低位）byte13 14 ---本包时间对应的公里标
    struct UINT16_TYPE km_flag_l;//公里标（低 高位）byte15 16
    uint8_t res1;//17
    struct UINT16_TYPE speed;//18 19 //数据开始时速度（第一个特征值时的速度）
    struct WTD_GPS_POS gps_pos;//20 29 //数据开始时经纬度

    struct TZ_VALUE_DATA tz_value_data;//30 57

    uint8_t res2[2];//58 59

    struct UINT8_DATE_TIME tcms_time;//60 65 //数据开始时间（列车网络时间）

    uint8_t res3[332];//66 397
    struct UINT16_TYPE part_check_sum;//398 399 校验和高 低字节
};

struct WTD_PW_ALARM_DATA
{
    uint8_t frame_head[2];//0 1  //0xAB 0xAB

    struct UINT16_TYPE message_length;//2 3
    union MESSAGE_ALARM_TYPE message_type;//4

    struct UINT16_TYPE packet_num;//5 6
    struct UINT16_TYPE packet_total;//7 8

    struct UINT8_DATE_TIME atp_time;//9 14 //数据开始时间（ATP时间）---填包时间，数据产生的时间
    uint8_t res[2];//15 16

    uint8_t carriage_number;//17 //车厢号1~17
    struct UINT16_TYPE km_flag_h;//公里标（低 低位）byte18 19 ---本包时间对应的公里标
    struct UINT16_TYPE km_flag_l;//公里标（低 高位）byte20 21
    struct UINT16_TYPE speed;//22 23 //数据开始时速度

    struct UINT8_DATE_TIME tcms_time;//24 29 //数据开始时间（列车网络时间）

    int8_t alarm_data[MSG_ALARM_DATA_SIZE];//30 997
    struct UINT16_TYPE part_check_sum;//998 999 校验和高 低字节
};

#else

struct WTD_TRAIN_PUBLIC_INFO
{
    uint8_t year;
    uint8_t mon;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;//38

    union S_MARSHALLING {
        struct
        {
            uint8_t bai : 4;
            uint8_t qian : 4;
            uint8_t ge : 4;
            uint8_t shi : 4;
        } bits;

        uint8_t byte[2];
    } marshalling;//编组编号 39 40

    uint8_t carriage_number;//车厢号1~17

    struct UINT16_TYPE km_flag_h;//公里标（低 低位）byte42 43
    struct UINT16_TYPE km_flag_l;//公里标（低 高位）byte44 45
};

struct TZ_VALUE_DATA
{
	struct UINT16_TYPE speed;//46 47

    uint8_t longitude_down;
    uint8_t longitude_mid_down;
    uint8_t longitude_mid_up;
    uint8_t longitude_up;//51

    uint8_t latitude_dir;//纬度方向
    uint8_t longitude_dir;//经度方向 53

    uint8_t latitude_down;
    uint8_t latitude_mid_down;
    uint8_t latitude_mid_up;
    uint8_t latitude_up;//57

    struct UINT16_TYPE side1_y_quota;//58 59
    struct UINT16_TYPE side2_y_quota;
    struct UINT16_TYPE side1_z_quota;
    struct UINT16_TYPE side2_z_quota;//64 65

    struct UINT16_TYPE y_peak_w1;
    struct UINT16_TYPE y_peak_w2;
    struct UINT16_TYPE y_root_w1;
    struct UINT16_TYPE y_root_w2;
    struct UINT16_TYPE z_root_w1;
    struct UINT16_TYPE z_root_w2;//76 77
};//32

struct WTD_PW_TZ_DATA
{
    struct SEND_DATA_HEAD data_head;//0-23  //Byte0-1为：0xAA57
    uint8_t frame_head[2];//24 25       //0xA5 0xA5

    struct UINT16_TYPE message_length;//26 27
    union MESSAGE_TZ_TYPE message_type;//28

    struct UINT16_TYPE packet_num;//29 30   //0-65535，10s 更新一次，WTD 根据跟包序号落地
    struct UINT16_TYPE packet_total;//31 32

    struct WTD_TRAIN_PUBLIC_INFO train_public_info;//33 45

    struct TZ_VALUE_DATA tz_value_data[10];//46 365

    uint8_t res1[56];//366 421
    struct UINT16_TYPE part_check_sum;//422 423 校验和高 低字节（26-421字节校验）

    uint8_t res2[624];//424 1047
    uint8_t check_sum[2];//1048 1049 校验和	累加和方式（2-1047字节校验）
};

struct WTD_PW_ALARM_DATA
{
    struct SEND_DATA_HEAD data_head;//0-23  Byte0-1为：0xAA58
    uint8_t frame_head[2];//24 25  0xA6 0xA6

    struct UINT16_TYPE message_length;//26 27
    union MESSAGE_ALARM_TYPE message_type;//28

    struct UINT16_TYPE packet_num;//29 30
    struct UINT16_TYPE packet_total;//31 32

    struct WTD_TRAIN_PUBLIC_INFO train_public_info;//33 45

    struct UINT16_TYPE speed;//46 47
#ifdef WTD_ALARM_DATA_AS_STANDARD
    int8_t alarm_data[MSG_ALARM_DATA_SIZE];//48 1021    (发ＷＴＤ：949+25，再补了25个字节--- 附件1 CR400AFCR300AF失稳系统、平稳系统数据落地协议-2021.06.21.pdf)
    struct UINT16_TYPE part_check_sum;//1022 1023 校验和高 低字节（26-996校验和）

    uint8_t res[24];//1024 1047
#else
    int8_t alarm_data[MSG_ALARM_DATA_SIZE];//48 996    (发ＷＴＤ：949+25，再补了25个字节--- 附件3：监控集成主机内部协议-以太网、CAN-平稳.xls)
    struct UINT16_TYPE part_check_sum;//997 998 校验和高 低字节（26-996校验和）

    uint8_t res[49];//999 1047
#endif
    uint8_t check_sum[2];//1048 1049 校验和	累加和方式（2-1047字节校验）
};

//struct WTD_PW_TZ_ALARM_DATA
//{
//	//---TZ DATA
//    uint8_t frame_head[2];//24 25       //0xA5 0xA5
//
//    struct UINT16_TYPE message_length;//26 27
//    union MESSAGE_TZ_TYPE message_type;//28
//
//    struct UINT16_TYPE packet_num;//29 30   //0-65535，10s 更新一次，WTD 根据跟包序号落地
//    struct UINT16_TYPE packet_total;//31 32
//
//    struct WTD_TRAIN_PUBLIC_INFO train_public_info;//33 45
//
//    struct TZ_VALUE_DATA tz_value_data[10];//46 365
//
//    uint8_t res1[56];//366 421
//    struct UINT16_TYPE part_check_sum;//422 423 校验和高 低字节（26-421字节校验）

//    //---ALARM DATA
//    uint8_t frame_head2[2];//24 25  0xA6 0xA6
//
//    struct UINT16_TYPE message_length2;//26 27
//    union MESSAGE_ALARM_TYPE message_type2;//28
//
//    struct UINT16_TYPE packet_num2;//29 30
//    struct UINT16_TYPE packet_total2;//31 32
//
//    struct WTD_TRAIN_PUBLIC_INFO train_public_info2;//33 45
//
//    struct UINT16_TYPE speed2;//46 47
//
//    uint8_t alarm_data2[MSG_ALARM_DATA_SIZE+MSG_ALARM_RES_DATA_SIZE];//48 996    (发ＷＴＤ：949+25，再补了25个字节--- 附件1 CR400AFCR300AF失稳系统、平稳系统数据落地协议-2021.06.21.pdf)
//    struct UINT16_TYPE part_check_sum2;//997 998 校验和高 低字节（26-996校验和）
//};
#endif
#endif

#ifdef ADD_DIAG_TZZ_DATA_FILE
struct DIAG_TZZ_DATA
{
	uint8_t head[2];//Byte0-1为：0xBB66 (大端)
	uint16_t packet_len;//包长度Byte2-3(大端)
	uint16_t packet_num;//存储包序号 Byte4-5(大端)

	struct TRAIN_PUBLIC_INFO train_public_info;//Byte6-38
	uint8_t data_type;//数据类型Byte39 pw板(0x66取前6为首，0x6N)算法特征数据:0x60

#if defined(INTERNAL_PROTOCOL_20210416) && !defined(PUB_INFO_REMOVE_KM_FLAG)
    uint8_t km_flag_ll;//公里标（低 低位）
    uint8_t km_flag_lh;//公里标（低 高位）
    uint8_t km_flag_hl;//公里标（高 低位）
    uint8_t km_flag_hh;//公里标（高 高位）//Byte43
#else
    uint8_t nc[4];//40 43
#endif

    //特征值部分，6个通道(CHANNEL_NUM):依次1位横、垂、纵向，2位横、垂、纵向-
    float pwx_index[CHANNEL_NUM];//平稳性指标//5s出结果－横、垂 //Byte44－67

    float wheel_avg_fre[CHANNEL_NUM];//车轮平均转频(hz)//5s出结果－垂//Byte68－91
    float wheel_max_fre[CHANNEL_NUM];//车轮最大主频(hz)//Byte92-115
    float wheel_max_amp[CHANNEL_NUM];//车轮最大幅值(单位:m/s^2)//Byte116-139

    float hc_fz[CHANNEL_NUM];//晃车峰值(单位:g)//2s出结果－横//Byte140-163

    float dc_rms[CHANNEL_NUM];//抖车均方根值(单位:g)//5s出结果－横、垂//Byte164-187

    float ten_hc_0d2_3_max_fre[CHANNEL_NUM];//晃车0.2~3Hz最大主频//10s出结果－横、垂//Byte188-211
    float ten_hc_0d2_3_max_amp[CHANNEL_NUM];//晃车0.2~3Hz最大幅值//Byte212-235
    //float ten_hc_0d2_3_fre_percent[CHANNEL_NUM];//晃车0.2~3Hz最大主频占比

	float ten_dc_3_15_max_fre[CHANNEL_NUM];//抖车3~15Hz最大主频//10s出结果－横、垂//Byte236-259
	float ten_dc_3_15_max_amp[CHANNEL_NUM];//抖车3~15Hz最大幅值//Byte260-283
	//float ten_dc_3_15_fre_percent[CHANNEL_NUM];//晃车3~15Hz最大主频占比

	float ten_bridge_z_max_fabs_amp[Z_CHANNEL_NUM];//桥梁时域10s　5120点对应的幅值的绝对值的最大值-垂Byte284-291
	float ten_bridge_z_max_fre[Z_CHANNEL_NUM];//垂向1~3Hz最大主频//10s出结果－垂//Byte292-299
	float ten_bridge_z_max_amp[Z_CHANNEL_NUM];//垂向1~3Hz最大幅值//Byte300-307
	float ten_bridge_avg_speed[Z_CHANNEL_NUM];//10s平均速度//10s出结果－垂//Byte308-315
	float ten_bridge_len[Z_CHANNEL_NUM];//10s桥梁长度//Byte316-323
	float ten_bridge_avg_len[Z_CHANNEL_NUM];//10s平均桥梁长度//Byte324-331
	uint8_t ten_bridge_flag[Z_CHANNEL_NUM];//10s桥梁识别标志//Byte332-333

	uint8_t ten_bridge_pwx_warn_status[Z_CHANNEL_NUM];//10s桥梁垂向平稳性预警状态－垂//Byte334-335

	uint8_t pwx_warn_status[CHANNEL_NUM];//平稳性预警状态//Byte336-341
	uint8_t pwx_alarm_status[CHANNEL_NUM];//平稳性报警状态//Byte342-347
	uint8_t wheel_alarm_status[CHANNEL_NUM];//车轮径跳报警状态//Byte348-353
	uint8_t hc_warn_status[CHANNEL_NUM];//平稳性预警状态//Byte354-359
	uint8_t hc_alarm_status[CHANNEL_NUM];//晃车报警状态//Byte360-365
	uint8_t dc_warn_status[CHANNEL_NUM];//抖车预警状态//Byte376-371
	uint8_t dc_alarm_status[CHANNEL_NUM];//抖车报警状态//Byte372-377

	uint8_t res[4];//Byte378-381

	uint8_t check_sum[2];//校验和//Byte382-383(大端)
};//384 Bytes(4字节对齐)
#endif

uint16_t check_sum(uint8_t *check_data,uint16_t data_len);
uint16_t sum_check_16(uint8_t *data, uint16_t crclen);


/*
Byte26:0	转向架 1 轴加速度传感器自检故障	上电时检测加速度传感器故障	0=正常，1=故障
Byte26:1	转向架 2 轴加速度传感器自检故障	上电时检测加速度传感器故障	0=正常，1=故障
Byte26:2	转向架 3 轴加速度传感器自检故障	上电时检测加速度传感器故障	0=正常，1=故障
Byte26:3	转向架 4 轴加速度传感器自检故障	上电时检测加速度传感器故障	0=正常，1=故障
Byte26:4	转向架 1 轴加速度传感器实时故障	实时检测加速度传感器故障	0=正常，1=故障
Byte26:5	转向架 2 轴加速度传感器实时故障	实时检测加速度传感器故障	0=正常，1=故障
Byte26:6	转向架 3 轴加速度传感器实时故障	实时检测加速度传感器故障	0=正常，1=故障
Byte26:7	转向架 4 轴加速度传感器实时故障	实时检测加速度传感器故障	0=正常，1=故障 */
union SWSENSOR_STATUS {
    struct
    {
        uint8_t bogie_axis1_self_test_err : 1;
        uint8_t bogie_axis2_self_test_err : 1;
        uint8_t bogie_axis3_self_test_err : 1;
        uint8_t bogie_axis4_self_test_err : 1;
        uint8_t bogie_axis1_real_time_err : 1;
        uint8_t bogie_axis2_real_time_err : 1;
        uint8_t bogie_axis3_real_time_err : 1;
        uint8_t bogie_axis4_real_time_err : 1;

    } bits;
    uint8_t byte;
};

/*
Byte27:0	速度输入（低速） 继电器故障
Byte27:1	速度输入（中速） 继电器故障
Byte27:2	速度输入（高速） 继电器故障
Byte27:4	转向架异常输出继电器故障
Byte27:5	装置故障输出继电器故障
Byte27:6	传感器试验输出继电器故障 */
union SWRELAY_STATUS {
    struct
    {
        uint8_t low_speed_in_relay_err : 1;
        uint8_t mid_speed_in_relay_err : 1;
        uint8_t high_speed_in_relay_err : 1;
        uint8_t res : 1;
        uint8_t bogie_out_relay_err : 1;
        uint8_t device_out_relay_err : 1;
        uint8_t sensor_test_out_relay_err : 1;
    } bits;
    uint8_t byte;
};

/*
Byte28:0	转向架 1 轴异常	检测出转向架 1 轴失稳故障	0=正常，1=故障
Byte28:1	转向架 2 轴异常	检测出转向架 2 轴失稳故障	0=正常，1=故障
Byte28:2	转向架 3 轴异常	检测出转向架 3 轴失稳故障	0=正常，1=故障
Byte28:3	转向架 4 轴异常	检测出转向架 4 轴失稳故障	0=正常，1=故障
Byte28:4	转向架故障	发生转向架失稳故障（任一轴失稳则置位）	0=正常，1=故障 */
union SWALARM_STATUS {
    struct
    {
        uint8_t bogie_axis1_alarm : 1;
        uint8_t bogie_axis2_alarm : 1;
        uint8_t bogie_axis3_alarm : 1;
        uint8_t bogie_axis4_alarm : 1;
        uint8_t bogie_alarm : 1;
        uint8_t res : 3;
    } bits;
    uint8_t byte;
};
/*
Byte29:0	转向架 1 轴预判	检测出转向架 1 轴失稳预判	0=正常，1=故障
Byte29:1	转向架 2 轴预判	检测出转向架 2 轴失稳预判	0=正常，1=故障
Byte29:2	转向架 3 轴预判	检测出转向架 3 轴失稳预判	0=正常，1=故障
Byte29:3	转向架 4 轴预判	检测出转向架 4 轴失稳预判	0=正常，1=故障
Byte29:4	转向架预判	发生转向架失稳预判（任一轴预判则置位）	0=正常，1=故障
Byte29:7	累计件数溢出	     默认填0                 0=正常，1=故障

*/
union SWWARN_STATUS {
    struct
    {
        uint8_t bogie_axis1_warn : 1;
        uint8_t bogie_axis2_warn : 1;
        uint8_t bogie_axis3_warn : 1;
        uint8_t bogie_axis4_warn : 1;
        uint8_t bogie_warn : 1;
#ifdef INTERNAL_PROTOCOL_20210416
        uint8_t res : 2;
        uint8_t total_bogie_overflow : 1;
#else
        uint8_t res : 3;
#endif
    } bits;
    uint8_t byte;
};

/*
Byte31:0-3	参数1 转向架异常检测参数等级	拨码开关设定的失稳报警判据中加速度阈值
Byte31:4-7	参数2 转向架异常检测参数等级	"拨码开关设定的失稳报警判据中波头数（波谷和波峰）"
Byte32:0-3	转向架异常检测参数1(百位)	1 个全波内波峰与波谷间转向架加速度最大值，用于监视记录
Byte32:4-7	转向架异常检测参数1(千位)
Byte33:0-3	转向架异常检测参数1（个位）
Byte33:4-7	转向架异常检测参数1（十位）
Byte34:0-3	转向架异常检测参数1(个位)	加速度超过加速度阈值的连续周期数，用于监视记录
Byte34:4-7	转向架异常检测参数1(十位)	 */
union SWBOGIE_ERR_PARA {
    struct
    {
        uint8_t para1_grade : 4;
        uint8_t para2_grade : 4;
        uint8_t max_amp_bai : 4;
        uint8_t max_amp_qian : 4;
        uint8_t max_amp_ge : 4;
        uint8_t max_amp_shi : 4;
        uint8_t over_amp_cycle_ge : 4;
        uint8_t over_amp_cycle_shi : 4;
    } bits;
    uint8_t byte[4];
};

union SWBOARD_ERR {
    struct
    {
        uint8_t sw_board_err : 1;
        uint8_t save_err : 1;
        uint8_t power1_err : 1;
        uint8_t power2_err : 1;
        uint8_t ad_err : 1;
        uint8_t watchdog :1;
        uint8_t res : 2;
    } bits;
    uint8_t byte;
};

union SWCOMM_ERR {
    struct
    {
        uint8_t ctrla_eth_err : 1; //与控制板A 以太网通信状态 0:正常 1：故障
        uint8_t ctrla_can_err : 1; //与控制板A CAN通信状态
        uint8_t ctrlb_eth_err : 1; //与控制板B 以太网通信状态
        uint8_t ctrlb_can_err : 1; //与控制板B CAN通信状态
        uint8_t res : 4;
    } bits;
    uint8_t byte;
};

union SWBOARD_SENSOR_ERR {
    struct
    {
        uint8_t axis1_sensor_err : 1;
        uint8_t axis2_sensor_err : 1;
        uint8_t axis3_sensor_err : 1;
        uint8_t axis4_sensor_err : 1;
        uint8_t res : 4;
    } bits;
    uint8_t byte;
};
/*
转向架 1 轴最小值特征值	范围0～256，1=0.1m/s2
转向架 2 轴最小值特征值	范围0～256，1=0.1m/s2
转向架 3 轴最小值特征值	范围0～256，1=0.1m/s2
转向架 4 轴最小值特征值	范围0～256，1=0.1m/s2
 */
struct TZ_MIN_VALUE
{
	uint8_t bogie_axis1_value;
	uint8_t bogie_axis2_value;
	uint8_t bogie_axis3_value;
	uint8_t bogie_axis4_value;
};

/*
转向架 1 轴均值特征值	范围0～256，1=0.1m/s2
转向架 2 轴均值特征值	范围0～256，1=0.1m/s2
转向架 3 轴均值特征值	范围0～256，1=0.1m/s2
转向架 4 轴均值特征值	范围0～256，1=0.1m/s2
 */
struct TZ_MEAN_VALUE
{
	uint8_t bogie_axis1_value;
	uint8_t bogie_axis2_value;
	uint8_t bogie_axis3_value;
	uint8_t bogie_axis4_value;
};

struct SW_AD_DATA
{
	uint16_t **buff;//AD缓存的二级指针
	uint16_t row_size;//AD缓存的行容量
	uint16_t row_index;//AD缓存的写的行下标，即写到多少行
	uint16_t column_size;//AD缓存的列容量
	uint16_t column_index;//AD缓存的写的列下标，即写到多少列
	uint16_t packet_id;//AD缓存的已经写过的总行号，除非重新采集，否则不会清零
	uint16_t ad_chnnel5[512];			//存放参考电压
};


struct SW_TZ_DATA
{
    struct SEND_DATA_HEAD data_head;//0 23
    uint8_t soft_version[2];//24 25
    union SWSENSOR_STATUS sensor_status;//26
    union SWRELAY_STATUS relay_status;//暂不需要（默认填0）//27
    union SWALARM_STATUS alarm_status;//28
    union SWWARN_STATUS warn_status;//29

    uint8_t total_bogie_err_cnt;//30
    union SWBOGIE_ERR_PARA bogie_err_para;//31 34

    union SWBOARD_ERR borad_err;//35
    union SWCOMM_ERR com_err;//36
    union SWBOARD_SENSOR_ERR sensor_err;//37
#if defined(INTERNAL_PROTOCOL_20210416)
    uint8_t rcc[2];//38 39
    uint8_t data_output_res;//40
#else
    uint8_t rcc[3];//38 40
#endif
    uint8_t company_data[8];//41 48

    struct TRAIN_PUBLIC_INFO train_public_info;//49 81

#if defined(INTERNAL_PROTOCOL_20210416)
    uint8_t km_flag_ll;//公里标（低 低位）82
    uint8_t km_flag_lh;//公里标（低 高位）83
    uint8_t km_flag_hl;//公里标（高 低位）84
    uint8_t km_flag_hh;//公里标（高 高位）85
    uint8_t res[4];	  //公共数据后统一预留4Bytes 默认填０ 86 89
#else
    uint8_t res[8];
#endif

    uint8_t total_power_on_times[2];
    uint8_t total_work_on_time[4];
    uint8_t sys_cur_work_on_time[4];

#if defined(INTERNAL_PROTOCOL_20210416)
    struct TZ_MIN_VALUE tz_min;//100 103
    struct TZ_MEAN_VALUE tz_mean;//104 107
    uint8_t sw_res[20];//失稳模块统一预留 108 127
    uint8_t company_res[126];//各厂家可自行使用预留 128 253
    uint8_t check_sum[2];//254 255
#else
    uint8_t km_scale[40];//100 139
    uint8_t resv[114];//140 253
    uint8_t check_sum[2];//254 255
#endif
};

struct SW_RAW_DATA
{
//    struct SEND_DATA_HEAD send_data_head;
	psw_save_data_head_t send_data_head;
    uint16_t ad[512];
    uint8_t check_sum[2];
};

struct SW_THRESHOLD_PARA
{
  float alarm_value;
  uint16_t h_cnt;
  uint16_t l_cnt;
};

struct SW_DIAGNOS_THRESHOLD_PARA
{
	struct SW_THRESHOLD_PARA side1_y_para;
	struct SW_THRESHOLD_PARA side2_y_para;
//	struct SW_THRESHOLD_PARA side3_y_para;
//	struct SW_THRESHOLD_PARA side4_y_para;
};

typedef struct SW_simulation{

     //以下所有状态  0--正常    1--故障
  uint8_t eth_A_err : 1; // 控制板A以太网通信故障
  uint8_t can_A_err : 1; // 控制板B以太网通信故障
  uint8_t eth_B_err : 1;// 控制板A CAN 通信故障
  uint8_t can_B_err : 1;// 控制板B CAN通信故障
  uint8_t board_err : 1;//板卡故障
  uint8_t save_err : 1;//存储故障
  uint8_t bogie_err : 1;//转向架故障
  uint8_t bogie_warning : 1;//转向架预判

  uint8_t bogie_one_err : 1;// 转向架1轴异常
  uint8_t bogie_one_warning : 1;// 转向架1轴预判
  uint8_t bogie_one_sensor_self : 1;//转向架1轴加速度传感器上电自检
  uint8_t bogie_one_sensor_state : 1;//转向架1轴加速度传感器状态
  uint8_t bogie_one_sensor_online : 1;//转向架1轴加速度传感器实时状态
  uint8_t watchdog_reset :1;
  uint8_t reserve : 2;
 }SW_SIMULATION;

//主机处理板相关配置参数
struct PW_CLB_CONFIG_PARA
{
	struct NET_ADDR  local_net_para;  	//本地以太网
	struct NET_ADDR   ptu_net_para;
#ifdef WTD_DATA_TRANSLATE_PROTOCOL
	struct WTD_NET_ADDR   wtd_net_para;
	struct UCAST_ADDR save_board_addr0;			//记录板ip地址（eth0）
#endif
	struct MCAST_ADDR pw_send_mcast_addr;		//平稳板发送组播地址
	struct MCAST_ADDR pw_recv_mcast_addr;		//平稳板接收组播地址
	struct MCAST_ADDR sw_send_mcast_addr;		//失稳板发送组播地址
	struct MCAST_ADDR sw_recv_mcast_addr;		//失稳板接收组播地址
	struct UCAST_ADDR save_board_addr;			//记录板ip地址
//	struct UCAST_ADDR ctrla_board_addr;			//控制板A ip
//	struct UCAST_ADDR ctrlb_board_addr;			//控制板B ip
	struct PW_DIAGNOS_THRESHOLD_PARA  pw_diagnos_para;	//平稳算法诊断参数
	struct SW_DIAGNOS_THRESHOLD_PARA  sw_diagnos_para;
	uint8_t trainnum;
};
void init_sw_data_para();

//typedef struct
//{
//	uint8_t  local_ip[4];
//	uint8_t  netmask[4];
//	uint8_t  gateway[4];
//	uint16_t local_port;
//    uint8_t  train_num;
//	sys_para_t sys_para;
//}paras_t;
//typedef struct
//{
//    uint8_t head[2];
//    paras_t paras;
//    uint8_t check_sum;
//}app_paras_t;
//
//typedef struct
//{
//	app_paras_t paras_inform;
//	uint8_t     train_num;
//}app_paras_save_env_t;



typedef struct
{
	char       config_ip[16];
	uint16_t   port;
}config_net_inform_t;
//typedef struct
//{
//	int                udp_socket_fd;
//	struct sockaddr_in udp_sockaddr;
//}recv_net_handle_t;
typedef struct
{
	config_net_inform_t config_net_inform;
	uint32_t            mcu_sn[3];
}app_paras_config_env_t;

extern app_paras_config_env_t app_paras_config_env;
typedef struct
{
	uint8_t data_head[2];
	uint8_t data_len[2];
	uint8_t factory_code;
	uint8_t device_code;
	uint8_t reserve[5];
	uint8_t cmd;
	uint8_t data_inform[];
}net_config_protocol_t, *pnet_config_protocol_t;


typedef enum
{
	APP_BROADCAST_CMD         = 0x01,
	APP_BROADCAST_ACK_CMD     = 0x02,
	APP_CONFIG_INFORM_CMD     = 0x03,
	APP_CONFIG_INFORM_ACK_CMD = 0x04,
}net_config_cmd_e;

typedef union
{
	struct
	{
		uint8_t record   :1;
		uint8_t txb      :1;
		uint8_t clb      :1;
		uint8_t tclb1    :1;
		uint8_t tclb2    :1;
		uint8_t zxb      :1;
		uint8_t reserve1 :2;

		uint8_t reserve2 :8;
	}bits;
	uint8_t byte[8];
}broadcast_cmd_t, *pbroadcast_cmd_t; //APP_BROADCAST_CMD

typedef union
{
	struct
	{
		uint8_t record   :1;
		uint8_t txb      :1;
		uint8_t clb      :1;
		uint8_t tclb1    :1;
		uint8_t tclb2    :1;
		uint8_t zxb      :1;
		uint8_t pswb     :1;
		uint8_t reserve1 :1;
	}bits;
	uint8_t byte;
}target_broad_t, *ptarget_broad_t;

typedef struct
{
	target_broad_t target_broad;
	uint8_t        device_code;
	uint8_t        sn[2];
	uint8_t        local_ip[4];
	uint8_t        mask[4];
	uint8_t        gateway[4];
	uint8_t        port[2];
	uint8_t        train_num;
	uint8_t        reserve[4];
	uint8_t        crc16[2];
}config_inform_cmd_t, *pconfig_inform_cmd_t; //APP_CONFIG_INFORM_CMD

typedef struct
{
	uint8_t device_code;
	uint8_t sn[2];
	uint8_t local_ip[4];
	uint8_t mask[4];
	uint8_t gateway[4];
	uint8_t port[2];
	uint8_t train_num;
	uint8_t reserve[4];
}broadcast_ack_cmd_t, *pbroadcast_ack_cmd_t; //APP_BROADCAST_ACK_CMD


typedef struct
{
	uint8_t data_head[2];
	uint8_t data_len[2];
	uint8_t factory_code;
	uint8_t device_code;
	uint8_t reserve[5];
	uint8_t cmd;
	uint8_t data_inform[sizeof(broadcast_ack_cmd_t)];
	uint8_t sun_crc[2];
}broadcast_ack_protocol_t, *pbroadcast_ack_protocol_t;

typedef struct
{
	uint8_t device_code;
	uint8_t sn[2];
	uint8_t state;
	uint8_t reserve;
}config_inform_ack_cmd_t, *pconfig_inform_ack_cmd_t; //APP_CONFIG_INFORM_ACK_CMD

typedef struct
{
	uint8_t data_head[2];
	uint8_t data_len[2];
	uint8_t factory_code;
	uint8_t device_code;
	uint8_t reserve[5];
	uint8_t cmd;
	uint8_t data_inform[sizeof(config_inform_ack_cmd_t)];
	uint8_t sun_crc[2];
}config_ack_protocol_t, *pconfig_ack_protocol_t;











#endif
