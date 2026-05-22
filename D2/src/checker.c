#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "checker.h"
#include "ast.h"
#include "error.h"
#include "value.h"

/* ================================================================
   Global declaration tables (checker-private)
   ================================================================ */

#define MAX_DECLS 256

/* Elements: indexed by symbol (e.g. "H") */
static char *ck_elem_sym[MAX_DECLS];
static int   ck_n_elems;

/* Compounds: name + formula string */
static char *ck_comp_name[MAX_DECLS];
static char *ck_comp_formula[MAX_DECLS];
static int   ck_n_comps;

/* Reactions */
static char *ck_rxn_name[MAX_DECLS];
static int   ck_n_rxns;

/* Functions: name + number of params + param types */
#define MAX_PARAMS 8
typedef struct {
    char *name;
    int   n_params;
    char *param_type[MAX_PARAMS]; /* "compound", "reaction", "element", or "" */
    char *param_name[MAX_PARAMS];
} FuncSig;

static FuncSig ck_funcs[MAX_DECLS];
static int      ck_n_funcs;

/* ================================================================
   Type environment (local scope for checker)
   ================================================================ */

#define TENV_SLOTS 256
typedef struct TypeEnv {
    struct TypeEnv *parent;
    char *names[TENV_SLOTS];
    char *types[TENV_SLOTS]; /* "number", "string", "compound", etc. */
    int   count;
} TypeEnv;

static TypeEnv *tenv_new(TypeEnv *parent) {
    TypeEnv *e = calloc(1, sizeof(TypeEnv));
    e->parent = parent;
    return e;
}

static void tenv_free(TypeEnv *e) {
    if (!e) return;
    for (int i = 0; i < e->count; i++) {
        free(e->names[i]);
        free(e->types[i]);
    }
    free(e);
}

static const char *tenv_lookup(TypeEnv *e, const char *name) {
    for (TypeEnv *cur = e; cur; cur = cur->parent) {
        for (int i = cur->count - 1; i >= 0; i--) {
            if (strcmp(cur->names[i], name) == 0) return cur->types[i];
        }
    }
    return NULL;
}

static void tenv_bind(TypeEnv *e, const char *name, const char *type) {
    if (e->count >= TENV_SLOTS) return;
    e->names[e->count] = strdup(name);
    e->types[e->count] = strdup(type);
    e->count++;
}

/* List types are represented as list<T> so the checker can preserve the
   element type through indexing and for-in loops. */
static int type_is_list(const char *type) {
    size_t len;
    if (!type) return 0;
    len = strlen(type);
    return len > 6 && strncmp(type, "list<", 5) == 0 && type[len - 1] == '>';
}

static char *make_list_type(const char *elem_type) {
    const char *elem = elem_type ? elem_type : "unknown";
    size_t len = strlen(elem) + strlen("list<>") + 1;
    char *out = malloc(len);
    snprintf(out, len, "list<%s>", elem);
    return out;
}

static char *list_element_type(const char *list_type) {
    size_t len;
    char *out;
    if (!type_is_list(list_type)) return strdup("unknown");
    len = strlen(list_type);
    out = malloc(len - 5); /* total len - "list<" - ">" + NUL */
    memcpy(out, list_type + 5, len - 6);
    out[len - 6] = '\0';
    return out;
}

/* ================================================================
   Lookup helpers
   ================================================================ */

static int elem_is_declared(const char *sym) {
    for (int i = 0; i < ck_n_elems; i++)
        if (strcmp(ck_elem_sym[i], sym) == 0) return 1;
    return 0;
}

static const char *comp_formula_for(const char *name) {
    for (int i = 0; i < ck_n_comps; i++)
        if (strcmp(ck_comp_name[i], name) == 0) return ck_comp_formula[i];
    return NULL;
}

static int comp_is_declared(const char *name) {
    return comp_formula_for(name) != NULL;
}

