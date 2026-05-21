#ifndef ANS_CHECKER_H
#define ANS_CHECKER_H

#include "ast.h"

/* Run the type checker over the program AST.
 * Reports errors via error_at() (sets g_had_error).
 * Does not stop on first error; continues to find more. */
void check_program(const AstNode *prog);

#endif /* ANS_CHECKER_H */
