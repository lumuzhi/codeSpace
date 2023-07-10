/*
 * ftp_client.c
 *
 *  Created on: Oct 23, 2019
 *      Author: www
 */

#include "ftp_client.h"
#include "user_data.h"
#include "ptu_app.h"


int iap_sock_fd;
unsigned short     g_Crc16Table[256] = {0};
extern struct PW_CLB_CONFIG_PARA *pw_clb_config_para;//


//extern char log_status[60];
//extern char log_kind[60] ;
//extern char log_detail[60] ;
//extern char log_file_name[64];

char send_sigal_data[1024] = {0};

/**
 * function     :ftp_connect
 * description  :FTP服务器连接，文件下载
 * input        :服务器IP
 * output       :none
 * return       :0-ok 非 0-err
 * others       :none
 */
static int ftp_connect(struct sockaddr_in tag_addr)
{
    int                    socket_fd = -1;
    int		        	   data_fd = -1;
    struct sockaddr_in     server_addr;
    struct sockaddr_in     data_addr;
    char		           buf[BUF_SIZE];
    char	               *port_buf = malloc(BUF_SIZE);
    int			           data_port = 0;
    uint32_t			   FileSize = 0,DataSize=0;
    int 				   recbytes;
    u_short 			   Percent = 0;
	char 				   server_ip[16];
	FILE 				   *FPtr;
	int						ret;
	char 					file_name[120];
	char 					file_name_temp[120];
	char 					get_file_size[100];
	char 					download_ftp_file[100];
	char 					*filename_p;


	char 					server_addr_len = 0;
	struct sockaddr_in temp_addr_client;
	server_addr_len = sizeof(temp_addr_client);
	char 				   client_ip[16];
	int get_name_num = -1;

	filename_p = NULL;
	//struct sockaddr_in temp_addr_client;
    /*Get the server IP by domain_name*/
	//sprintf(server_ip,"%d.%d.%d.%d",10,0,1,172);//测试使用
	strcpy(server_ip,inet_ntoa(tag_addr.sin_addr));//连接目标通信ftp服务IP


//	printf(" Get the server IP =%s \n",server_ip);

    /*Create a socket_fd: port, ip, and the address */
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);				//控制端口,发送命令(客户端命令端口号为Ｎ)
    if(socket_fd < 0)
    {
        printf("Fail to create a socket to connect with server*** \n");
        return -1;
    }
    //getsockname();
    get_name_num = getsockname(socket_fd,(struct sockaddr*)&temp_addr_client,(socklen_t *)&server_addr_len);


    strcpy(server_ip,inet_ntoa(tag_addr.sin_addr));

    strcpy(client_ip,inet_ntoa(temp_addr_client.sin_addr));//连接目标通信ftp服务IP
    printf("socket1:%s,%d,%d\n",client_ip,ntohs(temp_addr_client.sin_port),get_name_num);

   // strcpy(server_ip,inet_ntoa(tag_addr.sin_addr));

//    data_fd = socket(AF_INET, SOCK_STREAM, 0);					//数据端口,发送数据(客户端数据端口号为Ｎ　＋　１)			//客户端端口号应该＋１
//    if(data_fd < 0)
//    {
//		printf("Fail to create a socket to transmit the file data*** \n");
//		return -1;
//    }

    /* Connect with the server: master ftp port:21 */
    server_addr.sin_port = htons(21);
    server_addr.sin_family = AF_INET;

    ret = inet_aton(server_ip, &server_addr.sin_addr);
  //  printf("inet_aton_ret:%d\n",ret);

    //连接
    //第一个连接２１端口
    if( connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
    	close(socket_fd);
    	close(data_fd);
    	printf("connect fail1\n");
        printf("Fail to connect with the server***\n");
        return -1;
    }
    else
    {
    	//
    	ret=recv(socket_fd, buf, BUF_SIZE, 0);
    	if(ret<0)
    	{
    		printf("connect fail2\n");
			 printf("Recv Fail *server*\n");
			 return -1;
    	}
    	buf[ret]='\0';

    	//连接失败代码
    	if(strstr((const char *)buf,"220")==0)
    	{
    		printf("connect fail3\n");
    		printf("%s\n", buf);
    	   	close(socket_fd);
    	    close(data_fd);
    		return -1;
    	}
   // 	printf("connect_recv_from_socket_buff:%s\n", buf);
    	memset(buf, 0, BUF_SIZE);
    }

