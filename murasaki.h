#ifndef PRIVATE_MURASAKI_H_INCLUDED
#define PRIVATE_MURASAKI_H_INCLUDED
#include <stdio.h>
#include "MEM.h"
#include "MRSK.h"
#include "MRSK_dev.h"

#define smaller(a, b) ((a) < (b) ? (a) : (b))
#define larger(a, b) ((a) > (b) ? (a) : (b))

#define IDENTIFIER_TABLE_ALLOC_SIZE     (256)
#define STRING_LITERAL_TABLE_ALLOC_SIZE (256)
#define MESSAGE_ARGUMENT_MAX            (256)
#define LINE_BUF_SIZE                   (1024)
#define STACK_ALLOC_SIZE                (256)
#define ARRAY_ALLOC_SIZE                (256)
#define HEAP_THRESHOLD_SIZE             (1024 * 256)

typedef enum {
    PARSE_ERR = 1,
    CHARACTER_INVALID_ERR,
    FUNCTION_MULTIPLE_DEFINE_ERR,
    COMPILE_ERROR_COUNT_PLUS_1
} CompileError;

typedef enum {
    VARIABLE_NOT_FOUND_ERR = 1,
    FUNCTION_NOT_FOUND_ERR,
    ARGUMENT_TOO_MANY_ERR,
    ARGUMENT_TOO_FEW_ERR,
    NOT_BOOLEAN_TYPE_ERR,
    MINUS_OPERAND_TYPE_ERR,
    BAD_OPERAND_TYPE_ERR,
    NOT_BOOLEAN_OPERATOR_ERR,
    FOPEN_ARGUMENT_TYPE_ERR,
    FCLOSE_ARGUMENT_TYPE_ERR,
    FGETS_ARGUMENT_TYPE_ERR,
    FPUTS_ARGUMENT_TYPE_ERR,
    NOT_NONE_OPERATOR_ERR,
    DIVISION_BY_ZERO_ERR,
    GLOBAL_VARIABLE_NOT_FOUND_ERR,
    GLOBAL_STATEMENT_IN_TOPLEVEL_ERR,
    BAD_OPERATOR_FOR_STRING_ERR,
    NOT_LVALUE_ERR,
    INDEX_OPERAND_NOT_ARRAY_ERR,
    INDEX_OPERAND_NOT_INT_ERR,
    ARRAY_INDEX_OUT_OF_BOUNDS_ERR,
    NO_SUCH_METHOD_ERR,
    NEW_ARRAY_ARGUMENT_TYPE_ERR,
    INC_DEC_OPERAND_TYPE_ERR,
    ARRAY_RESIZE_ARGUMENT_ERR,
    RUNTIME_ERROR_COUNT_PLUS_1
} RuntimeError;

typedef enum {
    INT_MESSAGE_ARGUMENT = 1,
    DOUBLE_MESSAGE_ARGUMENT,
    STRING_MESSAGE_ARGUMENT,
    CHARACTER_MESSAGE_ARGUMENT,
    POINTER_MESSAGE_ARGUMENT,
    MESSAGE_ARGUMENT_END
} MessageArgumentType;

typedef struct {
    char *format;
} MessageFormat;

typedef struct Expression_tag Expression;

typedef enum {
    BOOLEAN_EXPRESSION = 1,
    INT_EXPRESSION,
    DOUBLE_EXPRESSION,
    STRING_EXPRESSION,
    IDENTIFIER_EXPRESSION,
    ASSIGN_EXPRESSION,
    ADD_EXPRESSION,
    SUB_EXPRESSION,
    MUL_EXPRESSION,
    DIV_EXPRESSION,
    MOD_EXPRESSION,
    EQ_EXPRESSION,
    NE_EXPRESSION,
    GT_EXPRESSION,
    GE_EXPRESSION,
    LT_EXPRESSION,
    LE_EXPRESSION,
    LOGICAL_AND_EXPRESSION,
    LOGICAL_OR_EXPRESSION,
    MINUS_EXPRESSION,
    FUNCTION_CALL_EXPRESSION,
    METHOD_CALL_EXPRESSION,
    NONE_EXPRESSION,
    ARRAY_EXPRESSION,
    INDEX_EXPRESSION,
    INCREMENT_EXPRESSION,
    DECREMENT_EXPRESSION,
    EXPRESSION_TYPE_COUNT_PLUS_1
} ExpressionType;

#define dkc_is_math_operator(operator) \
    ((operator) == ADD_EXPRESSION || (operator) == SUB_EXPRESSION \
     || (operator) == MUL_EXPRESSION || (operator) == DIV_EXPRESSION \
     || (operator) == MOD_EXPRESSION)