static const char *comp_name_for_formula(const char *formula_text) {
    FormulaVal *target = formula_parse(formula_text);
    const char *matched = NULL;

    for (int i = 0; i < ck_n_comps; i++) {
        FormulaVal *candidate = formula_parse(ck_comp_formula[i]);
        if (formula_equal(target, candidate)) {
            matched = ck_comp_name[i];
            formula_free(candidate);
            break;
        }
        formula_free(candidate);
    }

    formula_free(target);
    return matched;
}

static int rxn_is_declared(const char *name) {
    for (int i = 0; i < ck_n_rxns; i++)
        if (strcmp(ck_rxn_name[i], name) == 0) return 1;
    return 0;
}

static FuncSig *func_sig(const char *name) {
    for (int i = 0; i < ck_n_funcs; i++)
        if (strcmp(ck_funcs[i].name, name) == 0) return &ck_funcs[i];
    return NULL;
}

/* ================================================================
   Formula validation (all symbols must be declared elements)
   ================================================================ */

static void check_formula_symbols(const char *formula_text, int line) {
    FormulaVal *f = formula_parse(formula_text);
    for (int i = 0; i < f->size; i++) {
        if (!elem_is_declared(f->symbols[i])) {
            error_at(line, 0,
                "formula '%s' uses undeclared element '%s'",
                formula_text, f->symbols[i]);
        }
    }
    formula_free(f);
}

/* ================================================================
   Atom-map helpers for reaction balance check
   ================================================================ */

typedef struct { char sym[4]; double count; } AtomEntry;
typedef struct { AtomEntry e[64]; int n; } AtomMap;

static void amap_add(AtomMap *m, const char *sym, double cnt) {
    for (int i = 0; i < m->n; i++) {
        if (strcmp(m->e[i].sym, sym) == 0) { m->e[i].count += cnt; return; }
    }
    strncpy(m->e[m->n].sym, sym, 3);
    m->e[m->n].sym[3] = '\0';
    m->e[m->n].count = cnt;
    m->n++;
}

/* Build atom map from an ND_REACTION_EXPR node.
 * Unknown compound species are looked up in ck_comp_formula. */
static void build_amap(const AstNode *rxn_expr, AtomMap *m) {
    for (int i = 0; i < rxn_expr->n_children; i++) {
        const AstNode *term = rxn_expr->children[i]; /* ND_REACTION_TERM */
        double coeff = 1.0;
        const AstNode *species = NULL;

        if (term->n_children == 2) {
            coeff   = term->children[0]->num;
            species = term->children[1];
        } else if (term->n_children == 1) {
            species = term->children[0];
        } else continue;

        const char *formula_text = NULL;
        if (species->kind == ND_FORMULA) {
            formula_text = species->text;
        } else if (species->kind == ND_IDENT) {
            formula_text = comp_formula_for(species->text);
        }
        if (!formula_text) continue;

        FormulaVal *f = formula_parse(formula_text);
        for (int j = 0; j < f->size; j++)
            amap_add(m, f->symbols[j], coeff * f->counts[j]);
        formula_free(f);
    }
}

static int amaps_equal(const AtomMap *a, const AtomMap *b) {
    /* Check every symbol in a matches b */
    for (int i = 0; i < a->n; i++) {
        double bc = 0;
        for (int j = 0; j < b->n; j++)
            if (strcmp(a->e[i].sym, b->e[j].sym) == 0) { bc = b->e[j].count; break; }
        if (a->e[i].count != bc) return 0;
    }
    /* Check every symbol in b is covered by a */
    for (int i = 0; i < b->n; i++) {
        double ac = 0;
        for (int j = 0; j < a->n; j++)
            if (strcmp(b->e[i].sym, a->e[j].sym) == 0) { ac = a->e[j].count; break; }
        if (b->e[i].count != ac) return 0;
    }
    return 1;
}

/* ================================================================
   Expression type inference
   ================================================================ */

static const char *check_expr(const AstNode *node, TypeEnv *tenv);
static const char *check_assign_target(const AstNode *node, TypeEnv *tenv);

