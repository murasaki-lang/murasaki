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
    NONE_EXPRESSION,
    EXPRESSION_TYPE_COUNT_PLUS_1
} ExpressionType;

typedef struct {
    Expression *left;
    Expression *right;
} BinaryExpression;

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
    } u
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

typedef struct
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

typedef struct {
    StatementList *statement_list;
} Block;

typedef struct ParameterList_tag {
    char *name;
    struct ParameterList_tag *next;
} ParameterList;

typedef enum {
    MURASAKI_FUNCTION_DEFINITION = 1;
    NATIVE_FUNCTION_DEFINITION
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

struct MRSK_Interpreter_tag {
    MEM_Storage interpreter_storage;
    MEM_Storage execute_storage;
    Variable *variable;
    FunctionDefinition *function_list;
    StatementList *statement_list;
    int current_line_number;
};
