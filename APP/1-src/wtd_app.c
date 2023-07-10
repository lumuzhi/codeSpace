#include "wtd_app.h"

#ifdef WTD_DATA_TRANSLATE_PROTOCOL
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>

//#include "udp_client.h"
#include "user_data.h"
#include "self_queue.h"
//#include "pthread_policy.h"

sem_t send_wtd_sem;//允许发送wtd数据信号量

struct sockaddr_in save_ip_sockaddr;
struct sockaddr_in tz_wtd_ip_sockaddr;
struct sockaddr_in alarm_wtd_ip_sockaddr;
int send_wtd_fd_socket=-1;

extern struct WTD_PW_TZ_DATA wtd_pw_tz_data;
extern struct WTD_PW_ALARM_DATA wtd_pw_alarm_data;
extern struct PW_TZ_DATA pw_tz_data;
extern uint8_t first_diag_result_flag;
#ifndef WTD_DATA_PROTOCOL_20220822
extern struct LESS_HEAD_INFO head_info;
extern struct TZ_VALUE_DATA save_tz_value_data[10];
#endif
extern LiQueue *alarm_data_queue[CHANNEL_NUM];
extern struct DEQUEUE_DATA dequeue_org_data[CHANNEL_NUM];
extern struct MSG_ALARM_TRIG msg_alarm_trigger[CHANNEL_NUM];
extern struct MESSAGE_COUNT_CTRL alarm_ctrl[CHANNEL_NUM];
extern struct RECV_PUBLIC_PARA recv_public_para;

#ifdef WTD_SIMULATION_ALARM_TRIG
	extern uint8_t simulation_open_flag;
	extern uint8_t simulation_status[CHANNEL_NUM];
#endif

//save
static LiQueue *qt[CHANNEL_NUM]={NULL, NULL, NULL, NULL, NULL, NULL};//指针临时接管前一队
static LiQueue *qn[CHANNEL_NUM]={NULL, NULL, NULL, NULL, NULL, NULL};//解除后前一队未出队发送，建新队，接到前一队尾部
uint32_t save_trig_order = 0;//解除报警时从1开始保存到队列//alarm_remove_trig_order
uint8_t alarm_remove_cnt[CHANNEL_NUM] = {0,0,0,0,0,0};//解除30s(30次)计数

//send
uint32_t send_trig_order = 1;//从1开始发送//alarm_remove_trig_order
uint8_t save_ch_num = 0;//保存当前发送通道
#ifndef WTD_DATA_PROTOCOL_20220822
uint8_t resend_cnt[CHANNEL_NUM] = {0,0,0,0,0,0};//最后一包重发10次（10s）计数
#endif

extern uint8_t first_diag_result_flag;//第一个算法结果产生

void reset_wtd_trig_alarm_ctrl();
//	void save_msg_alarm_data(uint8_t ch_num, struct PW_RAW_DATA *buf, uint16_t size);
void save_msg_alarm_data(uint8_t ch_num, int8_t *buf, uint16_t size);
uint8_t get_msg_alarm_data(uint8_t ch_num, int8_t *buf, uint16_t size);
void send_save_wtd_pw_tz_data();
void send_save_wtd_pw_alarm_data();

extern uint16_t little_to_big_16bit(uint16_t value);
extern void reset_wtd_pw_alarm_data();

void reset_wtd_trig_alarm_ctrl()
{
	simulation_open_flag = 0;
	first_diag_result_flag = 0;
	memset(simulation_status, TRIG_OK, sizeof(simulation_status));

	save_trig_order = 0;
	send_trig_order = 1;
	memset(alarm_remove_cnt, 0, sizeof(alarm_remove_cnt));
	save_ch_num = 0;

	reset_wtd_pw_alarm_data();
}

