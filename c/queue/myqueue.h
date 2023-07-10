#ifndef __MYQUEUE_H
#define __MYQUEUE_H
#include <stdio.h>
#include <stdlib.h>

#define SIZE 10

typedef struct {
  int data[SIZE];
  int front;
  int rear;
} lineQueue;

typedef struct queue_node {
  int val;
  struct queue_node *next;
} myqueue;
// 链表
myqueue *myqueue_init();
int myqueue_push(myqueue *f, int node_val);
int myqueue_pop(myqueue *f);
int myqueue_destory(myqueue **f);
void myqueue_print(myqueue *f);
int myqueue_length(myqueue *f);

// 线性
#endif
