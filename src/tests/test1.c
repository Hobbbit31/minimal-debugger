#include <stdio.h>

int main() {
    int a = 1;
    int b = 2;
    int c = a + b; // BP
    printf("%d\n", c);
    return 0;
}

// gcc -O0 -no-pie ./tests/test1.c -o ./tests/test1      position independent executable
// objdump -d ./tests/test1
// break 0x401158