//void save_msg_alarm_data(uint8_t ch_num, struct PW_RAW_DATA *buf, uint16_t size)
void save_msg_alarm_data(uint8_t ch_num, int8_t *buf, uint16_t size)
{
	if(alarm_data_queue[ch_num] != NULL)//alarm_data_queue[ch_num]为主队:  主队(前一队)-新队．．．
	{
		/*1.首次入队*/
		if(alarm_data_queue[ch_num]->all_time_alarm_num_limit == 0)
		{
			qt[ch_num] = alarm_data_queue[ch_num];
		}

		/*7.等待该通道所有数据发送完后,再次入队*/
		if(msg_alarm_trigger[ch_num].alarm_trig_status == TRIG_ALARM_WAIT)
		{
			if(alarm_data_queue[ch_num]->all_time_alarm_num_limit == 0)//超限时等待发完
			{
				msg_alarm_trigger[ch_num].alarm_trig_status = TRIG_OK;
				msg_alarm_trigger[ch_num].alarm_tz_type = NO_ALARM_TYPE;
			}
			else
			{
				return;
			}
		}


	#ifdef WTD_SIMULATION_ALARM_TRIG
		/*7.模拟部分：等待该通道所有数据发送完后,再次入队*/
		if(simulation_open_flag)
		{
			if(simulation_status[ch_num] == TRIG_ALARM_WAIT)
			{
				if(alarm_data_queue[ch_num]->all_time_alarm_num_limit == 0)//超限时等待发完
				{
					simulation_status[ch_num] = TRIG_OK;
					msg_alarm_trigger[ch_num].alarm_trig_status = TRIG_OK;
					msg_alarm_trigger[ch_num].alarm_tz_type = NO_ALARM_TYPE;
				}
				else
				{
					return;
				}
			}
		}
	#endif

	#ifdef __WTD_DEBUG__
		WTD_DEBUG("alarm_data_queue[%d]->all_time_alarm_num_limit====%d\n", ch_num, alarm_data_queue[ch_num]->all_time_alarm_num_limit);
		if(alarm_data_queue[ch_num]->next)
		{
			WTD_DEBUG("alarm_data_queue[%d]->next->one_time_alarm_num_total====%d\n", ch_num, alarm_data_queue[ch_num]->next->one_time_alarm_num_total);
			WTD_DEBUG("alarm_data_queue[%d]->next->one_time_alarm_num====%d\n", ch_num, alarm_data_queue[ch_num]->next->one_time_alarm_num);
		}
	#endif


		if(alarm_data_queue[ch_num]->all_time_alarm_num_limit < QUEUE_QNODE_LIMIT_NUM)
		{
			/*4.2.报警解除后，保证持续存30s*/
			if(alarm_remove_cnt[ch_num]>0 && msg_alarm_trigger[ch_num].alarm_trig_status != TRIG_ALARM_REMOVE)
			{
				msg_alarm_trigger[ch_num].alarm_trig_status = TRIG_ALARM_REMOVE;//直到解除后30s结束，才恢复TRIG_OK
			}

			/*2.未触发报警(正常运行)*/
			if(msg_alarm_trigger[ch_num].alarm_trig_status == TRIG_OK)//算法诊断未触发报警只缓存前30s　6通道的数据．
			{
				if(qt[ch_num]->one_time_alarm_num >= QUEUE_CACHE_BEFORE_ALARM_NUM)
				{
					WTD_DEBUG("\r\nDeQueue_DelOneQNode---overleap 30s\r\n");
					DeQueue_DelOneQNode(qt[ch_num]);

					alarm_data_queue[ch_num]->all_time_alarm_num_limit--;
				}
			}
			else if(msg_alarm_trigger[ch_num].alarm_trig_status == TRIG_ALARM_REMOVE)/*4.报警解除*/
			{
				/*4.1.报警解除,计触发次序*/
				if(alarm_remove_cnt[ch_num] == 0)//报警刚解除,就算报警数据总条数(即总节点数)＝当前入队条数＋解除后30秒入队条数（30条）,
				{                                //每次报警出队时算总包数＝每次报警总条数／MSG_ALARM_DATA_SIZE＋(每次报警总条数％MSG_ALARM_DATA_SIZE>0?1:0)，
					qt[ch_num]->one_time_alarm_num_total = qt[ch_num]->one_time_alarm_num+QUEUE_CACHE_AFTER_ALARM_NUM;

					save_trig_order++;
					if(save_trig_order==0)//避免翻转为0时，几个通道发送次序仍为0（即从未触发过报警）
					{
						save_trig_order = 1;
					}

					qt[ch_num]->send_order = save_trig_order;

					WTD_DEBUG("save_msg_alarm_data---ch:%d, save_trig_order:%d\n", ch_num, save_trig_order);
				}

				alarm_remove_cnt[ch_num]++;

				/*4.3.报警解除后，持续存30s*/
				if(alarm_remove_cnt[ch_num] > QUEUE_CACHE_AFTER_ALARM_NUM)//报警解除后30s数据（无论正常还是再次报警）完成状态更新,以便出队发送时检查时判断是否出队完成．
				{
					if(msg_alarm_trigger[ch_num].alarm_trig_status == TRIG_ALARM_REMOVE)
					{
						msg_alarm_trigger[ch_num].alarm_valid_flag = ALARM_INVALID;

						msg_alarm_trigger[ch_num].alarm_trig_status = TRIG_OK;//放此处，以便新队入队

						msg_alarm_trigger[ch_num].alarm_tz_type = NO_ALARM_TYPE;

					#ifdef WTD_SIMULATION_ALARM_TRIG
						if(simulation_open_flag)
						{
							simulation_status[ch_num] = TRIG_OK;//放此处，以便新队入队
						}
					#endif
					}

					/*该通道，再建新队(即下次触发报警的队)，接到前一队后面*/
					qn[ch_num] = QueueInit();
					Queue_Append_NewQueue(qt[ch_num], qn[ch_num]);

					qt[ch_num] = qn[ch_num];//切换新队,为入队准备

//					WTD_DEBUG("save_msg_alarm_data--qn[%d]:%x\n", ch_num, qn[ch_num]);

					alarm_remove_cnt[ch_num] = 0;
				}
			}
//			else if(msg_alarm_trigger[ch_num].alarm_trig_status == TRIG_ALARM)/*3.触发报警－－－报警中*/
//			{
//				msg_alarm_trigger[ch_num].alarm_valid_flag = ALARM_VALID;//已经在诊断里修改为此状态，此处不用再赋值
//			}

		#ifdef WTD_DATA_PROTOCOL_20220822
			/*入队:未触发报警(正常运行)(0到30s不等)---解除后30s结束*/
			if(msg_alarm_trigger[ch_num].alarm_trig_status == TRIG_OK || msg_alarm_trigger[ch_num].alarm_trig_status == TRIG_ALARM_REMOVE)
		#endif
			{
				/*入队:未触发报警(正常运行)(0到30s不等)－－－触发报警－－－报警中－－－报警解除---解除后30s结束*/
				EnQueue(qt[ch_num], (int8_t *)buf, size);

				/*由主队总计数,来限制各通道内存使用量*/
				alarm_data_queue[ch_num]->all_time_alarm_num_limit++;
			}

		}
		else//报警数据超最大限制后不再缓存,直到解除,再缓存解除后30秒数据
		{
			/*6.3.报警解除后，保证持续存30s*/
			if(alarm_remove_cnt[ch_num]>0 && msg_alarm_trigger[ch_num].alarm_trig_status != TRIG_ALARM_REMOVE)
			{
				msg_alarm_trigger[ch_num].alarm_trig_status = TRIG_ALARM_REMOVE;//直到解除后30s结束，才恢复TRIG_OK
			}

			if(msg_alarm_trigger[ch_num].alarm_trig_status == TRIG_ALARM_REMOVE)/*6.报警解除*/
			{
				/*6.1.报警解除,计触发次序*/
				if(alarm_remove_cnt[ch_num] == 0)//报警刚解除,就算报警数据总条数(alarm_num)＝当前入队条数＋解除后30秒入队条数（30条）,
				{                                //出队时算总包数＝总条数／MSG_ALARM_DATA_SIZE＋(总条数％MSG_ALARM_DATA_SIZE>0?1:0)，
					qt[ch_num]->one_time_alarm_num_total = qt[ch_num]->one_time_alarm_num+QUEUE_CACHE_AFTER_ALARM_NUM;

					save_trig_order++;
					if(save_trig_order==0)//避免翻转为0时，几个通道发送次序仍为0（即从未触发过报警）
					{
						save_trig_order = 1;
					}

					qt[ch_num]->send_order = save_trig_order;

					WTD_DEBUG("save_msg_alarm_data---ch:%d, save_trig_order:%d\n", ch_num, save_trig_order);
				}

				alarm_remove_cnt[ch_num]++;

				/*6.4.报警解除后，持续存30s*/
				if(alarm_remove_cnt[ch_num] > QUEUE_CACHE_AFTER_ALARM_NUM)//报警解除后30s数据（无论正常还是再次报警）完成状态更新,以便出队发送时检查时判断是否出队完成．
				{
					if(msg_alarm_trigger[ch_num].alarm_trig_status == TRIG_ALARM_REMOVE)
					{
						msg_alarm_trigger[ch_num].alarm_valid_flag = ALARM_INVALID;

						msg_alarm_trigger[ch_num].alarm_trig_status = TRIG_ALARM_WAIT;//放此处，以便新队入队

					#ifdef WTD_SIMULATION_ALARM_TRIG
						if(simulation_open_flag)
						{
							simulation_status[ch_num] = TRIG_ALARM_WAIT;//放此处，以便新队入队
						}
					#endif
					}
				}
				else if(alarm_remove_cnt[ch_num]>0)/*6.2.报警解除后,入队存数*/
				{
					/*入队*/
					EnQueue(qt[ch_num], (int8_t *)buf, size);//入队解除后30s数据

					/*由主队总计数,来限制各通道内存使用量*/
					alarm_data_queue[ch_num]->all_time_alarm_num_limit++;
				}
			}
			else if(msg_alarm_trigger[ch_num].alarm_trig_status == TRIG_ALARM)/*5.报警中，超限不入队*/
			{
//				msg_alarm_trigger[ch_num].alarm_valid_flag = ALARM_VALID;//已经在诊断里修改为此状态，此处不用再赋值
				;
				WTD_DEBUG("\r\nDon't EnQueue---overleap 30min\r\n");
			}
		}//end if overleap 30min
	}//end if alarm_data_queue[ch_num] != NULL
}

