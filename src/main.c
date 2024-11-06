#include "common/flags.h"

#include <stdio.h>

static int ttl;

int main(int argc, char* argv[]) {
    flags_t parser;
    flags_int(&parser, &ttl, 't', 30);

    printf("Hello, World!\n");
    return 0;
}