static const char *check_call(const AstNode *call, TypeEnv *tenv) {
    /* call->children[0] = callee ident; [1..] = args */
    if (call->n_children < 1) return "unknown";
    const char *callee = call->children[0]->text;
    int n_args = call->n_children - 1;

    /* Built-in mol() */
    if (strcmp(callee, "mol") == 0) {
        if (n_args != 1) {
            error_at(call->line, 0, "mol() expects 1 argument, got %d", n_args);
            return "number";
        }
        const char *arg_type = check_expr(call->children[1], tenv);
        if (strcmp(arg_type, "number") != 0 &&
            strcmp(arg_type, "quantity") != 0 &&
            strcmp(arg_type, "unknown") != 0) {
            error_at(call->children[1]->line, 0,
                "mol() expects number or quantity, got %s", arg_type);
        }
        return "number";
    }

    FuncSig *sig = func_sig(callee);
    if (!sig) {
        error_at(call->line, 0, "call to undeclared function '%s'", callee);
        return "unknown";
    }
    if (n_args != sig->n_params) {
        error_at(call->line, 0,
            "function '%s' expects %d argument(s), got %d",
            callee, sig->n_params, n_args);
        return "unknown";
    }
    /* Check typed parameters */
    for (int i = 0; i < sig->n_params; i++) {
        const char *got = check_expr(call->children[i + 1], tenv);
        const char *expected = sig->param_type[i];
        if (!expected || expected[0] == '\0') continue; /* untyped */
        if (strcmp(got, expected) == 0) continue;
        if (strcmp(got, "unknown") == 0) continue; /* propagate errors silently */
        error_at(call->children[i+1]->line, 0,
            "argument %d of '%s': expected %s, got %s",
            i + 1, callee, expected, got);
    }
    return "unknown"; /* return type not statically known */
}