/*从alarm_data_queue[ad_ch_num]的出队一秒数据中取size个字节给该包buf[size]*/
uint8_t get_msg_alarm_data(uint8_t ad_ch_num, int8_t *buf, uint16_t size)
{
	uint8_t data_valid_flag = 1;

	if(dequeue_org_data[ad_ch_num].deq_buf_rear_len < size)//上次剩下的长度,也是上次剩下放save_buf中的    1.当rear_len<size时，该包buf[0~size] <=== save_buf[0~(rear_len-1)]+deq_buf[0~(size-rear_len-1)]
	{
		if(!QueueIsEmpty(alarm_data_queue[ad_ch_num]))//队非空
		{
			if(dequeue_org_data[ad_ch_num].deq_buf_rear_len > 0)
			{
				memmove(buf, dequeue_org_data[ad_ch_num].deq_buf, dequeue_org_data[ad_ch_num].deq_buf_rear_len);//当rear_len>0时，该包buf[0~(rear_len-1)] <=== deq_buf[0~(rear_len-1)]
				memset(dequeue_org_data[ad_ch_num].deq_buf, 0, dequeue_org_data[ad_ch_num].deq_buf_rear_len);

				if(alarm_ctrl[ad_ch_num].packet_num_cnt+1 == alarm_ctrl[ad_ch_num].packet_total)//防止最后一包还误出队
				{
					dequeue_org_data[ad_ch_num].deq_buf_rear_len = 0;
					dequeue_org_data[ad_ch_num].deq_buf_front_len = 0;
					data_valid_flag = 1;

					WTD_DEBUG("DeQueue!=NULL, Last package rear_len>0, DeQueue Over!\n");

					return data_valid_flag;
				}
			}
			else
			{
				//dequeue_org_data[ad_ch_num].deq_buf_rear_len = 0;

				if(alarm_ctrl[ad_ch_num].packet_num_cnt+1 == alarm_ctrl[ad_ch_num].packet_total)//防止最后一包还误出队
				{
					dequeue_org_data[ad_ch_num].deq_buf_rear_len = 0;
					dequeue_org_data[ad_ch_num].deq_buf_front_len = 0;
					data_valid_flag = 0;//rear_len=0时，不发送

					WTD_DEBUG("DeQueue!=NULL, Last package rear_len=0, DeQueue Over!\n");

					return data_valid_flag;
				}
			}

		#ifdef WTD_DATA_TEST
			printf("alarm_data_queue[%d]:\n", ad_ch_num);
			uint16_t i=0;
			for(i=0; i<SAMPLE_HZ; i++)
			{
				if(i%20==0)
					printf("\n");
				printf(" %d", ntohs(*((int16_t*)alarm_data_queue[ad_ch_num]->front->data+i)));
			}
			printf("\n");
		#endif

			//出队1s数据
			DeQueue(alarm_data_queue[ad_ch_num], dequeue_org_data[ad_ch_num].deq_buf, dequeue_org_data[ad_ch_num].deq_buf_len);

			/*由主队总计数,来限制各通道内存使用量*/
			alarm_data_queue[ad_ch_num]->all_time_alarm_num_limit--;

			dequeue_org_data[ad_ch_num].deq_buf_front_len = size-dequeue_org_data[ad_ch_num].deq_buf_rear_len;//该包从刚出除一秒数据中的待取长度
			memmove(buf+dequeue_org_data[ad_ch_num].deq_buf_rear_len, dequeue_org_data[ad_ch_num].deq_buf, dequeue_org_data[ad_ch_num].deq_buf_front_len);//该包buf[rear_len~(size-1)] <=== deq_buf[0~(front_len-1)]

			//剩下保存备下次用
			dequeue_org_data[ad_ch_num].deq_buf_rear_len = dequeue_org_data[ad_ch_num].deq_buf_len - dequeue_org_data[ad_ch_num].deq_buf_front_len;
			memmove(dequeue_org_data[ad_ch_num].deq_buf, dequeue_org_data[ad_ch_num].deq_buf+dequeue_org_data[ad_ch_num].deq_buf_front_len, dequeue_org_data[ad_ch_num].deq_buf_rear_len);

			data_valid_flag = 1;

			WTD_DEBUG("DeQueue!=NULL, Last package rear_len>=0, DeQueue One Node!\n");
		}
		else//队空时为最后一包，说明已经出队完，save_buf最后一包
		{
			if(dequeue_org_data[ad_ch_num].deq_buf_rear_len > 0)
			{
				if(alarm_ctrl[ad_ch_num].packet_num_cnt+1 == alarm_ctrl[ad_ch_num].packet_total)
				{
					memmove(buf, dequeue_org_data[ad_ch_num].deq_buf, dequeue_org_data[ad_ch_num].deq_buf_rear_len);//当rear_len>0时，该包buf[0~(rear_len-1)] <=== save_buf[0~(rear_len-1)]
					memset(dequeue_org_data[ad_ch_num].deq_buf, 0, dequeue_org_data[ad_ch_num].deq_buf_rear_len);
					dequeue_org_data[ad_ch_num].deq_buf_front_len = 0;
					dequeue_org_data[ad_ch_num].deq_buf_rear_len = 0;
					data_valid_flag = 1;

					WTD_DEBUG("DeQueue==NULL, Last package, 0<rear_len<size, DeQueue Over!\n");
				}
				else//该次报警解除后的报警数据还未入队完,入队后alarm_data_queue[ad_ch_num]非空
				{
					data_valid_flag = 0;//rear_len>0时，不发送

					WTD_DEBUG("DeQueue==NULL, Wait for package, 0<rear_len<size\n");
				}
			}
			else//当rear_len＝0时,存在两种情况：1.已报警数据已发完，实测跑不进来，在前面获取通道时检测到最后一包就禁止发送了；2．该次报警解除后的报警数据还未入队完．
			{
				//dequeue_org_data[ad_ch_num].deq_buf_rear_len = 0;

				data_valid_flag = 0;//rear_len=0时，不发送

				//WTD_DEBUG("DeQueue==NULL, Last package rear_len=0, DeQueue Over!\n");
				WTD_DEBUG("DeQueue==NULL, Wait for package, rear_len=0\n");
			}
		}
	}
	else //dequeue_org_data[ad_ch_num].deq_buf_rear_len >= size   2.save_buf[0~(rear_len-1)], 当rear_len>=size时，该包buf[0~(size-1)] <=== save_buf[0~(size-1)]   rear_len=rear_len-size
	{
		//取　size个字节　给　alarm_data[size]
		memmove(buf, dequeue_org_data[ad_ch_num].deq_buf, size);

		if(dequeue_org_data[ad_ch_num].deq_buf_rear_len > size)
		{
			//剩下保存备下次用
			dequeue_org_data[ad_ch_num].deq_buf_front_len = 0;//此处未用上，清零，直接用size直观
			dequeue_org_data[ad_ch_num].deq_buf_rear_len -= size;
			memset(dequeue_org_data[ad_ch_num].deq_buf, 0, size);//前半部清零
			memmove(dequeue_org_data[ad_ch_num].deq_buf, dequeue_org_data[ad_ch_num].deq_buf+size, dequeue_org_data[ad_ch_num].deq_buf_rear_len);//save_buf[0~(rear_len-1)] <=== save_buf[size~(size+rear_len-1)]
			memset(dequeue_org_data[ad_ch_num].deq_buf+dequeue_org_data[ad_ch_num].deq_buf_rear_len, 0, size);//后半部清零为下次
			data_valid_flag = 1;

			WTD_DEBUG("Not last package rear_len>size, DeQueue doing\n");
		}
		else//rear_len == size
		{
			memset(dequeue_org_data[ad_ch_num].deq_buf, 0, dequeue_org_data[ad_ch_num].deq_buf_len);
			dequeue_org_data[ad_ch_num].deq_buf_front_len = 0;
			dequeue_org_data[ad_ch_num].deq_buf_rear_len = 0;
			data_valid_flag = 1;

			if(alarm_ctrl[ad_ch_num].packet_num_cnt+1 == alarm_ctrl[ad_ch_num].packet_total)//最后一包
			{
				WTD_DEBUG("Last package rear_len=size, DeQueue Over!\n");
			}
			else
			{
				WTD_DEBUG("Not last package rear_len=size, DeQueue doing\n");
			}
		}
	}

	return data_valid_flag;
}

