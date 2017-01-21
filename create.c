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
