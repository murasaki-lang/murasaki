%{
#include <stdio.h>
#include "murasaki.h"
#define YYDEBUG 1
%}
%union {
    char *identifier;
    ParameterList *parameter_list;
    ArgumentList *argument_list;
    Expression *expression;
    ExpressionList *expression_list;
    Statement *statement;
    StatementList *statement_list;
    Block *block;
    Elif *elif;
    IdentifierList *identifier_list;
}
%token <expression> INT_LITERAL
%token <expression> DOUBLE_LITERAL
%token <expression> STRING_LITERAL
%token <identifier> IDENTIFIER
%token FUNCTION IF ELIF ELSE WHILE FOR RETURN_T BREAK CONTINUE NONE_T
       LP RP LC RC LB RB SEMICOLON COMMA ASSIGN LOGICAL_AND LOGICAL_OR
       EQ NE GT GE LT LE ADD SUB MUL DIV MOD TRUE_T FALSE_T GLOBAL_T
       DOT INCREMENT DECREMENT
%type <parameter_list> parameter_list
%type <argument_list> argument_list
%type <expression> expression expression_opt
      logical_and_expression logical_or_expression
      equality_expression relational_expression
      additive_expression multiplicative_expression
      unary_expression postfix_expression primary_expression array_literal
%type <expression_list> expression_list
%type <statement> statement global_statement
      if_statement while_statement for_statement
      return_statement break_statement continue_statement
%type <statement_list> statement_list
%type <block> block
%type <elif> elif elif_list
%type <identifier_list> identifier_list
%%
translation_unit
    : definition_or_statement
    | translation_unit definition_or_statement
    ;
definition_or_statement
    : function_definition
    | statement
    {
        MRSK_Interpreter *inter = mrsk_get_current_interpreter();
        inter->statement_list = mrsk_chain_statement_list(inter->statement_list, $1);
    }
    ;
function_definition
    : FUNCTION IDENTIFIER LP parameter_list RP block
    {
        mrsk_function_define($2, $4, $6);
    }
    | FUNCTION IDENTIFIER LP RP block
    {
        mrsk_function_define($2, NULL, $5);
    }
    ;
parameter_list
    : IDENTIFIER
    {
        $$ = mrsk_create_parameter($1);
    }
    | parameter_list COMMA IDENTIFIER
    {
        $$ = mrsk_chain_parameter($1, $3);
    }
    ;
argument_list
    : expression
    {
        $$ = mrsk_create_argument_list($1);
    }
    | argument_list COMMA expression
    {
        $$ = mrsk_chain_argument_list($1, $3);
    }
    ;
statement_list
    : statement
    {
        $$ = mrsk_create_statement_list($1);
    }
    | statement_list statement
    {
        $$ = mrsk_chain_statement_list($1, $2);
    }
    ;
expression
    : logical_or_expression
    | postfix_expression ASSIGN expression
    {
        $$ = mrsk_create_assign_expression($1, $3);
    }
    ;
logical_and_expression
    : equality_expression
    | logical_and_expression LOGICAL_AND equality_expression
    {
        $$ = mrsk_create_binary_expression(LOGICAL_AND_EXPRESSION, $1, $3);
    }
    ;
logical_or_expression
    : logical_and_expression
    | logical_or_expression LOGICAL_OR logical_and_expression
    {
        $$ = mrsk_create_binary_expression(LOGICAL_OR_EXPRESSION, $1, $3);
    }
    ;
equality_expression
    : relational_expression
    | equality_expression EQ relational_expression
    {
        $$ = mrsk_create_binary_expression(EQ_EXPRESSION, $1, $3);
    }
    | equality_expression NE relational_expression
    {
        $$ = mrsk_create_binary_expression(NE_EXPRESSION, $1, $3);
    }
    ;
relational_expression
    : additive_expression
    | relational_expression GT additive_expression
    {
        $$ = mrsk_create_binary_expression(GT_EXPRESSION, $1, $3);
    }
    | relational_expression GE additive_expression
    {
        $$ = mrsk_create_binary_expression(GE_EXPRESSION, $1, $3);
    }
    | relational_expression LT additive_expression
    {
        $$ = mrsk_create_binary_expression(LT_EXPRESSION, $1, $3);
    }
    | relational_expression LE additive_expression
    {
        $$ = mrsk_create_binary_expression(LE_EXPRESSION, $1, $3);
    }
    ;
additive_expression
    : multiplicative_expression
    | additive_expression ADD multiplicative_expression
    {
        $$ = mrsk_create_binary_expression(ADD_EXPRESSION, $1, $3);
    }
    | additive_expression SUB multiplicative_expression
    {
        $$ = mrsk_create_binary_expression(SUB_EXPRESSION, $1, $3);
    }
    ;
