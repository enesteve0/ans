#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

AstNode *ast_new(NodeKind kind, int line) {
    AstNode *n = calloc(1, sizeof(AstNode));
    if (!n) { perror("calloc"); exit(1); }
    n->kind = kind;
    n->line = line;
    return n;
}

void ast_set_text(AstNode *n, const char *start, int length) {
    free(n->text);
    n->text = malloc((size_t)length + 1);
    if (!n->text) { perror("malloc"); exit(1); }
    memcpy(n->text, start, (size_t)length);
    n->text[length] = '\0';
}

void ast_add_child(AstNode *parent, AstNode *child) {
    if (!child) return;
    if (parent->n_children == parent->cap_children) {
        int new_cap = parent->cap_children ? parent->cap_children * 2 : 4;
        parent->children = realloc(parent->children, (size_t)new_cap * sizeof(AstNode *));
        if (!parent->children) { perror("realloc"); exit(1); }
        parent->cap_children = new_cap;
    }
    parent->children[parent->n_children++] = child;
}

void ast_free(AstNode *n) {
    if (!n) return;
    for (int i = 0; i < n->n_children; i++) ast_free(n->children[i]);
    free(n->children);
    free(n->text);
    free(n);
}

/* ------------------------------------------------------------------ printer */

static const char *kind_name(NodeKind k) {
    switch (k) {
        case ND_PROGRAM:       return "Program";
        case ND_ELEMENT_DECL:  return "ElementDecl";
        case ND_COMPOUND_DECL: return "CompoundDecl";
        case ND_REACTION_DECL: return "ReactionDecl";
        case ND_FUNCTION_DECL: return "FunctionDecl";
        case ND_ELEMENT_FIELD: return "ElementField";
        case ND_COMPOUND_FIELD:return "CompoundField";
        case ND_REACTION_FIELD:return "ReactionField";
        case ND_PARAM:         return "Param";
        case ND_BLOCK:         return "Block";
        case ND_VAR_DECL:      return "VarDecl";
        case ND_ASSIGN:        return "Assign";
        case ND_AUG_ASSIGN:    return "AugAssign";
        case ND_RETURN:        return "Return";
        case ND_PRINT:         return "Print";
        case ND_EXPR_STMT:     return "ExprStmt";
        case ND_FOR:           return "For";
        case ND_IF:            return "If";
        case ND_BINARY:        return "Binary";
        case ND_UNARY:         return "Unary";
        case ND_CALL:          return "Call";
        case ND_INDEX:         return "Index";
        case ND_MEMBER:        return "Member";
        case ND_IDENT:         return "Ident";
        case ND_NUMBER:        return "Number";
        case ND_QUANTITY:      return "Quantity";
        case ND_STRING:        return "String";
        case ND_CHAR:          return "Char";
        case ND_FORMULA:       return "Formula";
        case ND_LIST:          return "List";
        case ND_REACTION_EXPR: return "ReactionExpr";
        case ND_REACTION_TERM: return "ReactionTerm";
        case ND_ENERGY_EXPR:   return "EnergyExpr";
        default:               return "?";
    }
}

/* Box-drawing tree printer — each node shows its kind and optional text. */
static void print_node(const AstNode *n, const char *prefix, int is_last) {
    const char *connector = is_last ? "└── " : "├── ";
    printf("%s%s%s", prefix, connector, kind_name(n->kind));
    if (n->text && n->text[0]) {
        printf(" [%s]", n->text);
    } else if (n->kind == ND_NUMBER) {
        printf(" [%g]", n->num);
    }
    printf("\n");

    char new_prefix[512];
    snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_last ? "    " : "│   ");

    for (int i = 0; i < n->n_children; i++) {
        print_node(n->children[i], new_prefix, i == n->n_children - 1);
    }
}

void ast_print(const AstNode *n, int indent) {
    (void)indent;
    if (!n) return;
    printf("%s", kind_name(n->kind));
    if (n->text && n->text[0]) printf(" [%s]", n->text);
    printf("\n");
    for (int i = 0; i < n->n_children; i++) {
        print_node(n->children[i], "", i == n->n_children - 1);
    }
}
