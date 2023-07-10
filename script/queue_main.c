#include "c/queue/myqueue.h"

int main()
{
    myqueue *f = myqueue_init();
    for(int i =1; i < 10; ++i)
        myqueue_push(f, i);
    myqueue_print(f);
    printf("f->val = %d, current length = %d\n", f->val, myqueue_length(f));
    for(int i = 1; i < 10; ++i)
        myqueue_pop(f);
    myqueue_print(f);
    myqueue_destory(&f);
    return 0;
}