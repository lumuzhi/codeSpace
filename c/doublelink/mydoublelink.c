#include "mydoublelink.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

DLink* createNode()
{
    DLink* newNode = (DLink * )malloc(sizeof(DLink));
    if(newNode == NULL)
        return NULL;
    return newNode;
}
HeadTail* DLinkInit()
{
    HeadTail* ht = (HeadTail * )malloc(sizeof(HeadTail));
    if(ht == NULL) return NULL;

    DLink* virHead = createNode();
    virHead->next = virHead->pre = NULL;
    virHead->val = ht->length = 0;

    ht->head = ht->tail = virHead;
    return ht;
}
//增 头插
int DLinkPush(HeadTail* ht, int val)
{
    if(NULL == ht) return -1;
    DLink* newNode = createNode();
    newNode->val = val;
    if(!ht->length){
        ht->tail = newNode;
        newNode->next = ht->head->next;
        newNode->pre = ht->head;
        ht->head->next = newNode;
    }else{
        ht->head->next->pre = newNode;
        newNode->next = ht->head->next;
        newNode->pre = ht->head;
        ht->head->next = newNode;
    }
    ++ht->length;
    return 0;
}
//删 按照位置删除
int DLinkPopPos(HeadTail* ht, int pos)
{
    if(NULL == ht) return -1;
    if(pos > ht->length) return -1;
    if(pos == ht->length){
        ht->tail = ht->tail->pre;
        free(ht->tail->next);
        ht->tail->next = NULL;
        --ht->length;
        return 0;
    }
    int searchDir = (2 * pos > ht->length)?(1):(0);
    DLink* temp = NULL;
    int count = 0;
    switch (searchDir)
    {
        case 1:
           temp = ht->tail;
            count = ht->length - pos;
            while(count){
                temp = temp->pre;
                --count;
            }
            temp->pre->next = temp->next;
            temp->next->pre = temp->pre;
            break;
        case 0:
            temp = ht->head;
            count = pos;
            while(count){
                temp = temp->next;
                --count;
            }
            temp->pre->next = temp->next;
            temp->next->pre = temp->pre;
        default:
            break;
    }
    --ht->length;
    free(temp);
    temp = NULL;
    return 0;
}
//删 按照值删除
int DLinkPopVal(HeadTail* ht, int val)
{
    if(!ht->length) return -1;
    if(ht->length == 1 && ht->head->next->val == val){
        free(ht->head->next);
        ht->head->next = NULL;
        return 0;
    }
    DLink *tempHead = ht->head->next, *tempTail = ht->tail;
    int left = 1, right = ht->length;
    while(left++ <= right--){
        // printf("head is %d, tail is %d\n", tempHead->val, tempTail->val);
        if(tempHead->val == val){
            tempHead->next->pre = tempHead->pre;
            if(tempHead->next != NULL)
                tempHead = tempHead->next;
            free(tempHead->pre->next);
            tempHead->pre->next = tempHead;   
            --ht->length;
        }else{
            if(tempHead->next != NULL)
                tempHead = tempHead->next;
        }

        if(tempTail->val == val){
            //最后一个
            if(tempTail->next == NULL){
                tempTail = tempTail->pre;
                free(tempTail->next);
                tempTail->next = NULL;
            }else{
                //不是最后一个
                tempTail->pre->next = tempTail->next;
                tempTail = tempTail->pre;
                free(tempTail->next->pre);
                tempTail->next->pre = tempTail;
            }
            --ht->length;
        }else{
            tempTail = tempTail->pre;
        }
    }
    return 0;
}


void DLinkPrint(HeadTail* ht)
{
    DLink* temp = ht->head->next;

    while(temp != NULL){
        printf("%d ", temp->val);
        temp = temp->next;
    }
    printf("\n");
}


int DLinkGetLength(DLink* head)
{
    int len = 0;
    if(NULL == head) return -1;
    DLink* temp = head->next;
    while(temp != NULL){
        ++len;
        temp = temp->next;
    }
    return len;
}



