/*
 * global_macro.h
 *
 *  Created on: Apr 17, 2021
 *      Author: linux-ls
 */

#ifndef _GLOBAL_MACRO_H_
#define _GLOBAL_MACRO_H_

/*******************************************
TF卡支持兼容变更，宏说明：
HDD_DEV_MMCBLK0P3_OR_TF//插TF卡，则数据存TF卡，不插TF卡，数据存EMMC第三分区.
HDD_DEV_MMCBLK0P3_REMOVE_TF//仅去TF卡，用自带EMMC分区/dev/mmcblk1p3存数据(注:若不插TF卡时,EMMC为/dev/mmcblk0-正常运行时必需去TF卡;若插TF卡时,TF为/dev/mmcblk0,EMMC为/dev/mmcblk1)
*******************************************/

/*******************************************
 * JXDS　　　　　	2214车	 (基于JXDS_ZNDC项目) *
 ******************************************/
//旧内部协议方案1：open(PW_DIAG_NO_WARN_ALARM) close(PROJ_JXDS_ZNDC \ WTD相关宏 \ HDD_DEV_MMCBLK0P3_OR_TF )---V20.11.01---20220301北京,此为临时方案用旧协议,下次采用新协议
//旧内部协议方案2：open(PW_DIAG_NO_WARN_ALARM) close(PROJ_JXDS_ZNDC \ WTD相关宏 \ HDD_DEV_MMCBLK0P3_OR_TF )---V20.01.01---20220404重庆，解决AD导致死机问题
//旧内部协议方案3：open(PW_DIAG_NO_WARN_ALARM) close(PROJ_JXDS_ZNDC \ HDD_DEV_MMCBLK0P3_OR_TF )---V20.10.01---20220407大版本升级
//#define PROJ_JXDS_ZNDC_2214_OLD
#ifdef PROJ_JXDS_ZNDC_2214_OLD
	#define PW_DIAG_NO_WARN_ALARM //20220616(V20.10.01　加WTD相关宏)//20220616(V20.01.01 去WTD相关宏)//20220301(V20.11.01 北京 加WTD相关宏)
#endif

//2214与2251内部协议相同方案(备用)：open( PROJ_JXDS_ZNDC_2214 \ PROJ_JXDS_ZNDC  \ WTD相关宏(测试项除外) V20.10) close ( PUB_INFO_REMOVE_KM_FLAG \ CAN_PROTOCOL_20210322 )//2214软件：2022.1.17邮件要求：失稳、平稳内部通信协议按照智能配置动车组（JXDSZN）的协议执行, 如果2214只单升平稳、失稳需关can协议
//20220523北京南 V20.11
//#define PROJ_JXDS_ZNDC_2214
#ifdef PROJ_JXDS_ZNDC_2214
	#define PROJ_JXDS_ZNDC
	#define PUB_INFO_REMOVE_KM_FLAG//去公里标信息，解决不保存数据问题，要求同智能动车，但为了不影响本次不升级的其它板子，公共信息同以前40字节
#endif

/*******************************************
 *阿尔斯通智能动车组AST01项目 （基于JXDS_ZNDC项目）*
 ******************************************/
//open(PROJ_JXDS_ZNDC  \ WTD相关宏(测试项除外) \ HDD_DEV_MMCBLK0P3_OR_TF)
#define PROJ_JXDS_ZNDC_AST01
#ifdef PROJ_JXDS_ZNDC_AST01
	#define PROJ_JXDS_ZNDC
	#define HDD_DEV_MMCBLK0P3_OR_TF//插TF卡，则数据存TF卡，不插TF卡，数据存EMMC第三分区.
#endif

/*******************************************
 * JXDS_ZNDC项目	 2251车					   *
 ******************************************/
//open( PROJ_JXDS_ZNDC  \ WTD相关宏(测试项除外) )
#if !defined(PROJ_JXDS_ZNDC_2214_OLD) && !defined(PROJ_JXDS_ZNDC_2214) && !defined(PROJ_JXDS_ZNDC_AST01)
	#define PROJ_JXDS_ZNDC