/*按报警解除触发次序发送，直到发完*/
uint8_t get_send_enable_channel()
{
	uint8_t ch_num = 0;
	uint32_t temp_order = 0;
	uint32_t alarm_data_queue_size = 0;

	if(alarm_ctrl[save_ch_num].send_ctrl == ALARM_SEND_ENABLE)//发送中
	{
	#ifdef WTD_DATA_PROTOCOL_20220822
		if(alarm_ctrl[save_ch_num].packet_num_cnt == alarm_ctrl[save_ch_num].packet_total)//发完不再发,计数清零
	#else
		if(alarm_ctrl[save_ch_num].packet_num_cnt+1 == alarm_ctrl[save_ch_num].packet_total)//发完不再发,计数清零
	#endif
		{
		#ifndef WTD_DATA_PROTOCOL_20220822
			if(resend_cnt[save_ch_num] >= ONE_PACKAGE_RESEND_NUM)//1:只发一次，新协议0.5s更新发送---20220822 //10:最后一包10次后禁发---20211208  1s发送10s更新
			{
				resend_cnt[save_ch_num] = 0;
				alarm_ctrl[save_ch_num].update_cnt = 0;
				alarm_ctrl[save_ch_num].not_first_flag = 0;
		#endif
				alarm_ctrl[save_ch_num].packet_num_cnt = 0;
				alarm_ctrl[save_ch_num].packet_total = 0;
				alarm_ctrl[save_ch_num].send_ctrl = ALARM_SEND_DISABLE;
				//WTD_DEBUG("get_send_enable_channel--save_ch_num:%d, ALARM_SEND_DISABLE\n", save_ch_num);
//				#ifdef __WTD_DEBUG__
//					if(save_ch_num==0 && alarm_data_queue[0]->next)
//						WTD_DEBUG("get_send_enable_channel--2-last-over--send_order_addr:%x\n",alarm_data_queue[save_ch_num]->next);
//				#endif
				/*如果后面接得有队,将主队指针切换其后接队,总节点数接管*/
				if(alarm_data_queue[save_ch_num]->next)
				{
					LiQueue *ptr = NULL;
					ptr = alarm_data_queue[save_ch_num]->next;
					ptr->all_time_alarm_num_limit = alarm_data_queue[save_ch_num]->all_time_alarm_num_limit;
					free(alarm_data_queue[save_ch_num]);
					alarm_data_queue[save_ch_num] = ptr;
				}
			#ifdef __WTD_DEBUG__
				if(save_ch_num==0)
				{
					//WTD_DEBUG("get_send_enable_channel--2-last-over--send_order_addr:%x\n",alarm_data_queue[save_ch_num]);
					WTD_DEBUG("get_send_enable_channel--send_order:%d\n",alarm_data_queue[save_ch_num]->send_order);
					WTD_DEBUG("get_send_enable_channel--limit_num:%d\n",alarm_data_queue[save_ch_num]->all_time_alarm_num_limit);
				}
			#endif

				save_ch_num = 0xff;

				return 0xff;
		#ifndef WTD_DATA_PROTOCOL_20220822
			}
			else//最后一包未满10次，继续发
			{
			#ifdef __WTD_DEBUG__
				if(save_ch_num==0)
					WTD_DEBUG("get_send_enable_channel--2-last\n");
			#endif
				return save_ch_num;
			}

			resend_cnt[save_ch_num]++;
		#endif
		}
		else//非最后一包，继续发
		{
		#ifdef __WTD_DEBUG__
			if(save_ch_num==0)
				WTD_DEBUG("get_send_enable_channel--3-nolast\n");
		#endif
			return save_ch_num;
		}
	}

	for(ch_num=0; ch_num<CHANNEL_NUM; ch_num++)
	{
	#ifdef __WTD_DEBUG__
		if(ch_num==0)
			WTD_DEBUG("get_send_enable_channel--0\n");
	#endif
		if(QueueIsEmpty(alarm_data_queue[ch_num]) && alarm_ctrl[ch_num].send_ctrl == ALARM_SEND_DISABLE)//队空禁发, 发送完队空排除(仍可能有余数未发,见出队)
		{
			continue;
		}
	#ifdef __WTD_DEBUG__
		if(ch_num==0)
			WTD_DEBUG("get_send_enable_channel--1\n");
	#endif

		if(alarm_ctrl[ch_num].send_ctrl == ALARM_SEND_DISABLE)//禁发中
		{
			temp_order = alarm_data_queue[ch_num]->send_order;
		#ifdef __WTD_DEBUG__
			//if(ch_num==0)
				WTD_DEBUG("get_send_enable_channel--4--ch:%d, temp_order:%d, send_trig_order:%d\n", ch_num, temp_order, send_trig_order);
		#endif
			if(temp_order == send_trig_order)//触发条件:按触发次序发送
			{
				WTD_DEBUG("get_send_enable_channel---CH:%d, send_trig_order:%d\n", ch_num, send_trig_order);

				alarm_ctrl[ch_num].send_ctrl = ALARM_SEND_ENABLE;

				send_trig_order++;//为下次触发次序
				if(send_trig_order==0)//避免翻转为0时，几个通道发送次序仍为0（即从未触发过报警）
				{
					send_trig_order = 1;
				}

				/*每次报警出队时算总包数＝每次报警总条数／MSG_ALARM_DATA_SIZE＋(每次报警总条数％MSG_ALARM_DATA_SIZE>0?1:0)*/
				alarm_data_queue_size = alarm_data_queue[ch_num]->one_time_alarm_num_total*SAMPLE_HZ*sizeof(int16_t);
				alarm_ctrl[ch_num].packet_total = alarm_data_queue_size/MSG_ALARM_DATA_SIZE + (alarm_data_queue_size%MSG_ALARM_DATA_SIZE>0?1:0);//ceil(alarm_data_queue_size/MSG_ALARM_DATA_SIZE)

				save_ch_num = ch_num;

				return ch_num;
			}
		}
	}

	return 0xff;
}

uint8_t get_alarm_position(uint8_t ch_num)
{
	uint8_t sensor_pos=FAULT_SENSOR_POS_ONE;

	switch(ch_num)
	{
	case pw1_y:
		sensor_pos = FAULT_SENSOR_POS_ONE;
		break;
	case pw1_z:
		sensor_pos = FAULT_SENSOR_POS_FOUR;
		break;
	case pw2_y:
		sensor_pos = FAULT_SENSOR_POS_FIVE;
		break;
	case pw2_z:
		sensor_pos = FAULT_SENSOR_POS_EIGHT;
		break;
	default:
		break;
	}

	return sensor_pos;
}

#ifdef WTD_DATA_PROTOCOL_20220822
void send_save_wtd_pw_tz_data()
{
	static uint16_t packet_num_cnt=0;
#ifndef TCMS_MSG_ADD_ATP_TIME
	struct LOCAL_TIME time_now;
#endif

	//printf("send_wtd_pw_tz_data---packet_num_cnt:%d\n", packet_num_cnt);

	*(uint16_t *)wtd_pw_tz_data.frame_head = htons(0xA5A5);

	wtd_pw_tz_data.carriage_number = pw_tz_data.train_public_info.carriage_number+1;//recv_public_para.board_data.train_public_info.carriage_number;

	wtd_pw_tz_data.message_type.bits.type = 0x01;
	*(uint16_t *)&wtd_pw_tz_data.packet_num = little_to_big_16bit(packet_num_cnt);
	packet_num_cnt++;

#ifdef TCMS_MSG_ADD_ATP_TIME
	memmove(&wtd_pw_tz_data.atp_time.year, &recv_public_para.recv_ctrl_board_data.train_public_info.atp_year, 6);
#else
	get_local_time(&time_now);
	wtd_pw_tz_data.atp_time.year = (uint8_t)(time_now.year-2000);
	wtd_pw_tz_data.atp_time.mon = (uint8_t)time_now.mon;
	wtd_pw_tz_data.atp_time.day = (uint8_t)time_now.day;
	wtd_pw_tz_data.atp_time.hour = (uint8_t)time_now.hour;
	wtd_pw_tz_data.atp_time.min = (uint8_t)time_now.min;
	wtd_pw_tz_data.atp_time.sec = (uint8_t)time_now.sec;
#endif

#if defined(INTERNAL_PROTOCOL_20210416) && !defined(PUB_INFO_REMOVE_KM_FLAG)
	wtd_pw_tz_data.km_flag_h.wh = pw_tz_data.km_flag_hh;//recv_public_para.board_data.train_public_info.km_flag_hh;
	wtd_pw_tz_data.km_flag_h.wl = pw_tz_data.km_flag_hl;//recv_public_para.board_data.train_public_info.km_flag_hl;
	wtd_pw_tz_data.km_flag_l.wh = pw_tz_data.km_flag_lh;//recv_public_para.board_data.train_public_info.km_flag_lh;
	wtd_pw_tz_data.km_flag_l.wl = pw_tz_data.km_flag_ll;//recv_public_para.board_data.train_public_info.km_flag_ll;
#endif

	wtd_pw_tz_data.speed.wh  = pw_tz_data.train_public_info.speed[0];
	wtd_pw_tz_data.speed.wl  = pw_tz_data.train_public_info.speed[1];

	memmove(&wtd_pw_tz_data.gps_pos, &pw_tz_data.train_public_info.gps_data.longitude_down, sizeof(struct WTD_GPS_POS));

#ifdef INTERNAL_PROTOCOL_20210725
	wtd_pw_tz_data.tz_value_data.side1_y_quota.wh = pw_tz_data.side1_y_quota.wh;//已经乘100
	wtd_pw_tz_data.tz_value_data.side1_y_quota.wl = pw_tz_data.side1_y_quota.wl;
	wtd_pw_tz_data.tz_value_data.side2_y_quota.wh = pw_tz_data.side2_y_quota.wh;
	wtd_pw_tz_data.tz_value_data.side2_y_quota.wl = pw_tz_data.side2_y_quota.wl;
	wtd_pw_tz_data.tz_value_data.side1_z_quota.wh = pw_tz_data.side1_z_quota.wh;
	wtd_pw_tz_data.tz_value_data.side1_z_quota.wl = pw_tz_data.side1_z_quota.wl;
	wtd_pw_tz_data.tz_value_data.side2_z_quota.wh = pw_tz_data.side2_z_quota.wh;
	wtd_pw_tz_data.tz_value_data.side2_z_quota.wl = pw_tz_data.side2_z_quota.wl;

	wtd_pw_tz_data.tz_value_data.y_peak_w1.wh = 0x0;
	wtd_pw_tz_data.tz_value_data.y_peak_w1.wl = pw_tz_data.acc_index.y_peak_w1;//已经乘1000
	wtd_pw_tz_data.tz_value_data.y_peak_w2.wh = 0x0;
	wtd_pw_tz_data.tz_value_data.y_peak_w2.wl = pw_tz_data.acc_index.y_peak_w2;
	wtd_pw_tz_data.tz_value_data.y_root_w1.wh = 0x0;
	wtd_pw_tz_data.tz_value_data.y_root_w1.wl = pw_tz_data.acc_index.y_root_w1;
	wtd_pw_tz_data.tz_value_data.y_root_w2.wh = 0x0;
	wtd_pw_tz_data.tz_value_data.y_root_w2.wl = pw_tz_data.acc_index.y_root_w2;
	wtd_pw_tz_data.tz_value_data.z_root_w1.wh = 0x0;
	wtd_pw_tz_data.tz_value_data.z_root_w1.wl = pw_tz_data.acc_index.z_root_w1;
	wtd_pw_tz_data.tz_value_data.z_root_w2.wh = 0x0;
	wtd_pw_tz_data.tz_value_data.z_root_w2.wl = pw_tz_data.acc_index.z_root_w2;
#endif

	memmove(&wtd_pw_tz_data.tcms_time.year, &pw_tz_data.train_public_info.year, 6);

	*(uint16_t *)&wtd_pw_tz_data.part_check_sum = htons(check_sum((uint8_t *)&wtd_pw_tz_data,sizeof(struct WTD_PW_TZ_DATA)-2));

//	printf("send_tz_wtd_IPaddr:%s\n",inet_ntoa(tz_wtd_ip_sockaddr.sin_addr));
//	printf("send_tz_wtd_port:%d\n",ntohs(tz_wtd_ip_sockaddr.sin_port));
//	printf("WTD tz data len:%d\n", sizeof(struct WTD_PW_TZ_DATA));
	sendto(send_wtd_fd_socket, (uint8_t*)&wtd_pw_tz_data, sizeof(struct WTD_PW_TZ_DATA), 0, (struct sockaddr *)&tz_wtd_ip_sockaddr, sizeof(tz_wtd_ip_sockaddr));
}

