#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#define A(x) x;x;x;x;x;x;x;x;x;x;

int main()
{
    int n = 1;
    A(A(printf("%d ", n++)));
    
    return 0;
}
