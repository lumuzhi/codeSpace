#ifndef __MYDOUBLELINK_h
#define __MYDOUBLELINK_H

typedef struct DLink{
    int val;
    struct DLink* pre;
    struct DLink* next;
}DLink;

typedef struct HeadTail{
    DLink* head;
    DLink* tail;
    int length;
}HeadTail;

HeadTail* DLinkInit();

DLink* createNode();
int DLinkPush(HeadTail* head, int val);

int DLinkPopPos(HeadTail* ht, int pos);

int DLinkPopVal(HeadTail* ht, int val);
void DLinkPrint(HeadTail* ht);

int DLinkGetLength(DLink* head);


#endif

