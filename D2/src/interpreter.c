#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "interpreter.h"
#include "ast.h"
#include "value.h"
#include "error.h"

/* ================================================================
   Global state
   ================================================================ */

static Env *g_global_env = NULL;

/* Separate function table (functions are not first-class values) */
#define MAX_FUNCS 256
static AstNode *g_func_decls[MAX_FUNCS];
static char    *g_func_names[MAX_FUNCS];
static int      g_n_funcs;

/* ================================================================
   Runtime error
   ================================================================ */

static void runtime_error(int line, const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "%s:%d: runtime error: ", g_filename ? g_filename : "<?>", line);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}

/* ================================================================
   Helpers
   ================================================================ */

/* Strip surrounding quotes from string literal text, handle escapes */
static char *unquote_string(const char *s, int len) {
    if (len < 2) return strdup("");
    char *result = malloc((size_t)(len));
    int ri = 0;
    for (int i = 1; i < len - 1; i++) {
        if (s[i] == '\\' && i + 1 < len - 1) {
            i++;
            switch (s[i]) {
                case 'n':  result[ri++] = '\n'; break;
                case 'r':  result[ri++] = '\r'; break;
                case 't':  result[ri++] = '\t'; break;
                case '"':  result[ri++] = '"';  break;
                case '\'': result[ri++] = '\''; break;
                case '\\': result[ri++] = '\\'; break;
                default:   result[ri++] = s[i]; break;
            }
        } else {
            result[ri++] = s[i];
        }
    }
    result[ri] = '\0';
    return result;
}

/* Extract char value from char literal text like 'H' or '\n' */
static char unquote_char(const char *s, int len) {
    if (len < 3) return '?';
    if (s[1] == '\\' && len >= 4) {
        switch (s[2]) {
            case 'n': return '\n';
            case 't': return '\t';
            case 'r': return '\r';
            case '\'': return '\'';
            case '\\': return '\\';
            default:   return s[2];
        }
    }
    return s[1];
}