//    printf("ftp_client1\n");
    //使用２１端口发送登录用户名
	ret=send(socket_fd, LOGIN_USER_NAME, strlen(LOGIN_USER_NAME), 0);
	if(ret<0)
	{
		 printf("Send Fail *USER XXXX*\n");
		 close(socket_fd);
		 close(data_fd);
		 return -1;
	}

//	printf("ftp_client2\n");
	ret=recv(socket_fd, buf, BUF_SIZE, 0);
	if(ret<0)
	{
		 printf("Recv Fail *USER XXXX*\n");
		 close(socket_fd);
		 close(data_fd);
		 return -1;
	}
//	printf("ftp_client3\n");
	buf[ret]='\0';
	if(strstr((const char *)buf,"331")==0)
	{
		printf("%s\n", buf);
	   	close(socket_fd);
	    close(data_fd);
		return -1;
	}
//    printf("send_user_name_recv_from_socket:%s\n", buf);
    memset(buf, 0, BUF_SIZE);
 //   printf("ftp_client4\n");
    //使用２１端口发送登录密码
    ret=send(socket_fd, LOGIN_USER_PASSWOAR, strlen(LOGIN_USER_PASSWOAR), 0);
	if(ret<0)
	{
		 printf("Send Fail *PASS XXXX*\n");
		   	close(socket_fd);
		    close(data_fd);
		 return -1;
	}

//	printf("ftp_client5\n");
	ret=recv(socket_fd, buf, BUF_SIZE, 0);
	if(ret<0)
	{
		 printf("Recv Fail *PASS XXXX*\n");
		   	close(socket_fd);
		    close(data_fd);
		 return -1;
	}
	buf[ret]='\0';
//	printf("ftp_client6\n");
	if(strstr((const char *)buf,"230")==0)
	{
		printf("%s\n", buf);
	   	close(socket_fd);
	    close(data_fd);
		return -1;
	}
    printf("%s\n", buf);
    memset(buf, 0, BUF_SIZE);

//    printf("ftp_client7\n");
    ret=send(socket_fd, "TYPE A\r\n", strlen("TYPE A\r\n"), 0);//TYPE I   TYPE A
	if(ret<0)
	{
		 printf("Send Fail *TYPE A*\n");
		 close(socket_fd);
		 close(data_fd);
		 return -1;
	}

//	printf("ftp_client8\n");
	ret=recv(socket_fd, buf, BUF_SIZE, 0);
	if(ret<0)
	{
		 printf("Recv Fail *TYPE A*\n");
		 close(socket_fd);
		 close(data_fd);
		 return -1;
	}
	buf[ret]='\0';
	if(strstr((const char *)buf,"200")==0)
	{
		printf("%s\n", buf);
	   	close(socket_fd);
	    close(data_fd);
		return -1;
	}
//    printf("%s\n", buf);
    memset(buf, 0, BUF_SIZE);

//    ret=send(socket_fd, "PORT 10,0,2,174,25,101\r\n", strlen("PORT 10,0,2,174,25,101\r\n"), 0);
// 	if(ret<0)
// 	{
// 		 printf("Send Fail *PORT*\n");
// 		 return -1;
// 	}
//
//     ret=recv(socket_fd, buf, BUF_SIZE, 0);
// 	if(ret<0)
// 	{
// 		 printf("Recv Fail *PORT*\n");
// 		 return -1;
// 	}
// 	buf[ret]='\0';

    ret=send(socket_fd, "PASV\r\n", strlen("PASV\r\n"), 0);
	if(ret<0)
	{
		 printf("Send Fail *PASV*\n");
		 close(socket_fd);
		 close(data_fd);
		 return -1;
	}

    ret=recv(socket_fd, buf, BUF_SIZE, 0);
	if(ret<0)
	{
		 printf("Recv Fail *PASV*\n");
		 close(socket_fd);
		 close(data_fd);
		 return -1;
	}