static const char *check_expr(const AstNode *node, TypeEnv *tenv) {
    if (!node) return "unknown";
    switch (node->kind) {
        case ND_NUMBER:   return "number";
        case ND_STRING:   return "string";
        case ND_CHAR:     return "char";
        case ND_FORMULA:  return "formula";
        case ND_QUANTITY: return "quantity";
        case ND_LIST: {
            if (node->n_children == 0) {
                error_at(node->line, 0,
                    "empty list requires an element type");
                return "list<unknown>";
            }
            const char *elem_type = check_expr(node->children[0], tenv);
            for (int i = 1; i < node->n_children; i++) {
                const char *t = check_expr(node->children[i], tenv);
                if (strcmp(elem_type, "unknown") != 0 && strcmp(t, "unknown") != 0
                    && strcmp(elem_type, t) != 0) {
                    error_at(node->children[i]->line, 0,
                        "list elements must all be the same type: "
                        "expected %s, got %s", elem_type, t);
                }
            }
            return make_list_type(elem_type);
        }

        case ND_IDENT: {
            /* Check local type env first, then global declarations */
            const char *t = tenv_lookup(tenv, node->text);
            if (t) return t;
            if (elem_is_declared(node->text))  return "element";
            if (comp_is_declared(node->text))  return "compound";
            if (rxn_is_declared(node->text))   return "reaction";
            if (func_sig(node->text))          return "function";
            if (strcmp(node->text, "mol") == 0) return "function";
            error_at(node->line, 0, "undeclared name '%s'", node->text);
            return "unknown";
        }

        case ND_UNARY: {
            const char *t = check_expr(node->children[0], tenv);
            if (strcmp(t, "number") != 0 && strcmp(t, "unknown") != 0)
                error_at(node->line, 0, "unary '-' requires number, got %s", t);
            return "number";
        }

        case ND_BINARY: {
            const char *ltype = check_expr(node->children[0], tenv);
            const char *rtype = check_expr(node->children[1], tenv);
            const char *op    = node->text;
            if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
                strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
                if (strcmp(ltype, "unknown") != 0 && strcmp(ltype, "number") != 0)
                    error_at(node->line, 0, "operator '%s' requires numbers, left is %s", op, ltype);
                if (strcmp(rtype, "unknown") != 0 && strcmp(rtype, "number") != 0)
                    error_at(node->line, 0, "operator '%s' requires numbers, right is %s", op, rtype);
                return "number";
            }
            /* Comparison */
            (void)ltype; (void)rtype;
            return "bool";
        }

        case ND_MEMBER: {
            const char *obj = check_expr(node->children[0], tenv);
            const char *field = node->text;
            if (strcmp(obj, "element") == 0) {
                if (strcmp(field, "atomic_mass") == 0) return "number";
                if (strcmp(field, "valence") == 0)     return "number";
                if (strcmp(field, "symbol") == 0)      return "string";
                error_at(node->line, 0, "element has no field '%s'", field);
                return "unknown";
            }
            if (strcmp(obj, "compound") == 0) {
                if (strcmp(field, "formula") == 0) return "formula";
                if (strcmp(field, "name") == 0)    return "string";
                error_at(node->line, 0, "compound has no field '%s'", field);
                return "unknown";
            }
            if (strcmp(obj, "reaction") == 0) {
                if (strcmp(field, "reactants") == 0 || strcmp(field, "products") == 0)
                    return "reaction_side";
                error_at(node->line, 0, "reaction has no field '%s'", field);
                return "unknown";
            }
            if (strcmp(obj, "unknown") != 0) {
                error_at(node->line, 0,
                    "member access '.%s' requires element, compound, or reaction, got %s",
                    field, obj);
            }
            return "unknown";
        }

        case ND_INDEX: {
            const char *obj = check_expr(node->children[0], tenv);
            const char *idx = check_expr(node->children[1], tenv);
            if (type_is_list(obj)) {
                if (strcmp(idx, "number") != 0 && strcmp(idx, "unknown") != 0)
                    error_at(node->children[1]->line, 0,
                        "list index must be number, got %s", idx);
                return list_element_type(obj);
            }
            if (strcmp(obj, "formula") == 0) {
                if (strcmp(idx, "element") != 0 && strcmp(idx, "unknown") != 0)
                    error_at(node->children[1]->line, 0,
                        "formula index must be element, got %s", idx);
                return "number";
            }
            if (strcmp(obj, "reaction_side") == 0) {
                if (strcmp(idx, "compound") != 0 && strcmp(idx, "formula") != 0
                    && strcmp(idx, "unknown") != 0)
                    error_at(node->children[1]->line, 0,
                        "reaction_side index must be compound, got %s", idx);
                return "number";
            }
            if (strcmp(obj, "unknown") != 0) {
                error_at(node->line, 0,
                    "index operator not supported for type %s", obj);
            }
            return "unknown";
        }

        case ND_CALL:
            return check_call(node, tenv);

        default:
            return "unknown";
    }
}

static const char *check_assign_target(const AstNode *node, TypeEnv *tenv) {
    if (!node) return "unknown";

    if (node->kind == ND_IDENT) {
        const char *type = check_expr(node, tenv);
        if (strcmp(type, "function") == 0) {
            error_at(node->line, 0,
                "function name cannot be assigned to");
            return "unknown";
        }
        return type;
    }

    if (node->kind == ND_INDEX) {
        const char *obj = check_expr(node->children[0], tenv);
        const char *idx = check_expr(node->children[1], tenv);

        if (type_is_list(obj)) {
            if (strcmp(idx, "number") != 0 && strcmp(idx, "unknown") != 0)
                error_at(node->children[1]->line, 0,
                    "list index must be number, got %s", idx);
            return list_element_type(obj);
        }

        if (strcmp(obj, "formula") == 0) {
            error_at(node->line, 0,
                "formula atom counts are read-only assignment targets");
            return "number";
        }

        if (strcmp(obj, "reaction_side") == 0) {
            error_at(node->line, 0,
                "reaction side coefficients are read-only assignment targets");
            return "number";
        }

        if (strcmp(obj, "unknown") != 0)
            error_at(node->line, 0,
                "assignment target is not writable");
        return "unknown";
    }

    if (node->kind == ND_MEMBER) {
        const char *type = check_expr(node, tenv);
        error_at(node->line, 0,
            "record fields are read-only assignment targets");
        return type;
    }

    if (node->kind == ND_CALL) {
        check_expr(node, tenv);
        error_at(node->line, 0,
            "function call cannot be an assignment target");
        return "unknown";
    }

    check_expr(node, tenv);
    error_at(node->line, 0,
        "invalid assignment target");
    return "unknown";
}

