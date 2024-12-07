#include <stdio.h>
#include <string.h>

typedef struct entity
{
    int a;
    int b;
} entity;

int main(int argc, char** argv) 
{
    entity e;
    e.a = 10;

    int a = 0;
    memcpy(&a, &e.a, 4);
    printf("a = %i", a);
}