//	buf[ret]='\0';
//	if(strstr((const char *)buf,"227")==0)
//	{
//		printf("%s\n", buf);
//		return -1;
//	}
   // printf("%s\n", buf);
 //   printf("ftp_client9:%s\n",buf);
    strncpy(port_buf, buf, BUF_SIZE);
    data_port = get_data_port(port_buf);
    //free(port_buf);
    memset(port_buf,0,BUF_SIZE);
    memset(buf, 0, sizeof(buf));

    data_fd = socket(AF_INET, SOCK_STREAM, 0);					//数据端口,发送数据(客户端数据端口号为Ｎ　＋　１)			//客户端端口号应该＋１
    if(data_fd < 0)
    {
		printf("Fail to create a socket to transmit the file data*** \n");
		return -1;
    }

    data_addr.sin_family = AF_INET;
    data_addr.sin_port = htons(data_port);
    inet_aton(server_ip, &data_addr.sin_addr);
	 if( connect(data_fd, (struct sockaddr*)&data_addr, sizeof(data_addr)) < 0)		//连接服务器数据端口
    {
		close(socket_fd);
		close(data_fd);
	    printf("Fail to connect with the data port*** ");
	    return -1;
    }
//	 printf("ftp_client10\n");

	 memset(file_name,0,sizeof(file_name));
	ret=send(socket_fd, GET_FTP_FILE_LIST, strlen(GET_FTP_FILE_LIST), 0);//
	if(ret<0)
	{
		printf("Send Fail *LIST*\n");
	   	close(socket_fd);
	    close(data_fd);
		return -1;
	}

	ret = read(data_fd,buf,2048);
	if(ret > 0)
	{
		//ret = read(data_fd,buf,2048);
		//memmove(file_name,buf+39,ret-39);
		memmove(file_name_temp,buf,ret);
//		printf("Recv_List_buf1:%s,ret:%d\n",buf,ret);
//		printf("file_name:%s\n",file_name);
	}

	filename_p = strstr(file_name_temp,"JXPW");
//		printf("filename_p:%s\n",filename_p-33);
		if(filename_p ==NULL)
		{
		   	close(socket_fd);
		    close(data_fd);
			return -1;
		}
		else
		{
			memmove(file_name,filename_p-33,53);		//获取到了需要升级的文件名字
		}

		file_name[53] = '\r';
		file_name[54] = '\n';

//		printf("file_name:%s\n",file_name);

//	if(strstr(file_name,"JXVIBR")==NULL)
//	{
//	   	close(socket_fd);
//	    close(data_fd);
//		return -1;
//	}

