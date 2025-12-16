#include <stdio.h>

int main() {
    int sum = 0;

    for (int i = 1; i <= 3; i++) {
        sum += i;          // BP
        printf("curr sum = %d\n" , sum);
    }

    printf("sum = %d\n", sum);
    return 0;
}


// gcc -O0 -no-pie ./tests/loop2.c -o ./tests/loop2      position independent executable
// objdump -d ./tests/loop2
// break 0x401155