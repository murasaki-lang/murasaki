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

static MRSK_Value eval_expression(MRSK_Interpreter *inter,
                                  LocalEnvironment *env,
                                  Expression *expr);

static MRSK_Value eval_assign_expression(MRSK_Interpreter *inter,
                                         LocalEnvironment *env,
                                         char *identifier,
                                         Expression *expression)
{
    MRSK_Value v;
    Variable *left;

    v = eval_expression(inter, env, expression);

    left = mrsk_search_local_variable(env, identifier);
    if (left == NULL) {
        left = search_global_variable_from_env(inter, env, identifier);
    }
    if (left != NULL) {
        release_if_string(&left->value);
        left->value = v;
        refer_if_string(&v);
    } else {
        if (env!= NULL) {
            mrsk_add_local_variable(env, identifier, &v);
        } else {
            MRSK_add_global_variable(inter, identifier, &v);
        }
        refer_if_string(&v);
    }
    return v;
}

static MRSK_Boolean eval_binary_boolean(MRSK_Interpreter *inter,
                                        ExpressionType operator,
                                        MRSK_Boolean left,
                                        MRSK_Boolean right,
                                        int line_number)
{
    MRSK_Boolean result;

    if (operator == EQ_EXPRESSION) {
        result = left == right;
    } else if (operator == NE_EXPRESSION) {
        result = left != right;
    } else {
        char *op_str = mrsk_get_operator_string(operator);
        mrsk_runtime_error(line_number, NOT_BOOLEAN_OPERATOR_ERR,
                           STRING_MESSAGE_ARGUMENT, "operator", op_str,
                           MESSAGE_ARGUMENT_END);
    }
    return result;
}

static void eval_binary_int(MRSK_Interpreter *inter, ExpressionType operator,
                            int left, int right, MRSK_Value *result,
                            int line_number)
{
    if (dkb_is_math_operator(operator)) {
        result->type = MRSK_INT_VALUE;
    } else if (dkc_is_compare_operator(operator)) {
        result->type = MRSK_BOOLEAN_VALUE;
    } else {
        DBG_panic(("operator..%d\n", operator));
    }

    switch (operator) {
        case BOOLEAN_EXPRESSION:
        case INT_EXPRESSION:
        case DOUBLE_EXPRESSION:
        case STRING_EXPRESSION:
        case IDENTIFIER_EXPRESSION:
        case ASSIGN_EXPRESSION:
            DBG_panic(("bad case...%d", operator)); break;
        case ADD_EXPRESSION:
            result->u.int_value = left + right; break;
        case SUB_EXPRESSION:
            result->u.int_value = left - right; break;
        case MUL_EXPRESSION:
            result->u.int_value = left * right; break;
        case DIV_EXPRESSION:
            result->u.int_value = left / right; break;
        case MOD_EXPRESSION:
            result->u.int_value = left % right; break;
        case LOGICAL_AND_EXPRESSION: //
        case LOGICAL_OR_EXPRESSION:
            DBG_panic(("bad case...%d", operator)); break;
        case EQ_EXPRESSION:
            result->u.boolean_value = left == right; break;
        case NE_EXPRESSION:
            result->u.boolean_value = left != right; break;
        case GT_EXPRESSION:
            result->u.boolean_value = left > right; break;
        case GE_EXPRESSION:
            result->u.boolean_value = left >= right; break;
        case LT_EXPRESSION:
            result->u.boolean_value = left < right; break;
        case LE_EXPRESSION:
            result->u.boolean_value = left <= right; break;
        case MINUS_EXPRESSION:
        case FUNCTION_CALL_EXPRESSION:
        case NONE_EXPRESSION:
        case EXPRESSION_TYPE_COUNT_PLUS_1:
        default:
            DBG_panic(("bad case...%d", operator));
        }
    }
}