void send_save_wtd_pw_alarm_data()
{
	uint8_t ch_num = 0;
	uint8_t data_valid_flag = 0;
#ifndef TCMS_MSG_ADD_ATP_TIME
	struct LOCAL_TIME time_now;
#endif

	//printf("send_save_wtd_pw_alarm_data---1\n");

	/*按报警解除触发次序发送，直到发完*/
	ch_num = get_send_enable_channel();

	WTD_DEBUG("send_save_wtd_pw_alarm_data--ch:%d\n", ch_num);

	if(ch_num == 0xff)
		return;

	if(alarm_ctrl[ch_num].send_ctrl == ALARM_SEND_DISABLE)
		return;

	WTD_DEBUG("send_save_wtd_pw_alarm_data--packet_total:%d\n", alarm_ctrl[ch_num].packet_total);
	WTD_DEBUG("send_wtd_pw_alarm_data---packet_num_cnt:%d\n", alarm_ctrl[ch_num].packet_num_cnt);

	//报警数据（报警前30秒　至　报警解除30秒后，至少1分钟数据量, 参考函数save_msg_alarm_data对数据量的控制）
	memset((int8_t*)wtd_pw_alarm_data.alarm_data, 0, MSG_ALARM_DATA_SIZE);
	data_valid_flag = get_msg_alarm_data(ch_num, (int8_t*)wtd_pw_alarm_data.alarm_data, MSG_ALARM_DATA_SIZE);
	if(!data_valid_flag)
	{
		return;
	}

#ifdef WTD_DATA_TEST
	printf("alarm_data[%d]:\n", ch_num);
	uint16_t i=0;
	for(i=0; i<MSG_ALARM_DATA_SIZE/2; i++)
	{
		if(i%20==0)
			printf("\n");
		printf(" %d", ntohs(*((int16_t*)wtd_pw_alarm_data.alarm_data+i)));
	}
	printf("\n");
#endif

	*(uint16_t *)wtd_pw_alarm_data.frame_head = htons(0xA6A6);
	*(uint16_t *)&wtd_pw_alarm_data.message_length = htons(WTD_ALARM_MEMSAGE_LENGTH);
	wtd_pw_alarm_data.message_type.bits.type = 0x02;
	wtd_pw_alarm_data.message_type.bits.fault_sensor_pos = get_alarm_position(ch_num);
	wtd_pw_alarm_data.message_type.bits.fault_data_flag = 0x1;

	*(uint16_t *)&wtd_pw_alarm_data.packet_num = htons(alarm_ctrl[ch_num].packet_num_cnt);
	*(uint16_t *)&wtd_pw_alarm_data.packet_total = htons(alarm_ctrl[ch_num].packet_total);

	alarm_ctrl[ch_num].packet_num_cnt++;


#ifdef TCMS_MSG_ADD_ATP_TIME
	memmove(&wtd_pw_alarm_data.atp_time.year, &recv_public_para.recv_ctrl_board_data.train_public_info.atp_year, 6);
#else
	get_local_time(&time_now);
	wtd_pw_alarm_data.atp_time.year = (uint8_t)(time_now.year-2000);
	wtd_pw_alarm_data.atp_time.mon = (uint8_t)time_now.mon;
	wtd_pw_alarm_data.atp_time.day = (uint8_t)time_now.day;
	wtd_pw_alarm_data.atp_time.hour = (uint8_t)time_now.hour;
	wtd_pw_alarm_data.atp_time.min = (uint8_t)time_now.min;
	wtd_pw_alarm_data.atp_time.sec = (uint8_t)time_now.sec;
#endif

	wtd_pw_alarm_data.carriage_number = pw_tz_data.train_public_info.carriage_number+1;//recv_public_para.board_data.train_public_info.carriage_number;

#if defined(INTERNAL_PROTOCOL_20210416) && !defined(PUB_INFO_REMOVE_KM_FLAG)
	wtd_pw_alarm_data.km_flag_h.wh = pw_tz_data.km_flag_hh;//recv_public_para.board_data.train_public_info.km_flag_hh;
	wtd_pw_alarm_data.km_flag_h.wl = pw_tz_data.km_flag_hl;//recv_public_para.board_data.train_public_info.km_flag_hl;
	wtd_pw_alarm_data.km_flag_l.wh = pw_tz_data.km_flag_lh;//recv_public_para.board_data.train_public_info.km_flag_lh;
	wtd_pw_alarm_data.km_flag_l.wl = pw_tz_data.km_flag_ll;//recv_public_para.board_data.train_public_info.km_flag_ll;
#endif

	wtd_pw_alarm_data.speed.wh  = pw_tz_data.train_public_info.speed[0];
	wtd_pw_alarm_data.speed.wl  = pw_tz_data.train_public_info.speed[1];

	memmove(&wtd_pw_alarm_data.tcms_time.year, &pw_tz_data.train_public_info.year, 6);

	*(uint16_t *)&wtd_pw_alarm_data.part_check_sum = htons(check_sum((uint8_t *)&wtd_pw_alarm_data, sizeof(struct WTD_PW_ALARM_DATA)-2));

//	printf("send_alarm_wtd_IPaddr:%s\n",inet_ntoa(alarm_wtd_ip_sockaddr.sin_addr));
//	printf("send_alarm_wtd_port:%d\n",ntohs(alarm_wtd_ip_sockaddr.sin_port));
//	printf("WTD alarm data len:%d\n", sizeof(struct WTD_PW_ALARM_DATA));
	sendto(send_wtd_fd_socket, (int8_t*)&wtd_pw_alarm_data, sizeof(struct WTD_PW_ALARM_DATA), 0, (struct sockaddr *)&alarm_wtd_ip_sockaddr, sizeof(alarm_wtd_ip_sockaddr));
}

