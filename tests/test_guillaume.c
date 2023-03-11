#include <stdio.h>

void f(int *a, int *b, int *c)
{
    *a = 1;
    *b = *a + 1;
    *c = *b + *a;
}
int main(int argc, char const *argv[])
{
    int a, b, c;
    f(&a, &b, &c);
    printf("%d %d %d\n", a, b, c);
    return 0;
}