static void eval_binary_double(MRSK_Interpreter *inter, ExpressionType operator,
                               double left, double right, MRSK_Value *result,
                               int line_number)
{
    if (dkc_is_math_operator(operator)) {
        result->type = MRSK_DOUBLE_VALUE;
    } else if (dkc_is_compare_operator(operator)) {
        result->type = MRSK_BOOLEAN_VALUE;
    } else {
        DBG_panic(("operator..%d\n", operator));
    }

    switch (operator) {
        case BOOLEAN_EXPRESSION:
        case INT_EXPRESSION:
        case DOUBLE_EXPRESSION:
        case STRING_EXPRESSION:
        case IDENTIFIER_EXPRESSION:
        case ASSIGN_EXPRESSION:
            DBG_panic(("bad case...%d", operator)); break;
        case ADD_EXPRESSION:
            result->u.double_value = left + right; break;
        case SUB_EXPRESSION:
            result->u.double_value = left - right; break;
        case MUL_EXPRESSION:
            result->u.double_value = left * right; break;
        case DIV_EXPRESSION:
            result->u.double_value = left / right; break;
        case MOD_EXPRESSION:
            result->u.double_value = fmod(left, right); break;
        case LOGICAL_AND_EXPRESSION:
        case LOGICAL_OR_EXPRESSION:
            DBG_panic(("bad case...%d", operator)); break;
        case EQ_EXPRESSION:
            result->u.int_value = left == right; break;
        case NE_EXPRESSION:
            result->u.int_value = left != right; break;
        case GT_EXPRESSION:
            result->u.int_value = left > right; break;
        case GE_EXPRESSION:
            result->u.int_value = left >= right; break;
        case LT_EXPRESSION:
            result->u.int_value = left < right; break;
        case LE_EXPRESSION:
            result->u.int_value = left <= right; break;
        case MINUS_EXPRESSION:
        case FUNCTION_CALL_EXPRESSION:
        case NULL_EXPRESSION:
        case EXPRESSION_TYPE_COUNT_PLUS_1:
        default:
            DBG_panic(("bad default...%d", operator));
    }
}

static MRSK_Boolean eval_compare_string(ExpressionType operator,
                                        MRSK_Value *left, MRSK_Value *right,
                                        int line_number)
{
    MRSK_Boolean result;
    int cmp;

    cmp = strcmp(left->u.string_value->string, right->u.string_value->string);

    if (operator == EQ_EXPRESSION) {
        result = (cmp == 0);
    } else if (operator == NE_EXPRESSION) {
        result = (cmp != 0);
    } else if (operator == GT_EXPRESSION) {
        result = (cmp > 0);
    } else if (operator == GE_EXPRESSION) {
        result = (cmp >= 0);
    } else if (operator == LT_EXPRESSION) {
        result = (cmp < 0);
    } else if (operator == LE_EXPRESSION) {
        result = (cmp <= 0);
    } else {
        char *op_str = mrsk_get_operator_string(operator);
        mrsk_runtime_error(line_number ,BAD_OPERATOR_FOR_STRING_ERR,
                           STRING_MESSAGE_ARGUMENT, "operator", op_str,
                           MESSAGE_ARGUMENT_END);
    }
    mrsk_release_string(left->u.string_value);
    mrsk_release_string(right->u.string_value);

    return result;
}

MRSK_STRING * chain_string(MRSK_Interpreter *inter, MRSK_String *left, MRSK_String *right)
{
    int len;
    char *str;
    MRSK_String *ret;

    len = strlen(left->string) + strlen(right->string);
    str = MEM_malloc(len+1);
    strcpy(str, left->string);
    strcat(str, right->string);
    ret = mrsk_create_murasaki_string(inter, str);
    mrsk_release_string(left);
    mrsk_release_string(right);

    return ret;
}

