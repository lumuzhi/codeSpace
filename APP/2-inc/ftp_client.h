/*
 * ftp_client.h
 *
 *  Created on: Oct 24, 2019
 *      Author: www
 */

#ifndef FTP_CLIENT_H_
#define FTP_CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "user_data.h"

#define BUF_SIZE            2048
#define CMD_LEN             64
#define IP_LEN              32

#define SELF_DEV ZW_SW_PW	//自身设备升级类型

#define LOGIN_USER_NAME      	"USER objectcode\r\n"//登陆用户名 匿名登陆使用 anonymous
#define LOGIN_USER_PASSWOAR   	"PASS objectcode.1234\r\n"//登陆密码    匿名登陆使用 anonymous
#define GET_FTP_FILE_SIZE     	"SIZE /pw/lh/"//获取FTP服务器指定路径文件大小
#define GET_FTP_FILE_LIST		"LIST /pw/lh/\r\n"		//获取FTP服务器指定路劲下的文件
#define OPEN_LOCAL_FILE 		"/media/local_config/JXDS_PW"//打开本地文件（文件名与FTP服务器一致）
#define DOWNLOAD_FTP_FILE		"RETR /pw/lh/"
//#define DOWNLOAD_FTP_FILE 		"RETR /local_config/JXDS_SW\r\n"//下载FTP文件
#define CP_IAP_TO_TAGPATH       "cp /media/local_config/JXDS_PW /media/local_config/app/" //拷贝可执行文件到目标脚本

//#define SELF_DEV HOST_ZD	//自身设备升级类型
////FTP IAP ERR
//#define LOGIN_USER_NAME      	"USER objectcode\r\n"//登陆用户名 匿名登陆使用 anonymous yy
//#define LOGIN_USER_PASSWOAR   	"PASS objectcode.1234\r\n"//登陆密码    匿名登陆使用 anonymous
//#define GET_FTP_FILE_SIZE		"SIZE /bvds/lh/"			//后面还需要接上文件名
////#define GET_FTP_FILE_SIZE     	"SIZE /bvds/lh/JXDS_VIBR\r\n"//获取FTP服务器指定路径文件大小
//#define GET_FTP_FILE_LIST		"LIST /bvds/lh/\r\n"		//获取FTP服务器指定路劲下的文件
//#define OPEN_LOCAL_FILE 		"/media/local_config/JXDS_VIBR"//打开本地文件（文件名与FTP服务器一致）
//#define DOWNLOAD_FTP_FILE		"RETR /bvds/lh/"
////#define DOWNLOAD_FTP_FILE 		"RETR /bvds/lh/JXDS_VIBR\r\n"//下载FTP文件
//#define CP_IAP_TO_TAGPATH       "cp /media/local_config/JXDS_VIBR /media/local_config/app/" //拷贝可执行文件到目标脚本

enum IAP_DEV
{
	WTD = 0x01,								//
	TCU = 0x02,								//
	TCMS = 0x03,							//
	PHM = 0x04,							 	//
	SALFE_HOST = 0x05,						//
	BC = 0x06,								//
	BCU = 0x07,								//
	HOST_PW_SW_ZD = 0x08,					//主机、失稳、平稳、振动
	ZW_SW_PW = 0X09,						//轴温、失稳、平稳
	ZW_CZ=0Xa ,								// 齿振///
	PIS=0XB, 								//
	FAS=0XC,								//
	ZW=0XD,
	HOST_ZD=0XE,							//主机、振动///
	EEMS=0XF,
	DCU=0X10,
	HVAC=0X11,

};

enum IAP_CMD
{
	START_CMD = 0x02,	//开始升级命令
	SEND_CMD = 0x03,	//文件数据发送中
	END_CMD = 0x04,		//文件数据结束发送
	RESEND_CMD = 0x05,	//子部件对WTD文件数据重发请求
	FINISH_CMD = 0x06,	//子部件升级完成报文
	FILE_SIGLE = 0x07,	//文件传输心跳报文
};


/**外部调用接口**/
void iap_recv_thread_entry();

/**内部调用接口**/
static int ftp_connect(struct sockaddr_in tag_addr);
static int get_data_port(char *buf);
static void iap_data_deal(uint8_t *data,int len,struct sockaddr_in ip_addr);
static void GetCrc16table();
static unsigned short GetCrc16(unsigned char* ucpData, unsigned long nLength);
static void start_iap_ack(uint8_t*send_data,int lenth,struct sockaddr_in _addr);
static void send_file_signel(uint8_t*send_data,int lenth,struct sockaddr_in _addr,uint8_t flag);

#endif /**/
