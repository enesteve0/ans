#ifndef ANS_SEMANTICS_H
#define ANS_SEMANTICS_H

#include "ast.h"

int ans_check_program(const AstNode *program);
int ans_execute_program(const AstNode *program);

#endif /* ANS_SEMANTICS_H */
