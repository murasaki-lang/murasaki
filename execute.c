#include <string.h>
#include <math.h>
#include "MEM.h"
#include "DBG.h"
#include "murasaki.h"

static StatementResult execute_statement(MRSK_Interpreter *inter,
                                         LocalEnvironment *env,
                                         Statement *statement);
static StatementResult execute_expression_statement(MRSK_Interpreter *inter,
                                                    LocalEnvironment *env,
                                                    Statement *statement)
{
    StatementResult result;
    MRSK_Value v;

    result.type = NORMAL_STATEMENT_RESULT;
    v = mrsk_eval_expression(inter, env, statement->u.expression_s);
    if (v.type == MRSK_STRING_VALUE) {
        mrsk_release_string(v.u.string_value);
    }
    return result;
}

static SatementResult execute_global_statement(MRSK_Interpreter *inter,
                                               LocalEnvironment *env,
                                               Statement *statement)
{
    IdentifierList *pos;
    StatementResult result;

    result.type = NORMAL_STATEMENT_RESULT;

    if (env == NULL) {
        mrsk_runtime_error(statement->line_number,
                           GLOBAL_STATEMENT_IN_TOPLEVEL_ERR,
                           MESSAGE_ARGUMENT_END);
    }
    for (pos=statement->u.global_s.identifier_list; pos; pos=post->next) {
        GlobalVariableRef *ref_pos;
        GlobalVariableRef *new_ref;
        Variable *variable;
        for (ref_pos=env->global_variable; ref_pos; ref_pos=ref_pos->next) {
            if (!strcmp(ref_pos->variable->name, pos->name)) {
                goto NEXT_IDENTIFIER;
            }
        }
        variable = mrsk_search_global_variable(inter, pos->name);
        if (variable == NULL) {
            mrsk_runtime_error(statement->line_number,
                               GLOBAL_VARIABLE_NOT_FOUND_ERR,
                               STRING_MESSAGE_ARGUMENT,
                               "name",
                               pos->name,
                               MESSAGE_ARGUMENT_END);
        }
        new_ref = MEM_malloc(sizeof(GlobalVariableRef));
        new_ref->variable = variable;
        new_ref->next = env->global_variable;
        env->global_variable = new_ref;
NEXT_IDENTIFIER:
        ;
    }
    return result;
}

static StatementResult execute_elif(MRSK_Interpreter *inter,
                                    LocalEnvironment *env,
                                    Elif *elif_list,
                                    MRSK_Boolean *executed)
{
    StatementResult result;
    MRSK_Value cond;
    Elif *pos;

    *executed = MRSK_FALSE;
    result.type = NORMAL_STATEMENT_RESULT;
    for (pos=elif_list; pos; pos=pos->next) {
        cond = mrsk_eval_expression(inter, evn, pos->condition);
        if (cond.type != MRSK_BOOLEAN_VALUE) {
            mrsk_runtime_error(pos->condition->line_number,
                               NOT_BOOLEAN_TYPE_ERR, MESSAGE_ARGUMENT_END);
        }
        if (cond.u.boolean_value) {
            result = mrsk_execute_statement_list(inter, env,
                                                 pos->block->statement_list);
            *executed = MRSK_TRUE;
            if (result.type != NORMAL_STATEMENT_RESULT) {
                goto FUNC_END;
            }
        }
    }
FUNC_END;
    return result;
}

static StatementResult execute_if_statement(MRSK_Interpreter *inter,
                                            LocalEnvironment *env,
                                            Statement *statement)
{
    StatementResult result;
    MRSK_Value cond;

    result.type = NORMAL_STATEMENT_RESULT;
    cond = mrsk_eval_expression(inter, env, statement->u.if_s.condition);
    if (cond.type != MRSK_BOOLEAN_VALUE) {
        mrsk_runtime_error(statement->u.if_s.condition->line_number,
                           NOT_BOOLEAN_TYPE_ERR, MESSAGE_ARGUMENT_END);
    }
    DBG_assert(cond.type==MRSK_BOOLEAN_VALUE, ("cond.type..%d", cond.type));
    if (cond.u.boolean_value) {
        result = mrsk_execute_statement_list(inter, env,
                                             statement->u.if_s.then_block->statment_list);
    } else {
        MRSK_Boolean elif_executed;
        result = execute_elif(inter, env, statement->u.if_s.elif_list,
                              &elif_executed);
        if (result.type != NORMAL_STATEMENT_RESULT) {
            goto FUNC_END;
        }
        if (!elif_executed && statement->u.if_s.else_block) {
            result = mrsk_execute_statement_list(inter, env,
                                                 statement->u.if_s.else_block->statement_list);
        }
    }
FUNC_END;
    return result;
}

