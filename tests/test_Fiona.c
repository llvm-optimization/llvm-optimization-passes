#include <stdio.h>

void f(int *a, int *d, int b, int c, int g, int e, int *h, int *i)
{
    *a = b + c + g;
    *d = b + c + e;
    *h = c + b;
    *i = c + g + b;
}

// clang -S -emit-llvm test.c
// opt -mem2reg -S test.ll -o testopt2.ll
int main()
{
    int a, b = 10, c = 10, d, g = 10, e = 10, h, i;

    f(&a, &d, b, c, g, e, &h, &i);

    printf("A = %d D= %d , H = %d I = %d\n", a, d, h, i);
    // A = 30 , D = 30, H = 20 , I = 30;
}
