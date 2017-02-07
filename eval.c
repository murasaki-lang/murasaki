#include <string.h>
#include <math.h>
#include "MEM.h"
#include "DBG.h"
#include "murasaki.h"

static void push_value(MRSK_Interpreter *inter, MRSK_Value *value)
{
    DBG_assert(inter->stack.stack_pointer <= inter->stack.stack_alloc_size,
               ("stack_pointer..%d, stack_alloc_size..%d\n",
                inter->stack.stack_pointer, inter->stack.stack_alloc_size));

    if (inter->stack.stack_pointer == inter->stack.stack_alloc_size) {
        inter->stack.stack_alloc_size += STACK_ALLOC_SIZE;
        inter->stack.stack = MEM_realloc(inter->stack.stack,
                                         sizeof(MRSK_Value) * inter->stack.stack_alloc_size);
    }
    inter->stack.stack[inter->stack.stack_pointer] = *value;
    inter->stack.stack_pointer++;
}

static MRSK_Value pop_value(MRSK_Interpreter *inter)
{
    MRSK_Value ret;

    ret = inter->stack.stack[inter->stack.stack_pointer-1];
    inter->stack.stack_pointer--;

    return ret;
}

static MRSK_Value * peek_stack(MRSK_Interpreter *inter, int index)
{
    return &inter->stack.stack[inter->stack.stack_pointer - index - 1];
}

static void shrink_stack(MRSK_Interpreter *inter, int shrink_size)
{
    inter->stack.stack_pointer -= shrink_size;
}

static void eval_boolean_expression(MRSK_Interpreter *inter, MRSK_Boolean boolean_value)
{
    MRSK_Value v;

    v.type = MRSK_BOOLEAN_VALUE;
    v.u.boolean_value = boolean_value;

    push_value(inter, &v);
}

static void eval_int_expression(MRSK_Interpreter *inter, int int_value)
{
    MRSK_Value v;

    v.type = MRSK_INT_VALUE;
    v.u.int_value = int_value;

    push_value(inter, &v);
}

static void eval_double_expression(MRSK_Interpreter *inter, double double_value)
{
    MRSK_Value   v;

    v.type = MRSK_DOUBLE_VALUE;
    v.u.double_value = double_value;

    push_value(inter, &v);
}

static void eval_string_expression(MRSK_Interpreter *inter, char *string_value)
{
    MRSK_Value v;

    v.type = MRSK_STRING_VALUE;
    v.u.object = mrsk_literal_to_mrsk_string(inter, string_value);

    push_value(inter, &v);
}

static void eval_none_expression(MRSK_Interpreter *inter)
{
    MRSK_Value v;

    v.type = MRSK_NONE_VALUE;

    push_value(inter, &v);
}