/* Parse a parameter declaration text: "compound c" → type="compound", name="c" */
static void parse_param(const char *text, char **type_out, char **name_out) {
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

/* Try to resolve a formula value to a matching declared compound */
static Value resolve_formula_to_compound(const FormulaVal *fv) {
    for (Env *e = g_global_env; e; e = e->parent) {
        for (int i = 0; i < e->count; i++) {
            if (e->values[i].type == VAL_COMPOUND) {
                CompoundVal *c = e->values[i].compound;
                if (formula_equal(c->formula, fv)) {
                    return make_compound(c);
                }
            }
        }
    }
    Value f; f.type = VAL_FORMULA; f.formula = (FormulaVal *)fv;
    return f; /* return as-is if no match */
}

/* Find formula of a value (Compound or Formula) */
static const FormulaVal *get_formula_of(const Value *v) {
    if (v->type == VAL_COMPOUND) return v->compound->formula;
    if (v->type == VAL_FORMULA)  return v->formula;
    return NULL;
}

/* Look up coefficient of a species in a ReactionSide by formula match */
static double reaction_side_coeff(const ReactionSideVal *rs, const Value *key) {
    const FormulaVal *kf = get_formula_of(key);
    if (!kf) return 0.0;
    for (int i = 0; i < rs->count; i++) {
        const FormulaVal *sf = get_formula_of(rs->terms[i].species);
        if (sf && formula_equal(sf, kf)) return rs->terms[i].coefficient;
    }
    return 0.0;
}

/* ================================================================
   Build runtime values from AST declaration nodes
   ================================================================ */

static ElementVal *build_element(const AstNode *decl) {
    ElementVal *e = calloc(1, sizeof(ElementVal));
    e->id = strdup(decl->text);
    for (int i = 0; i < decl->n_children; i++) {
        const AstNode *field = decl->children[i];
        if (strcmp(field->text, "atomic_mass") == 0)
            e->atomic_mass = field->children[0]->num;
        else if (strcmp(field->text, "valence") == 0)
            e->valence = (int)field->children[0]->num;
        else if (strcmp(field->text, "symbol") == 0) {
            const char *s = field->children[0]->text;
            e->symbol = unquote_string(s, (int)strlen(s));
        }
    }
    return e;
}

static CompoundVal *build_compound(const AstNode *decl) {
    CompoundVal *c = calloc(1, sizeof(CompoundVal));
    c->id = strdup(decl->text);
    for (int i = 0; i < decl->n_children; i++) {
        const AstNode *field = decl->children[i];
        if (strcmp(field->text, "formula") == 0) {
            c->formula = formula_parse(field->children[0]->text);
        } else if (strcmp(field->text, "name") == 0) {
            const char *s = field->children[0]->text;
            c->name = unquote_string(s, (int)strlen(s));
        }
    }
    return c;
}

/* Build ReactionSideVal from an ND_REACTION_EXPR node */
static ReactionSideVal build_reaction_side(const AstNode *rxn_expr, int line) {
    ReactionSideVal rs;
    rs.count = rxn_expr->n_children;
    rs.terms = calloc((size_t)(rs.count > 0 ? rs.count : 1), sizeof(ReactionTerm));

    for (int i = 0; i < rxn_expr->n_children; i++) {
        const AstNode *term = rxn_expr->children[i]; /* ND_REACTION_TERM */
        double coeff = 1.0;
        const AstNode *species_node = NULL;

        if (term->n_children == 2) {
            coeff        = term->children[0]->num;
            species_node = term->children[1];
        } else if (term->n_children == 1) {
            species_node = term->children[0];
        } else {
            runtime_error(line, "malformed reaction term");
        }

        rs.terms[i].coefficient = coeff;
        rs.terms[i].species = malloc(sizeof(Value));

        if (species_node->kind == ND_FORMULA) {
            /* Try to resolve formula literal to a compound */
            FormulaVal *fv = formula_parse(species_node->text);
            Value resolved = resolve_formula_to_compound(fv);
            if (resolved.type == VAL_COMPOUND) {
                formula_free(fv);
                *rs.terms[i].species = resolved;
            } else {
                *rs.terms[i].species = make_formula(fv);
            }
        } else { /* ND_IDENT = named compound */
            Value v;
            if (!env_lookup_opt(g_global_env, species_node->text, &v))
                runtime_error(line, "unknown reaction species '%s'", species_node->text);
            *rs.terms[i].species = v;
        }
    }
    return rs;
}

/* Compute atom counts for a ReactionSideVal (for balance check at runtime) */
typedef struct { char sym[4]; double cnt; } RAEntry;
typedef struct { RAEntry e[64]; int n; } RAtomMap;

static void ramap_add(RAtomMap *m, const char *sym, double c) {
    for (int i = 0; i < m->n; i++) {
        if (strcmp(m->e[i].sym, sym) == 0) { m->e[i].cnt += c; return; }
    }
    strncpy(m->e[m->n].sym, sym, 3); m->e[m->n].sym[3] = '\0';
    m->e[m->n].cnt = c; m->n++;
}

static RAtomMap reaction_side_atoms(const ReactionSideVal *rs) {
    RAtomMap m; m.n = 0;
    for (int i = 0; i < rs->count; i++) {
        const FormulaVal *f = get_formula_of(rs->terms[i].species);
        if (!f) continue;
        double coeff = rs->terms[i].coefficient;
        for (int j = 0; j < f->size; j++)
            ramap_add(&m, f->symbols[j], coeff * f->counts[j]);
    }
    return m;
}

static int ramaps_equal(const RAtomMap *a, const RAtomMap *b) {
    for (int i = 0; i < a->n; i++) {
        double bc = 0;
        for (int j = 0; j < b->n; j++)
            if (strcmp(a->e[i].sym, b->e[j].sym) == 0) { bc = b->e[j].cnt; break; }
        if (fabs(a->e[i].cnt - bc) > 1e-9) return 0;
    }
    for (int i = 0; i < b->n; i++) {
        double ac = 0;
        for (int j = 0; j < a->n; j++)
            if (strcmp(b->e[i].sym, a->e[j].sym) == 0) { ac = a->e[j].cnt; break; }
        if (fabs(b->e[i].cnt - ac) > 1e-9) return 0;
    }
    return 1;
}

static ReactionVal *build_reaction(const AstNode *decl) {
    ReactionVal *r = calloc(1, sizeof(ReactionVal));
    r->id = strdup(decl->text);

    const AstNode *rxn_reactants = NULL, *rxn_products = NULL;

    for (int i = 0; i < decl->n_children; i++) {
        const AstNode *field = decl->children[i]; /* ND_REACTION_FIELD */
        if (strcmp(field->text, "reactants") == 0) {
            rxn_reactants = field->children[0];
        } else if (strcmp(field->text, "products") == 0) {
            rxn_products = field->children[0];
        } else if (strcmp(field->text, "energy") == 0) {
            const AstNode *energy = field->children[0]; /* ND_ENERGY_EXPR */
            r->direction = strdup(energy->text);
            if (energy->n_children >= 1) {
                const AstNode *qty = energy->children[0];
                if (qty->kind == ND_QUANTITY) {
                    r->energy_value = qty->children[0]->num;
                    r->energy_unit  = parse_unit(qty->text);
                } else if (qty->kind == ND_NUMBER) {
                    r->energy_value = qty->num;
                    r->energy_unit  = UNIT_KJ;
                }
            }
        }
    }

    if (!rxn_reactants || !rxn_products)
        runtime_error(decl->line, "reaction '%s' missing reactants or products", decl->text);

    r->reactants = build_reaction_side(rxn_reactants, decl->line);
    r->products  = build_reaction_side(rxn_products,  decl->line);

    /* Runtime balance check (defence-in-depth after type checker) */
    RAtomMap left  = reaction_side_atoms(&r->reactants);
    RAtomMap right = reaction_side_atoms(&r->products);
    if (!ramaps_equal(&left, &right))
        runtime_error(decl->line, "reaction '%s' is not balanced", decl->text);

    return r;
}

/* ================================================================
   Function lookup
   ================================================================ */

static AstNode *find_func(const char *name, int call_line) {
    for (int i = 0; i < g_n_funcs; i++)
        if (strcmp(g_func_names[i], name) == 0) return g_func_decls[i];
    runtime_error(call_line, "call to undefined function '%s'", name);
    return NULL; /* unreachable */
}

/* ================================================================
   Forward declarations
   ================================================================ */

static Value eval_expr(const AstNode *node, Env *env);
static Value exec_stmt(const AstNode *node, Env *env);

/* ================================================================
   Function call
   ================================================================ */

static Value call_user_func(AstNode *func_decl, Value *args, int n_args, int call_line) {
    /* Count params: all children except last (body block) */
    int n_params = func_decl->n_children - 1;
    if (n_args != n_params)
        runtime_error(call_line,
            "function '%s' expects %d argument(s), got %d",
            func_decl->text, n_params, n_args);

    /* Create new scope with parent = global env (static scoping) */
    Env *call_env = env_new(g_global_env);

    for (int i = 0; i < n_params; i++) {
        char *ptype, *pname;
        parse_param(func_decl->children[i]->text, &ptype, &pname);

        Value arg = args[i];

        /* Formula-to-compound coercion for typed compound parameters */
        if (strcmp(ptype, "compound") == 0 && arg.type == VAL_FORMULA) {
            arg = resolve_formula_to_compound(arg.formula);
        }

        env_bind(call_env, pname, arg);
        free(ptype);
        free(pname);
    }

    /* Execute body (last child) */
    AstNode *body = func_decl->children[func_decl->n_children - 1];
    Value result = exec_stmt(body, call_env);
    env_free(call_env);

    if (result.type == VAL_RETURN) {
        Value ret = *result.return_val;
        free(result.return_val);
        return ret;
    }
    return make_void();
}

/* ================================================================
   Expression evaluator
   ================================================================ */

static Value eval_expr(const AstNode *node, Env *env) {
    if (!node) return make_void();

    switch (node->kind) {

        case ND_NUMBER:
            return make_number(node->num);

        case ND_STRING: {
            char *s = unquote_string(node->text, (int)strlen(node->text));
            return make_string_own(s);
        }

        case ND_CHAR:
            return make_char(unquote_char(node->text, (int)strlen(node->text)));

        case ND_FORMULA:
            return make_formula(formula_parse(node->text));

        case ND_QUANTITY: {
            double val = node->children[0]->num;
            Unit u = parse_unit(node->text);
            return make_quantity(val, u);
        }

        case ND_LIST: {
            int n = node->n_children;
            Value *items = malloc((size_t)(n > 0 ? n : 1) * sizeof(Value));
            for (int i = 0; i < n; i++) items[i] = eval_expr(node->children[i], env);
            Value list = make_list(items, n);
            free(items);
            return list;
        }

        case ND_IDENT: {
            Value v;
            if (env_lookup_opt(env, node->text, &v)) return v;
            runtime_error(node->line, "undefined name '%s'", node->text);
            return make_void(); /* unreachable */
        }

        case ND_UNARY: {
            Value operand = eval_expr(node->children[0], env);
            if (operand.type != VAL_NUMBER)
                runtime_error(node->line, "unary '-' requires a number");
            return make_number(-operand.number);
        }

        case ND_BINARY: {
            const char *op = node->text;
            Value left  = eval_expr(node->children[0], env);
            Value right = eval_expr(node->children[1], env);

            /* Comparison operators */
            if (strcmp(op, "==") == 0) {
                if (left.type == VAL_NUMBER && right.type == VAL_NUMBER)
                    return make_bool(left.number == right.number);
                if (left.type == VAL_BOOL && right.type == VAL_BOOL)
                    return make_bool(left.boolean == right.boolean);
                if (left.type == VAL_STRING && right.type == VAL_STRING)
                    return make_bool(strcmp(left.string, right.string) == 0);
                return make_bool(0);
            }
            if (strcmp(op, "!=") == 0) {
                if (left.type == VAL_NUMBER && right.type == VAL_NUMBER)
                    return make_bool(left.number != right.number);
                return make_bool(1);
            }
            if (left.type != VAL_NUMBER || right.type != VAL_NUMBER) {
                runtime_error(node->line,
                    "operator '%s' requires numbers", op);
            }
            if (strcmp(op, "<")  == 0) return make_bool(left.number <  right.number);
            if (strcmp(op, "<=") == 0) return make_bool(left.number <= right.number);
            if (strcmp(op, ">")  == 0) return make_bool(left.number >  right.number);
            if (strcmp(op, ">=") == 0) return make_bool(left.number >= right.number);
            /* Arithmetic */
            if (strcmp(op, "+") == 0) return make_number(left.number + right.number);
            if (strcmp(op, "-") == 0) return make_number(left.number - right.number);
            if (strcmp(op, "*") == 0) return make_number(left.number * right.number);
            if (strcmp(op, "/") == 0) {
                if (right.number == 0.0)
                    runtime_error(node->line, "division by zero");
                return make_number(left.number / right.number);
            }
            runtime_error(node->line, "unknown operator '%s'", op);
            return make_void();
        }

        case ND_MEMBER: {
            Value obj = eval_expr(node->children[0], env);
            const char *field = node->text;

            if (obj.type == VAL_ELEMENT) {
                ElementVal *e = obj.element;
                if (strcmp(field, "atomic_mass") == 0) return make_number(e->atomic_mass);
                if (strcmp(field, "valence")     == 0) return make_number((double)e->valence);
                if (strcmp(field, "symbol")      == 0) return make_string_own(strdup(e->symbol));
                runtime_error(node->line, "element has no field '%s'", field);
            }
            if (obj.type == VAL_COMPOUND) {
                CompoundVal *c = obj.compound;
                if (strcmp(field, "formula") == 0) {
                    /* Return a copy of the formula */
                    FormulaVal *copy = formula_parse(formula_to_cstr(c->formula));
                    return make_formula(copy);
                }
                if (strcmp(field, "name") == 0) return make_string_own(strdup(c->name));
                runtime_error(node->line, "compound has no field '%s'", field);
            }
            if (obj.type == VAL_REACTION) {
                ReactionVal *r = obj.reaction;
                if (strcmp(field, "reactants") == 0) {
                    ReactionSideVal *rs = malloc(sizeof(ReactionSideVal));
                    *rs = r->reactants;
                    return make_reaction_side(rs);
                }
                if (strcmp(field, "products") == 0) {
                    ReactionSideVal *rs = malloc(sizeof(ReactionSideVal));
                    *rs = r->products;
                    return make_reaction_side(rs);
                }
                runtime_error(node->line, "reaction has no field '%s'", field);
            }
            runtime_error(node->line,
                "member access '.%s' on non-record value (type %d)", field, obj.type);
            return make_void();
        }

        case ND_INDEX: {
            Value obj   = eval_expr(node->children[0], env);
            Value index = eval_expr(node->children[1], env);

            /* List indexing: bounds-checked */
            if (obj.type == VAL_LIST) {
                if (index.type != VAL_NUMBER)
                    runtime_error(node->line, "list index must be a number");
                int idx = (int)index.number;
                if (idx < 0 || idx >= obj.list.count)
                    runtime_error(node->line,
                        "list index %d out of range [0..%d]",
                        idx, obj.list.count - 1);
                return obj.list.items[idx];
            }

            /* Formula indexing: formula[element] → atom count */
            if (obj.type == VAL_FORMULA) {
                if (index.type != VAL_ELEMENT)
                    runtime_error(node->line, "formula index must be an element");
                ElementVal *e = index.element;
                FormulaVal *f = obj.formula;
                for (int i = 0; i < f->size; i++) {
                    if (strcmp(f->symbols[i], e->id) == 0)
                        return make_number((double)f->counts[i]);
                }
                return make_number(0.0); /* element not in formula */
            }

            /* ReactionSide indexing: side[compound] → coefficient */
            if (obj.type == VAL_REACTION_SIDE) {
                double coeff = reaction_side_coeff(obj.reaction_side, &index);
                return make_number(coeff);
            }

            runtime_error(node->line,
                "index operator not supported for this type (type %d)", obj.type);
            return make_void();
        }

        case ND_CALL: {
            /* children[0] = callee ident; [1..] = arguments */
            const char *callee_name = node->children[0]->text;
            int n_args = node->n_children - 1;

            /* Built-in: mol() */
            if (strcmp(callee_name, "mol") == 0) {
                if (n_args != 1)
                    runtime_error(node->line, "mol() expects 1 argument, got %d", n_args);
                Value arg = eval_expr(node->children[1], env);
                if (arg.type == VAL_NUMBER)   return arg;
                if (arg.type == VAL_QUANTITY) return make_number(arg.quantity.value);
                runtime_error(node->line, "mol() expects a number or quantity");
            }

            /* User-defined function */
            AstNode *func_decl = find_func(callee_name, node->line);

            Value args[MAX_FUNCS]; /* safe: max args bounded by MAX_FUNCS */
            for (int i = 0; i < n_args && i < MAX_FUNCS; i++)
                args[i] = eval_expr(node->children[i + 1], env);

            return call_user_func(func_decl, args, n_args, node->line);
        }

        default:
            runtime_error(node->line, "unknown expression node kind %d", node->kind);
            return make_void();
    }
}

/* ================================================================
   Statement executor
   ================================================================ */

static Value exec_stmt(const AstNode *node, Env *env) {
    if (!node) return make_void();

    switch (node->kind) {

        case ND_BLOCK: {
            for (int i = 0; i < node->n_children; i++) {
                Value r = exec_stmt(node->children[i], env);
                if (r.type == VAL_RETURN) return r;
            }
            return make_void();
        }

        case ND_VAR_DECL: {
            Value v = eval_expr(node->children[0], env);
            env_bind(env, node->text, v);
            return make_void();
        }

        case ND_ASSIGN: {
            Value rhs = eval_expr(node->children[1], env);
            const AstNode *lhs = node->children[0];
            if (lhs->kind == ND_IDENT) {
                if (!env_set(env, lhs->text, rhs))
                    runtime_error(node->line,
                        "assignment to undeclared variable '%s'", lhs->text);
            } else if (lhs->kind == ND_INDEX) {
                /* List element assignment */
                Value obj   = eval_expr(lhs->children[0], env);
                Value index = eval_expr(lhs->children[1], env);
                if (obj.type != VAL_LIST || index.type != VAL_NUMBER)
                    runtime_error(node->line, "invalid assignment target");
                int idx = (int)index.number;
                if (idx < 0 || idx >= obj.list.count)
                    runtime_error(node->line,
                        "list index %d out of range [0..%d]",
                        idx, obj.list.count - 1);
                obj.list.items[idx] = rhs;
            } else {
                runtime_error(node->line, "unsupported assignment target");
            }
            return make_void();
        }

        case ND_AUG_ASSIGN: {
            /* lhs op= rhs  →  lhs = lhs op rhs */
            const AstNode *lhs_node = node->children[0];
            Value rhs = eval_expr(node->children[1], env);

            if (lhs_node->kind != ND_IDENT)
                runtime_error(node->line, "augmented assignment requires a simple variable");

            Value cur;
            if (!env_lookup_opt(env, lhs_node->text, &cur))
                runtime_error(node->line,
                    "augmented assignment to undeclared variable '%s'", lhs_node->text);

            if (cur.type != VAL_NUMBER || rhs.type != VAL_NUMBER)
                runtime_error(node->line, "augmented assignment requires numbers");

            const char *op = node->text;
            double result;
            if (strcmp(op, "+=") == 0) result = cur.number + rhs.number;
            else if (strcmp(op, "-=") == 0) result = cur.number - rhs.number;
            else if (strcmp(op, "*=") == 0) result = cur.number * rhs.number;
            else if (strcmp(op, "/=") == 0) {
                if (rhs.number == 0.0)
                    runtime_error(node->line, "division by zero");
                result = cur.number / rhs.number;
            } else {
                runtime_error(node->line, "unknown augmented operator '%s'", op);
                result = 0;
            }
            env_set(env, lhs_node->text, make_number(result));
            return make_void();
        }

        case ND_PRINT: {
            Value v = eval_expr(node->children[0], env);
            value_print(&v);
            printf("\n");
            return make_void();
        }

        case ND_RETURN: {
            Value v = (node->n_children > 0)
                ? eval_expr(node->children[0], env)
                : make_void();
            return make_return_val(v);
        }

        case ND_EXPR_STMT:
            eval_expr(node->children[0], env); /* discard result */
            return make_void();

        case ND_IF: {
            /* Children: alternating (cond, block) pairs; optional trailing else block */
            int i = 0;
            while (i + 1 < node->n_children) {
                Value cond = eval_expr(node->children[i], env);
                if (cond.type != VAL_BOOL)
                    runtime_error(node->line, "if condition must be a boolean");
                if (cond.boolean) {
                    return exec_stmt(node->children[i + 1], env);
                }
                i += 2;
            }
            /* Optional else block (last child when n_children is odd) */
            if (i < node->n_children) {
                return exec_stmt(node->children[i], env);
            }
            return make_void();
        }

        case ND_FOR: {
            /* children[0]=var_ident, [1]=iterable_expr, [2]=body_block */
            const char *var_name = node->children[0]->text;
            Value iterable = eval_expr(node->children[1], env);
            const AstNode *body = node->children[2];

            if (iterable.type == VAL_LIST) {
                for (int i = 0; i < iterable.list.count; i++) {
                    Env *scope = env_new(env);
                    env_bind(scope, var_name, iterable.list.items[i]);
                    Value r = exec_stmt(body, scope);
                    env_free(scope);
                    if (r.type == VAL_RETURN) return r;
                }
            } else if (iterable.type == VAL_FORMULA) {
                /* Iterate elements in first-appearance order */
                FormulaVal *f = iterable.formula;
                for (int i = 0; i < f->size; i++) {
                    Value elem;
                    if (!env_lookup_opt(g_global_env, f->symbols[i], &elem) ||
                        elem.type != VAL_ELEMENT) {
                        runtime_error(node->line,
                            "formula references undeclared element '%s'", f->symbols[i]);
                    }
                    Env *scope = env_new(env);
                    env_bind(scope, var_name, elem);
                    Value r = exec_stmt(body, scope);
                    env_free(scope);
                    if (r.type == VAL_RETURN) return r;
                }
            } else if (iterable.type == VAL_REACTION_SIDE) {
                /* Iterate species in declaration order (§4.4 extended) */
                ReactionSideVal *rs = iterable.reaction_side;
                for (int i = 0; i < rs->count; i++) {
                    Env *scope = env_new(env);
                    env_bind(scope, var_name, *rs->terms[i].species);
                    Value r = exec_stmt(body, scope);
                    env_free(scope);
                    if (r.type == VAL_RETURN) return r;
                }
            } else {
                runtime_error(node->line,
                    "for-in requires a list, formula, or reaction side");
            }
            return make_void();
        }

        /* Top-level declarations processed in exec_stmt for uniformity */
        case ND_ELEMENT_DECL: {
            ElementVal *e = build_element(node);
            env_bind(env, e->id, make_element(e));
            return make_void();
        }

        case ND_COMPOUND_DECL: {
            CompoundVal *c = build_compound(node);
            env_bind(env, c->id, make_compound(c));
            return make_void();
        }

        case ND_REACTION_DECL: {
            ReactionVal *r = build_reaction(node);
            env_bind(env, r->id, make_reaction(r));
            return make_void();
        }

        case ND_FUNCTION_DECL: {
            /* Store in global function table */
            if (g_n_funcs < MAX_FUNCS) {
                g_func_names[g_n_funcs] = node->text;
                g_func_decls[g_n_funcs] = (AstNode *)node;
                g_n_funcs++;
            }
            return make_void();
        }

        default:
            runtime_error(node ? node->line : 0,
                "unknown statement node kind %d", node ? node->kind : -1);
            return make_void();
    }
}

/* ================================================================
   Public API
   ================================================================ */

void interpret_program(const AstNode *prog) {
    g_n_funcs    = 0;
    g_global_env = env_new(NULL);

    for (int i = 0; i < prog->n_children; i++) {
        Value r = exec_stmt(prog->children[i], g_global_env);
        /* Top-level return is an error but we ignore it gracefully */
        (void)r;
    }

    env_free(g_global_env);
    g_global_env = NULL;
}
