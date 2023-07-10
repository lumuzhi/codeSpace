#include "c/doublelink/mydoublelink.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//单词倒序
// i am linwanrong !
char* fun(char* str)
{
    int len = strlen(str);
    int pre = len;
    int pos = 0;
    char* res = (char*)malloc((len + 1) * sizeof(char)); // 动态分配内存
    memset(res, 0, (len + 1) * sizeof(char)); // 初始化内存
    for(int i = len - 1; i >= 0; --i){
        if(str[i] == ' ' || i == 0){
            if(i == 0){ 
                res[pos] = str[i];
                break;
            }
            for(int j = i + 1; j < pre; ++j){
                sprintf(res + pos, "%c", str[j]);
                ++pos;
            }
            res[pos++] = ' ';
            pre = i;
        }   
    }
    return res;
}

int fun1(int n)
{
    if(n == 0 || n == 1)
        return n;
    return ( 2 * fun1(n - 1) + 3 * fun1(n - 2) );
}
void fun2(int nums1[], int nums2[], int* nums3, int len1, int len2)
{
    int pos = 0;
    int swi = ( nums1[0] >= nums2[0] ) ? 1 : 2;
    int i = 0, j = 0;
    switch (swi)
    {
        case 1:
            nums3[pos++] = nums2[0];
            i = 0, j = 1;
            while( i < len1 || j < len2 ){
                if(nums1[i] > nums2[j]){
                    nums3[pos++] = nums1[i];
                    ++i;
                }
                else{
                    nums3[pos++] = nums2[j];
                    ++j;
                }
            }
            if(i < len1){
                for(int k = i; k < len1; ++k)
                    nums3[pos++] = nums1[i];
            }else{
                for(int k = j; k < len2; ++k)
                    nums3[pos++] = nums1[j];
            }
            break;
        case 2:
            nums3[pos++] = nums1[0];
            i = 0, j = 1;
            while( i < len1 || j < len2 ){
                if(nums1[i] > nums2[j]){
                    nums3[pos++] = nums2[j];
                    ++j;
                }
                else{
                    nums3[pos++] = nums1[i];
                    ++i;
                }
            }
            if(i < len1){
                for(int k = i; k < len1; ++k)
                    nums3[pos++] = nums1[i];
            }else{
                for(int k = j; k < len2; ++k)
                    nums3[pos++] = nums1[j];
            }
            break;
        default:
            break;
    }
}


int main()
{
    // HeadTail* ht = DLinkInit();
    // DLinkPush(ht, 9);
    // for(int i = 1; i < 10; ++i){
    //     DLinkPush(ht, i);
    // }
    // DLinkPrint(ht);
    // printf("len = %d \n", ht->length);
    // DLinkPopVal(ht, 9);
    // DLinkPrint(ht);
    // printf("len = %d \n", ht->length);

    return 0;
}