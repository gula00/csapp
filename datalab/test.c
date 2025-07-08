#include <stdio.h>

int tmin(void) {
  return 1 << 31;  // 返回最小的 32 位有符号整数
}

int main() {
  printf("tmin = %d\n", tmin());           // 打印为有符号十进制：-2147483648
  // printf("tmin = 0x%x\n", tmin());         // 打印为十六进制：0x80000000
  return 0;
}


// gcc -m32 test.c -o test  
// ./test 