#ifndef PUBLIC_MRSK_H_INCLUDED
#define PUBLIC_MRSK_H_INCLUDED
#include <stdio.h>

typedef struct MRSK_Interpreter_tag MRSK_Interpreter;

MRSK_Interpreter *MRSK_create_interpreter(void);
void MRSK_compile(MRSK_Interpreter *interpreter, FILE *fp);
void MRSK_interpret(MRSK_Interpreter *interpreter);
void MRSK_dispose_interpreter(MRSK_Interpreter *interpreter);

#endif
