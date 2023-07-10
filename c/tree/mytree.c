#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mytree.h"

#define __DEBUG__

#ifdef __DEBUG__
    #define DEBUG(...) printf(__VA_ARGS__)
#else
    #define DEBUG(...)
#endif


TreeNode* createNode(int val)
{
    TreeNode* node = ( TreeNode * )malloc( sizeof( TreeNode ) );
    if(NULL == node) return NULL;
    node->val = val;
    node->left = node->right = NULL;
    return node;
}

TreeNode* buildCompleteBinaryTree(int *arr, int start, int len)
{
    if(start >= len) return NULL;
    TreeNode* root = createNode(arr[start]);
    if(NULL == root) return NULL;
    int leftIndex = 2 * start + 1;
    int rightIndex = 2 * start + 2;
    root->left = buildCompleteBinaryTree(arr, leftIndex, len);
    root->right = buildCompleteBinaryTree(arr, rightIndex, len);
    return root;
}

/*      recursion start     */

/* 先序遍历(递归) */
void preOrder(TreeNode* root)
{
    if(root == NULL) return;
    DEBUG("%d ", root->val);
    //printf("%d ", root->val);
    preOrder(root->left);
    preOrder(root->right);
}

/* 后序遍历(递归) */
void postOrder(TreeNode* root, int arr[], int len)
{
    static int p = 0;
    if( root == NULL ) return;
    if(p > len) return;
    //DEBUG("%d ", root->val);
    arr[p++] = root->val;
    postOrder(root->right, arr, len);
    postOrder(root->left, arr, len);
}

/* 中序遍历(递归) */
void inOrder(TreeNode* root)
{
    if( root == NULL ) return;
    inOrder(root->left);
    DEBUG("%d ", root->val);
    inOrder(root->right);
}

/*      recursion end     */

/*      iteration function start     */






/*      iteration function end     */






/* 数组倒序 */
void reverseArray(int arr[], int size)
{
    for(int i = 0; i < size / 2; ++i){
        int tail = size - 1 - i;
        arr[i] = arr[i] + arr[tail];
        arr[tail] = arr[i] - arr[tail];
        arr[i] = arr[i] - arr[tail];
    } 
}