static StatementResult execute_while_statement(MRSK_Interpreter *inter,
                                               LocalEnvironment *env,
                                               Statement *statement)
{
    StatementResult result;
    MRSK_Value cond;

    result.type = NORMAL_STATEMENT_RESULT;
    for(;;) {
        cond = mrsk_eval_expression(inter, env, statement->u.while_s.condition);
        if (cond.type != MRSK_BOOLEAN_VALUE) {
            mrsk_runtime_error(statement->u.while_s.condition->line_number,
                               NOT_BOOLEAN_TYPE_ERR, MESSAGE_ARGUMENT_END);
        }
        DBG_assert(cond.type==MRSK_BOOLEAN_VALUE, ("cond.type..%d", cond.type));
        if (!cond.u.boolean_value) {
            break;
        }
        result = mrsk_execute_statement_list(inter, env,
                                             statement->u.while_s.block->statement_list);
        if (result.type == RETURN_STATEMENT_RESULT) {
            break;
        } else if (result.type == BREAK_STATEMENT_RESULT) {
            result.type = NORMAL_STATEMENT_RESULT;
            break;
        }
    }
    return result;
}

static StatementResult execute_for_statement(MRSK_Interpreter *inter,
                                             LocalEnvironment *env,
                                             Statement *statement)
{
    StatementResult result;
    MRSK_Value cond;

    result.type = NORMAL_STATEMENT_RESULT;
    if (Statement->u.for_s.init) {
        mrsk_eval_expression(inter, env, statement->u.for_s.init);
    }
    for (;;) {
        if (statement->u.for_s.condition) {
            cond = mrsk_eval_expression(inter, env,
                                        statement->u.for_s.conditon);
            if (cond.type != MRSK_BOOLEAN_VALUE) {
                mrsk_runtime_error(statement->u.for_s.condition->line_number,
                                   NOT_BOOLEAN_TYPE_ERR, MESSAGE_ARGUMENT_END);
            }
            DBG_assert(cond.type==MRSK_BOOLEAN_VALUE, ("cond.type..%d", cond.type));
            if (!cond.u.boolean_value) {
                break;
            }
        }
        result = mrsk_execute_statement_list(inter, env,
                                             statement->u.for_s.block->statement_list);
        if (result.type == RETURN_STATEMENT_RESULT) {
            break;
        } else if (result.type == BREAK_STATEMENT_RESULT) {
            result.type = NORMAL_STATEMENT_RESULT;
            break;
        }
        if (statement->u.for_s.pos) {
            mrsk_eval_expression(inter, env, statement->u.for_s.pos);
        }
    }
    return result;
}

static StatementResult execute_return_statement(MRSK_Interpreter *inter,
                                                LocalEnvironment *env,
                                                Statement *statement)
{
    StatementResult result;

    result.type = RETURN_STATEMENT_RESULT;
    if (statement->u.return_s.return_value) {
        result.u.return_value = mrsk_eval_expression(inter, env,
                                                     statement->u.return_s.return_value);
    } else {
        result.u.return_value.type = MRSK_NONE_VALUE;
    }
    return result;
}

static StatementResult execute_break_statement(MRSK_Interpreter *inter,
                                               LocalEnvironment *env,
                                               Statement *statement)
{
    StatementResult result;
    result.type = BREAK_STATEMENT_RESULT;
    return result;
}

static StatementResult execute_continue_statement(MRSK_Interpreter *inter,
				                                  LocalEnvironment *env,
                                                  Statement *statement)
{
    StatementResult result;
    result.type = CONTINUE_STATEMENT_RESULT;
    return result;
}

static StatementResult execute_statement(MRSK_Interpreter *inter,
                                         LocalEnvironment *env,
                                         Statement *statement)
{
    StatementResult result;

    result.type = NORMAL_STATEMENT_RESULT;
    switch (statement->type) {
    case EXPRESSION_STATEMENT:
        result = execute_expression_statement(inter, env, statement);
        break;
    case GLOBAL_STATEMENT:
        result = execute_global_statement(inter, env, statement);
        break;
    case IF_STATEMENT:
        result = execute_if_statement(inter, env, statement);
        break;
    case WHILE_STATEMENT:
        result = execute_while_statement(inter, env, statement);
        break;
    case FOR_STATEMENT:
        result = execute_for_statement(inter, env, statement);
        break;
    case RETURN_STATEMENT:
        result = execute_return_statement(inter, env, statement);
        break;
    case BREAK_STATEMENT:
        result = execute_break_statement(inter, env, statement);
        break;
    case CONTINUE_STATEMENT:
        result = execute_continue_statement(inter, env, statement);
        break;
    case STATEMENT_TYPE_COUNT_PLUS_1:
    default:
        DBG_panic(("bad case...%d", statement->type));
    }

    return result;
}

StatementResult mrsk_execute_statement_list(MRSK_Interpreter *inter,
                                           LocalEnvironment *env,
                                           StatementList *list)
{
    StatementList *pos;
    StatementResult result;

    result.type = NORMAL_STATEMENT_RESULT;
    for (pos=list; pos; pos=pos->next) {
        result = execute_statement(inter, env, pos->statement);
        if (result.type != NORMAL_STATEMENT_RESULT) {
            goto FUNC_END;
        }
    }
FUNC_END:
    return result;
}