/* ================================================================
   Statement checker
   ================================================================ */

static void check_stmt(const AstNode *node, TypeEnv *tenv);

static void check_block(const AstNode *block, TypeEnv *tenv) {
    for (int i = 0; i < block->n_children; i++)
        check_stmt(block->children[i], tenv);
}

static void check_stmt(const AstNode *node, TypeEnv *tenv) {
    if (!node) return;
    switch (node->kind) {
        case ND_VAR_DECL: {
            const char *t = check_expr(node->children[0], tenv);
            tenv_bind(tenv, node->text, t);
            break;
        }
        case ND_ASSIGN: {
            const char *ltype = check_assign_target(node->children[0], tenv);
            const char *rtype = check_expr(node->children[1], tenv);
            if (strcmp(ltype, "unknown") != 0 && strcmp(rtype, "unknown") != 0
                && strcmp(ltype, rtype) != 0)
                error_at(node->line, 0,
                    "assignment type mismatch: variable is %s, value is %s",
                    ltype, rtype);
            break;
        }
        case ND_AUG_ASSIGN: {
            const char *ltype = check_assign_target(node->children[0], tenv);
            const char *rtype = check_expr(node->children[1], tenv);
            if (strcmp(ltype, "unknown") != 0 && strcmp(ltype, "number") != 0)
                error_at(node->line, 0,
                    "augmented assignment requires number, left is %s", ltype);
            if (strcmp(rtype, "unknown") != 0 && strcmp(rtype, "number") != 0)
                error_at(node->line, 0,
                    "augmented assignment requires number, right is %s", rtype);
            break;
        }
        case ND_PRINT:
            check_expr(node->children[0], tenv);
            break;
        case ND_RETURN:
            if (node->n_children > 0) check_expr(node->children[0], tenv);
            break;
        case ND_EXPR_STMT:
            check_expr(node->children[0], tenv);
            break;
        case ND_BLOCK:
            check_block(node, tenv);
            break;
        case ND_FOR: {
            /* children: [0]=var_ident, [1]=iterable, [2]=body */
            const char *iter_type = check_expr(node->children[1], tenv);
            /* Determine the loop variable type */
            const char *var_type = "unknown";
            if (strcmp(iter_type, "formula") == 0)        var_type = "element";
            else if (type_is_list(iter_type))              var_type = list_element_type(iter_type);
            else if (strcmp(iter_type, "reaction_side") == 0) var_type = "compound";
            TypeEnv *inner = tenv_new(tenv);
            tenv_bind(inner, node->children[0]->text, var_type);
            check_block(node->children[2], inner);
            tenv_free(inner);
            break;
        }
        case ND_IF: {
            /* alternating cond/block pairs, optional final else block */
            int i = 0;
            while (i + 1 < node->n_children) {
                const char *ctype = check_expr(node->children[i], tenv);
                if (strcmp(ctype, "bool") != 0 && strcmp(ctype, "unknown") != 0)
                    error_at(node->children[i]->line, 0,
                        "if/elif condition must be bool, got %s", ctype);
                check_block(node->children[i+1], tenv);
                i += 2;
            }
            if (i < node->n_children) check_block(node->children[i], tenv);
            break;
        }
        default: break;
    }
}

/* ================================================================
   Declaration checkers
   ================================================================ */