//
////
	ret=recv(socket_fd, buf, BUF_SIZE, 0);
	printf("Recv_List_buf end\n");
	if(ret<0)
	{
		printf("Recv Fail *LIST*\n");
	   	close(socket_fd);
	    close(data_fd);
		return -1;
	}
	buf[ret]='\0';
	printf("before TYPE I\r\n");

	close(data_fd);
	data_fd = -1;

    ret=send(socket_fd, "TYPE I\r\n", strlen("TYPE I\r\n"), 0);//TYPE I   TYPE A
	if(ret<0)
	{
		 printf("Send Fail *TYPE I*\n");
		   	close(socket_fd);
		    close(data_fd);
		 return -1;
	}
	ret=recv(socket_fd, buf, BUF_SIZE, 0);
	if(ret<0)
	{
		 printf("Recv Fail *TYPE I*\n");
		   	close(socket_fd);
		    close(data_fd);
		 return -1;
	}
	buf[ret]='\0';
	printf("after TYPE I\r\n");


	  ret=send(socket_fd, "PASV\r\n", strlen("PASV\r\n"), 0);						//设置被动模式
		if(ret<0)
		{
			 printf("Send Fail *PASV*\n");
			   	close(socket_fd);
			    close(data_fd);
			 return -1;
		}

	    ret=recv(socket_fd, buf, BUF_SIZE, 0);
		if(ret<0)
		{
			 printf("Recv Fail *PASV*\n");
			   	close(socket_fd);
			    close(data_fd);
			 return -1;
		}

	    strncpy(port_buf, buf, BUF_SIZE);
	    data_port = get_data_port(port_buf);
	    free(port_buf);


	    data_fd = socket(AF_INET, SOCK_STREAM, 0);					//数据端口,发送数据
	    if(data_fd < 0)
	    {
	    	printf("Fail to create a socket to transmit the file data*** \n");
		   	close(socket_fd);
		    close(data_fd);
	    	return -1;
	     }

	    data_addr.sin_family = AF_INET;
	    data_addr.sin_port = htons(data_port);
	    inet_aton(server_ip, &data_addr.sin_addr);
	    if( connect(data_fd, (struct sockaddr*)&data_addr, sizeof(data_addr)) < 0)
	    {
	    	printf("Fail to connect with the data port*** ");
		   	close(socket_fd);
		    close(data_fd);
	    	return -1;
	     }


		memset(get_file_size,0,sizeof(get_file_size));
	    strcat(get_file_size,GET_FTP_FILE_SIZE);
	    strcat(get_file_size,file_name);

	    memset(buf, 0, BUF_SIZE);
	    printf("before send fail size\n");
	    ret=send(socket_fd, get_file_size, strlen(get_file_size), 0);//获取待升级的文件大小
	    if(ret<0)
	    {
	    	printf("Send Fail *SIZE*\n");
		   	close(socket_fd);
		    close(data_fd);
	    	return -1;
	    }
	    ret=recv(socket_fd, buf, BUF_SIZE, 0);
	    if(ret<0)
	    {
	    	printf("Recv Fail *SIZE*\n");
		   	close(socket_fd);
		    close(data_fd);
	    	return -1;
	    }
	    buf[ret]='\0';
	    printf("ftp_client12:%s\n",buf);
	    if(strstr((const char *)buf,"213")==0)
	    {
	    	printf("%s\n", buf);
		   	close(socket_fd);
		    close(data_fd);
	    	return -1;
	    }
	    printf("%s\n", buf);
	    sscanf(buf,"%d %d",&FileSize,&FileSize);
	    printf("file size:%d \n",FileSize);
	    memset(buf, 0, BUF_SIZE);

	    memset(download_ftp_file,0,sizeof(download_ftp_file));
	    strcat(download_ftp_file,DOWNLOAD_FTP_FILE);
	    strcat(download_ftp_file,file_name);

	    printf("download_ftp_file:%s\n",download_ftp_file);
	    FPtr = fopen(OPEN_LOCAL_FILE, "w+");		//新建一个文件,若已经存在则清除以前内容
	    if(FPtr==NULL)
	    {
	    	printf("Fail to open the local file**** \n");
	    	fclose(FPtr);
		   	close(socket_fd);
		    close(data_fd);
	    	return -1;
	    }
    printf("ftp_client13\n");
	ret = send(socket_fd, download_ftp_file, strlen(download_ftp_file), 0);//
	if(ret<0)
	{
		 printf("Send Fail *DOWNLOAD_FTP_FILE*\n");
		 fclose(FPtr);
		   	close(socket_fd);
		    close(data_fd);
		 return -1;
	}
	ret=recv(socket_fd, buf, BUF_SIZE, 0);
		if(ret<0)
		{
			 printf("Recv Fail *DOWNLOAD_FTP_FILE*\n");
			 fclose(FPtr);
			   	close(socket_fd);
			    close(data_fd);
			 return -1;
		}
		buf[ret]='\0';
		printf("ftp_client14:%s\n",buf);

	while(DataSize < FileSize)
	{
		printf("download_test\n");
		if(-1 == (recbytes = read(data_fd,buf,2048)))
		{
			printf("read data fail ***\r\n");
			fclose(FPtr);
		   	close(socket_fd);
		    close(data_fd);
			return -1;
		}
		printf("recbytes:%d\n",recbytes);
		if(recbytes == 0 && DataSize < FileSize)
		{
			printf("FILE IAP FAILED\n");
			fclose(FPtr);
		   	close(socket_fd);
		    close(data_fd);
			return -1;
		}
		DataSize += recbytes;
		fwrite(buf,recbytes,1,FPtr);			//wirte data to file
		if (Percent != DataSize*100/FileSize)
		{
			Percent = DataSize*100/FileSize;
			send_file_signel((uint8_t*)send_sigal_data, 1,tag_addr,Percent);
			//Percent:为当前进度
			printf("Recv: %d%% RecvSize: %d\r\n",Percent,DataSize);
		}
	}

	printf("FINISH FILE IAP!!!\n");
	fclose(FPtr);
	close(socket_fd);
	close(data_fd);
    return 0;
}

/**
 * function     :ftp_connect
 * description  :获取PASV模式 端口
 * input        :
 * output       :none
 * return       :0-ok 非0-err
 * others       :none
 */