#define dkc_is_compare_operator(operator) \
    ((operator) == EQ_EXPRESSION || (operator) == NE_EXPRESSION \
     || (operator) == GT_EXPRESSION || (operator) == GE_EXPRESSION \
     || (operator) == LT_EXPRESSION || (operator) == LE_EXPRESSION)

#define dkc_is_logical_operator(operator) \
    ((operator) == LOGICAL_AND_EXPRESSION || (operator) == LOGICAL_OR_EXPRESSION)

typedef struct ArgumentList_tag {
    Expression *expression;
    struct ArgumentList_tag *next;
} ArgumentList;

typedef struct {
    Expression *left;
    Expression *operand;
} AssignExpression;

typedef struct {
    Expression *left;
    Expression *right;
} BinaryExpression;

typedef struct {
    char *identifier;
    ArgumentList *argument;
} FunctionCallExpression;

typedef struct ExpressionList_tag {
    Expression *expression;
    struct ExpressionList_tag *next;
} ExpressionList;

typedef struct {
    Expression *array;
    Expression *index;
} IndexExpression;

typedef struct {
    Expression *expression;
    char *identifier;
    ArgumentList *argument;
} MethodCallExpression;

typedef struct {
    Expression *operand;
} IncrementOrDecrement;

struct Expression_tag {
    ExpressionType type;
    int line_number;
    union {
        MRSK_Boolean boolean_value;
        int int_value;
        double double_value;
        char *string_value;
        char *identifier;
        AssignExpression assign_expression;
        BinaryExpression binary_expression;
        Expression *minus_expression;
        FunctionCallExpression function_call_expression;
        MethodCallExpression method_call_expression;
        ExpressionList *array_literal;
        IndexExpression index_expression;
        IncrementOrDecrement inc_dec;
    } u;
};

typedef struct Statement_tag Statement;

typedef struct StatementList_tag {
    Statement *statement;
    struct StatementList_tag *next;
} StatementList;

typedef struct {
    StatementList *statement_list;
} Block;

typedef struct IdentifierList_tag {
    char *name;
    struct IdentifierList_tag *next;
} IdentifierList;

typedef struct {
    IdentifierList *identifier_list;
} GlobalStatement;

typedef struct Elif_tag {
    Expression *condition;
    Block *block;
    struct Elif_tag *next;
} Elif;

typedef struct {
    Expression *condition;
    Block *then_block;
    Elif *elif_list;
    Block *else_block;
} IfStatement;

typedef struct {
    Expression *condition;
    Block *block;
} WhileStatement;

typedef struct {
    Expression *init;
    Expression *condition;
    Expression *post;
    Block *block;
} ForStatement;

typedef struct {
    Expression *return_value;
} ReturnStatement;

typedef enum {
    EXPRESSION_STATEMENT = 1,
    GLOBAL_STATEMENT,
    IF_STATEMENT,
    WHILE_STATEMENT,
    FOR_STATEMENT,
    RETURN_STATEMENT,
    BREAK_STATEMENT,
    CONTINUE_STATEMENT,
    STATEMENT_TYPE_COUNT_PLUS_1
} StatementType;

struct Statement_tag {
    StatementType type;
    int line_number;
    union {
        Expression *expression_s;
        GlobalStatement global_s;
        IfStatement if_s;
        WhileStatement while_s;
        ForStatement for_s;
        ReturnStatement return_s;
    } u;
};

typedef struct ParameterList_tag {
    char *name;
    struct ParameterList_tag *next;
} ParameterList;

typedef enum {
    MURASAKI_FUNCTION_DEFINITION = 1,
    NATIVE_FUNCTION_DEFINITION,
    FUNCTION_DEFINITION_TYPE_COUNT_PLUS_1
} FunctionDefinitionType;

typedef struct FunctionDefinition_tag {
    char *name;
    FunctionDefinitionType type;
    union {
        struct {
            ParameterList *parameter;
            Block *block;
         } murasaki_f;
        struct {
            MRSK_NativeFunctionProc *proc;
        } native_f;
    } u;
    struct FunctionDefinition_tag *next;
} FunctionDefinition;

typedef struct Variable_tag {
    char *name;
    MRSK_Value value;
    struct Variable_tag *next;
} Variable;