#else

void send_save_wtd_pw_tz_data()
{
	static uint8_t not_first_flag = 0;//0为首次发送，1为非首次发送
	static uint8_t tz_cnt = 0;
	uint16_t resend_flag = 0x55;//0x55：表示首次发送该包数据，0xAA：表示重发该包数据

//	printf("send_wtd_pw_tz_data---tz_cnt:%d\n", tz_cnt);
	if(!not_first_flag)
	{
		tz_cnt = 0;

		save_tz_value_data[0].speed.wh  = pw_tz_data.train_public_info.speed[0];
		save_tz_value_data[0].speed.wl  = pw_tz_data.train_public_info.speed[1];
		memmove(&save_tz_value_data[0].longitude_down, &pw_tz_data.train_public_info.gps_data.longitude_down, 4);
		save_tz_value_data[0].latitude_dir = pw_tz_data.train_public_info.gps_data.latitude_dir;
		save_tz_value_data[0].longitude_dir = pw_tz_data.train_public_info.gps_data.longitude_dir;
		memmove(&save_tz_value_data[0].latitude_down, &pw_tz_data.train_public_info.gps_data.latitude_down, 4);
		save_tz_value_data[0].longitude_dir = pw_tz_data.train_public_info.gps_data.longitude_dir;

#ifdef INTERNAL_PROTOCOL_20210725
		save_tz_value_data[0].side1_y_quota.wh = pw_tz_data.side1_y_quota.wh;//已经乘100
		save_tz_value_data[0].side1_y_quota.wl = pw_tz_data.side1_y_quota.wl;
		save_tz_value_data[0].side2_y_quota.wh = pw_tz_data.side2_y_quota.wh;
		save_tz_value_data[0].side2_y_quota.wl = pw_tz_data.side2_y_quota.wl;
		save_tz_value_data[0].side1_z_quota.wh = pw_tz_data.side1_z_quota.wh;
		save_tz_value_data[0].side1_z_quota.wl = pw_tz_data.side1_z_quota.wl;
		save_tz_value_data[0].side2_z_quota.wh = pw_tz_data.side2_z_quota.wh;
		save_tz_value_data[0].side2_z_quota.wl = pw_tz_data.side2_z_quota.wl;

		save_tz_value_data[0].y_peak_w1.wh = 0x0;
		save_tz_value_data[0].y_peak_w1.wl = pw_tz_data.acc_index.y_peak_w1;//已经乘1000
		save_tz_value_data[0].y_peak_w2.wh = 0x0;
		save_tz_value_data[0].y_peak_w2.wl = pw_tz_data.acc_index.y_peak_w2;
		save_tz_value_data[0].y_root_w1.wh = 0x0;
		save_tz_value_data[0].y_root_w1.wl = pw_tz_data.acc_index.y_root_w1;
		save_tz_value_data[0].y_root_w2.wh = 0x0;
		save_tz_value_data[0].y_root_w2.wl = pw_tz_data.acc_index.y_root_w2;
		save_tz_value_data[0].z_root_w1.wh = 0x0;
		save_tz_value_data[0].z_root_w1.wl = pw_tz_data.acc_index.z_root_w1;
		save_tz_value_data[0].z_root_w2.wh = 0x0;
		save_tz_value_data[0].z_root_w2.wl = pw_tz_data.acc_index.z_root_w2;
#else
		save_tz_value_data[0].side1_y_quota.wh = 0;//已经乘100
		save_tz_value_data[0].side1_y_quota.wl = pw_tz_data.side1_y_quota;
		save_tz_value_data[0].side2_y_quota.wh = 0;
		save_tz_value_data[0].side2_y_quota.wl = pw_tz_data.side2_y_quota;
		save_tz_value_data[0].side1_z_quota.wh = 0;
		save_tz_value_data[0].side1_z_quota.wl = pw_tz_data.side1_z_quota;
		save_tz_value_data[0].side2_z_quota.wh = 0;
		save_tz_value_data[0].side2_z_quota.wl = pw_tz_data.side2_z_quota;
#endif
	}
	else
	{
		save_tz_value_data[tz_cnt].speed.wh  = pw_tz_data.train_public_info.speed[0];
		save_tz_value_data[tz_cnt].speed.wl  = pw_tz_data.train_public_info.speed[1];
		memmove(&save_tz_value_data[tz_cnt].longitude_down, &pw_tz_data.train_public_info.gps_data.longitude_down, 4);
		save_tz_value_data[tz_cnt].latitude_dir = pw_tz_data.train_public_info.gps_data.latitude_dir;
		save_tz_value_data[tz_cnt].longitude_dir = pw_tz_data.train_public_info.gps_data.longitude_dir;
		memmove(&save_tz_value_data[tz_cnt].latitude_down, &pw_tz_data.train_public_info.gps_data.latitude_down, 4);
		save_tz_value_data[tz_cnt].longitude_dir = pw_tz_data.train_public_info.gps_data.longitude_dir;

#ifdef INTERNAL_PROTOCOL_20210725
		save_tz_value_data[tz_cnt].side1_y_quota.wh = pw_tz_data.side1_y_quota.wh;//已经乘100
		save_tz_value_data[tz_cnt].side1_y_quota.wl = pw_tz_data.side1_y_quota.wl;
		save_tz_value_data[tz_cnt].side2_y_quota.wh = pw_tz_data.side2_y_quota.wh;
		save_tz_value_data[tz_cnt].side2_y_quota.wl = pw_tz_data.side2_y_quota.wl;
		save_tz_value_data[tz_cnt].side1_z_quota.wh = pw_tz_data.side1_z_quota.wh;
		save_tz_value_data[tz_cnt].side1_z_quota.wl = pw_tz_data.side1_z_quota.wl;
		save_tz_value_data[tz_cnt].side2_z_quota.wh = pw_tz_data.side2_z_quota.wh;
		save_tz_value_data[tz_cnt].side2_z_quota.wl = pw_tz_data.side2_z_quota.wl;

		save_tz_value_data[tz_cnt].y_peak_w1.wh = 0x0;
		save_tz_value_data[tz_cnt].y_peak_w1.wl = pw_tz_data.acc_index.y_peak_w1;//已经乘1000
		save_tz_value_data[tz_cnt].y_peak_w2.wh = 0x0;
		save_tz_value_data[tz_cnt].y_peak_w2.wl = pw_tz_data.acc_index.y_peak_w2;
		save_tz_value_data[tz_cnt].y_root_w1.wh = 0x0;
		save_tz_value_data[tz_cnt].y_root_w1.wl = pw_tz_data.acc_index.y_root_w1;
		save_tz_value_data[tz_cnt].y_root_w2.wh = 0x0;
		save_tz_value_data[tz_cnt].y_root_w2.wl = pw_tz_data.acc_index.y_root_w2;
		save_tz_value_data[tz_cnt].z_root_w1.wh = 0x0;
		save_tz_value_data[tz_cnt].z_root_w1.wl = pw_tz_data.acc_index.z_root_w1;
		save_tz_value_data[tz_cnt].z_root_w2.wh = 0x0;
		save_tz_value_data[tz_cnt].z_root_w2.wl = pw_tz_data.acc_index.z_root_w2;
#else
		save_tz_value_data[tz_cnt].side1_y_quota.wh = 0;
		save_tz_value_data[tz_cnt].side1_y_quota.wl = pw_tz_data.side1_y_quota*10;//已经乘10
		save_tz_value_data[tz_cnt].side2_y_quota.wh = 0;
		save_tz_value_data[tz_cnt].side2_y_quota.wl = pw_tz_data.side2_y_quota*10;
		save_tz_value_data[tz_cnt].side1_z_quota.wh = 0;
		save_tz_value_data[tz_cnt].side1_z_quota.wl = pw_tz_data.side1_z_quota*10;
		save_tz_value_data[tz_cnt].side2_z_quota.wh = 0;
		save_tz_value_data[tz_cnt].side2_z_quota.wl = pw_tz_data.side2_z_quota*10;
#endif
	}

	if(tz_cnt==0)
	{
		resend_flag = 0x55;
	}
	else
	{
		resend_flag = 0xAA;
	}

	tz_cnt++;

//	if(resend_flag == 0xAA)
//	{
//		wtd_pw_tz_data.data_head.resend_flag = resend_flag;
//		*(uint16_t *)&wtd_pw_tz_data.packet_total = 1;//固定1//little_to_big_16bit(packet_num_cnt+1);//时代同包序号//little_to_big_16bit(packet_total_cnt);
//	}
//	else
	if(resend_flag == 0x55)//统型要求重发与首次一至,不需再设重发值0xAA
	{
		*(uint16_t *)wtd_pw_tz_data.data_head.head = htons(0xAA57);
		*(uint16_t *)wtd_pw_tz_data.data_head.len = htons(sizeof(struct WTD_PW_TZ_DATA));  //数据头24字节
		wtd_pw_tz_data.data_head.company_id = LUHANG;    //板卡供应商编号 参考供应商定义
		wtd_pw_tz_data.data_head.board_id = PW_BOARD;    //本身板卡编号 参考宏定义LOCAL_BOARD

		if(first_diag_result_flag)
		{
			*(uint16_t *)wtd_pw_tz_data.data_head.life_signal =  htons(head_info.life_signal);//生命信号，每秒加1
		}
		*(uint16_t *)wtd_pw_tz_data.data_head.target_board_group = htons(head_info.board_set); //目标板卡的位集合
		wtd_pw_tz_data.data_head.resend_flag = resend_flag; //"0x55：表示首次发送该包数据，0xAA：表示重发该包数据；1s发送一次，10s后更新一次;即更新1次，重发9次,即每一包发10次，无需应答。
		wtd_pw_tz_data.data_head.ack_flag = 0;//ack_flag;   //"0x5A:目标板需要返回给请求板收到一包数据的应答帧   0x00:无需应答其它无效"
		//wtd_pw_tz_data.data_head.packet_num = 0x07;       //当前数据类型发送的总包数
		memset(wtd_pw_tz_data.data_head.res,0,sizeof(wtd_pw_tz_data.data_head.res));

		*(uint16_t *)wtd_pw_tz_data.frame_head = htons(0xA5A5);
		*(uint16_t *)&wtd_pw_tz_data.message_length = htons(WTD_TZ_MEMSAGE_LENGTH);
		wtd_pw_tz_data.message_type.bits.type = 0x01;
		*(uint16_t *)&wtd_pw_tz_data.packet_num = 0;//固定0//little_to_big_16bit(packet_num_cnt);
		*(uint16_t *)&wtd_pw_tz_data.packet_total = 1;//固定1//little_to_big_16bit(packet_num_cnt+1);//时代同包序号//little_to_big_16bit(packet_total_cnt);

		memmove(&wtd_pw_tz_data.train_public_info.year, &recv_public_para.recv_ctrl_board_data.train_public_info.year, 6);
		wtd_pw_tz_data.train_public_info.marshalling.bits.ge = recv_public_para.recv_ctrl_board_data.train_public_info.marshalling.bits.ge;
		wtd_pw_tz_data.train_public_info.marshalling.bits.shi = recv_public_para.recv_ctrl_board_data.train_public_info.marshalling.bits.shi;
		wtd_pw_tz_data.train_public_info.marshalling.bits.bai = recv_public_para.recv_ctrl_board_data.train_public_info.marshalling.bits.bai;
		wtd_pw_tz_data.train_public_info.marshalling.bits.qian = recv_public_para.recv_ctrl_board_data.train_public_info.marshalling.bits.qian;
		wtd_pw_tz_data.train_public_info.carriage_number = recv_public_para.recv_ctrl_board_data.train_public_info.carriage_number;
	#if defined(INTERNAL_PROTOCOL_20210416) && !defined(PUB_INFO_REMOVE_KM_FLAG)
		memmove(&wtd_pw_tz_data.train_public_info.km_flag_h, &recv_public_para.recv_ctrl_board_data.train_public_info.km_flag_hh, 4);
	#endif
	}

	if(!not_first_flag || (not_first_flag && tz_cnt >= ONE_PACKAGE_RESEND_NUM))//首次更新　或　非首次10s更新
	{
		if(!not_first_flag)//首次更新
		{
			memmove(wtd_pw_tz_data.tz_value_data, save_tz_value_data, sizeof(struct TZ_VALUE_DATA));//1s tz data
			not_first_flag = 1;
		}
		else//非首次10s更新
		{
			memmove(wtd_pw_tz_data.tz_value_data, save_tz_value_data, 10*sizeof(struct TZ_VALUE_DATA));//10s tz data
		}

		*(uint16_t *)&wtd_pw_tz_data.part_check_sum = htons(check_sum((uint8_t *)&wtd_pw_tz_data+TZ_PART_CHECK_SUM_START_INDEX,sizeof(struct WTD_PW_TZ_DATA)-TZ_PART_CHECK_SUM_OUT_SIZE));
		*(uint16_t *)&wtd_pw_tz_data.check_sum = htons(check_sum((uint8_t *)&wtd_pw_tz_data+TZ_CHECK_SUM_START_INDEX,sizeof(struct WTD_PW_TZ_DATA)-TZ_CHECK_SUM_OUT_SIZE));

		tz_cnt = 0;
	}

//	printf("send_tz_wtd_IPaddr:%s\n",inet_ntoa(tz_wtd_ip_sockaddr.sin_addr));
//	printf("send_tz_wtd_port:%d\n",ntohs(tz_wtd_ip_sockaddr.sin_port));
//	printf("WTD tz data len:%d\n", sizeof(struct WTD_PW_TZ_DATA));
	//sendto(send_wtd_fd_socket, (struct WTD_PW_TZ_DATA*)&wtd_pw_tz_data, sizeof(struct WTD_PW_TZ_DATA), 0, (struct sockaddr *)&save_ip_sockaddr, sizeof(save_ip_sockaddr));
	sendto(send_wtd_fd_socket, (uint8_t*)&wtd_pw_tz_data+TZ_COPY_START_INDEX, sizeof(struct WTD_PW_TZ_DATA)-TZ_COPY_OUT_SIZE, 0, (struct sockaddr *)&tz_wtd_ip_sockaddr, sizeof(tz_wtd_ip_sockaddr));
}