multiplicative_expression
    : unary_expression
    | multiplicative_expression MUL unary_expression
    {
        $$ = mrsk_create_binary_expression(MUL_EXPRESSION, $1, $3);
    }
    | multiplicative_expression DIV unary_expression
    {
        $$ = mrsk_create_binary_expression(DIV_EXPRESSION, $1, $3);
    }
    | multiplicative_expression MOD unary_expression
    {
        $$ = mrsk_create_binary_expression(MOD_EXPRESSION, $1, $3);
    }
    ;
unary_expression
    : postfix_expression
    | SUB unary_expression
    {
        $$ = mrsk_create_minus_expression($2);
    }
    ;
postfix_expression
    : primary_expression
    | postfix_expression LB expression RB
    {
        $$ = mrsk_create_index_expression($1, $3);
    }
    | postfix_expression DOT IDENTIFIER LP argument_list RP
    {
        $$ = mrsk_create_method_call_expression($1, $3, $5);
    }
    | postfix_expression DOT IDENTIFIER LP RP
    {
        $$ = mrsk_create_method_call_expression($1, $3, NULL);
    }
    | postfix_expression INCREMENT
    {
        $$ = mrsk_create_incdec_expression($1, INCREMENT_EXPRESSION);
    }
    | postfix_expression DECREMENT
    {
        $$ = mrsk_create_incdec_expression($1, DECREMENT_EXPRESSION);
    }
    ;
primary_expression
    : IDENTIFIER LP argument_list RP
    {
        $$ = mrsk_create_function_call_expression($1, $3);
    }
    | IDENTIFIER LP RP
    {
        $$ = mrsk_create_function_call_expression($1, NULL);
    }
    | LP expression RP
    {
        $$ = $2;
    }
    | IDENTIFIER
    {
        $$ = mrsk_create_identifier_expression($1);
    }
    | INT_LITERAL
    | DOUBLE_LITERAL
    | STRING_LITERAL
    | TRUE_T
    {
        $$ = mrsk_create_boolean_expression(MRSK_TRUE);
    }
    | FALSE_T
    {
        $$ = mrsk_create_boolean_expression(MRSK_FALSE);
    }
    | NONE_T
    {
        $$ = mrsk_create_none_expression();
    }
    | array_literal
    ;
array_literal
    : LC expression_list RC
    {
        $$ = mrsk_create_array_expression($2);
    }
    | LC expression_list COMMA RC
    {
        $$ = mrsk_create_array_expression($2);
    }
    ;
expression_list
    :
    {
        $$ = NULL;
    }
    | expression
    {
        $$ = mrsk_create_expression_list($1);
    }
    | expression COMMA expression
    {
        $$ = mrsk_chain_expression_list($1, $3);
    }
    ;
statement
    : expression SEMICOLON
    {
        $$ = mrsk_create_expression_statement($1);
    }
    | global_statement
    | if_statement
    | while_statement
    | for_statement
    | break_statement
    | continue_statement
    | return_statement
    ;
global_statement
    : GLOBAL_T identifier_list SEMICOLON
    {
        $$ = mrsk_create_global_statement($2);
    }
    ;
identifier_list
    : IDENTIFIER
    {
        $$ = mrsk_create_global_identifier($1);
    }
    | identifier_list COMMA IDENTIFIER
    {
        $$ = mrsk_chain_identifier($1, $3);
    }
    ;
if_statement
    : IF LP expression RP block
    {
        $$ = mrsk_create_if_statement($3, $5, NULL, NULL);
    }
    | IF LP expression RP block ELSE block
    {
        $$ = mrsk_create_if_statement($3, $5, NULL, $7);
    }
    | IF LP expression RP block elif_list
    {
        $$ = mrsk_create_if_statement($3, $5, $6, NULL);
    }
    | IF LP expression RP block elif_list ELSE block
    {
        $$ = mrsk_create_if_statement($3, $5, $6, $8);
    }
    ;
elif_list
    : elif
    | elif_list elif
    {
        $$ = mrsk_chain_elif_list($1, $2);
    }
    ;
elif
    : ELIF LP expression RP block
    {
        $$ = mrsk_create_elif($3, $5);
    }
    ;
while_statement
    : WHILE LP expression RP block
    {
        $$ = mrsk_create_while_statement($3, $5);
    }
    ;
for_statement
    : FOR LP expression_opt SEMICOLON expression_opt SEMICOLON
      expression_opt RP block
    {
        $$ = mrsk_create_for_statement($3, $5, $7, $9);
    }
    ;
expression_opt
    :
    {
        $$ = NULL;
    }
    | expression
    ;
break_statement
    : BREAK SEMICOLON
    {
        $$ = mrsk_create_break_statement();
    }
    ;
continue_statement
    : CONTINUE SEMICOLON
    {
        $$ = mrsk_create_continue_statement();
    }
    ;
return_statement
    : RETURN_T expression_opt SEMICOLON
    {
        $$ = mrsk_create_return_statement($2);
    }
    ;
block
    : LC statement_list RC
    {
        $$ = mrsk_create_block($2);
    }
    | LC RC
    {
        $$ = mrsk_create_block(NULL);
    }
    ;
%%