#endif

/*******************************************
 * 公共部分					               *
 ******************************************/
#ifdef PROJ_JXDS_ZNDC
	#define ADD_MOTOR_BOARD

	#ifndef PROJ_JXDS_ZNDC_2214
		#define CAN_PROTOCOL_20210322
	#endif

	#define INTERNAL_PROTOCOL_20210416
	#define INTERNAL_PROTOCOL_20210725
#endif

#define ORIGINAL_DATA_SAVE_ACC//open:acc close:ad
//	#define TWO_POWER_TO_BOARD_ERR //开口项

//--------------------------------
#if defined(PROJ_JXDS_ZNDC) || defined(PROJ_JXDS_ZNDC_2214_OLD) || defined(PROJ_JXDS_ZNDC_2214)
//	#define WTD_DATA_TRANSLATE_PROTOCOL//pw sw  X8  10.0.0.1 ---20211208
//	#define WTD_ALARM_DATA_AS_STANDARD //因平稳内部协议的报警数据长度比落地协议小，以ＷＴＤ为准,(失稳内部协议是一致的不用改)
//	#define WTD_DATA_PROTOCOL_20220822//新协议0.5s更新发送
//	#define TCMS_MSG_ADD_ATP_TIME//四方更新了失稳、平稳、振动落地协议，网络通信协议增加了ATP时间---20220825
//	#define WTD_SIMULATION_ALARM_TRIG //仅测试时用，模拟报警触发

	//#define WTD_DNS_GW_TEST//仅测试时用
//	#define WTD_DATA_TEST//仅测试时用 __WTD_DEBUG__
	//#define WTD_FIXED_DATA_TEST
	//--------------------------------
	//#define S350_JXZNDC_17_SFE68
#endif
//--------------------------------
#define ADD_TZ_DATA_FILE
#ifdef PROJ_JXDS_ZNDC
//	#define ADD_DIAG_TZZ_DATA_FILE//添加算法特征值文件
#endif
//--------------------------------
/*******************************************
 * 算法部分      					           *
 ******************************************/
#define PW_DIAG
#ifdef PW_DIAG
	#define PW_MODIFY_20211112
//	#define PW_DIAG_GUOBIAO_5S//国标5秒计算
#endif

//#define JT_DIAG

//#define HC_DIAG
#ifdef HC_DIAG
	#define HC_MODIFY_20210527
	#define HC_MODIFY_20211112
#endif

//#define DC_DIAG
#ifdef DC_DIAG
	#define DC_MODIFY_20210527
	#define DC_DIAG_ADD_PW_JUDGE
	#define DC_MODIFY_20211112
#endif
#define DC_HC_USE_FFT_FILTER

#define PP_TZZ_DIAG_1S_FILLTER
//#define CT_10S_DIAG
//#define BRIDGE_DIAG //桥梁滤波算法

/*******************************************
 * 算法测试部分      					      　*
 ******************************************/
//#define SELF_TEST_NUM_TWO
//#define DIAG_DATA_SAVE_FOR_TEST //算法数据存储用于核验

//#define DATA_CONVERT_TXT_TO_BIN
//#define DATA_CENTER_TEST//pwx test

//#define SIMULATION_AD_SINE_WAVE_TEST
//------------------------------------------
//#define TEST_UPLOAD_AND_RECREATE_TIME_1MIN

//-----------------------------------
#define SD_CARD_SOFTWARE_MOUNT
#define SOFTWARE_CHMOD
//#define SOFTWARE_CREATE_DIR

//#define COMM_CHECK_TIMEOUT_INC
//-----------------------------------
#if 0
#define CAN_LED_TOGGLE_NO_ERR  //CAN灯灭状态强制闪  FOR CHECK
#endif

