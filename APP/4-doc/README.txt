CAN设备测试命令：
1、设置CAN的波特率： ip link set can0 type can bitrate 50000
2、设置CAN本地回环模式（测试自发自收）:ip link set can0 type can bitrate 50000 loopback on
3、使能CAN设备：ip link set can0 up　　或者　ifconfig cano up
4、一次发送8个字节:cansend can0 0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88　