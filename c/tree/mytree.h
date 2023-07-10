#ifndef __MYTREE_h
#define __MYTREE_H

typedef struct TreeNode
{
    int val;
    struct TreeNode *left;
    struct TreeNode *right;
}TreeNode;

TreeNode* createNode(int val);
TreeNode* buildCompleteBinaryTree(int *nums, int start, int len);

void preOrder(TreeNode* root);

#endif