static void check_element_decl(const AstNode *decl) {
    int has_mass = 0, has_val = 0, has_sym = 0;
    for (int i = 0; i < decl->n_children; i++) {
        const AstNode *field = decl->children[i];
        const char *key = field->text;
        if (strcmp(key, "atomic_mass") == 0) {
            has_mass++;
            if (field->n_children < 1 || field->children[0]->kind != ND_NUMBER)
                error_at(field->line, 0, "element.atomic_mass must be a number");
        } else if (strcmp(key, "valence") == 0) {
            has_val++;
            if (field->n_children < 1 || field->children[0]->kind != ND_NUMBER)
                error_at(field->line, 0, "element.valence must be a number");
        } else if (strcmp(key, "symbol") == 0) {
            has_sym++;
            if (field->n_children < 1 || field->children[0]->kind != ND_STRING)
                error_at(field->line, 0, "element.symbol must be a string");
        }
    }
    if (!has_mass) error_at(decl->line, 0, "element '%s' missing atomic_mass", decl->text);
    if (!has_val)  error_at(decl->line, 0, "element '%s' missing valence",     decl->text);
    if (!has_sym)  error_at(decl->line, 0, "element '%s' missing symbol",      decl->text);
    if (has_mass > 1 || has_val > 1 || has_sym > 1)
        error_at(decl->line, 0, "element '%s' has duplicate fields", decl->text);
}

static void check_compound_decl(const AstNode *decl) {
    int has_formula = 0, has_name = 0;
    const char *formula_text = NULL;
    for (int i = 0; i < decl->n_children; i++) {
        const AstNode *field = decl->children[i];
        const char *key = field->text;
        if (strcmp(key, "formula") == 0) {
            has_formula++;
            if (field->n_children >= 1 && field->children[0]->kind == ND_FORMULA) {
                formula_text = field->children[0]->text;
                check_formula_symbols(formula_text, field->line);
            } else {
                error_at(field->line, 0, "compound.formula must be a formula literal");
            }
        } else if (strcmp(key, "name") == 0) {
            has_name++;
            if (field->n_children < 1 || field->children[0]->kind != ND_STRING)
                error_at(field->line, 0, "compound.name must be a string");
        }
    }
    if (!has_formula) error_at(decl->line, 0, "compound '%s' missing formula", decl->text);
    if (!has_name)    error_at(decl->line, 0, "compound '%s' missing name",    decl->text);
    if (has_formula > 1 || has_name > 1)
        error_at(decl->line, 0, "compound '%s' has duplicate fields", decl->text);
}

