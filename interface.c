#include "MEM.h"
#include "DBG.h"
#define GLOBAL_VARIABLE_DEFINE
#include "murasaki.h"

static void add_native_functions(MRSK_Interpreter *inter)
{
    MRSK_add_native_function(inter, "print", mrsk_nv_print_proc);
    MRSK_add_native_function(inter, "fopen", mrsk_nv_fopen_proc);
    MRSK_add_native_function(inter, "fclose", mrsk_nv_fclose_proc);
    MRSK_add_native_function(inter, "fgets", mrsk_nv_fgets_proc);
    MRSK_add_native_function(inter, "fputs", mrsk_nv_fputs_proc);
    MRSK_add_native_function(inter, "new_array", mrsk_nv_new_array_proc);
}

MRSK_Interpreter * MRSK_create_interpreter(void)
{
    MEM_Storage storage;
    MRSK_Interpreter *interpreter;

    storage = MEM_open_storage(0);
    interpreter = MEM_storage_malloc(storage, sizeof(struct MRSK_Interpreter_tag));
    interpreter->interpreter_storage = storage;
    interpreter->execute_storage = NULL;
    interpreter->variable = NULL;
    interpreter->function_list = NULL;
    interpreter->statement_list = NULL;
    interpreter->current_line_number = 1;
    interpreter->stack.stack_alloc_size = 0;
    interpreter->stack.stack_pointer = 0;
    interpreter->stack.stack = MEM_malloc(sizeof(MRSK_Value) * STACK_ALLOC_SIZE);
    interpreter->heap.current_heap_size = 0;
    interpreter->heap.current_threshold = HEAP_THRESHOLD_SIZE;
    interpreter->heap.header = NULL;
    interpreter->top_environment = NULL;

    mrsk_set_current_interpreter(interpreter);
    add_native_functions(interpreter);

    return interpreter;
}

void MRSK_compile(MRSK_Interpreter *interpreter, FILE *fp)
{
    extern int yyparse(void);
    extern FILE *yyin;

    mrsk_set_current_interpreter(interpreter);

    yyin = fp;

    if (yyparse()) {
        fprintf(stderr, "Error ! ! !\n");
        exit(1);
    }
    mrsk_reset_string_literal_buffer();
}

void MRSK_interpret(MRSK_Interpreter *interpreter)
{
    interpreter->execute_storage = MEM_open_storage(0);
    mrsk_add_std_fp(interpreter);
    mrsk_execute_statement_list(interpreter, NULL, interpreter->statement_list);
    mrsk_garbage_collect(interpreter);
}

static void release_global_strings(MRSK_Interpreter *interpreter)
{
    while (interpreter->variable) {
        Variable *temp = interpreter->variable;
        interpreter->variable = temp->next;
    }
}

void MRSK_dispose_interpreter(MRSK_Interpreter *interpreter)
{
    release_global_strings(interpreter);

    if (interpreter->execute_storage) {
        MEM_dispose_storage(interpreter->execute_storage);
    }
    interpreter->variable = NULL;
    mrsk_garbage_collect(interpreter);
    DBG_assert(interpreter->heap.current_heap_size==0,
               ("%d bytes leaked.\n", interpreter->heap.current_heap_size));
    MEM_free(interpreter->stack.stack);
    MEM_dispose_storage(interpreter->interpreter_storage);
}

void MRSK_add_native_function(MRSK_Interpreter *interpreter, char *name,
                              MRSK_NativeFunctionProc *proc)
{
    FunctionDefinition *fd;

    fd = mrsk_malloc(sizeof(FunctionDefinition));
    fd->name = name;
    fd->type = NATIVE_FUNCTION_DEFINITION;
    fd->u.native_f.proc = proc;
    fd->next = interpreter->function_list;

    interpreter->function_list = fd;
}
