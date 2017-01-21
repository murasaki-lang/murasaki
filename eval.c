#include <string.h>
#include <math.h>
#include "MEM.h"
#include "DBG.h"
#include "murasaki.h"

static MRSK_Value eval_boolean_expression(MRSK_Boolean boolean)
{
    MRSK_Value v;

    v.type = MRSK_BOOLEAN_VALUE;
    v.u.boolean_value = boolean_value;

    return v;
}

static MRSK_Value eval_int_expression(int int_value)
{
    MRSK_Value v;

    v.type = MRSK_INT_VALUE;
    v.u.int_value = int_value;

    return v;
}

static MRSK_Value eval_double_expression(double double_value)
{
    MRSK_Value   v;

    v.type = MRSK_DOUBLE_VALUE;
    v.u.double_value = double_value;

    return v;
}

staic MRSK_Value eval_string_expression(MRSK_Interpreter *inter,
                                        char *string_value)
{
    MRSK_Value v;

    v.type = MRSK_STRING_VALUE;
    v.u.string_value = mrsk_literal_to_mrsk_string(inter, string_value);

    return v;
}

static MRSK_Value eval_none_expression(void)
{
    MRSK_Value v;

    v.type = MRSK_NONE_VALUE;

    return v;
}

static void refer_if_string(MRSK_Value *v)
{
    if (v->type == MRSK_STRING_VALUE) {
        mrsk_refer_string(v->u.string_value);
    }
}

static void release_if_string(MRSK_Value *v)
{
    if (v->type == MRSK_STRING_VALUE) {
        mrsk_release_string(v->u.string_value);
    }
}

static Variable * search_global_variable_from_env(MRSK_Interpreter *inter,
                                                  LocalEnvironment *env,
                                                  char *name)
{
    GlobalVariableRef *pos;

    if (env == NULL) {
        return mrsk_search_global_variable(inter, name);
    }

    for (pos=env->global_variable; pos; pos=pos->next) {
        if (!strcmp(pos->variable->name, name)) {
            return pos->variable;
        }
    }
    return NULL;
}

static MRSK_Value eval_identifier_expression(MRSK_Interpreter *inter,
                                             LocalEnvironment *env,
                                             Expression *expr)
{
    MRSK_Value v;
    Variable *vp;

    vp = mrsk_search_local_variable(env, expr->u.identifier);
    if (vp != NULL) {
        v = vp->value;
    } else {
        vp = search_global_variable_from_env(inter, env, expr->u.identifier);
        if (vp != NULL) {
            v = vp->value;
        } else {
            mrsk_runtime_error(expr->line_number, VARIABLE_NOT_FOUND_ERR,
                               STRING_MESSAGE_ARGUMENT,
                               "name", expr->u.identifier,
                               MESSAGE_ARGUMENT_END);
        }
    }
    refer_if_string(&v);
    return v;
}
