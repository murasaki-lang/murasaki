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
                           STRING_MESSAGE_ARGUMJENT, "name",
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

Expression * mrsk_create_expresion_list(Expression *expression)
{
    ExpressionList *el;

    el = mrsk_malloc(sizeof(ExpressionList));
    el->expression = expression;
    el->next = NULL;

    return el;
}

ExpressionList * mrsk_chain_expression_list(ExpressionList *list, Expression *expr)
{
    ExpresssionList *pos;

    for(pos=list; pos->next; pos=pos->next) ;

    pos->next = mrsk_create_expression_list(expr);

    return list;
}

StatementList * mrsk_create_statement_list(Statement *statement)
{
    Statement *sl;

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

Expression * mrsk_create_binary_expresssion(ExpressionType operator,
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
        v = mrsk_eval_minus_expression(mrsk_get_curent_interpreter(),
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
