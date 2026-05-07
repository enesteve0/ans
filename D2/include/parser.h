#ifndef ANS_PARSER_H
#define ANS_PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer  *lexer;
    Token   current;
    Token   previous;
} Parser;

void     parser_init(Parser *p, Lexer *l);
AstNode *parse_program(Parser *p);

#endif /* ANS_PARSER_H */
