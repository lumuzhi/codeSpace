#include "myqueue.h"
// 初始化queue
myqueue *myqueue_init()
{
    myqueue *head = (myqueue *)malloc(sizeof(myqueue));
    if(NULL == head)
        return NULL;
    head->val = 0;
    head->next = NULL;
    return head;
}
// 添加元素
int myqueue_push(myqueue *f, int node_val)
{
    if(NULL == f)
        return -1;
    myqueue *new_node = (myqueue *)malloc(sizeof(myqueue));
    new_node->val = node_val;
    new_node->next = NULL;
    myqueue *temp = f;
    // 存在虚拟头结点
    while(temp->next != NULL)
        temp = temp->next;
    temp->next = new_node;
    return 0;
}
// 删除头元素
int myqueue_pop(myqueue *f)
{
    if(NULL == f || f->next == NULL)
        return -1;
    int res = f->next->val;
    myqueue *temp = NULL;
    if(myqueue_length(f) == 1){
        temp = f->next;
        f->next = NULL;
    }else{
        temp = f->next;
        f->next = f->next->next;
    }
    free(temp);
    temp = NULL;
    return res;
}
// 销毁队列
int myqueue_destory(myqueue **f)
{
    myqueue *head = *f;
    while(NULL != head->next){
        myqueue *temp = head;
        head = head->next;
        free(temp);
    }
    free(head);
    *f = NULL;
    return 0;
}
// 队列打印
void myqueue_print(myqueue *f)
{
    if(f->next == NULL){
        printf("NULL\n");
    }else{
        myqueue *temp = f->next;
        while(temp != NULL){
            printf("%d ", temp->val);
            temp = temp->next;
        }
        printf("\n");
    }
}
// 队列长度
int myqueue_length(myqueue *f)
{
    if(f->next == NULL)
        return 0;
    int len = 0;
    myqueue *temp = f->next;
    while(temp != NULL){
        temp = temp->next;
        ++len;
    }
    return len;
}


// 线性
 