void send_save_wtd_pw_alarm_data()
{
	uint16_t resend_flag = 0x55;
	uint8_t ch_num = 0;
	uint8_t data_valid_flag = 0;

//	printf("send_wtd_pw_alarm_data---1\n");

	/*按报警解除触发次序发送，直到发完*/
	ch_num = get_send_enable_channel();

	WTD_DEBUG("send_save_wtd_pw_alarm_data--ch:%d\n", ch_num);

	if(ch_num == 0xff)
		return;

	if(alarm_ctrl[ch_num].send_ctrl == ALARM_SEND_DISABLE)
		return;

	WTD_DEBUG("send_save_wtd_pw_alarm_data--packet_total:%d\n", alarm_ctrl[ch_num].packet_total);
	WTD_DEBUG("send_wtd_pw_alarm_data---packet_num_cnt:%d\n", alarm_ctrl[ch_num].packet_num_cnt);

	if(!alarm_ctrl[ch_num].not_first_flag)
	{
		alarm_ctrl[ch_num].update_cnt = 0;
		alarm_ctrl[ch_num].packet_num_cnt = 0;
	}
	else
	{
		if(alarm_ctrl[ch_num].update_cnt==0)
		{
			alarm_ctrl[ch_num].packet_num_cnt++;
		}
	}

	if(alarm_ctrl[ch_num].update_cnt==0)//first time
	{
		resend_flag = 0x55;
	}
	else
	{
		resend_flag = 0xAA;
	}

	alarm_ctrl[ch_num].update_cnt++;

//	if(resend_flag == 0xAA)
//	{
//		wtd_pw_alarm_data.data_head.resend_flag = resend_flag;
//		*(uint16_t *)&wtd_pw_alarm_data.packet_total = little_to_big_16bit(alarm_ctrl[ch_num].packet_num_cnt+1);//时代同包序号//little_to_big_16bit(alarm_ctrl[ch_num].packet_total_cnt);
//	}
//	else
	if(resend_flag == 0x55)//统型要求重发与首次一至,不需再设重发值0xAA
	{
		//报警数据（报警前30秒　至　报警解除30秒后，至少1分钟数据量, 参考函数save_msg_alarm_data对数据量的控制）
		memset((int8_t*)wtd_pw_alarm_data.alarm_data, 0, MSG_ALARM_DATA_SIZE);
		data_valid_flag = get_msg_alarm_data(ch_num, (int8_t*)wtd_pw_alarm_data.alarm_data, MSG_ALARM_DATA_SIZE);
		if(!data_valid_flag)
		{
			return;
		}

		*(uint16_t *)wtd_pw_alarm_data.data_head.head = htons(0xAA58);
		*(uint16_t *)wtd_pw_alarm_data.data_head.len = htons(sizeof(struct WTD_PW_ALARM_DATA)); //数据头24字节
		wtd_pw_alarm_data.data_head.company_id = LUHANG;          //板卡供应商编号 参考供应商定义
		wtd_pw_alarm_data.data_head.board_id = PW_BOARD;          //本身板卡编号 参考宏定义LOCAL_BOARD
		*(uint16_t *)wtd_pw_alarm_data.data_head.life_signal =  htons(head_info.life_signal); //生命信号，每秒加1
		*(uint16_t *)wtd_pw_alarm_data.data_head.target_board_group = htons(head_info.board_set); //目标板卡的位集合
		wtd_pw_alarm_data.data_head.resend_flag = resend_flag; //"0x55：表示首次发送该包数据，0xAA：表示重发该包数据；重发时的数据与首次发送的数据需全部一样，1s发送一次，10s后更新一次;即更新1次，重发9次,即每一包发10次，无需应答。"
		wtd_pw_alarm_data.data_head.ack_flag = 0x0;            //"0x5A:目标板需要返回给请求板收到一包数据的应答帧   0x00:无需应答其它无效"
		//wtd_pw_alarm_data.data_head.packet_num = 0x07;       //当前数据类型发送的总包数
		memset(wtd_pw_alarm_data.data_head.res,0,sizeof(wtd_pw_alarm_data.data_head.res));

		*(uint16_t *)wtd_pw_alarm_data.frame_head = htons(0xA6A6);
		*(uint16_t *)&wtd_pw_alarm_data.message_length = htons(WTD_ALARM_MEMSAGE_LENGTH);
		wtd_pw_alarm_data.message_type.bits.type = 0x02;
		wtd_pw_alarm_data.message_type.bits.fault_sensor_pos = get_alarm_position(ch_num);
		wtd_pw_alarm_data.message_type.bits.fault_data_flag = 0x1;

		*(uint16_t *)&wtd_pw_alarm_data.packet_num = htons(alarm_ctrl[ch_num].packet_num_cnt);
		*(uint16_t *)&wtd_pw_alarm_data.packet_total = htons(alarm_ctrl[ch_num].packet_total);

		memmove(&wtd_pw_alarm_data.train_public_info.year, &recv_public_para.recv_ctrl_board_data.train_public_info.year, 6);
		wtd_pw_alarm_data.train_public_info.marshalling.bits.ge = recv_public_para.recv_ctrl_board_data.train_public_info.marshalling.bits.ge;
		wtd_pw_alarm_data.train_public_info.marshalling.bits.shi = recv_public_para.recv_ctrl_board_data.train_public_info.marshalling.bits.shi;
		wtd_pw_alarm_data.train_public_info.marshalling.bits.bai = recv_public_para.recv_ctrl_board_data.train_public_info.marshalling.bits.bai;
		wtd_pw_alarm_data.train_public_info.marshalling.bits.qian = recv_public_para.recv_ctrl_board_data.train_public_info.marshalling.bits.qian;
		wtd_pw_alarm_data.train_public_info.carriage_number = recv_public_para.recv_ctrl_board_data.train_public_info.carriage_number;
	#if defined(INTERNAL_PROTOCOL_20210416) && !defined(PUB_INFO_REMOVE_KM_FLAG)
		memmove(&wtd_pw_alarm_data.train_public_info.km_flag_h, &recv_public_para.recv_ctrl_board_data.train_public_info.km_flag_hh, 4);
	#endif

		wtd_pw_alarm_data.speed.wh  = pw_tz_data.train_public_info.speed[0];
		wtd_pw_alarm_data.speed.wl  = pw_tz_data.train_public_info.speed[1];
	}

	if((!alarm_ctrl[ch_num].not_first_flag && alarm_ctrl[ch_num].update_cnt==1) || (alarm_ctrl[ch_num].not_first_flag && alarm_ctrl[ch_num].update_cnt>=ONE_PACKAGE_RESEND_NUM))
	{
		*(uint16_t *)&wtd_pw_alarm_data.part_check_sum = htons(check_sum((uint8_t *)&wtd_pw_alarm_data+ALARM_PART_CHECK_SUM_START_INDEX, sizeof(struct WTD_PW_ALARM_DATA)-ALARM_PART_CHECK_SUM_OUT_SIZE));
		*(uint16_t *)&wtd_pw_alarm_data.check_sum = htons(check_sum((uint8_t *)&wtd_pw_alarm_data+ALARM_CHECK_SUM_START_INDEX, sizeof(struct WTD_PW_ALARM_DATA)-ALARM_CHECK_SUM_OUT_SIZE));
	}

	if(!alarm_ctrl[ch_num].not_first_flag)
	{
		alarm_ctrl[ch_num].not_first_flag = 1;
	}
	else if(alarm_ctrl[ch_num].not_first_flag && alarm_ctrl[ch_num].update_cnt>=ONE_PACKAGE_RESEND_NUM)
	{
		alarm_ctrl[ch_num].update_cnt = 0;
	}

//	printf("send_alarm_wtd_IPaddr:%s\n",inet_ntoa(alarm_wtd_ip_sockaddr.sin_addr));
//	printf("send_alarm_wtd_port:%d\n",ntohs(alarm_wtd_ip_sockaddr.sin_port));
//	printf("WTD alarm data len:%d\n", sizeof(struct WTD_PW_ALARM_DATA));
	//sendto(send_wtd_fd_socket, (struct WTD_PW_ALARM_DATA*)&wtd_pw_alarm_data, sizeof(struct WTD_PW_ALARM_DATA), 0, (struct sockaddr *)&save_ip_sockaddr, sizeof(save_ip_sockaddr));
	sendto(send_wtd_fd_socket, (int8_t*)&wtd_pw_alarm_data+ALARM_COPY_START_INDEX, sizeof(struct WTD_PW_ALARM_DATA)-ALARM_COPY_OUT_SIZE, 0, (struct sockaddr *)&alarm_wtd_ip_sockaddr, sizeof(alarm_wtd_ip_sockaddr));
}
#endif
/*************************************************
Function:    ptu_recv_thread_entry
Description: ptu接收线程
Input:
Output:
Return:
Others:
*************************************************/
void wtd_data_send_thread()
{
#ifdef RT_LINUX
    pthread_attr_t attr;       //线程属性  pthread_attr_t 为线程属性结构体
    struct sched_param sched;  //调度策略
    DEBUG ("set SCHED_RR policy\n");
    api_set_thread_policy(&attr, SCHED_RR);
#endif

    WTD_DEBUG("---wtd_data_send_thread---\n");

    while(1)
    {
    	 sem_wait(&send_wtd_sem);

    	 WTD_DEBUG("wtd_data_send_thread---send_wtd_sem---trig\n");

    	 if(first_diag_result_flag)//上电后第一次出算法结果才开始有效
    	 {
	#ifdef S350_JXZNDC_17_SFE68
			system("echo 'nameserver 10.0.0.1' > /etc/resolv.conf");
	#endif
			send_save_wtd_pw_tz_data();//1s发10s更新
			send_save_wtd_pw_alarm_data();//1s发10s更新
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
int init_wtd_thread()
{
	pthread_t wtd_thread_id;
	int ret;

	ret=pthread_create(&wtd_thread_id, NULL, (void *)wtd_data_send_thread, NULL);
	if(ret!=0)
	{
//		printf("net save thread error!\n");
		return ret;
	}

	return 0;
}
#endif
