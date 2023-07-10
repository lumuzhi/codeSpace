#include "c/tree/mytree.h"
#include <stdio.h>
int main()
{
    int nums[7] = {1, 2, 3, 4,5 ,6, 7};
    int res[7] = {0};
    TreeNode* root = buildCompleteBinaryTree(nums, 0, 7);
    inOrder(root);
    printf("\n");
    return 0;
}