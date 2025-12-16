#include <stdio.h>

void foo() {
    printf("inside foo\n");   // BP1
    printf("inside foo again\n"); // BP2
}

int main() {
    foo();
    return 0;
}

// gcc -O0 -no-pie ./tests/test2.c -o ./tests/test2      position independent executable
// objdump -d ./tests/test2
// break 0x401148
// break 0x401157
