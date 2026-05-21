#ifndef ANS_INTERPRETER_H
#define ANS_INTERPRETER_H

#include "ast.h"
#include "value.h"

/* Execute the fully type-checked program AST.
 * Assumes check_program() has already been called and found no errors. */
void interpret_program(const AstNode *prog);

#endif /* ANS_INTERPRETER_H */