static Variable * search_global_variable_from_env(MRSK_Interpreter *inter,
                                                  MRSK_LocalEnvironment *env,
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

static void eval_identifier_expression(MRSK_Interpreter *inter,
                                       MRSK_LocalEnvironment *env,
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
    push_value(inter, &v);
}

static void eval_expression(MRSK_Interpreter *inter, MRSK_LocalEnvironment *env,
                            Expression *expr);

static MRSK_Value * get_identifier_lvalue(MRSK_Interpreter *inter, MRSK_LocalEnvironment *env,
                                          char *identifier)
{
    Variable *new_var;
    Variable *left;

    left = mrsk_search_local_variable(env, identifier);
    if (left == NULL) {
        left = search_global_variable_from_env(inter, env, identifier);
    }
    if (left != NULL) {
        return &left->value;
    }
    if (env != NULL) {
        new_var = mrsk_add_local_variable(env, identifier);
        left = new_var;
    } else {
        new_var = mrsk_add_global_variable(inter, identifier);
        left = new_var;
    }

    return &left->value;
}

MRSK_Value * get_array_element_lvalue(MRSK_Interpreter *inter, MRSK_LocalEnvironment *env,
                                      Expression *expr)
{
    MRSK_Value array;
    MRSK_Value index;

    eval_expression(inter, env, expr->u.index_expression.array);
    eval_expression(inter, env, expr->u.index_expression.index);
    index = pop_value(inter);
    array = pop_value(inter);

    if (array.type != MRSK_ARRAY_VALUE) {
        mrsk_runtime_error(expr->line_number, INDEX_OPERAND_NOT_ARRAY_ERR,
                           MESSAGE_ARGUMENT_END);
    }
    if (index.type != MRSK_INT_VALUE) {
        mrsk_runtime_error(expr->line_number, INDEX_OPERAND_NOT_INT_ERR,
                           MESSAGE_ARGUMENT_END);
    }

    if (index.u.int_value < 0
        || index.u.int_value >= array.u.object->u.array.size) {
        mrsk_runtime_error(expr->line_number, ARRAY_INDEX_OUT_OF_BOUNDS_ERR,
                           INT_MESSAGE_ARGUMENT,
                           "size", array.u.object->u.array.size,
                           INT_MESSAGE_ARGUMENT, "index", index.u.int_value,
                           MESSAGE_ARGUMENT_END);
    }
    return &array.u.object->u.array.array[index.u.int_value];
}

MRSK_Value * get_lvalue(MRSK_Interpreter *inter, MRSK_LocalEnvironment *env,
                        Expression *expr)
{
    MRSK_Value *dest;

    if (expr->type == IDENTIFIER_EXPRESSION) {
        dest = get_identifier_lvalue(inter, env, expr->u.identifier);
    } else if (expr->type == INDEX_EXPRESSION) {
        dest = get_array_element_lvalue(inter, env, expr);
    } else {
        mrsk_runtime_error(expr->line_number, NOT_LVALUE_ERR,
                           MESSAGE_ARGUMENT_END);
    }

    return dest;
}

static void eval_assign_expression(MRSK_Interpreter *inter, MRSK_LocalEnvironment *env,
                                   Expression *left, Expression *expression)
{
    MRSK_Value *src;
    MRSK_Value *dest;

    eval_expression(inter, env, expression);
    src = peek_stack(inter, 0);

    dest = get_lvalue(inter, env, left);
    *dest = *src;
}

static MRSK_Boolean eval_binary_boolean(MRSK_Interpreter *inter, ExpressionType operator,
                                        MRSK_Boolean left, MRSK_Boolean right,
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
    if (dkc_is_math_operator(operator)) {
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
        case LOGICAL_AND_EXPRESSION:
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
        case METHOD_CALL_EXPRESSION:
        case NONE_EXPRESSION:
        case ARRAY_EXPRESSION:
        case INDEX_EXPRESSION:
        case INCREMENT_EXPRESSION:
        case DECREMENT_EXPRESSION:
        case EXPRESSION_TYPE_COUNT_PLUS_1:
        default:
            DBG_panic(("bad case...%d", operator));

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
        case METHOD_CALL_EXPRESSION:
        case NONE_EXPRESSION:
        case ARRAY_EXPRESSION:
        case INDEX_EXPRESSION:
        case INCREMENT_EXPRESSION:
        case DECREMENT_EXPRESSION:
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

    cmp = strcmp(left->u.object->u.string.string, right->u.object->u.string.string);

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

    return result;
}


static MRSK_Boolean eval_binary_null(MRSK_Interpreter *inter,
                                    ExpressionType operator,
                                    MRSK_Value *left, MRSK_Value *right,
                                    int line_number)
{
    MRSK_Boolean result;

    if (operator == EQ_EXPRESSION) {
        result = left->type == MRSK_NONE_VALUE
                 && right->type == MRSK_NONE_VALUE;
    } else if (operator == NE_EXPRESSION) {
        result =  !(left->type == MRSK_NONE_VALUE
                    && right->type == MRSK_NONE_VALUE);
    } else {
        char *op_str = mrsk_get_operator_string(operator);
        mrsk_runtime_error(line_number, NOT_NONE_OPERATOR_ERR,
                           STRING_MESSAGE_ARGUMENT, "operator", op_str,
                           MESSAGE_ARGUMENT_END);
    }

    return result;
}

void chain_string(MRSK_Interpreter *inter, MRSK_Value *left,
                  MRSK_Value *right, MRSK_Value *result)
{
    char *right_str;
    MRSK_Object *right_obj;
    int len;
    char *str;

    right_str = MRSK_value_to_string(right);
    right_obj = mrsk_create_murasaki_string_i(inter, right_str);

    result->type = MRSK_STRING_VALUE;
    len = strlen(left->u.object->u.string.string)
        + strlen(right_obj->u.string.string);
    str = MEM_malloc(len + 1);
    strcpy(str, left->u.object->u.string.string);
    strcat(str, right_obj->u.string.string);
    result->u.object = mrsk_create_murasaki_string_i(inter, str);
}


static void eval_binary_expression(MRSK_Interpreter *inter,
                                   MRSK_LocalEnvironment *env,
                                   ExpressionType operator,
                                   Expression *left, Expression *right)
{
    MRSK_Value *left_val;
    MRSK_Value *right_val;
    MRSK_Value result;

    eval_expression(inter, env, left);
    eval_expression(inter, env, right);
    left_val = peek_stack(inter, 1);
    right_val = peek_stack(inter, 0);

    if (left_val->type == MRSK_INT_VALUE
        && right_val->type == MRSK_INT_VALUE) {
        eval_binary_int(inter, operator,
                        left_val->u.int_value, right_val->u.int_value,
                        &result, left->line_number);
    } else if (left_val->type == MRSK_DOUBLE_VALUE
               && right_val->type == MRSK_DOUBLE_VALUE) {
        eval_binary_double(inter, operator,
                           left_val->u.double_value, right_val->u.double_value,
                           &result, left->line_number);
    } else if (left_val->type == MRSK_INT_VALUE
               && right_val->type == MRSK_DOUBLE_VALUE) {
        eval_binary_double(inter, operator,
                           (double)left_val->u.int_value,
                           right_val->u.double_value,
                           &result, left->line_number);
    } else if (left_val->type == MRSK_DOUBLE_VALUE
               && right_val->type == MRSK_INT_VALUE) {
        eval_binary_double(inter, operator,
                           left_val->u.double_value,
                           (double)right_val->u.int_value,
                           &result, left->line_number);
    } else if (left_val->type == MRSK_BOOLEAN_VALUE
               && right_val->type == MRSK_BOOLEAN_VALUE) {
        result.type = MRSK_BOOLEAN_VALUE;
        result.u.boolean_value
            = eval_binary_boolean(inter, operator,
                                  left_val->u.boolean_value,
                                  right_val->u.boolean_value,
                                  left->line_number);
    } else if (left_val->type == MRSK_STRING_VALUE
               && operator == ADD_EXPRESSION) {
        chain_string(inter, left_val, right_val, &result);
    } else if (left_val->type == MRSK_STRING_VALUE
               && right_val->type == MRSK_STRING_VALUE) {
        result.type = MRSK_BOOLEAN_VALUE;
        result.u.boolean_value
            = eval_compare_string(operator, left_val, right_val,
                                  left->line_number);
    } else if (left_val->type == MRSK_NONE_VALUE
               || right_val->type == MRSK_NONE_VALUE) {
        result.type = MRSK_BOOLEAN_VALUE;
        result.u.boolean_value
            = eval_binary_null(inter, operator, left_val, right_val,
                               left->line_number);
    } else {
        char *op_str = mrsk_get_operator_string(operator);
        mrsk_runtime_error(left->line_number, BAD_OPERAND_TYPE_ERR,
                           STRING_MESSAGE_ARGUMENT, "operator", op_str,
                           MESSAGE_ARGUMENT_END);
    }
    pop_value(inter);
    pop_value(inter);
    push_value(inter, &result);
}

MRSK_Value mrsk_eval_binary_expression(MRSK_Interpreter *inter,
                                       MRSK_LocalEnvironment *env,
                                       ExpressionType operator,
                                       Expression *left, Expression *right)
{
    eval_binary_expression(inter, env, operator, left, right);
    return pop_value(inter);
}

static void eval_logical_and_or_expression(MRSK_Interpreter *inter,
                                           MRSK_LocalEnvironment *env,
                                           ExpressionType operator,
                                           Expression *left, Expression *right)
{
    MRSK_Value left_val;
    MRSK_Value right_val;
    MRSK_Value result;

    result.type = MRSK_BOOLEAN_VALUE;
    eval_expression(inter, env, left);
    left_val = pop_value(inter);

    if (left_val.type != MRSK_BOOLEAN_VALUE) {
        mrsk_runtime_error(left->line_number, NOT_BOOLEAN_TYPE_ERR,
                          MESSAGE_ARGUMENT_END);
    }
    if (operator == LOGICAL_AND_EXPRESSION) {
        if (!left_val.u.boolean_value) {
            result.u.boolean_value = MRSK_FALSE;
            goto FUNC_END;
        }
    } else if (operator == LOGICAL_OR_EXPRESSION) {
        if (left_val.u.boolean_value) {
            result.u.boolean_value = MRSK_TRUE;
            goto FUNC_END;
        }
    } else {
        DBG_panic(("bad operator..%d\n", operator));
    }

    eval_expression(inter, env, right);
    right_val = pop_value(inter);

    result.u.boolean_value = right_val.u.boolean_value;

FUNC_END:
    push_value(inter, &result);
}

static void eval_minus_expression(MRSK_Interpreter *inter,
                                  MRSK_LocalEnvironment *env,
                                  Expression *operand)
{
    MRSK_Value operand_val;
    MRSK_Value result;

    eval_expression(inter, env, operand);
    operand_val = pop_value(inter);
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

    push_value(inter, &result);
}


MRSK_Value mrsk_eval_minus_expression(MRSK_Interpreter *inter,
                                      MRSK_LocalEnvironment *env,
                                      Expression *operand)
{
    eval_minus_expression(inter, env, operand);
    return pop_value(inter);
}

static MRSK_LocalEnvironment * alloc_local_environment(MRSK_Interpreter *inter)
{
    MRSK_LocalEnvironment *ret;

    ret = MEM_malloc(sizeof(MRSK_LocalEnvironment));
    ret->variable = NULL;
    ret->global_variable = NULL;
    ret->ref_in_native_method = NULL;

    ret->next = inter->top_environment;
    inter->top_environment = ret;

    return ret;
}

static void dispose_ref_in_native_method(MRSK_LocalEnvironment *env)
{
    RefInNativeFunc *ref;

    while (env->ref_in_native_method) {
        ref = env->ref_in_native_method;
        env->ref_in_native_method = ref->next;
        MEM_free(ref);
    }
}

static void dispose_local_environment(MRSK_Interpreter *inter)
{
    MRSK_LocalEnvironment *env = inter->top_environment;

    while (env->variable) {
        Variable *temp;
        temp = env->variable;
        env->variable = temp->next;
        MEM_free(temp);
    }
    while (env->global_variable) {
        GlobalVariableRef *ref;
        ref = env->global_variable;
        env->global_variable = ref->next;
        MEM_free(ref);
    }
    dispose_ref_in_native_method(env);

    inter->top_environment = env->next;
    MEM_free(env);
}

static void call_native_function(MRSK_Interpreter *inter,
                                 MRSK_LocalEnvironment *env,
                                 MRSK_LocalEnvironment *caller_env,
                                 Expression *expr, MRSK_NativeFunctionProc *proc)
{
    MRSK_Value value;
    int arg_count;
    ArgumentList *arg_p;
    MRSK_Value *args;

    for (arg_count=0, arg_p=expr->u.function_call_expression.argument;
         arg_p; arg_p=arg_p->next) {
        eval_expression(inter, caller_env, arg_p->expression);
        arg_count++;
    }
    args = &inter->stack.stack[inter->stack.stack_pointer-arg_count];
    value = proc(inter, env, arg_count, args);
    shrink_stack(inter, arg_count);

    push_value(inter, &value);
}

static void call_murasaki_function(MRSK_Interpreter *inter,
                                   MRSK_LocalEnvironment *env,
                                   MRSK_LocalEnvironment *caller_env,
                                   Expression *expr,
                                   FunctionDefinition *func)
{
    MRSK_Value value;
    StatementResult result;
    ArgumentList *arg_p;
    ParameterList *param_p;

    for (arg_p=expr->u.function_call_expression.argument,
         param_p=func->u.murasaki_f.parameter;
         arg_p;
         arg_p=arg_p->next, param_p=param_p->next) {
        Variable *new_var;
        MRSK_Value arg_val;

        if (param_p == NULL) {
            mrsk_runtime_error(expr->line_number, ARGUMENT_TOO_MANY_ERR,
                               MESSAGE_ARGUMENT_END);
        }
        eval_expression(inter, caller_env, arg_p->expression);
        arg_val = pop_value(inter);
        new_var = mrsk_add_local_variable(env, param_p->name);
        new_var->value = arg_val;
    }
    if (param_p) {
        mrsk_runtime_error(expr->line_number, ARGUMENT_TOO_FEW_ERR,
                           MESSAGE_ARGUMENT_END);
    }
    result = mrsk_execute_statement_list(inter, env,
                                         func->u.murasaki_f.block->statement_list);
    if (result.type == RETURN_STATEMENT_RESULT) {
        value = result.u.return_value;
    } else {
        value.type = MRSK_NONE_VALUE;
    }

    push_value(inter, &value);
}


static void eval_function_call_expression(MRSK_Interpreter *inter,
                                          MRSK_LocalEnvironment *env,
                                          Expression *expr)
{
    FunctionDefinition *func;
    MRSK_LocalEnvironment *local_env;

    char *identifier = expr->u.function_call_expression.identifier;

    func = mrsk_search_function(identifier);
    if (func == NULL) {
        mrsk_runtime_error(expr->line_number, FUNCTION_NOT_FOUND_ERR,
                           STRING_MESSAGE_ARGUMENT, "name", identifier,
                           MESSAGE_ARGUMENT_END);
    }

    local_env = alloc_local_environment(inter);

    switch (func->type) {
        case MURASAKI_FUNCTION_DEFINITION:
            call_murasaki_function(inter, local_env, env, expr, func);
            break;
        case NATIVE_FUNCTION_DEFINITION:
            call_native_function(inter, local_env, env, expr,
                                 func->u.native_f.proc);
            break;
        case FUNCTION_DEFINITION_TYPE_COUNT_PLUS_1:
        default:
            DBG_panic(("bad case..%d\n", func->type));
    }
    dispose_local_environment(inter);
}

static void check_method_argument_count(int line_number,
                                        ArgumentList *arg_list,
                                        int arg_count)
{
    ArgumentList *arg_p;
    int count = 0;

    for (arg_p=arg_list; arg_p; arg_p=arg_p->next) {
        count++;
    }

    if (count < arg_count) {
        mrsk_runtime_error(line_number, ARGUMENT_TOO_FEW_ERR,
                           MESSAGE_ARGUMENT_END);
    } else if (count > arg_count) {
        mrsk_runtime_error(line_number, ARGUMENT_TOO_MANY_ERR,
                           MESSAGE_ARGUMENT_END);
    }
}

static void eval_method_call_expression(MRSK_Interpreter *inter,
                                        MRSK_LocalEnvironment *env,
                                        Expression *expr)
{
    MRSK_Value *left;
    MRSK_Value result;
    MRSK_Boolean error_flag = MRSK_FALSE;

    eval_expression(inter, env, expr->u.method_call_expression.expression);
    left = peek_stack(inter, 0);

    if (left->type == MRSK_ARRAY_VALUE) {
        if (!strcmp(expr->u.method_call_expression.identifier, "add")) {
            MRSK_Value *add;
            check_method_argument_count(expr->line_number,
                                        expr->u.method_call_expression
                                        .argument, 1);
            eval_expression(inter, env,
                            expr->u.method_call_expression.argument
                            ->expression);
            add = peek_stack(inter, 0);
            mrsk_array_add(inter, left->u.object, *add);
            pop_value(inter);
            result.type = MRSK_NONE_VALUE;
        } else if (!strcmp(expr->u.method_call_expression.identifier,
                           "size")) {
            check_method_argument_count(expr->line_number,
                                        expr->u.method_call_expression
                                        .argument, 0);
            result.type = MRSK_INT_VALUE;
            result.u.int_value = left->u.object->u.array.size;
        } else if (!strcmp(expr->u.method_call_expression.identifier,
                           "resize")) {
            MRSK_Value new_size;
            check_method_argument_count(expr->line_number,
                                        expr->u.method_call_expression
                                        .argument, 1);
            eval_expression(inter, env,
                            expr->u.method_call_expression.argument
                            ->expression);
            new_size = pop_value(inter);
            if (new_size.type != MRSK_INT_VALUE) {
                mrsk_runtime_error(expr->line_number,
                                  ARRAY_RESIZE_ARGUMENT_ERR,
                                  MESSAGE_ARGUMENT_END);
            }
            mrsk_array_resize(inter, left->u.object, new_size.u.int_value);
            result.type = MRSK_NONE_VALUE;
        } else {
            error_flag = MRSK_TRUE;
        }

    } else if (left->type == MRSK_STRING_VALUE) {
        if (!strcmp(expr->u.method_call_expression.identifier, "length")) {
            check_method_argument_count(expr->line_number,
                                        expr->u.method_call_expression
                                        .argument, 0);
            result.type = MRSK_INT_VALUE;
            result.u.int_value = strlen(left->u.object->u.string.string);
        } else {
            error_flag = MRSK_TRUE;
        }
    } else {
        error_flag = MRSK_TRUE;
    }
    if (error_flag) {
        mrsk_runtime_error(expr->line_number, NO_SUCH_METHOD_ERR,
                          STRING_MESSAGE_ARGUMENT, "method_name",
                          expr->u.method_call_expression.identifier,
                          MESSAGE_ARGUMENT_END);
    }
    pop_value(inter);
    push_value(inter, &result);
}

static void eval_array_expression(MRSK_Interpreter *inter,
                                  MRSK_LocalEnvironment *env,
                                  ExpressionList *list)
{
    MRSK_Value v;
    int size;
    ExpressionList *pos;
    int i;

    size = 0;
    for (pos=list; pos; pos=pos->next) {
        size++;
    }
    v.type = MRSK_ARRAY_VALUE;
    v.u.object = mrsk_create_array_i(inter, size);
    push_value(inter, &v);

    for (pos=list, i=0; pos; pos=pos->next, i++) {
        eval_expression(inter, env, pos->expression);
        v.u.object->u.array.array[i] = pop_value(inter);
    }

}

static void eval_index_expression(MRSK_Interpreter *inter,
                                  MRSK_LocalEnvironment *env,
                                  Expression *expr)
{
    MRSK_Value *left;

    left = get_array_element_lvalue(inter, env, expr);

    push_value(inter, left);
}

static void eval_inc_dec_expression(MRSK_Interpreter *inter,
                                    MRSK_LocalEnvironment *env,
                                    Expression *expr)
{
    MRSK_Value   *operand;
    MRSK_Value   result;
    int         old_value;
    
    operand = get_lvalue(inter, env, expr->u.inc_dec.operand);
    if (operand->type != MRSK_INT_VALUE) {
        mrsk_runtime_error(expr->line_number, INC_DEC_OPERAND_TYPE_ERR,
                           MESSAGE_ARGUMENT_END);
    }
    old_value = operand->u.int_value;
    if (expr->type == INCREMENT_EXPRESSION) {
        operand->u.int_value++;
    } else {
        DBG_assert(expr->type == DECREMENT_EXPRESSION,
                   ("expr->type..%d\n", expr->type));
        operand->u.int_value--;
    }

    result.type = MRSK_INT_VALUE;
    result.u.int_value = old_value;
    push_value(inter, &result);
}

static void eval_expression(MRSK_Interpreter *inter,
                            MRSK_LocalEnvironment *env,
                            Expression *expr)
{
    switch (expr->type) {
        case BOOLEAN_EXPRESSION:
            eval_boolean_expression(inter, expr->u.boolean_value);
            break;
        case INT_EXPRESSION:
            eval_int_expression(inter, expr->u.int_value);
            break;
        case DOUBLE_EXPRESSION:
            eval_double_expression(inter, expr->u.double_value);
            break;
        case STRING_EXPRESSION:
            eval_string_expression(inter, expr->u.string_value);
            break;
        case IDENTIFIER_EXPRESSION:
            eval_identifier_expression(inter, env, expr);
            break;
        case ASSIGN_EXPRESSION:
            eval_assign_expression(inter, env,
                                   expr->u.assign_expression.left,
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
            eval_binary_expression(inter, env, expr->type,
                                   expr->u.binary_expression.left,
                                   expr->u.binary_expression.right);
            break;
        case LOGICAL_AND_EXPRESSION:
        case LOGICAL_OR_EXPRESSION:
            eval_logical_and_or_expression(inter, env, expr->type,
                                           expr->u.binary_expression.left,
                                           expr->u.binary_expression.right);
            break;
        case MINUS_EXPRESSION:
            eval_minus_expression(inter, env, expr->u.minus_expression);
            break;
        case FUNCTION_CALL_EXPRESSION:
            eval_function_call_expression(inter, env, expr);
            break;
        case METHOD_CALL_EXPRESSION:
            eval_method_call_expression(inter, env, expr);
            break;
        case NONE_EXPRESSION:
            eval_none_expression(inter);
            break;
        case ARRAY_EXPRESSION:
            eval_array_expression(inter, env, expr->u.array_literal);
            break;
        case INDEX_EXPRESSION:
            eval_index_expression(inter, env, expr);
            break;
        case INCREMENT_EXPRESSION:
        case DECREMENT_EXPRESSION:
            eval_inc_dec_expression(inter, env, expr);
            break;
        case EXPRESSION_TYPE_COUNT_PLUS_1:
        default:
            DBG_panic(("bad case. type..%d\n", expr->type));
    }
}

MRSK_Value mrsk_eval_expression(MRSK_Interpreter *inter,
                                MRSK_LocalEnvironment *env,
                                Expression *expr)
{
    eval_expression(inter, env, expr);
    return pop_value(inter);
}

void MRSK_shrink_stack(MRSK_Interpreter *inter, int shrink_size)
{
    shrink_stack(inter, shrink_size);
}