static void check_reaction_decl(const AstNode *decl) {
    int has_reactants = 0, has_products = 0, has_check = 0, has_energy = 0;
    const AstNode *rxn_reactants = NULL, *rxn_products = NULL;
    int line = decl->line;

    for (int i = 0; i < decl->n_children; i++) {
        const AstNode *field = decl->children[i];
        const char *key = field->text;

        if (strcmp(key, "reactants") == 0) {
            has_reactants++;
            rxn_reactants = field->children[0]; /* ND_REACTION_EXPR */
            /* Validate all species formulas */
            for (int j = 0; j < rxn_reactants->n_children; j++) {
                const AstNode *term = rxn_reactants->children[j];
                const AstNode *sp = term->n_children == 2 ? term->children[1] : term->children[0];
                if (sp->kind == ND_FORMULA) {
                    check_formula_symbols(sp->text, sp->line);
                    if (!comp_name_for_formula(sp->text))
                        error_at(sp->line, 0,
                            "reaction species formula '%s' must match a declared compound",
                            sp->text);
                } else if (sp->kind == ND_IDENT && !comp_is_declared(sp->text)) {
                    error_at(sp->line, 0, "unknown compound '%s' in reaction", sp->text);
                }
            }
        } else if (strcmp(key, "products") == 0) {
            has_products++;
            rxn_products = field->children[0];
            for (int j = 0; j < rxn_products->n_children; j++) {
                const AstNode *term = rxn_products->children[j];
                const AstNode *sp = term->n_children == 2 ? term->children[1] : term->children[0];
                if (sp->kind == ND_FORMULA) {
                    check_formula_symbols(sp->text, sp->line);
                    if (!comp_name_for_formula(sp->text))
                        error_at(sp->line, 0,
                            "reaction species formula '%s' must match a declared compound",
                            sp->text);
                } else if (sp->kind == ND_IDENT && !comp_is_declared(sp->text)) {
                    error_at(sp->line, 0, "unknown compound '%s' in reaction", sp->text);
                }
            }
        } else if (strcmp(key, "check") == 0) {
            has_check++;
        } else if (strcmp(key, "energy") == 0) {
            has_energy++;
            if (field->n_children >= 1) {
                const AstNode *energy = field->children[0]; /* ND_ENERGY_EXPR */
                if (energy->kind == ND_ENERGY_EXPR) {
                    if (strcmp(energy->text, "exothermic") != 0 &&
                        strcmp(energy->text, "endothermic") != 0) {
                        error_at(energy->line, 0,
                            "reaction energy must be exothermic or endothermic");
                    }
                    if (energy->n_children >= 1) {
                        const AstNode *qty = energy->children[0];
                        if (qty->kind == ND_QUANTITY) {
                            Unit u = parse_unit(qty->text);
                            if (u != UNIT_KJ && u != UNIT_KJ_MOL)
                                error_at(qty->line, 0,
                                    "reaction energy unit must be kJ or kJ/mol");
                        }
                    }
                }
            }
        }
    }

    if (!has_reactants) error_at(line, 0, "reaction '%s' missing reactants", decl->text);
    if (!has_products)  error_at(line, 0, "reaction '%s' missing products",  decl->text);
    if (!has_check)     error_at(line, 0, "reaction '%s' missing check",     decl->text);
    if (!has_energy)    error_at(line, 0, "reaction '%s' missing energy",    decl->text);
    if (has_reactants > 1 || has_products > 1 || has_check > 1 || has_energy > 1)
        error_at(line, 0, "reaction '%s' has duplicate fields", decl->text);

    /* Balance check */
    if (rxn_reactants && rxn_products) {
        AtomMap left = {{{0}}, 0}, right = {{{0}}, 0};
        build_amap(rxn_reactants, &left);
        build_amap(rxn_products,  &right);
        if (!amaps_equal(&left, &right))
            error_at(line, 0,
                "reaction '%s' is not balanced (atom counts differ)", decl->text);
    }
}

/* Parse a parameter text like "compound c" or just "c" */
static void parse_param_text(const char *text,
                              char **type_out, char **name_out) {
    const char *sp = strchr(text, ' ');
    if (sp) {
        size_t tlen = (size_t)(sp - text);
        *type_out = malloc(tlen + 1);
        memcpy(*type_out, text, tlen);
        (*type_out)[tlen] = '\0';
        *name_out = strdup(sp + 1);
    } else {
        *type_out = strdup("");
        *name_out = strdup(text);
    }
}

static void check_function_decl(const AstNode *decl, TypeEnv *global_tenv) {
    /* Parameters: all children except last (body block) */
    int n_params = decl->n_children - 1;
    /* Parent = global_tenv so top-level lets are visible inside functions */
    TypeEnv *inner = tenv_new(global_tenv);

    for (int i = 0; i < n_params; i++) {
        const AstNode *param = decl->children[i];
        char *ptype, *pname;
        parse_param_text(param->text, &ptype, &pname);

        /* Validate type annotation */
        if (ptype[0] != '\0') {
            if (strcmp(ptype, "element") != 0 &&
                strcmp(ptype, "compound") != 0 &&
                strcmp(ptype, "reaction") != 0) {
                error_at(param->line, 0,
                    "unknown type annotation '%s'", ptype);
            }
        }
        const char *bound_type = (ptype[0] != '\0') ? ptype : "unknown";
        tenv_bind(inner, pname, bound_type);
        free(ptype);
        free(pname);
    }

    /* Check body */
    if (decl->n_children > 0) {
        AstNode *body = decl->children[decl->n_children - 1];
        check_block(body, inner);
    }
    tenv_free(inner);
}

/* ================================================================
   Pass 1: collect declaration names
   ================================================================ */

