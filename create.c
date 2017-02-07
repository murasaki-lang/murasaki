#include "MEM.h"
#include "DBG.h"
#include "murasaki.h"

void mrsk_function_define(char *identifier, ParameterList *parameter_list,
                          Block *block)
{
    FunctionDefinition *f;
    MRSK_Interpreter *inter;

    if (mrsk_search_function(identifier)) {
        mrsk_compile_error(FUNCTION_MULTIPLE_DEFINE_ERR,
                           STRING_MESSAGE_ARGUMENT, "name",
                           identifier, MESSAGE_ARGUMENT_END);
        return;
    }
    inter = mrsk_get_current_interpreter();

    f = mrsk_malloc(sizeof(FunctionDefinition));
    f->name = identifier;
    f->type = MURASAKI_FUNCTION_DEFINITION;
    f->u.murasaki_f.parameter = parameter_list;
    f->u.murasaki_f.block = block;
    f->next = inter->function_list;
    inter->function_list = f;
}

ParameterList * mrsk_create_parameter(char *identifier)
{
    ParameterList *p;

    p = mrsk_malloc(sizeof(ParameterList));
    p->name = identifier;
    p->next = NULL;

    return p;
}

ParameterList * mrsk_chain_parameter(ParameterList *list, char *identifier)
{
    ParameterList *pos;

    for (pos=list; pos->next; pos=pos->next) ;

    pos->next = mrsk_create_parameter(identifier);

    return list;
}

ArgumentList * mrsk_create_argument_list(Expression *expression)
{
    ArgumentList *al;

    al = mrsk_malloc(sizeof(ArgumentList));
    al->expression = expression;
    al->next = NULL;

    return al;
}
ArgumentList * mrsk_chain_argument_list(ArgumentList *list, Expression *expr)
{
    ArgumentList *pos;

    for (pos=list; pos->next; pos=pos->next) ;

    pos->next = mrsk_create_argument_list(expr);

    return list;
}

ExpressionList * mrsk_create_expression_list(Expression *expression)
{
    ExpressionList *el;

    el = mrsk_malloc(sizeof(ExpressionList));
    el->expression = expression;
    el->next = NULL;

    return el;
}

ExpressionList * mrsk_chain_expression_list(ExpressionList *list, Expression *expr)
{
    ExpressionList *pos;

    for(pos=list; pos->next; pos=pos->next) ;

    return list;
}

StatementList * mrsk_create_statement_list(Statement *statement)
{
    StatementList *sl;

    sl = mrsk_malloc(sizeof(StatementList));
    sl->statement = statement;
    sl->next = NULL;

    return sl;
}

StatementList * mrsk_chain_statement_list(StatementList *list, Statement *statement)
{
    StatementList *pos;

    if (list == NULL) {
        return mrsk_create_statement_list(statement);
    }

    for (pos=list; pos->next; pos=pos->next) ;

    pos->next = mrsk_create_statement_list(statement);

    return list;
}


Expression * mrsk_alloc_expression(ExpressionType type)
{
    Expression *exp;
    exp = mrsk_malloc(sizeof(Expression));
    exp->type = type;
    exp->line_number = mrsk_get_current_interpreter()->current_line_number;
    return exp;
}

Expression * mrsk_create_assign_expression(Expression *left, Expression *operand)
{
    Expression *exp;

    exp = mrsk_alloc_expression(ASSIGN_EXPRESSION);
    exp->u.assign_expression.left = left;
    exp->u.assign_expression.operand = operand;

    return exp;
}

static Expression convert_value_to_expression(MRSK_Value *v)
{
    Expression expr;

    if (v->type == MRSK_INT_VALUE) {
        expr.type == INT_EXPRESSION;
        expr.u.int_value = v->u.int_value;
    } else if (v->type == MRSK_DOUBLE_VALUE) {
        expr.type == DOUBLE_EXPRESSION;
        expr.u.double_value = v->u.double_value;
    } else {
        DBG_assert(v->type==MRSK_BOOLEAN_VALUE,
                   ("v->type..%d\n", v->type));
        expr.type = BOOLEAN_EXPRESSION;
        expr.u.boolean_value = v->u.boolean_value;
    }
    return expr;
}

Expression * mrsk_create_binary_expression(ExpressionType operator,
                                            Expression *left, Expression *right)
{
    if ((left->type == INT_EXPRESSION || left->type == DOUBLE_EXPRESSION)
         && (right->type == INT_EXPRESSION || right->type == DOUBLE_EXPRESSION)) {
        MRSK_Value v;
        v = mrsk_eval_binary_expression(mrsk_get_current_interpreter(), NULL,
                                        operator, left, right);
        *left = convert_value_to_expression(&v);
        return left;
    } else {
        Expression *exp;
        exp = mrsk_alloc_expression(operator);
        exp->u.binary_expression.left = left;
        exp->u.binary_expression.right = right;
        return exp;
    }
}

Expression * mrsk_create_minus_expression(Expression *operand)
{
    if (operand->type == INT_EXPRESSION
        || operand->type == DOUBLE_EXPRESSION) {
        MRSK_Value v;
        v = mrsk_eval_minus_expression(mrsk_get_current_interpreter(),
                                       NULL, operand);
        *operand = convert_value_to_expression(&v);
        return operand;
    } else {
        Expression *exp;
        exp = mrsk_alloc_expression(MINUS_EXPRESSION);
        exp->u.minus_expression = operand;
        return exp;
    }
}

