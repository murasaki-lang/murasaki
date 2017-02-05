#include <stdio.h>
#include "MRSK.h"
#include "MEM.h"

int main(int argc, char **argv)
{
    MRSK_Interpreter *interpreter;
    FILE *fp;

    if (argc != 2) {
        fprintf(stderr, "usage:%s filename", argv[0]);
        exit(1);
    }

    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        fprintf(stderr, "%s not found \n", argv[1]);
        exit(1);
    }

    interpreter = MRSK_create_interpreter();
    MRSK_compile(interpreter, fp);
    MRSK_interpret(interpreter);
    MRSK_dispose_interpreter(interpreter);

    MEM_dump_blocks(stdout);

    return 0;
}