static void collect_declarations(const AstNode *prog) {
    /* Register mol() as a built-in function */
    ck_funcs[ck_n_funcs].name = "mol";
    ck_funcs[ck_n_funcs].n_params = 1;
    ck_funcs[ck_n_funcs].param_type[0] = "";
    ck_funcs[ck_n_funcs].param_name[0] = "x";
    ck_n_funcs++;

    for (int i = 0; i < prog->n_children; i++) {
        const AstNode *node = prog->children[i];
        switch (node->kind) {
            case ND_ELEMENT_DECL:
                if (elem_is_declared(node->text)) {
                    error_at(node->line, 0,
                        "element '%s' already declared", node->text);
                } else if (ck_n_elems < MAX_DECLS) {
                    ck_elem_sym[ck_n_elems++] = node->text;
                }
                break;
            case ND_COMPOUND_DECL: {
                if (comp_is_declared(node->text)) {
                    error_at(node->line, 0,
                        "compound '%s' already declared", node->text);
                    break;
                }
                if (ck_n_comps >= MAX_DECLS) break;
                ck_comp_name[ck_n_comps] = node->text;
                /* Extract formula string */
                ck_comp_formula[ck_n_comps] = "";
                for (int j = 0; j < node->n_children; j++) {
                    const AstNode *f = node->children[j];
                    if (strcmp(f->text, "formula") == 0 && f->n_children > 0)
                        ck_comp_formula[ck_n_comps] = f->children[0]->text;
                }
                ck_n_comps++;
                break;
            }
            case ND_REACTION_DECL:
                if (rxn_is_declared(node->text)) {
                    error_at(node->line, 0,
                        "reaction '%s' already declared", node->text);
                } else if (ck_n_rxns < MAX_DECLS) {
                    ck_rxn_name[ck_n_rxns++] = node->text;
                }
                break;
            case ND_FUNCTION_DECL: {
                if (func_sig(node->text)) {
                    error_at(node->line, 0,
                        "function '%s' already declared", node->text);
                    break;
                }
                if (ck_n_funcs >= MAX_DECLS) break;
                FuncSig *sig = &ck_funcs[ck_n_funcs++];
                sig->name = node->text;
                int n_params = node->n_children - 1;
                sig->n_params = n_params > MAX_PARAMS ? MAX_PARAMS : n_params;
                for (int j = 0; j < sig->n_params; j++) {
                    const AstNode *param = node->children[j];
                    char *ptype, *pname;
                    parse_param_text(param->text, &ptype, &pname);
                    sig->param_type[j] = ptype; /* leak is fine for course project */
                    sig->param_name[j] = pname;
                }
                break;
            }
            default: break;
        }
    }
}

/* ================================================================
   Pass 2: validate each declaration and top-level statement
   ================================================================ */

static void validate_all(const AstNode *prog) {
    TypeEnv *global_tenv = tenv_new(NULL);

    /* Add global variable bindings as we encounter them */
    for (int i = 0; i < prog->n_children; i++) {
        const AstNode *node = prog->children[i];
        switch (node->kind) {
            case ND_ELEMENT_DECL:
                check_element_decl(node);
                tenv_bind(global_tenv, node->text, "element");
                break;
            case ND_COMPOUND_DECL:
                check_compound_decl(node);
                tenv_bind(global_tenv, node->text, "compound");
                break;
            case ND_REACTION_DECL:
                check_reaction_decl(node);
                tenv_bind(global_tenv, node->text, "reaction");
                break;
            case ND_FUNCTION_DECL:
                check_function_decl(node, global_tenv);
                tenv_bind(global_tenv, node->text, "function");
                break;
            case ND_VAR_DECL: {
                const char *t = check_expr(node->children[0], global_tenv);
                tenv_bind(global_tenv, node->text, t);
                break;
            }
            default:
                check_stmt(node, global_tenv);
                break;
        }
    }
    tenv_free(global_tenv);
}

/* ================================================================
   Public API
   ================================================================ */

void check_program(const AstNode *prog) {
    /* Reset checker tables */
    ck_n_elems = ck_n_comps = ck_n_rxns = ck_n_funcs = 0;

    collect_declarations(prog);
    validate_all(prog);
}