typedef enum {
    NORMAL_STATEMENT_RESULT = 1,
    RETURN_STATEMENT_RESULT,
    BREAK_STATEMENT_RESULT,
    CONTINUE_STATEMENT_RESULT,
    STATEMENT_RESULT_TYPE_COUNT_PLUS_1
} StatementResultType;

typedef struct {
    StatementResultType type;
    union {
        MRSK_Value return_value;
    } u;
} StatementResult;

typedef struct GlobalVariableRef_tag {
    Variable *variable;
    struct GlobalVariableRef_tag *next;
} GlobalVariableRef;

typedef struct RefInNativeFunc_tag {
    MRSK_Object *object;
    struct RefInNativeFunc_tag *next;
} RefInNativeFunc;

struct MRSK_LocalEnvironment_tag {
    Variable *variable;
    GlobalVariableRef *global_variable;
    RefInNativeFunc *ref_in_native_method;
    struct MRSK_LocalEnvironment_tag *next;
};

typedef struct {
    int stack_alloc_size;
    int stack_pointer;
    MRSK_Value *stack;
} Stack;

typedef struct {
    int current_heap_size;
    int current_threshold;
    MRSK_Object *header;
} Heap;

struct MRSK_Interpreter_tag {
    MEM_Storage interpreter_storage;
    MEM_Storage execute_storage;
    Variable *variable;
    FunctionDefinition *function_list;
    StatementList *statement_list;
    int current_line_number;
    Stack stack;
    Heap heap;
    MRSK_LocalEnvironment *top_environment;
};

struct MRSK_Array_tag {
    int size;
    int alloc_size;
    MRSK_Value *array;
};

struct MRSK_String_tag {
    MRSK_Boolean is_literal;
    char *string;
};

typedef enum {
    ARRAY_OBJECT = 1,
    STRING_OBJECT,
    OBJECT_TYPE_COUNT_PLUS_1
} ObjectType;

#define dkc_is_object_value(type) \
    ((type) == MRSK_STRING_VALUE || (type == MRSK_ARRAY_VALUE))

struct MRSK_Object_tag {
    ObjectType type;
    unsigned int marked:1;
    union {
        MRSK_Array array;
        MRSK_String string;
    } u;
    struct MRSK_Object_tag *prev;
    struct MRSK_Object_tag *next;
};

typedef struct {
    char *string;
} VString;


/* create.c */
void mrsk_function_define(char *identifier, ParameterList *parameter_list,
                         Block *block);
ParameterList *mrsk_create_parameter(char *identifier);
ParameterList *mrsk_chain_parameter(ParameterList *list,
                                   char *identifier);
ArgumentList *mrsk_create_argument_list(Expression *expression);
ArgumentList *mrsk_chain_argument_list(ArgumentList *list, Expression *expr);
ExpressionList *mrsk_create_expression_list(Expression *expression);
ExpressionList *mrsk_chain_expression_list(ExpressionList *list,
                                        Expression *expr);
StatementList *mrsk_create_statement_list(Statement *statement);
StatementList *mrsk_chain_statement_list(StatementList *list,
                                        Statement *statement);
Expression *mrsk_alloc_expression(ExpressionType type);
Expression *mrsk_create_assign_expression(Expression *left,
                                         Expression *operand);
Expression *mrsk_create_binary_expression(ExpressionType operator,
                                         Expression *left,
                                         Expression *right);
Expression *mrsk_create_minus_expression(Expression *operand);
Expression *mrsk_create_index_expression(Expression *array, Expression *index);
Expression *mrsk_create_incdec_expression(Expression *operand,
                                         ExpressionType inc_or_dec);
Expression *mrsk_create_identifier_expression(char *identifier);
Expression *mrsk_create_function_call_expression(char *func_name,
                                                ArgumentList *argument);
Expression *mrsk_create_method_call_expression(Expression *expression,
                                              char *method_name,
                                              ArgumentList *argument);
Expression *mrsk_create_boolean_expression(MRSK_Boolean value);
Expression *mrsk_create_none_expression(void);
Statement *mrsk_create_global_statement(IdentifierList *identifier_list);
IdentifierList *mrsk_create_global_identifier(char *identifier);
IdentifierList *mrsk_chain_identifier(IdentifierList *list, char *identifier);
Expression *mrsk_create_array_expression(ExpressionList *list);

Statement *mrsk_create_if_statement(Expression *condition,
                                    Block *then_block, Elif *elif_list,
                                    Block *else_block);
Elif *mrsk_chain_elif_list(Elif *list, Elif *add);
Elif *mrsk_create_elif(Expression *expr, Block *block);
Statement *mrsk_create_while_statement(Expression *condition, Block *block);
Statement *mrsk_create_for_statement(Expression *init, Expression *cond,
                                    Expression *post, Block *block);
