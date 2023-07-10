
/*

*broadcast_client.c - 多播的服务器

*/

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <unistd.h>

#include <sys/types.h>

#include <sys/socket.h>

#include <arpa/inet.h>

#include <netinet/in.h>

#define MCAST_PORT 2710

#define MCAST_ADDR "239.255.17.10" /*一个局部连接多播地址，路由器不进行转发*/

#define MCAST_INTERVAL 5 /*发送间隔时间*/

#define BUFF_SIZE 256 /*接收缓冲区大小*/

//int main(int argc, char *argv[])
//{
//
//    int s; /*套接字文件描述符*/
//
//    struct sockaddr_in local_addr; /*本地地址*/
//
//    int err = -1;
//
//    s = socket(AF_INET, SOCK_DGRAM, 0); /*建立套接字*/
//
//    if (s == -1)
//
//    {
//
//        perror("socket()");
//
//        return -1;
//    }
//
//    /**socket复用 */
//    int reuse=1;
//    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
//    {
//        perror("Reusing ADDR failed");
//        exit(1);
//    }
//
//    /*初始化地址*/
//
//    memset(&local_addr, 0, sizeof(local_addr));
//
//    local_addr.sin_family = AF_INET;
//
//    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//
//    local_addr.sin_port = htons(MCAST_PORT);
//
//    /*绑定socket*/
//    err = bind(s, (struct sockaddr *)&local_addr, sizeof(local_addr));
//    if (err < 0)
//    {
//        perror("bind()");
//        return -2;
//    }
//
//    /*设置回环许可:当接收者加入到一个多播组以后，再向这个多播组发送数据，这个字段的设置是否允许再返回到本身*/
//    int loop = 0;
//    err = setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
//    if (err < 0)
//    {
//        perror("setsockopt():IP_MULTICAST_LOOP");
//        return -3;
//    }
//
//    /**默认情况下，多播报文的ＴＴＬ被设置成了１，也就是说到这个报文在网络传送的时候，它只能在自己所在的网络传送，当要向外发送的时候，路由器把ＴＴＬ减１以后变成了０，这个报文就已经被Discard了*/
//    unsigned char ttl = 1;
//    err = setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
//    if (err < 0)
//    {
//        perror("setsockopt():IP_MULTICAST_TTL");
//        return -4;
//    }
//
//    /**参数addr是希望多播输出接口的IP地址，使用INADDR_ANY地址回送到默认接口。 */
//    struct in_addr addr;
//    addr.s_addr = htonl(INADDR_ANY);
//    setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, &addr, sizeof(addr));
//
//    /*将本机加入多播组*/
//    struct ip_mreq mreq;                               /*加入多播组*/
//    mreq.imr_multiaddr.s_addr = inet_addr(MCAST_ADDR); /*多播地址*/
//    mreq.imr_interface.s_addr = htonl(INADDR_ANY);     /*网络接口为默认*/
//    err = setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
//    if (err < 0)
//    {
//        perror("setsockopt():IP_ADD_MEMBERSHIP");
//        return -4;
//    }
//
//    int addr_len = 0;
//
//    char buff[BUFF_SIZE];
//
//    int n = 0;
//
//    while (1)
//    {
//        addr_len = sizeof(local_addr);
//
//        memset(buff, 0, BUFF_SIZE); /*清空接收缓冲区*/
//
//        /*接收数据*/
//
//        n = recvfrom(s, buff, BUFF_SIZE, 0, (struct sockaddr *)&local_addr,
//
//                     &addr_len);
//
//        if (n == -1)
//
//        {
//
//            perror("recvfrom()");
//        }
//
//        /*打印信息*/
//        system("date");
//        printf("Recv message from server:%s\n", buff);
//    }
//
//    /*退出多播组*/
//
//    err = setsockopt(s, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof
//
//                     (mreq));
//
//    close(s);
//
//    return 0;
//}
