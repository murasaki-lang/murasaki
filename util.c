#include <stdio.h>
#include <string.h>
#include "MEM.h"
#include "DBG.h"
#include "murasaki.h"

static MRSK_Interpreter *st_current_interpreter;

MRSK_Interpreter * mrsk_get_current_interpreter(void)
{
    return st_current_interpreter;
}

void mrsk_set_current_interpreter(MRSK_Interpreter *inter)
{
    st_current_interpreter = inter;
}

FunctionDefinition * mrsk_search_function(char *name)
{
    FunctionDefinition *pos;
    MRSK_Interpreter *inter;

    inter = mrsk_get_current_interpreter();
    for (pos=inter->function_list; pos; pos=pos->next) {
        if (!strcmp(pos->name, name)) {
            break;
        }
    }
    return pos;
}

void * mrsk_malloc(size_t size)
{
    void *p;
    MRSK_Interpreter *inter;

    inter = mrsk_get_current_interpreter();
    p = MEM_storage_malloc(inter->interpreter_storage, size);

    return p;
}

void * mrsk_execute_malloc(MRSK_Interpreter *inter, size_t size)
{
    void *p;

    p = MEM_storage_malloc(inter->execute_storage, size);

    return p;
}

Variable * mrsk_search_local_variable(MRSK_LocalEnvironment *env, char *identifier)
{
    Variable *pos;

    if (env == NULL) {
        return NULL;
    }

    for (pos=env->variable; pos; pos=pos->next) {
        if (!strcmp(pos->name, identifier)) {
            break;
        }
    }

    if (pos == NULL) {
        return NULL;
    } else {
        return pos;
    }
}

Variable * mrsk_search_global_variable(MRSK_Interpreter *inter, char *identifier)
{
    Variable *pos;

    for (pos=inter->variable; pos; pos=pos->next) {
        if (!strcmp(pos->name, identifier)) {
            return pos;
        }
    }

    return NULL;
}

Variable * mrsk_add_local_variable(MRSK_LocalEnvironment *env, char *identifier)
{
    Variable *new_variable;

    new_variable = MEM_malloc(sizeof(Variable));
    new_variable->name = identifier;
    new_variable->next = env->variable;
    env->variable = new_variable;

    return new_variable;
}

Variable * mrsk_add_global_variable(MRSK_Interpreter *inter, char *identifier)
{
    Variable *new_variable;

    new_variable = mrsk_execute_malloc(inter, sizeof(Variable));
    new_variable->name = mrsk_execute_malloc(inter, strlen(identifier)+1);
    strcpy(new_variable->name, identifier);
    new_variable->next = inter->variable;
    inter->variable = new_variable;

    return new_variable;
}

void MRSK_add_global_variable(MRSK_Interpreter *inter, char *identifier, MRSK_Value *value)
{
    Variable *new_variable;

    new_variable = mrsk_add_global_variable(inter, identifier);
    new_variable->value = *value;

}

char * mrsk_get_operator_string(ExpressionType type)
{
    char *str;

    switch (type) {
        case BOOLEAN_EXPRESSION:    /* FALLTHRU */
        case INT_EXPRESSION:        /* FALLTHRU */
        case DOUBLE_EXPRESSION:     /* FALLTHRU */
        case STRING_EXPRESSION:     /* FALLTHRU */
        case IDENTIFIER_EXPRESSION:
            DBG_panic(("bad expression type..%d\n", type));
            break;
        case ASSIGN_EXPRESSION:
            str = "=";
            break;
        case ADD_EXPRESSION:
            str = "+";
            break;
        case SUB_EXPRESSION:
            str = "-";
            break;
        case MUL_EXPRESSION:
            str = "*";
            break;
        case DIV_EXPRESSION:
            str = "/";
            break;
        case MOD_EXPRESSION:
            str = "%";
            break;
        case EQ_EXPRESSION:
            str = "==";
            break;
        case NE_EXPRESSION:
            str = "!=";
            break;
        case GT_EXPRESSION:
            str = "<";
            break;
        case GE_EXPRESSION:
            str = "<=";
            break;
        case LT_EXPRESSION:
            str = ">";
            break;
        case LE_EXPRESSION:
            str = ">=";
            break;
        case LOGICAL_AND_EXPRESSION:
            str = "and";
            break;
        case LOGICAL_OR_EXPRESSION:
            str = "or";
            break;
        case MINUS_EXPRESSION:
            str = "-";
            break;
        case FUNCTION_CALL_EXPRESSION:  /* FALLTHRU */
        case METHOD_CALL_EXPRESSION:  /* FALLTHRU */
        case NONE_EXPRESSION:  /* FALLTHRU */
        case ARRAY_EXPRESSION:  /* FALLTHRU */
        case INDEX_EXPRESSION:  /* FALLTHRU */
        case INCREMENT_EXPRESSION:  /* FALLTHRU */
        case DECREMENT_EXPRESSION:  /* FALLTHRU */
        case EXPRESSION_TYPE_COUNT_PLUS_1:
        default:
            DBG_panic(("bad expression type..%d\n", type));
        }

    return str;
}

void mrsk_vstr_clear(VString *v)
{
    v->string = NULL;
}

static int my_strlen(char *str)
{
    if (str == NULL) {
        return 0;
    }
    return strlen(str);
}

void mrsk_vstr_append_string(VString *v, char *str)
{
    int new_size;
    int old_len;

    old_len = my_strlen(v->string);
    new_size = old_len + strlen(str) + 1;
    v->string = MEM_realloc(v->string, new_size);
    strcpy(&v->string[old_len], str);
}

void mrsk_vstr_append_character(VString *v, int ch)
{
    int current_len;

    current_len = my_strlen(v->string);
    v->string = MEM_realloc(v->string, current_len + 2);
    v->string[current_len] = ch;
    v->string[current_len+1] = '\0';
}

char * MRSK_value_to_string(MRSK_Value *value)
{
    VString vstr;
    char buf[LINE_BUF_SIZE];
    int i;

    mrsk_vstr_clear(&vstr);

    switch(value->type) {
        case MRSK_BOOLEAN_VALUE:
            if (value->u.boolean_value) {
                mrsk_vstr_append_string(&vstr, "true");
            } else {
                mrsk_vstr_append_string(&vstr, "false");
            }
            break;
        case MRSK_INT_VALUE:
            sprintf(buf, "%d", value->u.int_value);
            mrsk_vstr_append_string(&vstr, buf);
            break;
        case MRSK_DOUBLE_VALUE:
            sprintf(buf, "%f", value->u.double_value);
            mrsk_vstr_append_string(&vstr, buf);
            break;
        case MRSK_STRING_VALUE:
            mrsk_vstr_append_string(&vstr, value->u.object->u.string.string);
            break;
        case MRSK_NATIVE_POINTER_VALUE:
            sprintf(buf, "(%s:%p)",
                    value->u.native_pointer.info->name,
                    value->u.native_pointer.pointer);
            mrsk_vstr_append_string(&vstr, buf);
            break;
        case MRSK_NONE_VALUE:
            mrsk_vstr_append_string(&vstr, "(");
            for (i=0; i<value->u.object->u.array.size; i++) {
                char *new_str;
                if (i > 0) {
                    mrsk_vstr_append_string(&vstr, ", ");
                }
                new_str = MRSK_value_to_string(&value->u.object->u.array.array[i]);
                mrsk_vstr_append_string(&vstr, new_str);
                MEM_free(new_str);
            }
            mrsk_vstr_append_string(&vstr, ")");
            break;
        default:
            DBG_panic(("value-type..%d\n", value->type));
        }

        return vstr.string;
}
