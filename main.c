#include <stdio.h>

#define debug(x) printf("%s:%x\n", #x, x)

int main(int argc, char **argv)
{
    printf("Hello World!\n");
    debug(argc);
    for (int i = 0; i < argc; i++)
    {
        printf("%d: %s\n", i, argv[i]);
    }
}