#ifndef ANS_AST_H
#define ANS_AST_H

#include <stddef.h>

typedef enum {
    ND_PROGRAM,

    /* Declarations */
    ND_ELEMENT_DECL,
    ND_COMPOUND_DECL,
    ND_REACTION_DECL,
    ND_FUNCTION_DECL,
    ND_ELEMENT_FIELD,
    ND_COMPOUND_FIELD,
    ND_REACTION_FIELD,
    ND_PARAM,

    /* Statements */
    ND_BLOCK,
    ND_VAR_DECL,
    ND_ASSIGN,
    ND_AUG_ASSIGN,
    ND_RETURN,
    ND_PRINT,
    ND_EXPR_STMT,
    ND_FOR,
    ND_IF,

    /* Expressions */
    ND_BINARY,
    ND_UNARY,
    ND_CALL,
    ND_INDEX,
    ND_MEMBER,
    ND_IDENT,
    ND_NUMBER,
    ND_QUANTITY,
    ND_STRING,
    ND_CHAR,
    ND_FORMULA,
    ND_LIST,

    /* Reaction sub-grammar */
    ND_REACTION_EXPR,
    ND_REACTION_TERM,
    ND_ENERGY_EXPR,
} NodeKind;

typedef struct AstNode {
    NodeKind        kind;
    int             line;

    /* Inline string payload (name, lexeme, operator text, field key, etc.) */
    char           *text;

    /* Numeric payload: literal value or operator code */
    double          num;

    /* Children */
    struct AstNode **children;
    int              n_children;
    int              cap_children;
} AstNode;

AstNode *ast_new(NodeKind kind, int line);
void     ast_set_text(AstNode *n, const char *start, int length);
void     ast_add_child(AstNode *parent, AstNode *child);
void     ast_print(const AstNode *n, int indent);
void     ast_free(AstNode *n);

#endif /* ANS_AST_H */
