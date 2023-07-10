/*
 * self_queue.h
 *
 *  Created on: Dec 18, 2021
 *      Author: linux-ls
 */

#ifndef _SELF_QUEUE_H_
#define _SELF_QUEUE_H_

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

typedef uint16_t QNode_Num;
typedef uint32_t QNode_Order;
//typedef uint8_t  QNode_CH;
//
//typedef struct QNode_Head_Info
//{
//	QNode_Order trig_order;
//	QNode_CH trig_ch;
//}QNode_Head;

typedef struct QNode{
	int8_t *data;
	struct QNode *next;
}QNode;

typedef struct LiQueue{
	QNode_Order send_order;//根据报警解除次序(且alarm_num>0)发送
	QNode_Num one_time_alarm_num;//每次报警节点数计数
	QNode_Num one_time_alarm_num_total;//每次报警总节点数(加上解除后30s)，报警刚解除才计算,（序号＋1＝＝总包数时，最后一包发送后禁发）
	QNode_Num all_time_alarm_num_limit;//用于限制一次或多次报警总节点数,避免内存空间不够．
	QNode *front;
	QNode *rear;
	struct LiQueue *next;//用于接新报警链队
}LiQueue;

LiQueue* QueueInit();
void QueueDeInit(LiQueue *q);
void ClearQueue(LiQueue *q);
uint8_t QueueIsEmpty(LiQueue *q);
void EnQueue(LiQueue *q, int8_t *buf, uint16_t buf_len);
void DeQueue(LiQueue *q, int8_t *buf, uint16_t buf_len);
void DeQueue_DelOneQNode(LiQueue *q);
void Queue_Append_NewQueue(LiQueue *q, LiQueue *qn);
void Queue_DeleteLastQueue(LiQueue *q);
#endif /* _SELF_QUEUE_H_ */
