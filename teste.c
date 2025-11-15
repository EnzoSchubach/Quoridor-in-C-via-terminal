#include <stdio.h>

int main(){
    typedef enum {o = 0, i = 1} teste;
    teste a = o;
    teste b = i;

    printf("%d%d", a, b);
}