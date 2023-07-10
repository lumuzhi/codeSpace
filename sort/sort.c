#include <stdio.h>
#include <string.h>
/*     快速排序     */
void quicksort(int *num, int start, int end)
{
    if(start >= end)
        return;
    int base_val = num[start];
    int left = start, right = end;
    while (left < right)
    {
        while(left < right && base_val <= num[right])
            --right;
        num[left] = num[right];
        while(left < right && base_val >= num[left])
            ++left;
        num[right] = num[left];
    }
    num[left] = base_val;
    quicksort(num, start, left - 1);
    quicksort(num, left + 1, end);
    
}
/*    冒泡排序  从小到大  */
void bubblesort(int *num, int len)
{
    for(int i = 0; i < len - 1; ++i){
        for(int j = 1; j <= len - i - 1; ++j){
            if(num[j] <= num[j - 1]){
                int temp = num[j];
                num[j] = num[j - 1];
                num[j - 1] = temp;
            }
        }
    }
}

/*   堆排序   */
void heapsort(int *num)
{

}

/*    插入排序   */
void insertsort(int *num, int len)
{
    int j = 0,key = 0, i = 0;
    // for(i = 1; i < len; ++i){
    //     key = num[i];
    //     j = i - 1;
    //     while(j >= 0 && num[j] > key){
    //         num[j + 1] = num[j];
    //         j = j - 1;
    //     }
    //     num[j + 1] = key;
    // }
    for(i = 1; i < len; ++i){
        if(num[i - 1] > num[i]){
            key = num[i];
            num[i] = num[i - 1];
            num[i - 1] = key;
            j = i - 1;
            while(j > 0 && key < num[j -1]){
                num[j] = num[j - 1];
                num[j - 1] = key;
                --j;
            }
        }
    }
}
/*   选择排序   */
void selectsort(int *num, int len)
{
    for(int i = 0; i < len; ++i){
        int min_index = i;
        for(int j = i + 1; j < len; ++j)
            if(num[j] < num[min_index])
                min_index = j;
        int temp = num[i];
        num[i] = num[min_index];
        num[min_index] = temp;
    }
}

/*   希尔排序    */

void shellsort(int arr[], int n) {
    int gap, i, j, temp;
    
    // 初始步长设定为数组长度的一半
    for (gap = n / 2; gap > 0; gap /= 2) {
        // 对每个步长进行插入排序
        for (i = gap; i < n; i++) {
            temp = arr[i];
            
            // 在已排序的元素中找到合适的位置插入
            for (j = i; j >= gap && arr[j - gap] > temp; j -= gap) {
                arr[j] = arr[j - gap];
            }
            
            arr[j] = temp;
        }
    }
}


/*    实现 strcmp函数功能*/


int mystrcmp(const char *str1, const char *str2)
{
    while(*str1 && *str1 == *str2){
        ++str1;
        ++str2;
    }   
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}

int main()
{
    // int a[10] = {3, 4,7, 9, 1, 6, 8, 10, 2, 5};
    // quicksort(a, 0, 9);
    // bubblesort(a, 10);
    // insertsort(a, 10);
    // selectsort(a, 10);
    // shellsort(a, 10);
    // for(int i = 0; i < 10; ++i)
    //     printf("%d ", a[i]);
    // printf("\n");
    return 0;
}