#if 0
#define CAN_ERR_JUDGE_NEW_STYTLE
#define CAN_ERR_REBOOT_TWO_TIMES
#define CAN_ERR_REBOOT
#endif

#if 1
#define CAN_ONE_FRAME_COUNT_CLEAR  //default one clear  no modify
#endif

//-------------------------------------------------
#define AD7606_COMPLEMENTARY_TO_ORIGINAL //补码转原码

#define OUTER_COMPANY_TEST_20211026 //外包测试用20211027  1.堆换成栈，2.去掉当ad跑飞后软件计数重启的功能．
                                    //mem_config－大量占用栈空间，易导致我们的底包板子程序运行到power打印时卡住; 外包的底包板子不会卡住．

#ifndef OUTER_COMPANY_TEST_20211026
	#define AD7606_STOP_START_CTRL　　　//定时检测ad7606是否正常采样．
	//#define AD7606_STOP_DATA_TIMER_TRIGGER
	#define AD7606_ERR_WATCHDOG_RESET//当ad跑飞(非正常采样)后,软件计数重启的功能
#endif

//#define AD_REF_VOLT_ERR_REBOOT
#ifdef AD_REF_VOLT_ERR_REBOOT
//	#define AD_REBOOT_FLAG
//	#define AD_GPIO_ERR_REC
#endif
//#define AD_RESET  //需要修改驱动
//--------------------------------------------------
#if 1//select only one
#define HISTORY_FEED_WATCHDOG
//#define ONLY_CAN_ETH_FEED_WATCHDOG //only use to test
#endif

#define NEW_DAY_RECTEATE_DIR

#define ORIGINAL_SAVE_PUBLIC_INFO //原始数据存储日期、编组编号、速度、车箱号, PTU解析对应，以便数据中心数据核对.

//-----------------------------------

/*******************************************
 * 版本号					               *
 ******************************************/
//以注释形式保留上次发布版本号及时间，上下并列对齐
#if defined(PROJ_JXDS_ZNDC_AST01)

	#define SOFT_VERSION_VAL       0x0103//0x0200//0x0101 　//软件版本
	#define SOFT_UPDATE_TIME       20220514//20220401//20210719//软件发布时间user_data
	#define SMALL_VERSION  		   0x01
	#define SOFT_VERSION_VAL_PRINTF  printf("LH-JXDSZN_AST01 JXDS_PW V%02x.%02x.%02x %s %s\n", (SOFT_VERSION_VAL>>8)&0xff, SOFT_VERSION_VAL&0xff, SMALL_VERSION&0xff, __DATE__, __TIME__)

#elif defined(PROJ_JXDS_ZNDC) && !defined(PROJ_JXDS_ZNDC_2214)

	#define SOFT_VERSION_VAL       0x0103//0x0101 　//软件版本
	#define SOFT_UPDATE_TIME       20220408//20210719//软件发布时间user_data
	#define SMALL_VERSION          0x01
	#define SOFT_VERSION_VAL_PRINTF  printf("LH-JXDSZN JXDS_PW V%02x.%02x.%02x %s %s\n", (SOFT_VERSION_VAL>>8)&0xff, SOFT_VERSION_VAL&0xff, SMALL_VERSION&0xff, __DATE__, __TIME__)

#else//2214

	#define SOFT_VERSION_VAL       0x2011//0x2001//00x2011//0x2001 //软件版本
	#define SOFT_UPDATE_TIME       20220522//20220404//20220301//20210617 //软件发布时间user_data
	#define SMALL_VERSION          0x01
	#define SOFT_VERSION_VAL_PRINTF  printf("LH-JXDS JXDS_PW V%02x.%02x.%02x %s %s\n", (SOFT_VERSION_VAL>>8)&0xff, SOFT_VERSION_VAL&0xff, SMALL_VERSION&0xff, __DATE__, __TIME__)

#endif

#endif /* _GLOBAL_MACRO_H_ */