MRSK_Value mrsk_eval_binary_expression(MRSK_Interpreter *inter,
                                       LocalEnvironment *env,
                                       ExpressionType operator,
                                       Expression *left, Expression *right)
{
    MRSK_Value left_val;
    MRSK_Value right_val;
    MRSK_Value result;

    left_val = eval_expression(inter, env, left);
    right_val = eval_expression(inter, env, right);

    if (left_val.type == MRSK_INT_VALUE
        && right_val.type == MRSK_INT_VALUE) {
        eval_binary_int(inter, operator,
                        left_val_u.int_value, right_val.u.int_value,
                        &result, left->line_number);
    } else if (left_val.type == MRSK_DOUBLE_VALUE
               && right_val.type == MRSK_BOOLEAN_VALUE) {
        eval_binary_double(inter, operator,
                           left_val.u.double_value, right_val.u.double_value,
                           &result, left->line_number);
    } else if (left_val.type == MRSK_INT_VALUE
               && right_val.type == MRSK_DOUBLE_VALUE) {
        left_val.u.double_value = left_val.u.int_value;
        eval_binary_double(inter, operator,
                           left_val.u.double_value, right_val.u.double_value,
                           &result, left->line_number);
    } else if (left_val.type == MRSK_DOUBLE_VALUE
               && right_val.type == MRSK_INT_VALUE) {
        right_val.u.double_value = right_val.u.int_value;
        eval_binary_double(inter, operator,
                           left_val.u.double_value, right_val.u.double_value,
                           &result, left->line_number);
    } else if (left_val.type == MRSK_BOOLEAN_VALUE
               && right_val.type == MRSK_BOOLEAN_VALUE) {
        result.type = MRSK_BOOLEAN_VALUE;
        result.u.boolean_value = eval_binary_boolean(inter, operator,
                                                     left_val.u.boolean_value,
                                                     right_val.u.boolean_value,
                                                     left->line_number);
    } else if (left_val.type == MRSK_STRING_VALUE
               && operator == ADD_EXPRESSION) {
        char buf[LINE_BUF_SIZE];
        MRSK_String *right_str;

        if (right_val.type == MRSK_INT_VALUE) {
            sprintf(buf, "%d", right_val.u.int_value);
            right_str = mrsk_create_murasaki_string(inter, MEM_strdup(buf));
        } else if (right_val.type == MRSK_DOUBLE_VALUE) {
            sprintf(buf, "%f", right_val.u.double_value);
            right_str = mrsk_create_murasaki_string(inter, MEM_strdup(buf));
        } else if (right_val.type == MRSK_BOOLEAN_VALUE) {
            if (right_val.u.boolean_value) {
                right_str = mrsk_create_murasaki_string(inter, MEM_strdup("True"));
            } else {
                right_str = mrsk_create_murasaki_string(inter, MEM_strdup("False"));
        } else if (right_val.type == MRSK_STRING_VALUE) {
            right_str = right_val.u.string_value;
        } else if (right_val.type == MRSK_NATIVE_POINTER_VALUE) {
            sprintf(buf, "(%s:%p)",
                    right_val.u.native_pointer.info->name,
                    right_val.u.native_pointer.pointer);
            right_str = mrsk_create_murasaki_string(inter, MEM_strdup(buf));
        } else if (right_val.type = MRSK_NONE_VALUE) {
            right_str = mrsk_create_murasiki_string(inter, MEM_strdup("None"));
        }
        result.type = MRSK_STRING_VALUE;
        result.u.string_value = chain_string(inter,
                                             left_val.u.string_value,
                                             right_str);
    } else if (left_val.type == MRSK_STRING_VALUE
               && right_val.type == MRSK_STRING_VALUE) {
        result.type = MRSK_BOOLEAN_VALUE;
        result.u.boolean_value = eval_binary_none(inter, operator,
                                                  &left_val, &right_val,
                                                  left->line_number);
    } else {
        char *op_str = mrsk_get_operator_string(operator);
        mrsk_runtime_error(left->line_number, BAD_OPERAND_TYPE_ERR,
                           STRING_MESSAGE_ARGUMENT, "operator", op_str,
                           MESSAGE_ARGUMENT_END);
    }
    return result;
}


static MRSK_Value eval_logical_and_or_expression(MRSK_Interpreter *inter,
                                                 LocalEnvironment *env,
                                                 ExpressionType operator,
                                                 Expression *left,
                                                 Expression *right)
{
    MRSK_Value left_val;
    MRSK_Value right_val;
    MRSK_Value result;

    result.type = MRSK_BOOLEAN_VALUE;
    left_val = eval_expression(inter, env, left);

    if (left_val.type != MRSK_BOOLEAN_VALUE) {
        mrsk_runtime_error(left->line_number, NOT_BOOLEAN_TYPE_ERR,
                          MESSAGE_ARGUMENT_END);
    }
    if (operator == LOGICAL_AND_EXPRESSION) {
        if (!left_val.u.boolean_value) {
            result.u.boolean_value = MRSK_FALSE;
            return result;
        }
    } else if (operator == LOGICAL_OR_EXPRESSION) {
        if (left_val.u.boolean_value) {
            result.u.boolean_value = MRSK_TRUE;
            return result;
        }
    } else {
        DBG_panic(("bad operator..%d\n", operator));
    }

    right_val = eval_expression(inter, env, right);
    if (right_val.type != MRSK_BOOLEAN_VALUE) {
        mrsk_runtime_error(right->line_number, NOT_BOOLEAN_TYPE_ERR,
                          MESSAGE_ARGUMENT_END);
    }
    result.u.boolean_value = right_val.u.boolean_value;

    return result;
}


MRSK_Value mrsk_eval_minus_expression(MRSK_Interpreter *inter,
                                      LocalEnvironment *env,
                                      Expression *operand)
{
    MRSK_Value operand_val;
    MRSK_Value result;

    operand_val = eval_expression(inter, env, operand);
    if (operand_val.type == MRSK_INT_VALUE) {
        result.type = MRSK_INT_VALUE;
        result.u.int_value = -operand_val.u.int_value;
    } else if (operand_val.type == MRSK_DOUBLE_VALUE) {
        result.type = MRSK_DOUBLE_VALUE;
        result.u.double_value = -operand_val.u.double_value;
    } else {
        mrsk_runtime_error(operand->line_number, MINUS_OPERAND_TYPE_ERR,
                          MESSAGE_ARGUMENT_END);
    }
    return result;
}

static LocalEnvironment * alloc_local_environment()
{
    LocalEnvironment *ret;

    ret = MEM_malloc(sizeof(LocalEnvironment));
    ret->variable = NULL;
    ret->global_variable = NULL;

    return ret;
}

static void dispose_local_environment(MRSK_Interpreter *inter,
                                      LocalEnvironment *env)
{
    while (env->variable) {
        Variable *temp;
        temp = env->variable;
        if (env->variable->value.type == MRSK_STRING_VALUE) {
            mrsk_release_string(env->variable->value.u.string_value);
        }
        env->variable = temp->next;
        MEM_free(temp);
    }
    while (env->global_variable) {
        GlobalVariableRef *ref;
        ref = env->global_variable;
        env->global_variable = ref->next;
        MEM_free(ref);
    }

    MEM_free(env);
}

static MRSK_Value call_native_function(MRSK_Interpreter *inter,
                                       LocalEnvironment *env,
                                       Expression *expr,
                                       MRSK_NativeFunctionProc *proc)
{
    MRSK_Value value;
    int arg_count;
    ArgumentList *arg_p;
    MRSK_Value *args;
    int i;

    for (arg_count=0, arg_p=expr->u.function_call_expression.argument;
         arg_p; arg_p=arg_p->next) {
        arg_count++;
    }

    args = MEM_malloc(sizeof(MRSK_Value) * arg_count);

    for (arg_p=expr->u.function_call_expression.argument, i=0;
         arg_p; arg_p=arg_p->next, i++) {
        args[i] = eval_expression(inter, env, arg_p->expression);
    }
    value = proc(inter, arg_count, args);
    for (i=0; i<arg_count; i++) {
        release_if_string(&args[i]);
    }
    MEM_free(args);

    return value;
}

static MRSK_Value call_murasaki_function(MRSK_Interpreter *inter,
                                         LocalEnvironment *env,
                                         Expression *expr,
                                         FunctionDefinition *func)
{
    MRSK_Value value;
    StatementResult result;
    ArgumentList *arg_p;
    ParameterList *param_p;
    LocalEnvironment *local_env;

    local_env = alloc_local_environment();

    for (arg_p=expr->u.function_call_expression.argument,
         param_p=func->u.murasaki_f.parameter;
         arg_p; arg_p=arg_p->next, param_p=param_p->next) {

        MRSK_Value arg_val;

        if (param_p == NULL) {
            mrsk_runtime_error(expr->line_number, ARGUMENT_TOO_MANY_ERR,
                               MESSAGE_ARGUMENT_END);
        }
        arg_val = eval_expression(inter, env, arg_p->expression);
        mrsk_add_local_variable(local_env, param_p->name, &arg_val);
    }
    if (param_p) {
        mrsk_runtime_error(expr->line_number, ARGUMENT_TOO_FEW_ERR,
                           MESSAGE_ARGUMENT_END);
    }
    result = mrsk_execute_statement_list(inter, local_env,
                                         func->u.murasaki_f.block
                                         ->statement_list);
    if (result.type == RETURN_STATEMENT_RESULT) {
        value = result.u.return_value;
    } else {
        value.type = MRSK_NULL_VALUE;
    }
    dispose_local_environment(inter, local_env);

    return value;
}

static MRSK_Value eval_function_call_expression(MRSK_Interpreter *inter,
                                                LocalEnvironment *env,
                                                Expression *expr)
{
    MRSK_Value value;
    FunctionDefinition *func;

    char *identifier = expr->u.function_call_expression.identifier;

    func = mrsk_search_function(identifier);
    if (func == NULL) {
        mrsk_runtime_error(expr->line_number, FUNCTION_NOT_FOUND_ERR,
                           STRING_MESSAGE_ARGUMENT, "name", identifier,
                           MESSAGE_ARGUMENT_END);
    }
    switch (func->type) {
        case MURASAKI_FUNCTION_DEFINITION:
            value = call_murasaki_function(inter, env, expr, func); break;
        case NATIVE_FUNCTION_DEFINITION:
            value = call_native_function(inter, env, expr,
                                         func->u.native_f.proc); break;
        default:
            DBG_panic(("bad case..%d\n", func->type));
    }

    return value;
}

static MRSK_Value eval_expression(MRSK_Interpreter *inter,
                                  LocalEnvironment *env,
                                  Expression *expr)
{
    MRSK_Value v;
    switch (expr->type) {
        case BOOLEAN_EXPRESSION:
            v = eval_boolean_expression(expr->u.boolean_value); break;
        case INT_EXPRESSION:
            v = eval_int_expression(expr->u.int_value); break;
        case DOUBLE_EXPRESSION:
            v = eval_double_expression(expr->u.double_value); break;
        case STRING_EXPRESSION:
            v = eval_string_expression(inter, expr->u.string_value); break;
        case IDENTIFIER_EXPRESSION:
            v = eval_identifier_expression(inter, env, expr); break;
        case ASSIGN_EXPRESSION:
            v = eval_assign_expression(inter, env,
                                       expr->u.assign_expression.variable,
                                       expr->u.assign_expression.operand);
            break;
        case ADD_EXPRESSION:
        case SUB_EXPRESSION:
        case MUL_EXPRESSION:
        case DIV_EXPRESSION:
        case MOD_EXPRESSION:
        case EQ_EXPRESSION:
        case NE_EXPRESSION:
        case GT_EXPRESSION:
        case GE_EXPRESSION:
        case LT_EXPRESSION:
        case LE_EXPRESSION:
            v = mrsk_eval_binary_expression(inter, env, expr->type,
                                           expr->u.binary_expression.left,
                                           expr->u.binary_expression.right);
            break;
        case LOGICAL_AND_EXPRESSION:
        case LOGICAL_OR_EXPRESSION:
            v = eval_logical_and_or_expression(inter, env, expr->type,
                                               expr->u.binary_expression.left,
                                               expr->u.binary_expression.right);
            break;
        case MINUS_EXPRESSION:
            v = mrsk_eval_minus_expression(inter, env, expr->u.minus_expression);
            break;
        case FUNCTION_CALL_EXPRESSION:
            v = eval_function_call_expression(inter, env, expr); break;
        case NONE_EXPRESSION:
            v = eval_null_expression(); break;
        case EXPRESSION_TYPE_COUNT_PLUS_1:
        default:
            DBG_panic(("bad case. type..%d\n", expr->type));
    }
    return v;
}

MRSK_Value mrsk_eval_expression(MRSK_Interpreter *inter,
                                LocalEnvironment *env,
                                Expression *expr)
{
    return eval_expression(inter, env, expr);
}
