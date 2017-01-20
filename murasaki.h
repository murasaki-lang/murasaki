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