Block *mrsk_create_block(StatementList *statement_list);
Statement *mrsk_create_expression_statement(Expression *expression);
Statement *mrsk_create_return_statement(Expression *expression);
Statement *mrsk_create_break_statement(void);
Statement *mrsk_create_continue_statement(void);

/* string.c */
char *mrsk_create_identifier(char *str);
void mrsk_open_string_literal(void);
void mrsk_add_string_literal(int letter);
void mrsk_reset_string_literal_buffer(void);
char *mrsk_close_string_literal(void);

/* execute.c */
StatementResult
mrsk_execute_statement_list(MRSK_Interpreter *inter,
                           MRSK_LocalEnvironment *env, StatementList *list);

/* eval.c */
MRSK_Value mrsk_eval_binary_expression(MRSK_Interpreter *inter,
                                     MRSK_LocalEnvironment *env,
                                     ExpressionType operator,
                                     Expression *left, Expression *right);
MRSK_Value mrsk_eval_minus_expression(MRSK_Interpreter *inter,
                                    MRSK_LocalEnvironment *env,
                                    Expression *operand);
MRSK_Value mrsk_eval_expression(MRSK_Interpreter *inter,
                              MRSK_LocalEnvironment *env, Expression *expr);

/* heap.c */
MRSK_Object *mrsk_literal_to_mrsk_string(MRSK_Interpreter *inter, char *str);
MRSK_Object *mrsk_create_murasaki_string_i(MRSK_Interpreter *inter, char *str);
MRSK_Object *mrsk_create_array_i(MRSK_Interpreter *inter, int size);
void mrsk_array_add(MRSK_Interpreter *inter, MRSK_Object *obj, MRSK_Value v);
void
mrsk_array_resize(MRSK_Interpreter *inter, MRSK_Object *obj, int new_size);
void mrsk_garbage_collect(MRSK_Interpreter *inter);


/* util.c */
MRSK_Interpreter *mrsk_get_current_interpreter(void);
void mrsk_set_current_interpreter(MRSK_Interpreter *inter);
void *mrsk_malloc(size_t size);
void *mrsk_execute_malloc(MRSK_Interpreter *inter, size_t size);
Variable *mrsk_search_local_variable(MRSK_LocalEnvironment *env,
                                    char *identifier);
Variable *
mrsk_search_global_variable(MRSK_Interpreter *inter, char *identifier);
Variable *mrsk_add_local_variable(MRSK_LocalEnvironment *env, char *identifier);
Variable *mrsk_add_global_variable(MRSK_Interpreter *inter, char *identifier);
MRSK_NativeFunctionProc *
mrsk_search_native_function(MRSK_Interpreter *inter, char *name);
FunctionDefinition *mrsk_search_function(char *name);
char *mrsk_get_operator_string(ExpressionType type);
void mrsk_vstr_clear(VString *v);
void mrsk_vstr_append_string(VString *v, char *str);
void mrsk_vstr_append_character(VString *v, int ch);

/* error.c */
void mrsk_compile_error(CompileError id, ...);
void mrsk_runtime_error(int line_number, RuntimeError id, ...);

/* native.c */
MRSK_Value mrsk_nv_print_proc(MRSK_Interpreter *interpreter,
                            MRSK_LocalEnvironment *env,
                            int arg_count, MRSK_Value *args);
MRSK_Value mrsk_nv_fopen_proc(MRSK_Interpreter *interpreter,
                            MRSK_LocalEnvironment *env,
                            int arg_count, MRSK_Value *args);
MRSK_Value mrsk_nv_fclose_proc(MRSK_Interpreter *interpreter,
                            MRSK_LocalEnvironment *env,
                             int arg_count, MRSK_Value *args);
MRSK_Value mrsk_nv_fgets_proc(MRSK_Interpreter *interpreter,
                            MRSK_LocalEnvironment *env,
                            int arg_count, MRSK_Value *args);
MRSK_Value mrsk_nv_fputs_proc(MRSK_Interpreter *interpreter,
                            MRSK_LocalEnvironment *env,
                            int arg_count, MRSK_Value *args);
MRSK_Value mrsk_nv_new_array_proc(MRSK_Interpreter *interpreter,
                                MRSK_LocalEnvironment *env,
                                int arg_count, MRSK_Value *args);
void mrsk_add_std_fp(MRSK_Interpreter *inter);


#endif /* PRIVATE_MURASAKI_H_INCLUDED */
