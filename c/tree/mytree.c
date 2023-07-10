#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mytree.h"

TreeNode* createNode(int val){
    TreeNode* node = ( TreeNode * )malloc( sizeof( TreeNode ) );
    if(NULL == node) return NULL;
    node->val = val;
    node->left = node->right = NULL;
    return node;
}

TreeNode* buildCompleteBinaryTree(int *nums, int start, int len){
    if(start >= len) return NULL;
    TreeNode* root = createNode(nums[start]);
    if(NULL == root) return NULL;
    int leftIndex = 2 * start + 1;
    int rightIndex = 2 * start + 2;
    root->left = buildCompleteBinaryTree(nums, leftIndex, len);
    root->right = buildCompleteBinaryTree(nums, rightIndex, len);
    return root;
}

void preOrder(TreeNode* root){
    if(root == NULL) return;
    printf("%d ", root->val);
    preOrder(root->left);
    preOrder(root->right);
}