static int get_data_port(char *buf)
{
    char			temp[CMD_LEN];
    char			*pc_1 = NULL;
    char			*pc_2 = NULL;
    int	         	        port_low = 0;
    int 			port_high = 0;
    int				port = 0;
    int				count = 0;

//IP & PORT: (192,168,2,89,71,98) 	"192,168,2,89"--IP, "71"--port_high, "98"--port_low
    pc_1 = strstr(buf, "(");
    while(1)
    {
		if(*pc_1 == ',')
		{
			count++;
		}
		pc_1++;
		if(count == 4)
		{
				pc_2 = pc_1;
			break;
		}
    }

    //pc_1 point at '7', pc_2 point at ',', temp == "71"
    pc_2 = strstr(pc_1, ",");
    strncpy(temp, pc_1, pc_2-pc_1);
    port_high = atoi(temp);
    memset(temp, 0, sizeof(temp));
    //pc_1 point at '9', pc_2 point at ')', temp == "98"
    pc_1 = ++pc_2;
    pc_2 = strstr(pc_1, ")");
    strncpy(temp, pc_1, pc_2-pc_1);
    port_low = atoi(temp);
    //port == port_high*256 + port_low
    port = port_high*256 + port_low;

    return port;
}



/**
 * function     :iap_recv_thread_entry
 * description  :接收IAP单播消息
 * input        :none
 * output       :none
 * return       :0-ok 非0-err
 * others       :none
 */
 void iap_recv_thread_entry()
{
    struct sockaddr_in client_addr;
    struct sockaddr_in my_addr;
    socklen_t len = sizeof(client_addr);
    char iap_recv_buff[1024];
    char iap_recv_ip[15];
    uint8_t servic_eth_ip[4];
    int servic_eth_port;
    sleep(3);
    iap_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

//    servic_eth_ip[0]=pw_clb_config_para->ptu_net_para.self_ip[0];
//    servic_eth_ip[1]=pw_clb_config_para->ptu_net_para.self_ip[1];
//    servic_eth_ip[2]=pw_clb_config_para->ptu_net_para.self_ip[2];
//    servic_eth_ip[3]=pw_clb_config_para->ptu_net_para.self_ip[3];
//    servic_eth_port=pw_clb_config_para->ptu_net_para.net_port;

    sprintf(iap_recv_ip,"%d.%d.%d.%d",servic_eth_ip[0],servic_eth_ip[1],servic_eth_ip[2],servic_eth_ip[3]);

    /**socket复用 */
    int reuse = 1; //必须赋值为非零常数
    if (setsockopt(iap_sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)				//端口复用
    {
        perror("reusing socket failed");
        return;
    }

    /**绑定本地地址 */
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(servic_eth_port);
    my_addr.sin_addr.s_addr = inet_addr(iap_recv_ip); //htonl(INADDR_ANY); //本地任意地址 inet_addr("192.168.40.100");
    int err = bind(iap_sock_fd, (struct sockaddr *)&my_addr, sizeof(my_addr));
    if (err != 0)
    {
        perror("bind");
    }
    printf("##init ftp checktable##\n");
    GetCrc16table();

    while (1)
    {
        int r = recvfrom(iap_sock_fd, iap_recv_buff, sizeof(iap_recv_buff) - 1, 0, (struct sockaddr *)&client_addr, &len);

        if (r > 0)
        {
        	iap_data_deal((uint8_t*)iap_recv_buff, r, client_addr);
        }
    }
}



 /*************************************************
  Function:    init_net_thread
  Description: 初始化PTU网络线程
  Input:
  Output:
  Return:
  Others:
  *************************************************/
int init_iap_recv_thread()
 {
 	pthread_t iap_recv_thread_id;
 	int ret;
 	sleep(1);
 	//ptu_data.ptu_net_fd = -1;
    //pthread_mutex_init(&tcp_send_mutex, NULL);
 	ret = pthread_create(&iap_recv_thread_id, NULL, (void *) iap_recv_thread_entry,
 			NULL);
 	if (ret != 0)
 	{
 		DEBUG("net save thread error!\n");
 		return ret;
 	}
 	return 0;
 }


/*************************************************
Function:    iap_data_deal
Description: iap数据解析
Input:　被解析数据:data
	   数据长度:len
Output:
Return:
Others:
*************************************************/
static void iap_data_deal(uint8_t *data,int len,struct sockaddr_in ip_addr)
{

	static uint8_t ftp_iap_flag=0;
	char ip[64];

	uint16_t poart_t;
	uint8_t result;
	//uint16_t test_cnt = 0;
	int ret;
	strcpy(ip,inet_ntoa(ip_addr.sin_addr));
	poart_t=ntohs(ip_addr.sin_port);
	printf("recv from addr=%s poart=%d\n",ip,poart_t);

	if(data[4]!=SELF_DEV)
		return;
//	printf("recv ftp service data:");
//	for(test_cnt = 0;test_cnt < len;test_cnt ++)
//	{
//		printf("%x,",data[test_cnt]);
//	}
//	printf("\n");
//	printf("11111111111111 len:%d h:%x low:%x \n",len,data[9],data[10]);
	if((data[9]*256+data[10])==len)
	{
		uint16_t net_sum = ntohs(*(uint16_t *)&data[len - 2]);
		uint16_t host_sum=GetCrc16(data,len-2);
//		printf("net_sum:%x,host_sum:%x \n",net_sum,host_sum);
		if((host_sum == net_sum)&&(*(uint16_t*)(&data[0])==0x66cc))
		{
//			printf("222222222222222\n");
			switch(data[11])
			{
				case START_CMD:
					if(ftp_iap_flag==0)
					{
//						printf("333333333333\n");
						/*1 ack start*/
						ftp_iap_flag=0xff;
						if(data[12]==0x1)  //data[12]的bit0 为1 时表示需要对方应答
						{
							//printf("----->data[12]<-----:%d\n",data[12]);
							start_iap_ack(data,len,ip_addr);//ack
						}
						memset(send_sigal_data,0,sizeof(send_sigal_data));
						memmove(send_sigal_data,data,len);
						/*2 FTP  download*/
						result=0;			//最开始,进度为0
//						send_file_signel(data, len,ip_addr,result);
						send_file_signel((uint8_t*)send_sigal_data, len,ip_addr,result);
						ret=ftp_connect(ip_addr);
						if(ret<0)
						{
							result=0xff;
							send_file_signel((uint8_t*)send_sigal_data, len,ip_addr,result);
						}
						else
							result=100;
						ftp_iap_flag=result;

						/*3 send flie signel*/
//						send_file_signel(data, len,ip_addr,result);

						if(ftp_iap_flag==100)
						{
							system(CP_IAP_TO_TAGPATH);
//							sprintf(log_status,"ok");
//							sprintf(log_kind,"upgrade_program");
//							sprintf(log_detail,"upgrade program finish");
//							//sprintf(log_name,"%s%s",LOCAL_LOG_DIR,LOCAL_LOG_FILE_NAME);
//							write_log(log_status,log_kind,log_detail,log_file_name);									//对时成功,记录上电时间
							sleep(1);
							system("reboot -nf");
						}
						else
						{
//							sprintf(log_status,"err");
//							sprintf(log_kind,"upgrade_program");
//							sprintf(log_detail,"upgrade program error");
//							write_log(log_status,log_kind,log_detail,log_file_name);
							printf("FTP IAP ERR:%x\n",ftp_iap_flag);
							ftp_iap_flag=0;
						}
					}

					break;

				case SEND_CMD:
					break;

				case END_CMD:
					break;

				case RESEND_CMD:
					break;

				case FINISH_CMD:
					break;

				case FILE_SIGLE:

					if((data[12]==0))
					{
						printf("recv WTD stop file single");
						/*子部件收到WTD响应报文后停止发送心跳报文*/
					}
					break;

				default:
					break;
			}

		}
	}

}


/*************************************************
Function:    GetCrc16table
Description: 初始化CRC表
Input:
Output:
Return:
Others:
*************************************************/
static void GetCrc16table()
{
	 static const unsigned short sm_uscPolynomial = 0x1021;
	 unsigned short usValue = 0;
	 int i = 0;
	 int j = 0;
	 for( i = 0; i < 256; ++i)
	 {
		   usValue = i;
		   for( j = 0; j < 8; ++j)
		   {
				if((usValue ^ (i << (8 + j))) & 0x8000)
				{
					usValue = (usValue << 1) ^ sm_uscPolynomial;
				}
				else
				{
					usValue <<= 1;
				}
		   }
		   g_Crc16Table[i] = usValue;
	 }
}


/*************************************************
Function:    GetCrc16
Description: 计算CRC值
Input:　被解析数据:data
	   数据长度:len
Output:
Return:
Others:*注：CRC校验使用CRC16产生式：G(X)=X16+X12+X5+1，初始值0x2357*
*************************************************/
static unsigned short GetCrc16(unsigned char* ucpData, unsigned long nLength)
{
	 unsigned short usCheck = 0x2357;    //初始值
	 unsigned long i = 0;
	 for(i = 0; i < nLength; ++i)
	 {
		 usCheck = (usCheck << 8) ^ g_Crc16Table[(usCheck >> 8) ^ ucpData[i]];
	 }
	 return usCheck;
}

/*************************************************
Function:    start_iap_ack
Description: 应当WTD开始升级指令
Input:　被解析数据:data
	   数据长度:len
	   目标地址:_addr
Output:
Return:
Others:
*************************************************/
static void start_iap_ack(uint8_t*send_data,int lenth,struct sockaddr_in _addr)
{
	uint8_t net_data[120];
	memmove(net_data,send_data,lenth);
	net_data[3]=send_data[4];
	net_data[4]=send_data[3];
	net_data[12]=0x0;// no ack
	net_data[13]=0x1;//FTP 0x1 eth 0x2

	uint16_t send_crc=GetCrc16(net_data,lenth-2);
	net_data[lenth-2]=(uint8_t)(send_crc>>8);
	net_data[lenth-1]=(uint8_t)send_crc;
	sendto(iap_sock_fd, net_data, lenth, 0,(struct sockaddr*)( &_addr), sizeof(_addr));
}




/*************************************************
Function:    send_file_signel
Description:发送file心跳包信号
Input:　被解析数据:data
	   数据长度:len
	   目标地址:_addr
Output:
Return:
Others:
*************************************************/
static void send_file_signel(uint8_t*send_data,int lenth,struct sockaddr_in _addr,uint8_t flag)
{
	uint8_t net_data[30];
	memmove(net_data,send_data,26);
	net_data[3]=send_data[4];
	net_data[4]=send_data[3];
	net_data[9]=0x0;//
	net_data[10]=30;//len
	net_data[11]=0x7;// 文件传输心跳报文
	net_data[12]=0x1;//  ack
	net_data[13]=0x1;//FTP 0x1 eth 0x2
	net_data[26]=flag;//0-100表示所有文件传输完成且成功；0xFF表示子部件FTP取目标码文件失败，本次升级任务失败。
	//net_data[26]可表示进度
	if(flag==0xff)
		net_data[27]=0x2;//1：升级成功，2：升级失败，3：正在获取目标码文件
	else if(flag==100)
		net_data[27]=0x1;		//升级成功
	else
		net_data[27]=0x3;		//正在获取目标码

	uint16_t send_crc=GetCrc16(net_data,28);
	net_data[28]=(uint8_t)(send_crc>>8);
	net_data[29]=(uint8_t)send_crc;
	sendto(iap_sock_fd, net_data, 30, 0,(struct sockaddr*)( &_addr), sizeof(_addr));
}

/*
void send_file_signel_new(struct sockaddr_in _addr,uint8_t flag)
{
	uint8_t net_data[30];
	//memmove(net_data,send_data,26);
	net_data[0] = 0xcc;
	net_data[1] = 0x66;
	net_data[3]=send_data[4];
	net_data[4]=send_data[3];
	net_data[9]=0x0;//
	net_data[10]=30;//len
	net_data[11]=0x7;// 文件传输心跳报文
	net_data[12]=0x1;//  ack
	net_data[13]=0x1;//FTP 0x1 eth 0x2
	net_data[26]=flag;//0-100表示所有文件传输完成且成功；0xFF表示子部件FTP取目标码文件失败，本次升级任务失败。
	//net_data[26]可表示进度
	if(flag==0xff)
		net_data[27]=0x2;//1：升级成功，2：升级失败，3：正在获取目标码文件
	else if(flag==100)
		net_data[27]=0x1;		//升级成功
	else
		net_data[27]=0x3;		//正在获取目标码

	uint16_t send_crc=GetCrc16(net_data,28);
	net_data[28]=(uint8_t)(send_crc>>8);
	net_data[29]=(uint8_t)send_crc;
	sendto(iap_sock_fd, net_data, 30, 0,(struct sockaddr*)( &_addr), sizeof(_addr));
}
*/



