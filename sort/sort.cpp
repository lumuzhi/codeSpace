#include <iostream>
#include <vector>
#include <string>
using namespace std;


class Sort{
    public:
        /*    快速排序    */
        void quicksort(vector<int>& num, int start, int end){
            if(start >= end)
                return;
            int left = start, right = end;
            int baseVal = num[left];
            while(left < right){
                while( left < right && baseVal <= num[right])
                    --right;
                num[left] = num[right];
                while(left < right && baseVal >= num[left])
                    ++left;
                num[right] = num[left];
            }
            num[left] = baseVal;
            quicksort(num, start, left - 1);
            quicksort(num, left + 1, end);
        }
        /*   冒泡排序 从小到大*/
        void bubblesort(vector<int>& num, int len){
            if(!len) return;
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
        /*    插入排序*/
        void insertsort(vector<int>& num, int len){
            int j = 0,key = 0, i = 0;
            for(i = 1; i < len; ++i){
                key = num[i];
                j = i - 1;
                while(j >= 0 && num[j] > key){
                    num[j + 1] = num[j];
                    j = j - 1;
                }
                num[j + 1] = key;
            }
        }
};

#include <iostream>
#include <string>
#include <vector>

using namespace std;

class Solution {
public:
    string longestPalindrome(string s) {
        int n = s.size();
        if (n < 2) {
            return s;
        }

        int maxLen = 1;
        int begin = 0;
        // dp[i][j] 表示 s[i..j] 是否是回文串
        vector<vector<int>> dp(n, vector<int>(n));
        // 初始化：所有长度为 1 的子串都是回文串
        for (int i = 0; i < n; i++) {
            dp[i][i] = true;
        }
        // 递推开始
        // 先枚举子串长度
        for (int L = 2; L <= n; L++) {
            // 枚举左边界，左边界的上限设置可以宽松一些
            for (int i = 0; i < n; i++) {
                // 由 L 和 i 可以确定右边界，即 j - i + 1 = L 得
                int j = L + i - 1;
                // 如果右边界越界，就可以退出当前循环
                if (j >= n) {
                    break;
                }

                if (s[i] != s[j]) {
                    dp[i][j] = false;
                } else {
                    if (j - i < 3) {
                        dp[i][j] = true;
                    } else {
                        dp[i][j] = dp[i + 1][j - 1];
                    }
                }

                // 只要 dp[i][L] == true 成立，就表示子串 s[i..L] 是回文，此时记录回文长度和起始位置
                if (dp[i][j] && j - i + 1 > maxLen) {
                    maxLen = j - i + 1;
                    begin = i;
                }
            }
        }
        return s.substr(begin, maxLen);
    }
};


int main()
{
    // Sort sort;
    // vector<int> num = {2, 5, 1, 3, 4};
    // sort.quicksort(num, 0, num.size() - 1);
    // sort.bubblesort(num, 5);
    // sort.insertsort(num, 5);
    // for(auto i:num)
    //     cout << i << " ";
    // cout << endl;
    Solution su;
    string str = "abcde";
    string pe = su.longestPalindrome(str);
    cout << pe << endl;
    return 0;
}