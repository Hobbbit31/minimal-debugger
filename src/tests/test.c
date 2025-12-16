#include <stdio.h>
int pan = 0;

int main() {
    pan = 1;  //BP1
    printf("%d\n", pan);
    pan = 2; //BP2
    printf("%d\n", pan);
    return 0;
}


// gcc -O0 -no-pie ./tests/test.c -o ./tests/test
// objdump -d ./tests/test
// break 0x40113e   on pan =1
// break 0x401164   on pan =2