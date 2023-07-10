/*
 * self_queue.c
 *
 *  Created on: Dec 18, 2021
 *      Author: linux-ls
 */

#include "self_queue.h"


LiQueue* QueueInit()
{
	LiQueue* q = NULL;

	q = (LiQueue*)malloc(sizeof(LiQueue));
	q->front = q->rear = NULL;//无头节点

	q->send_order = 0;
	q->one_time_alarm_num = 0;
	q->one_time_alarm_num_total = 0;
	q->all_time_alarm_num_limit = 0;

	q->next = NULL;
//	printf("Queue Init Over!\n");

	return q;
}

void QueueDeInit(LiQueue *q)
{
	QNode* s = NULL;

	while(q->front)//head:q->front
	{
		s = q->front;
		q->front = s->next;
//		q->limit_num--;

		if(s->data)
		{
			free(s->data);
			s->data = NULL;
		}

		if(s)
		{
			free(s);
			s = NULL;
		}
	}

	if(q)
	{
		free(q);//Error:double free or corruption (fasttop)
		q = NULL;
	}
//	printf("Queue DeInit Over!\n");
}

void ClearQueue(LiQueue *q)
{
	QNode* s = NULL;

	while(q->front)//head:q->front
	{
		s = q->front;
		q->front = s->next;

		if(s->data)
		{
			free(s->data);
			s->data = NULL;
		}

		if(s)
		{
			free(s);
			s = NULL;
		}
	}

	q->send_order = 0;
	q->one_time_alarm_num = 0;
	q->one_time_alarm_num_total = 0;
	q->all_time_alarm_num_limit = 0;

	q->rear = q->front = NULL;

//	printf("Queue Clear Over!\n");
}

uint8_t QueueIsEmpty(LiQueue *q)
{
	if(q->front == NULL)
		return 1;
	else
		return 0;
}

void EnQueue(LiQueue *q, int8_t *buf, uint16_t buf_len)
{
	QNode *s = (QNode*)malloc(sizeof(QNode));
	s->data = (int8_t*)malloc(buf_len);

	memmove(s->data, buf, buf_len);
	s->next = NULL;

	if(QueueIsEmpty(q))
	{
		q->rear = q->front = s;//首节点
	}
	else
	{
		//尾插
		q->rear->next = s;
		q->rear = s;
	}

	q->one_time_alarm_num++;
}

void DeQueue(LiQueue *q, int8_t *buf, uint16_t buf_len)
{
	QNode *s = NULL;

	if(!QueueIsEmpty(q))
	{
		s = q->front;
		memmove(buf, s->data, buf_len);
		q->front = s->next;
		q->one_time_alarm_num--;

		if(s->data)
		{
			free(s->data);
			s->data = NULL;
		}

		if(s)
		{
			free(s);
			s = NULL;
		}
	}
}

void DeQueue_DelOneQNode(LiQueue *q)
{
	QNode *s = NULL;

	if(!QueueIsEmpty(q))
	{
		s = q->front;
		q->front = s->next;
		q->one_time_alarm_num--;

		if(s->data)
		{
			free(s->data);
			s->data = NULL;
		}

		if(s)
		{
			free(s);
			s = NULL;
		}
	}
}

//LiQueue *qn = QueueInit()
/*追加一个新（报警）链队*/
void Queue_Append_NewQueue(LiQueue *q, LiQueue *qn)
{
	q->next = qn;
}

/*递归删除最后一个（报警）链队*/
void Queue_DeleteLastQueue(LiQueue *q)
{
	if(q->next)
	{
		Queue_DeleteLastQueue(q->next);
	}
	else
	{
		QueueDeInit(q);//删除最后一个链队
	}
}
//end
