#include <stdio.h>

int main() {
    for (int i = 0; i < 3; i++) {
        printf("i = %d\n", i);   // BP
    }
    return 0;
}



// gcc -O0 -no-pie ./tests/loop1.c -o ./tests/loop1      position independent executable
// objdump -d ./tests/loop1
// break 0x40114b