Expression * mrsk_create_index_expression(Expression *array, Expression *index)
{
    Expression *exp;

    exp = mrsk_alloc_expression(INDEX_EXPRESSION);
    exp->u.index_expression.array = array;
    exp->u.index_expression.index = index;

    return exp;
}

Expression * mrsk_create_incdec_expression(Expression *operand, ExpressionType inc_or_dec)
{
    Expression *exp;

    exp = mrsk_alloc_expression(inc_or_dec);
    exp->u.inc_dec.operand = operand;

    return exp;
}

Expression * mrsk_create_identifier_expression(char *identifier)
{
    Expression *exp;

    exp = mrsk_alloc_expression(IDENTIFIER_EXPRESSION);
    exp->u.identifier = identifier;

    return exp;
}

Expression * mrsk_create_function_call_expression(char *func_name, ArgumentList *argument)
{
    Expression *exp;

    exp = mrsk_alloc_expression(FUNCTION_CALL_EXPRESSION);
    exp->u.function_call_expression.identifier = func_name;
    exp->u.function_call_expression.argument = argument;

    return exp;
}

Expression * mrsk_create_method_call_expression(Expression *expression,
                                                char *method_name, ArgumentList *argument)
{
    Expression *exp;

    exp = mrsk_alloc_expression(METHOD_CALL_EXPRESSION);
    exp->u.method_call_expression.expression = expression;
    exp->u.method_call_expression.identifier = method_name;
    exp->u.method_call_expression.argument = argument;

    return exp;
}

Expression * mrsk_create_boolean_expression(MRSK_Boolean value)
{
    Expression *exp;

    exp = mrsk_alloc_expression(BOOLEAN_EXPRESSION);
    exp->u.boolean_value = value;

    return exp;
}

Expression * mrsk_create_none_expression(void)
{
    Expression *exp;
    exp = mrsk_alloc_expression(NONE_EXPRESSION);

    return exp;
}

Expression * mrsk_create_array_expression(ExpressionList *list)
{
    Expression *exp;

    exp = mrsk_alloc_expression(ARRAY_EXPRESSION);
    exp->u.array_literal = list;

    return exp;
}

static Statement * alloc_statement(StatementType type)
{
    Statement *st;

    st = mrsk_malloc(sizeof(Statement));
    st->type = type;
    st->line_number = mrsk_get_current_interpreter()->current_line_number;

    return st;
}

Statement * mrsk_create_global_statement(IdentifierList *identifier_list)
{
    Statement *st;

    st = alloc_statement(GLOBAL_STATEMENT);
    st->u.global_s.identifier_list = identifier_list;

    return st;
}

IdentifierList * mrsk_create_global_identifier(char *identifier)
{
    IdentifierList *i_list;

    i_list = mrsk_malloc(sizeof(IdentifierList));
    i_list->name = identifier;
    i_list->next = NULL;

    return i_list;

}

IdentifierList * mrsk_chain_identifier(IdentifierList *list, char *identifier)
{
    IdentifierList *pos;

    for (pos=list; pos->next; pos=pos->next) ;

    pos->next = mrsk_create_global_identifier(identifier);

    return list;
}

Statement * mrsk_create_if_statement(Expression *condition,
                                     Block *then_block, Elif *elif_list,
                                     Block *else_block)
{
    Statement *st;

    st = alloc_statement(IF_STATEMENT);
    st->u.if_s.condition = condition;
    st->u.if_s.then_block = then_block;
    st->u.if_s.elif_list = elif_list;
    st->u.if_s.else_block = else_block;

    return st;
}

Elif * mrsk_chain_elif_list(Elif *list, Elif *add)
{
    Elif *pos;

    for (pos=list; pos->next; pos=pos->next) ;

    pos->next = add;

    return list;
}

Elif * mrsk_create_elif(Expression *expr, Block *block)
{
    Elif *ei;

    ei = mrsk_malloc(sizeof(Elif));
    ei->condition = expr;
    ei->block = block;
    ei->next = NULL;

    return ei;
}

Statement * mrsk_create_while_statement(Expression *condition, Block *block)
{
    Statement *st;

    st = alloc_statement(WHILE_STATEMENT);
    st->u.while_s.condition = condition;
    st->u.while_s.block = block;

    return st;
}

Statement * mrsk_create_for_statement(Expression *init, Expression *cond,
                                      Expression *post, Block *block)
{
    Statement *st;

    st = alloc_statement(FOR_STATEMENT);
    st->u.for_s.init = init;
    st->u.for_s.condition = cond;
    st->u.for_s.post = post;
    st->u.for_s.block = block;

    return st;
}

Block * mrsk_create_block(StatementList *statement_list)
{
    Block *block;

    block = mrsk_malloc(sizeof(Block));
    block->statement_list = statement_list;

    return block;
}

Statement * mrsk_create_expression_statement(Expression *expression)
{
    Statement *st;

    st = alloc_statement(EXPRESSION_STATEMENT);
    st->u.expression_s = expression;

    return st;
}

Statement * mrsk_create_return_statement(Expression *expression)
{
    Statement *st;

    st = alloc_statement(RETURN_STATEMENT);
    st->u.return_s.return_value = expression;

    return st;
}

Statement * mrsk_create_break_statement(void)
{
    return alloc_statement(BREAK_STATEMENT);
}

Statement * mrsk_create_continue_statement(void)
{
    return alloc_statement(CONTINUE_STATEMENT);
}
