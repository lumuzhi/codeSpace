#include<string.h>
#include<stdio.h>
#include<iostream>
using namespace std;
int main(){
    char p1[10] = "abcd", *p2, str[10] = "xyz";  
    p2 = "ABCD";
    strcpy(str + 2, strcat(p1 + 2, p2 + 1));
    printf(" %s", str);
}