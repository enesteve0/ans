#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "semantics.h"
#include "error.h"

/* ================================================================
   Shared helpers
   ================================================================ */

static char *dup_cstr(const char *s) {
    size_t n = strlen(s);
    char *out = malloc(n + 1);
    if (!out) { perror("malloc"); exit(1); }
    memcpy(out, s, n + 1);
    return out;
}

static char *dup_slice(const char *s, int n) {
    char *out = malloc((size_t)n + 1);
    if (!out) { perror("malloc"); exit(1); }
    memcpy(out, s, (size_t)n);
    out[n] = '\0';
    return out;
}

static int text_eq(const char *a, const char *b) {
    return a && b && strcmp(a, b) == 0;
}

static const AstNode *field_child(const AstNode *decl, const char *name) {
    for (int i = 0; i < decl->n_children; i++) {
        const AstNode *f = decl->children[i];
        if (f->text && strcmp(f->text, name) == 0 && f->n_children > 0) {
            return f->children[0];
        }
    }
    return NULL;
}

static const AstNode *function_block(const AstNode *fn) {
    if (!fn || fn->n_children == 0) return NULL;
    return fn->children[fn->n_children - 1];
}

static int function_arity(const AstNode *fn) {
    int n = 0;
    for (int i = 0; i < fn->n_children; i++) {
        if (fn->children[i]->kind == ND_PARAM) n++;
    }
    return n;
}

/* ================================================================
   Formula maps
   ================================================================ */

typedef struct {
    char *symbol;
    int   count;
} AtomCount;

typedef struct {
    AtomCount *items;
    int        count;
    int        cap;
} FormulaInfo;

static void formula_add(FormulaInfo *f, const char *sym, int count) {
    for (int i = 0; i < f->count; i++) {
        if (strcmp(f->items[i].symbol, sym) == 0) {
            f->items[i].count += count;
            return;
        }
    }
    if (f->count == f->cap) {
        int new_cap = f->cap ? f->cap * 2 : 4;
        f->items = realloc(f->items, (size_t)new_cap * sizeof(AtomCount));
        if (!f->items) { perror("realloc"); exit(1); }
        f->cap = new_cap;
    }
    f->items[f->count].symbol = dup_cstr(sym);
    f->items[f->count].count = count;
    f->count++;
}

static int formula_get(const FormulaInfo *f, const char *sym) {
    for (int i = 0; i < f->count; i++) {
        if (strcmp(f->items[i].symbol, sym) == 0) return f->items[i].count;
    }
    return 0;
}

static int parse_formula_text(const char *text, int line, FormulaInfo *out) {
    memset(out, 0, sizeof(*out));
    for (int i = 0; text[i]; ) {
        if (!isupper((unsigned char)text[i])) {
            error_at(line, 1, "invalid formula literal '%s'", text);
            return 0;
        }

        char sym[4];
        int len = 0;
        sym[len++] = text[i++];
        if (islower((unsigned char)text[i])) sym[len++] = text[i++];
        sym[len] = '\0';

        int n = 0;
        while (isdigit((unsigned char)text[i])) {
            n = n * 10 + (text[i] - '0');
            i++;
        }
        if (n == 0) n = 1;
        formula_add(out, sym, n);
    }
    return 1;
}

static int formula_equal(const FormulaInfo *a, const FormulaInfo *b) {
    if (a->count != b->count) return 0;
    for (int i = 0; i < a->count; i++) {
        if (formula_get(b, a->items[i].symbol) != a->items[i].count) return 0;
    }
    return 1;
}

/* ================================================================
   Static types and semantic context
   ================================================================ */

typedef enum {
    TY_ERROR,
    TY_ANY,
    TY_VOID,
    TY_NUMBER,
    TY_STRING,
    TY_CHAR,
    TY_BOOL,
    TY_ELEMENT,
    TY_COMPOUND,
    TY_FORMULA,
    TY_REACTION,
    TY_REACTION_SIDE,
    TY_QUANTITY,
    TY_LIST,
    TY_FUNCTION
} TypeTag;

typedef struct {
    TypeTag tag;
    TypeTag elem;
} Type;

static Type ty(TypeTag tag) {
    Type t;
    t.tag = tag;
    t.elem = TY_ANY;
    return t;
}

static Type ty_list(TypeTag elem) {
    Type t;
    t.tag = TY_LIST;
    t.elem = elem;
    return t;
}

static const char *type_name(Type t) {
    switch (t.tag) {
        case TY_ERROR:         return "<error>";
        case TY_ANY:           return "any";
        case TY_VOID:          return "void";
        case TY_NUMBER:        return "number";
        case TY_STRING:        return "string";
        case TY_CHAR:          return "char";
        case TY_BOOL:          return "bool";
        case TY_ELEMENT:       return "element";
        case TY_COMPOUND:      return "compound";
        case TY_FORMULA:       return "formula";
        case TY_REACTION:      return "reaction";
        case TY_REACTION_SIDE: return "reaction_side";
        case TY_QUANTITY:      return "quantity";
        case TY_LIST:          return "list";
        case TY_FUNCTION:      return "function";
        default:               return "?";
    }
}

static int type_compatible(Type expected, Type got) {
    if (expected.tag == TY_ERROR || got.tag == TY_ERROR) return 1;
    if (expected.tag == TY_ANY || got.tag == TY_ANY) return 1;
    if (expected.tag != got.tag) return 0;
    if (expected.tag == TY_LIST) {
        return expected.elem == TY_ANY || got.elem == TY_ANY || expected.elem == got.elem;
    }
    return 1;
}

static Type merge_types(Type a, Type b) {
    if (a.tag == TY_ERROR) return b;
    if (b.tag == TY_ERROR) return a;
    if (type_compatible(a, b) && type_compatible(b, a)) {
        if (a.tag == TY_LIST && a.elem != b.elem) return ty_list(TY_ANY);
        return a.tag == TY_ANY ? b : a;
    }
    return ty(TY_ANY);
}

typedef struct TypeBinding {
    char *name;
    Type  type;
} TypeBinding;

typedef struct TypeScope {
    TypeBinding      *items;
    int               count;
    int               cap;
    struct TypeScope *parent;
} TypeScope;

static TypeScope *scope_new(TypeScope *parent) {
    TypeScope *s = calloc(1, sizeof(TypeScope));
    if (!s) { perror("calloc"); exit(1); }
    s->parent = parent;
    return s;
}

static Type *scope_lookup(TypeScope *s, const char *name) {
    for (TypeScope *cur = s; cur; cur = cur->parent) {
        for (int i = cur->count - 1; i >= 0; i--) {
            if (strcmp(cur->items[i].name, name) == 0) return &cur->items[i].type;
        }
    }
    return NULL;
}

static void scope_define(TypeScope *s, const char *name, Type type, int line) {
    for (int i = 0; i < s->count; i++) {
        if (strcmp(s->items[i].name, name) == 0) {
            error_at(line, 1, "duplicate declaration of '%s'", name);
            s->items[i].type = type;
            return;
        }
    }
    if (s->count == s->cap) {
        int new_cap = s->cap ? s->cap * 2 : 8;
        s->items = realloc(s->items, (size_t)new_cap * sizeof(TypeBinding));
        if (!s->items) { perror("realloc"); exit(1); }
        s->cap = new_cap;
    }
    s->items[s->count].name = dup_cstr(name);
    s->items[s->count].type = type;
    s->count++;
}

typedef struct {
    char *symbol;
} ElementInfo;

typedef struct {
    char       *name;
    char       *formula_text;
    FormulaInfo formula;
} CompoundInfo;

typedef struct {
    CompoundInfo **species;
    double        *coeffs;
    int            count;
    int            cap;
} ReactionSideInfo;

typedef struct {
    char             *name;
    ReactionSideInfo  reactants;
    ReactionSideInfo  products;
} ReactionInfo;

typedef struct {
    char        *name;
    const AstNode *node;
    char       **param_names;
    Type        *param_types;
    int          arity;
    Type         ret_type;
    int          ret_known;
    int          checking;
} FunctionSig;

typedef struct {
    ElementInfo *elements;
    int          n_elements;
    int          cap_elements;

    CompoundInfo *compounds;
    int           n_compounds;
    int           cap_compounds;

    ReactionInfo *reactions;
    int           n_reactions;
    int           cap_reactions;

    FunctionSig *funcs;
    int          n_funcs;
    int          cap_funcs;

    TypeScope   *global;
} SemCtx;

static ElementInfo *sem_find_element(SemCtx *ctx, const char *symbol) {
    for (int i = 0; i < ctx->n_elements; i++) {
        if (strcmp(ctx->elements[i].symbol, symbol) == 0) return &ctx->elements[i];
    }
    return NULL;
}

static CompoundInfo *sem_find_compound(SemCtx *ctx, const char *name) {
    for (int i = 0; i < ctx->n_compounds; i++) {
        if (strcmp(ctx->compounds[i].name, name) == 0) return &ctx->compounds[i];
    }
    return NULL;
}

static CompoundInfo *sem_find_compound_by_formula(SemCtx *ctx, const char *formula_text, int line) {
    FormulaInfo f;
    if (!parse_formula_text(formula_text, line, &f)) return NULL;
    for (int i = 0; i < ctx->n_compounds; i++) {
        if (formula_equal(&ctx->compounds[i].formula, &f)) return &ctx->compounds[i];
    }
    return NULL;
}

static ReactionInfo *sem_find_reaction(SemCtx *ctx, const char *name) {
    for (int i = 0; i < ctx->n_reactions; i++) {
        if (strcmp(ctx->reactions[i].name, name) == 0) return &ctx->reactions[i];
    }
    return NULL;
}

static FunctionSig *sem_find_func(SemCtx *ctx, const char *name) {
    for (int i = 0; i < ctx->n_funcs; i++) {
        if (strcmp(ctx->funcs[i].name, name) == 0) return &ctx->funcs[i];
    }
    return NULL;
}

static void parse_param_text(const char *text, Type *type, char **name) {
    const char *sp = strchr(text, ' ');
    if (!sp) {
        *type = ty(TY_ANY);
        *name = dup_cstr(text);
        return;
    }

    char *prefix = dup_slice(text, (int)(sp - text));
    *name = dup_cstr(sp + 1);
    if (strcmp(prefix, "element") == 0) *type = ty(TY_ELEMENT);
    else if (strcmp(prefix, "compound") == 0) *type = ty(TY_COMPOUND);
    else if (strcmp(prefix, "reaction") == 0) *type = ty(TY_REACTION);
    else *type = ty(TY_ANY);
    free(prefix);
}

static int formula_symbols_declared(SemCtx *ctx, const FormulaInfo *formula, int line, const char *owner) {
    int ok = 1;
    for (int i = 0; i < formula->count; i++) {
        if (!sem_find_element(ctx, formula->items[i].symbol)) {
            error_at(line, 1, "%s uses undeclared element '%s'", owner, formula->items[i].symbol);
            ok = 0;
        }
    }
    return ok;
}

static void side_add(ReactionSideInfo *side, CompoundInfo *compound, double coeff) {
    for (int i = 0; i < side->count; i++) {
        if (side->species[i] == compound) {
            side->coeffs[i] += coeff;
            return;
        }
    }
    if (side->count == side->cap) {
        int new_cap = side->cap ? side->cap * 2 : 4;
        side->species = realloc(side->species, (size_t)new_cap * sizeof(CompoundInfo *));
        side->coeffs = realloc(side->coeffs, (size_t)new_cap * sizeof(double));
        if (!side->species || !side->coeffs) { perror("realloc"); exit(1); }
        side->cap = new_cap;
    }
    side->species[side->count] = compound;
    side->coeffs[side->count] = coeff;
    side->count++;
}

static CompoundInfo *resolve_species(SemCtx *ctx, const AstNode *node) {
    if (node->kind == ND_IDENT) {
        CompoundInfo *c = sem_find_compound(ctx, node->text);
        if (!c) error_at(node->line, 1, "unknown compound '%s' in reaction", node->text);
        return c;
    }
    if (node->kind == ND_FORMULA) {
        CompoundInfo *c = sem_find_compound_by_formula(ctx, node->text, node->line);
        if (!c) error_at(node->line, 1, "formula '%s' does not match a declared compound", node->text);
        return c;
    }
    error_at(node->line, 1, "invalid reaction species");
    return NULL;
}

static void parse_reaction_side_info(SemCtx *ctx, const AstNode *rxn_expr, ReactionSideInfo *side) {
    for (int i = 0; i < rxn_expr->n_children; i++) {
        const AstNode *term = rxn_expr->children[i];
        double coeff = 1.0;
        const AstNode *species = NULL;
        if (term->n_children == 1) {
            species = term->children[0];
        } else if (term->n_children >= 2) {
            coeff = term->children[0]->num;
            species = term->children[1];
        }
        if (!species) {
            error_at(term->line, 1, "reaction term is missing a species");
            continue;
        }
        CompoundInfo *compound = resolve_species(ctx, species);
        if (compound) side_add(side, compound, coeff);
    }
}

static double side_atom_count(const ReactionSideInfo *side, const char *symbol) {
    double total = 0.0;
    for (int i = 0; i < side->count; i++) {
        total += side->coeffs[i] * (double)formula_get(&side->species[i]->formula, symbol);
    }
    return total;
}

static void check_reaction_balance(SemCtx *ctx, const ReactionInfo *r, int line) {
    for (int i = 0; i < ctx->n_elements; i++) {
        const char *sym = ctx->elements[i].symbol;
        double left = side_atom_count(&r->reactants, sym);
        double right = side_atom_count(&r->products, sym);
        if (fabs(left - right) > 1e-9) {
            error_at(line, 1,
                     "reaction '%s' is not balanced for element %s (%g vs %g)",
                     r->name, sym, left, right);
        }
    }
}

static void collect_element(SemCtx *ctx, const AstNode *decl) {
    if (sem_find_element(ctx, decl->text)) {
        error_at(decl->line, 1, "duplicate element '%s'", decl->text);
        return;
    }
    FormulaInfo f;
    if (!parse_formula_text(decl->text, decl->line, &f) || f.count != 1 || f.items[0].count != 1) {
        error_at(decl->line, 1, "element name '%s' must be a single element symbol", decl->text);
    }
    if (ctx->n_elements == ctx->cap_elements) {
        int new_cap = ctx->cap_elements ? ctx->cap_elements * 2 : 8;
        ctx->elements = realloc(ctx->elements, (size_t)new_cap * sizeof(ElementInfo));
        if (!ctx->elements) { perror("realloc"); exit(1); }
        ctx->cap_elements = new_cap;
    }
    ctx->elements[ctx->n_elements++].symbol = dup_cstr(decl->text);
}

static void collect_compound(SemCtx *ctx, const AstNode *decl) {
    if (sem_find_compound(ctx, decl->text)) {
        error_at(decl->line, 1, "duplicate compound '%s'", decl->text);
        return;
    }
    const AstNode *formula = field_child(decl, "formula");
    const AstNode *name = field_child(decl, "name");
    if (!formula || formula->kind != ND_FORMULA) {
        error_at(decl->line, 1, "compound '%s' is missing formula field", decl->text);
        return;
    }
    if (!name || name->kind != ND_STRING) {
        error_at(decl->line, 1, "compound '%s' is missing name field", decl->text);
    }

    if (ctx->n_compounds == ctx->cap_compounds) {
        int new_cap = ctx->cap_compounds ? ctx->cap_compounds * 2 : 8;
        ctx->compounds = realloc(ctx->compounds, (size_t)new_cap * sizeof(CompoundInfo));
        if (!ctx->compounds) { perror("realloc"); exit(1); }
        ctx->cap_compounds = new_cap;
    }
    CompoundInfo *c = &ctx->compounds[ctx->n_compounds++];
    memset(c, 0, sizeof(*c));
    c->name = dup_cstr(decl->text);
    c->formula_text = dup_cstr(formula->text);
    parse_formula_text(formula->text, formula->line, &c->formula);
    formula_symbols_declared(ctx, &c->formula, formula->line, c->name);
}

static void collect_reaction(SemCtx *ctx, const AstNode *decl) {
    if (sem_find_reaction(ctx, decl->text)) {
        error_at(decl->line, 1, "duplicate reaction '%s'", decl->text);
        return;
    }
    const AstNode *reactants = field_child(decl, "reactants");
    const AstNode *products = field_child(decl, "products");
    if (!reactants || !products) {
        error_at(decl->line, 1, "reaction '%s' must have reactants and products", decl->text);
        return;
    }

    if (ctx->n_reactions == ctx->cap_reactions) {
        int new_cap = ctx->cap_reactions ? ctx->cap_reactions * 2 : 8;
        ctx->reactions = realloc(ctx->reactions, (size_t)new_cap * sizeof(ReactionInfo));
        if (!ctx->reactions) { perror("realloc"); exit(1); }
        ctx->cap_reactions = new_cap;
    }
    ReactionInfo *r = &ctx->reactions[ctx->n_reactions++];
    memset(r, 0, sizeof(*r));
    r->name = dup_cstr(decl->text);
    parse_reaction_side_info(ctx, reactants, &r->reactants);
    parse_reaction_side_info(ctx, products, &r->products);
    if (field_child(decl, "check")) check_reaction_balance(ctx, r, decl->line);
}

static void collect_function(SemCtx *ctx, const AstNode *fn) {
    if (sem_find_func(ctx, fn->text)) {
        error_at(fn->line, 1, "duplicate function '%s'", fn->text);
        return;
    }
    if (ctx->n_funcs == ctx->cap_funcs) {
        int new_cap = ctx->cap_funcs ? ctx->cap_funcs * 2 : 8;
        ctx->funcs = realloc(ctx->funcs, (size_t)new_cap * sizeof(FunctionSig));
        if (!ctx->funcs) { perror("realloc"); exit(1); }
        ctx->cap_funcs = new_cap;
    }
    FunctionSig *sig = &ctx->funcs[ctx->n_funcs++];
    memset(sig, 0, sizeof(*sig));
    sig->name = dup_cstr(fn->text);
    sig->node = fn;
    sig->arity = function_arity(fn);
    sig->param_names = calloc((size_t)sig->arity, sizeof(char *));
    sig->param_types = calloc((size_t)sig->arity, sizeof(Type));
    if (!sig->param_names || !sig->param_types) { perror("calloc"); exit(1); }
    int p = 0;
    for (int i = 0; i < fn->n_children; i++) {
        if (fn->children[i]->kind == ND_PARAM) {
            parse_param_text(fn->children[i]->text, &sig->param_types[p], &sig->param_names[p]);
            p++;
        }
    }
    sig->ret_type = ty(TY_ANY);
}

/* ================================================================
   Static expression and statement checking
   ================================================================ */

typedef struct {
    int  has_return;
    Type ret_type;
} CheckResult;

static Type check_expr(SemCtx *ctx, TypeScope *scope, const AstNode *expr);
static CheckResult check_stmt(SemCtx *ctx, TypeScope *scope, const AstNode *stmt);

static Type check_formula_expr(SemCtx *ctx, const AstNode *expr) {
    if (sem_find_element(ctx, expr->text)) return ty(TY_ELEMENT);
    if (sem_find_compound_by_formula(ctx, expr->text, expr->line)) return ty(TY_COMPOUND);

    FormulaInfo f;
    if (parse_formula_text(expr->text, expr->line, &f)) {
        formula_symbols_declared(ctx, &f, expr->line, expr->text);
    }
    return ty(TY_FORMULA);
}

static Type check_member(SemCtx *ctx, TypeScope *scope, const AstNode *expr) {
    (void)ctx;
    Type base = check_expr(ctx, scope, expr->children[0]);
    const char *field = expr->text;
    if (base.tag == TY_ANY || base.tag == TY_ERROR) return base;

    if (base.tag == TY_ELEMENT) {
        if (text_eq(field, "atomic_mass") || text_eq(field, "valence")) return ty(TY_NUMBER);
        if (text_eq(field, "symbol")) return ty(TY_STRING);
    } else if (base.tag == TY_COMPOUND) {
        if (text_eq(field, "formula")) return ty(TY_FORMULA);
        if (text_eq(field, "name")) return ty(TY_STRING);
    } else if (base.tag == TY_REACTION) {
        if (text_eq(field, "reactants") || text_eq(field, "products")) return ty(TY_REACTION_SIDE);
        if (text_eq(field, "energy")) return ty(TY_QUANTITY);
    }

    error_at(expr->line, 1, "type '%s' has no member '%s'", type_name(base), field);
    return ty(TY_ERROR);
}

static Type check_index(SemCtx *ctx, TypeScope *scope, const AstNode *expr) {
    Type base = check_expr(ctx, scope, expr->children[0]);
    Type idx = check_expr(ctx, scope, expr->children[1]);
    if (base.tag == TY_LIST) {
        if (idx.tag != TY_NUMBER && idx.tag != TY_ANY && idx.tag != TY_ERROR) {
            error_at(expr->line, 1, "list index must be number, got %s", type_name(idx));
        }
        return ty(base.elem == TY_ANY ? TY_ANY : base.elem);
    }
    if (base.tag == TY_FORMULA) {
        if (idx.tag != TY_ELEMENT && idx.tag != TY_ANY && idx.tag != TY_ERROR) {
            error_at(expr->line, 1, "formula index must be element, got %s", type_name(idx));
        }
        return ty(TY_NUMBER);
    }
    if (base.tag == TY_REACTION_SIDE) {
        if (idx.tag != TY_COMPOUND && idx.tag != TY_ANY && idx.tag != TY_ERROR) {
            error_at(expr->line, 1, "reaction side index must be compound, got %s", type_name(idx));
        }
        return ty(TY_NUMBER);
    }
    if (base.tag == TY_ANY || base.tag == TY_ERROR) return ty(TY_ANY);
    error_at(expr->line, 1, "type '%s' is not indexable", type_name(base));
    return ty(TY_ERROR);
}

static Type check_call(SemCtx *ctx, TypeScope *scope, const AstNode *expr) {
    if (expr->n_children == 0) return ty(TY_ERROR);
    const AstNode *callee = expr->children[0];
    if (callee->kind != ND_IDENT) {
        error_at(expr->line, 1, "only named functions can be called");
        return ty(TY_ERROR);
    }

    if (strcmp(callee->text, "mol") == 0) {
        if (expr->n_children != 2) {
            error_at(expr->line, 1, "mol expects 1 argument, got %d", expr->n_children - 1);
            return ty(TY_ERROR);
        }
        Type arg = check_expr(ctx, scope, expr->children[1]);
        if (arg.tag != TY_NUMBER && arg.tag != TY_QUANTITY && arg.tag != TY_ANY && arg.tag != TY_ERROR) {
            error_at(expr->children[1]->line, 1, "mol expects number or quantity, got %s", type_name(arg));
        }
        return ty(TY_NUMBER);
    }

    FunctionSig *sig = sem_find_func(ctx, callee->text);
    if (!sig) {
        error_at(callee->line, 1, "unknown function '%s'", callee->text);
        return ty(TY_ERROR);
    }
    int argc = expr->n_children - 1;
    if (argc != sig->arity) {
        error_at(expr->line, 1, "function '%s' expects %d arguments, got %d",
                 sig->name, sig->arity, argc);
    }
    int n = argc < sig->arity ? argc : sig->arity;
    for (int i = 0; i < n; i++) {
        Type got = check_expr(ctx, scope, expr->children[i + 1]);
        if (!type_compatible(sig->param_types[i], got)) {
            error_at(expr->children[i + 1]->line, 1,
                     "argument %d of '%s' expects %s, got %s",
                     i + 1, sig->name, type_name(sig->param_types[i]), type_name(got));
        }
    }
    return sig->ret_known ? sig->ret_type : ty(TY_ANY);
}

static Type check_expr(SemCtx *ctx, TypeScope *scope, const AstNode *expr) {
    switch (expr->kind) {
        case ND_NUMBER:   return ty(TY_NUMBER);
        case ND_STRING:   return ty(TY_STRING);
        case ND_CHAR:     return ty(TY_CHAR);
        case ND_QUANTITY: return ty(TY_QUANTITY);
        case ND_FORMULA:  return check_formula_expr(ctx, expr);
        case ND_IDENT: {
            Type *found = scope_lookup(scope, expr->text);
            if (found) return *found;
            if (sem_find_func(ctx, expr->text)) return ty(TY_FUNCTION);
            error_at(expr->line, 1, "undefined name '%s'", expr->text);
            return ty(TY_ERROR);
        }
        case ND_LIST: {
            Type elem = ty(TY_ERROR);
            for (int i = 0; i < expr->n_children; i++) {
                Type t = check_expr(ctx, scope, expr->children[i]);
                if (i == 0) {
                    elem = t;
                } else if (!type_compatible(elem, t) || !type_compatible(t, elem)) {
                    error_at(expr->children[i]->line, 1,
                             "list literal mixes %s and %s", type_name(elem), type_name(t));
                    elem = ty(TY_ERROR);
                } else {
                    elem = merge_types(elem, t);
                }
            }
            return ty_list(expr->n_children == 0 ? TY_ANY : elem.tag);
        }
        case ND_UNARY: {
            Type right = check_expr(ctx, scope, expr->children[0]);
            if (right.tag != TY_NUMBER && right.tag != TY_ANY && right.tag != TY_ERROR) {
                error_at(expr->line, 1, "unary '-' expects number, got %s", type_name(right));
            }
            return ty(TY_NUMBER);
        }
        case ND_BINARY: {
            Type left = check_expr(ctx, scope, expr->children[0]);
            Type right = check_expr(ctx, scope, expr->children[1]);
            if (text_eq(expr->text, "+") || text_eq(expr->text, "-") ||
                text_eq(expr->text, "*") || text_eq(expr->text, "/")) {
                if (left.tag != TY_NUMBER && left.tag != TY_ANY && left.tag != TY_ERROR) {
                    error_at(expr->children[0]->line, 1, "operator '%s' expects number, got %s",
                             expr->text, type_name(left));
                }
                if (right.tag != TY_NUMBER && right.tag != TY_ANY && right.tag != TY_ERROR) {
                    error_at(expr->children[1]->line, 1, "operator '%s' expects number, got %s",
                             expr->text, type_name(right));
                }
                return ty(TY_NUMBER);
            }
            if (text_eq(expr->text, "<") || text_eq(expr->text, "<=") ||
                text_eq(expr->text, ">") || text_eq(expr->text, ">=")) {
                if (left.tag != TY_NUMBER && left.tag != TY_ANY && left.tag != TY_ERROR) {
                    error_at(expr->children[0]->line, 1, "comparison expects number, got %s",
                             type_name(left));
                }
                if (right.tag != TY_NUMBER && right.tag != TY_ANY && right.tag != TY_ERROR) {
                    error_at(expr->children[1]->line, 1, "comparison expects number, got %s",
                             type_name(right));
                }
                return ty(TY_BOOL);
            }
            if (!type_compatible(left, right) && !type_compatible(right, left)) {
                error_at(expr->line, 1, "cannot compare %s and %s", type_name(left), type_name(right));
            }
            return ty(TY_BOOL);
        }
        case ND_MEMBER: return check_member(ctx, scope, expr);
        case ND_INDEX:  return check_index(ctx, scope, expr);
        case ND_CALL:   return check_call(ctx, scope, expr);
        default:
            error_at(expr->line, 1, "internal type checker error on node kind %d", expr->kind);
            return ty(TY_ERROR);
    }
}

static Type check_assignable(SemCtx *ctx, TypeScope *scope, const AstNode *lhs) {
    if (lhs->kind == ND_CALL) {
        error_at(lhs->line, 1, "function call is not assignable");
        return ty(TY_ERROR);
    }
    return check_expr(ctx, scope, lhs);
}

static CheckResult check_block(SemCtx *ctx, TypeScope *parent, const AstNode *block) {
    TypeScope *scope = scope_new(parent);
    CheckResult out;
    out.has_return = 0;
    out.ret_type = ty(TY_VOID);
    for (int i = 0; i < block->n_children; i++) {
        CheckResult r = check_stmt(ctx, scope, block->children[i]);
        if (r.has_return) {
            out.ret_type = out.has_return ? merge_types(out.ret_type, r.ret_type) : r.ret_type;
            out.has_return = 1;
        }
    }
    return out;
}

static CheckResult check_stmt(SemCtx *ctx, TypeScope *scope, const AstNode *stmt) {
    CheckResult none;
    none.has_return = 0;
    none.ret_type = ty(TY_VOID);

    switch (stmt->kind) {
        case ND_VAR_DECL: {
            Type t = check_expr(ctx, scope, stmt->children[0]);
            scope_define(scope, stmt->text, t, stmt->line);
            return none;
        }
        case ND_ASSIGN: {
            Type left = check_assignable(ctx, scope, stmt->children[0]);
            Type right = check_expr(ctx, scope, stmt->children[1]);
            if (!type_compatible(left, right)) {
                error_at(stmt->line, 1, "cannot assign %s to %s", type_name(right), type_name(left));
            }
            return none;
        }
        case ND_AUG_ASSIGN: {
            Type left = check_assignable(ctx, scope, stmt->children[0]);
            Type right = check_expr(ctx, scope, stmt->children[1]);
            if (left.tag != TY_NUMBER && left.tag != TY_ANY && left.tag != TY_ERROR) {
                error_at(stmt->line, 1, "augmented assignment expects numeric left side");
            }
            if (right.tag != TY_NUMBER && right.tag != TY_ANY && right.tag != TY_ERROR) {
                error_at(stmt->line, 1, "augmented assignment expects numeric right side");
            }
            return none;
        }
        case ND_RETURN: {
            CheckResult r;
            r.has_return = 1;
            r.ret_type = stmt->n_children ? check_expr(ctx, scope, stmt->children[0]) : ty(TY_VOID);
            return r;
        }
        case ND_PRINT:
            check_expr(ctx, scope, stmt->children[0]);
            return none;
        case ND_EXPR_STMT:
            check_expr(ctx, scope, stmt->children[0]);
            return none;
        case ND_FOR: {
            Type iter = check_expr(ctx, scope, stmt->children[1]);
            Type item = ty(TY_ERROR);
            if (iter.tag == TY_LIST) item = ty(iter.elem == TY_ANY ? TY_ANY : iter.elem);
            else if (iter.tag == TY_FORMULA) item = ty(TY_ELEMENT);
            else if (iter.tag == TY_REACTION_SIDE) item = ty(TY_COMPOUND);
            else if (iter.tag == TY_ANY) item = ty(TY_ANY);
            else {
                error_at(stmt->line, 1, "for-in cannot iterate over %s", type_name(iter));
                item = ty(TY_ANY);
            }
            TypeScope *loop = scope_new(scope);
            scope_define(loop, stmt->children[0]->text, item, stmt->children[0]->line);
            return check_block(ctx, loop, stmt->children[2]);
        }
        case ND_IF: {
            CheckResult result;
            result.has_return = 0;
            result.ret_type = ty(TY_VOID);
            int i = 0;
            while (i + 1 < stmt->n_children) {
                Type cond = check_expr(ctx, scope, stmt->children[i]);
                if (cond.tag != TY_BOOL && cond.tag != TY_ANY && cond.tag != TY_ERROR) {
                    error_at(stmt->children[i]->line, 1, "if condition must be bool, got %s",
                             type_name(cond));
                }
                CheckResult branch = check_block(ctx, scope, stmt->children[i + 1]);
                if (branch.has_return) {
                    result.ret_type = result.has_return ? merge_types(result.ret_type, branch.ret_type)
                                                        : branch.ret_type;
                    result.has_return = 1;
                }
                i += 2;
            }
            if (i < stmt->n_children) {
                CheckResult branch = check_block(ctx, scope, stmt->children[i]);
                if (branch.has_return) {
                    result.ret_type = result.has_return ? merge_types(result.ret_type, branch.ret_type)
                                                        : branch.ret_type;
                    result.has_return = 1;
                }
            }
            return result;
        }
        case ND_BLOCK:
            return check_block(ctx, scope, stmt);
        case ND_FUNCTION_DECL:
        case ND_ELEMENT_DECL:
        case ND_COMPOUND_DECL:
        case ND_REACTION_DECL:
            return none;
        default:
            error_at(stmt->line, 1, "internal type checker error on statement kind %d", stmt->kind);
            return none;
    }
}

static void check_function_body(SemCtx *ctx, FunctionSig *sig) {
    if (sig->checking || sig->ret_known) return;
    sig->checking = 1;
    TypeScope *scope = scope_new(ctx->global);
    for (int i = 0; i < sig->arity; i++) {
        scope_define(scope, sig->param_names[i], sig->param_types[i], sig->node->line);
    }
    CheckResult r = check_block(ctx, scope, function_block(sig->node));
    sig->ret_type = r.has_return ? r.ret_type : ty(TY_VOID);
    sig->ret_known = 1;
    sig->checking = 0;
}

int ans_check_program(const AstNode *program) {
    SemCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.global = scope_new(NULL);

    for (int i = 0; i < program->n_children; i++) {
        if (program->children[i]->kind == ND_ELEMENT_DECL) collect_element(&ctx, program->children[i]);
    }
    for (int i = 0; i < ctx.n_elements; i++) {
        scope_define(ctx.global, ctx.elements[i].symbol, ty(TY_ELEMENT), 1);
    }

    for (int i = 0; i < program->n_children; i++) {
        if (program->children[i]->kind == ND_COMPOUND_DECL) collect_compound(&ctx, program->children[i]);
    }
    for (int i = 0; i < ctx.n_compounds; i++) {
        scope_define(ctx.global, ctx.compounds[i].name, ty(TY_COMPOUND), 1);
    }

    for (int i = 0; i < program->n_children; i++) {
        if (program->children[i]->kind == ND_REACTION_DECL) collect_reaction(&ctx, program->children[i]);
    }
    for (int i = 0; i < ctx.n_reactions; i++) {
        scope_define(ctx.global, ctx.reactions[i].name, ty(TY_REACTION), 1);
    }

    for (int i = 0; i < program->n_children; i++) {
        if (program->children[i]->kind == ND_FUNCTION_DECL) collect_function(&ctx, program->children[i]);
    }
    for (int i = 0; i < ctx.n_funcs; i++) {
        scope_define(ctx.global, ctx.funcs[i].name, ty(TY_FUNCTION), 1);
    }
    for (int i = 0; i < ctx.n_funcs; i++) check_function_body(&ctx, &ctx.funcs[i]);

    for (int i = 0; i < program->n_children; i++) {
        const AstNode *n = program->children[i];
        if (n->kind == ND_ELEMENT_DECL || n->kind == ND_COMPOUND_DECL ||
            n->kind == ND_REACTION_DECL || n->kind == ND_FUNCTION_DECL) {
            continue;
        }
        check_stmt(&ctx, ctx.global, n);
    }

    return g_had_error ? 1 : 0;
}

/* ================================================================
   Runtime values
   ================================================================ */

typedef struct ElementObj ElementObj;
typedef struct FormulaObj FormulaObj;
typedef struct CompoundObj CompoundObj;
typedef struct ReactionSideObj ReactionSideObj;
typedef struct ReactionObj ReactionObj;
typedef struct ListObj ListObj;
typedef struct FunctionObj FunctionObj;

typedef enum {
    VAL_NULL,
    VAL_NUMBER,
    VAL_BOOL,
    VAL_STRING,
    VAL_CHAR,
    VAL_ELEMENT,
    VAL_COMPOUND,
    VAL_FORMULA,
    VAL_REACTION,
    VAL_REACTION_SIDE,
    VAL_QUANTITY,
    VAL_LIST,
    VAL_FUNCTION
} ValueKind;

typedef struct Value {
    ValueKind kind;
    union {
        double number;
        int boolean;
        char ch;
        char *string;
        ElementObj *element;
        CompoundObj *compound;
        FormulaObj *formula;
        ReactionObj *reaction;
        ReactionSideObj *side;
        ListObj *list;
        FunctionObj *function;
        struct {
            double amount;
            char *unit;
        } quantity;
    } as;
} Value;

struct ElementObj {
    char  *symbol;
    double atomic_mass;
    int    valence;
    char  *display;
};

typedef struct {
    ElementObj *element;
    int         count;
} FormulaEntry;

struct FormulaObj {
    char         *text;
    FormulaEntry *entries;
    int           count;
};

struct CompoundObj {
    char       *id;
    char       *name;
    FormulaObj *formula;
};

struct ReactionSideObj {
    CompoundObj **species;
    double       *coeffs;
    int           count;
};

struct ReactionObj {
    char            *id;
    ReactionSideObj *reactants;
    ReactionSideObj *products;
    char            *energy_kind;
    double           energy_amount;
    char            *energy_unit;
};

struct ListObj {
    Value *items;
    int    count;
};

typedef struct EnvBinding {
    char  *name;
    Value  value;
} EnvBinding;

typedef struct Env {
    EnvBinding *items;
    int         count;
    int         cap;
    struct Env *parent;
} Env;

struct FunctionObj {
    char        *name;
    const AstNode *node;
    char       **param_names;
    Type        *param_types;
    int          arity;
    Env         *closure;
};

typedef struct {
    Env *global;
    int  failed;

    ElementObj  **elements;
    int           n_elements;
    int           cap_elements;

    CompoundObj **compounds;
    int           n_compounds;
    int           cap_compounds;
} RtCtx;

static Value v_null(void) {
    Value v;
    memset(&v, 0, sizeof(v));
    v.kind = VAL_NULL;
    return v;
}

static Value v_number(double x) {
    Value v = v_null();
    v.kind = VAL_NUMBER;
    v.as.number = x;
    return v;
}

static Value v_bool(int b) {
    Value v = v_null();
    v.kind = VAL_BOOL;
    v.as.boolean = b ? 1 : 0;
    return v;
}

static Env *env_new(Env *parent) {
    Env *e = calloc(1, sizeof(Env));
    if (!e) { perror("calloc"); exit(1); }
    e->parent = parent;
    return e;
}

static void env_define(Env *env, const char *name, Value value) {
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->items[i].name, name) == 0) {
            env->items[i].value = value;
            return;
        }
    }
    if (env->count == env->cap) {
        int new_cap = env->cap ? env->cap * 2 : 8;
        env->items = realloc(env->items, (size_t)new_cap * sizeof(EnvBinding));
        if (!env->items) { perror("realloc"); exit(1); }
        env->cap = new_cap;
    }
    env->items[env->count].name = dup_cstr(name);
    env->items[env->count].value = value;
    env->count++;
}

static Value *env_lookup(Env *env, const char *name) {
    for (Env *cur = env; cur; cur = cur->parent) {
        for (int i = cur->count - 1; i >= 0; i--) {
            if (strcmp(cur->items[i].name, name) == 0) return &cur->items[i].value;
        }
    }
    return NULL;
}

static int env_assign(Env *env, const char *name, Value value) {
    Value *slot = env_lookup(env, name);
    if (!slot) return 0;
    *slot = value;
    return 1;
}

static void runtime_error(RtCtx *ctx, int line, const char *message) {
    error_at(line, 1, "%s", message);
    ctx->failed = 1;
}

static char *unquote(const char *text) {
    size_t n = strlen(text);
    if (n < 2) return dup_cstr(text);
    char *out = malloc(n);
    if (!out) { perror("malloc"); exit(1); }
    int j = 0;
    for (size_t i = 1; i + 1 < n; i++) {
        if (text[i] == '\\' && i + 1 < n - 1) {
            i++;
            switch (text[i]) {
                case 'n': out[j++] = '\n'; break;
                case 'r': out[j++] = '\r'; break;
                case 't': out[j++] = '\t'; break;
                default:  out[j++] = text[i]; break;
            }
        } else {
            out[j++] = text[i];
        }
    }
    out[j] = '\0';
    return out;
}

static char unquote_char(const char *text) {
    if (text[1] == '\\') {
        switch (text[2]) {
            case 'n': return '\n';
            case 'r': return '\r';
            case 't': return '\t';
            default:  return text[2];
        }
    }
    return text[1];
}

static void rt_add_element(RtCtx *ctx, ElementObj *e) {
    if (ctx->n_elements == ctx->cap_elements) {
        int new_cap = ctx->cap_elements ? ctx->cap_elements * 2 : 8;
        ctx->elements = realloc(ctx->elements, (size_t)new_cap * sizeof(ElementObj *));
        if (!ctx->elements) { perror("realloc"); exit(1); }
        ctx->cap_elements = new_cap;
    }
    ctx->elements[ctx->n_elements++] = e;
}

static void rt_add_compound(RtCtx *ctx, CompoundObj *c) {
    if (ctx->n_compounds == ctx->cap_compounds) {
        int new_cap = ctx->cap_compounds ? ctx->cap_compounds * 2 : 8;
        ctx->compounds = realloc(ctx->compounds, (size_t)new_cap * sizeof(CompoundObj *));
        if (!ctx->compounds) { perror("realloc"); exit(1); }
        ctx->cap_compounds = new_cap;
    }
    ctx->compounds[ctx->n_compounds++] = c;
}

static ElementObj *rt_find_element(RtCtx *ctx, const char *symbol) {
    for (int i = 0; i < ctx->n_elements; i++) {
        if (strcmp(ctx->elements[i]->symbol, symbol) == 0) return ctx->elements[i];
    }
    return NULL;
}

static CompoundObj *rt_find_compound_by_formula(RtCtx *ctx, const char *formula_text, int line) {
    FormulaInfo f;
    if (!parse_formula_text(formula_text, line, &f)) return NULL;
    for (int i = 0; i < ctx->n_compounds; i++) {
        FormulaInfo existing;
        parse_formula_text(ctx->compounds[i]->formula->text, line, &existing);
        if (formula_equal(&existing, &f)) return ctx->compounds[i];
    }
    return NULL;
}

static FormulaObj *rt_formula_from_text(RtCtx *ctx, const char *text, int line) {
    FormulaInfo info;
    if (!parse_formula_text(text, line, &info)) {
        ctx->failed = 1;
        return NULL;
    }
    FormulaObj *f = calloc(1, sizeof(FormulaObj));
    if (!f) { perror("calloc"); exit(1); }
    f->text = dup_cstr(text);
    f->entries = calloc((size_t)info.count, sizeof(FormulaEntry));
    if (!f->entries) { perror("calloc"); exit(1); }
    f->count = info.count;
    for (int i = 0; i < info.count; i++) {
        ElementObj *e = rt_find_element(ctx, info.items[i].symbol);
        if (!e) {
            char msg[160];
            snprintf(msg, sizeof(msg), "formula '%s' uses undeclared element '%s'",
                     text, info.items[i].symbol);
            runtime_error(ctx, line, msg);
            e = calloc(1, sizeof(ElementObj));
            if (!e) { perror("calloc"); exit(1); }
            e->symbol = dup_cstr(info.items[i].symbol);
        }
        f->entries[i].element = e;
        f->entries[i].count = info.items[i].count;
    }
    return f;
}

static int formula_count_for(FormulaObj *f, ElementObj *e) {
    for (int i = 0; i < f->count; i++) {
        if (strcmp(f->entries[i].element->symbol, e->symbol) == 0) return f->entries[i].count;
    }
    return 0;
}

static double side_coeff_for(ReactionSideObj *side, CompoundObj *compound) {
    for (int i = 0; i < side->count; i++) {
        if (side->species[i] == compound) return side->coeffs[i];
    }
    return -1.0;
}

static Value eval_expr(RtCtx *ctx, Env *env, const AstNode *expr);

typedef enum {
    EX_OK,
    EX_RETURN,
    EX_ERROR
} ExecStatus;

typedef struct {
    ExecStatus status;
    Value      value;
} ExecResult;

static ExecResult exec_stmt(RtCtx *ctx, Env *env, const AstNode *stmt);

static Value eval_member(RtCtx *ctx, Env *env, const AstNode *expr) {
    Value base = eval_expr(ctx, env, expr->children[0]);
    if (ctx->failed) return v_null();
    const char *field = expr->text;

    if (base.kind == VAL_ELEMENT) {
        if (text_eq(field, "atomic_mass")) return v_number(base.as.element->atomic_mass);
        if (text_eq(field, "valence")) return v_number((double)base.as.element->valence);
        if (text_eq(field, "symbol")) {
            Value v = v_null();
            v.kind = VAL_STRING;
            v.as.string = base.as.element->display;
            return v;
        }
    } else if (base.kind == VAL_COMPOUND) {
        if (text_eq(field, "formula")) {
            Value v = v_null();
            v.kind = VAL_FORMULA;
            v.as.formula = base.as.compound->formula;
            return v;
        }
        if (text_eq(field, "name")) {
            Value v = v_null();
            v.kind = VAL_STRING;
            v.as.string = base.as.compound->name;
            return v;
        }
    } else if (base.kind == VAL_REACTION) {
        if (text_eq(field, "reactants")) {
            Value v = v_null();
            v.kind = VAL_REACTION_SIDE;
            v.as.side = base.as.reaction->reactants;
            return v;
        }
        if (text_eq(field, "products")) {
            Value v = v_null();
            v.kind = VAL_REACTION_SIDE;
            v.as.side = base.as.reaction->products;
            return v;
        }
        if (text_eq(field, "energy")) {
            Value v = v_null();
            v.kind = VAL_QUANTITY;
            v.as.quantity.amount = base.as.reaction->energy_amount;
            v.as.quantity.unit = base.as.reaction->energy_unit;
            return v;
        }
    }

    char msg[160];
    snprintf(msg, sizeof(msg), "runtime member error: no field '%s'", field);
    runtime_error(ctx, expr->line, msg);
    return v_null();
}

static Value eval_index(RtCtx *ctx, Env *env, const AstNode *expr) {
    Value base = eval_expr(ctx, env, expr->children[0]);
    Value idx = eval_expr(ctx, env, expr->children[1]);
    if (ctx->failed) return v_null();

    if (base.kind == VAL_LIST && idx.kind == VAL_NUMBER) {
        int n = (int)idx.as.number;
        if (n < 0 || n >= base.as.list->count || fabs(idx.as.number - (double)n) > 1e-9) {
            runtime_error(ctx, expr->line, "list index out of range");
            return v_null();
        }
        return base.as.list->items[n];
    }
    if (base.kind == VAL_FORMULA && idx.kind == VAL_ELEMENT) {
        return v_number((double)formula_count_for(base.as.formula, idx.as.element));
    }
    if (base.kind == VAL_REACTION_SIDE && idx.kind == VAL_COMPOUND) {
        double coeff = side_coeff_for(base.as.side, idx.as.compound);
        if (coeff < 0.0) {
            runtime_error(ctx, expr->line, "reaction side does not contain requested compound");
            return v_null();
        }
        return v_number(coeff);
    }
    runtime_error(ctx, expr->line, "invalid index operation at runtime");
    return v_null();
}

static ExecResult exec_block(RtCtx *ctx, Env *parent, const AstNode *block) {
    Env *env = env_new(parent);
    ExecResult out;
    out.status = EX_OK;
    out.value = v_null();
    for (int i = 0; i < block->n_children; i++) {
        ExecResult r = exec_stmt(ctx, env, block->children[i]);
        if (r.status != EX_OK) return r;
    }
    return out;
}

static Value call_function(RtCtx *ctx, Env *env, const AstNode *call) {
    const AstNode *callee = call->children[0];
    if (callee->kind == ND_IDENT && strcmp(callee->text, "mol") == 0) {
        Value arg = eval_expr(ctx, env, call->children[1]);
        if (arg.kind == VAL_QUANTITY) return v_number(arg.as.quantity.amount);
        if (arg.kind == VAL_NUMBER) return arg;
        runtime_error(ctx, call->line, "mol expects number or quantity");
        return v_null();
    }

    Value *fn_val = callee->kind == ND_IDENT ? env_lookup(env, callee->text) : NULL;
    if (!fn_val || fn_val->kind != VAL_FUNCTION) {
        runtime_error(ctx, callee->line, "called value is not a function");
        return v_null();
    }
    FunctionObj *fn = fn_val->as.function;
    int argc = call->n_children - 1;
    if (argc != fn->arity) {
        runtime_error(ctx, call->line, "wrong number of function arguments");
        return v_null();
    }
    Value *args = calloc((size_t)argc, sizeof(Value));
    if (!args) { perror("calloc"); exit(1); }
    for (int i = 0; i < argc; i++) args[i] = eval_expr(ctx, env, call->children[i + 1]);
    if (ctx->failed) return v_null();

    Env *call_env = env_new(fn->closure);
    for (int i = 0; i < argc; i++) env_define(call_env, fn->param_names[i], args[i]);
    ExecResult r = exec_block(ctx, call_env, function_block(fn->node));
    if (r.status == EX_ERROR) return v_null();
    if (r.status == EX_RETURN) return r.value;
    return v_null();
}

static int value_truthy(Value v) {
    if (v.kind == VAL_BOOL) return v.as.boolean;
    if (v.kind == VAL_NUMBER) return fabs(v.as.number) > 1e-12;
    return v.kind != VAL_NULL;
}

static Value eval_expr(RtCtx *ctx, Env *env, const AstNode *expr) {
    switch (expr->kind) {
        case ND_NUMBER:
            return v_number(expr->num);
        case ND_STRING: {
            Value v = v_null();
            v.kind = VAL_STRING;
            v.as.string = unquote(expr->text);
            return v;
        }
        case ND_CHAR: {
            Value v = v_null();
            v.kind = VAL_CHAR;
            v.as.ch = unquote_char(expr->text);
            return v;
        }
        case ND_QUANTITY: {
            Value v = v_null();
            v.kind = VAL_QUANTITY;
            v.as.quantity.amount = expr->children[0]->num;
            v.as.quantity.unit = dup_cstr(expr->text);
            return v;
        }
        case ND_IDENT: {
            Value *v = env_lookup(env, expr->text);
            if (!v) {
                char msg[160];
                snprintf(msg, sizeof(msg), "undefined name '%s'", expr->text);
                runtime_error(ctx, expr->line, msg);
                return v_null();
            }
            return *v;
        }
        case ND_FORMULA: {
            ElementObj *e = rt_find_element(ctx, expr->text);
            if (e) {
                Value v = v_null();
                v.kind = VAL_ELEMENT;
                v.as.element = e;
                return v;
            }
            CompoundObj *c = rt_find_compound_by_formula(ctx, expr->text, expr->line);
            if (c) {
                Value v = v_null();
                v.kind = VAL_COMPOUND;
                v.as.compound = c;
                return v;
            }
            Value v = v_null();
            v.kind = VAL_FORMULA;
            v.as.formula = rt_formula_from_text(ctx, expr->text, expr->line);
            return v;
        }
        case ND_LIST: {
            ListObj *list = calloc(1, sizeof(ListObj));
            if (!list) { perror("calloc"); exit(1); }
            list->count = expr->n_children;
            list->items = calloc((size_t)list->count, sizeof(Value));
            if (!list->items && list->count) { perror("calloc"); exit(1); }
            for (int i = 0; i < list->count; i++) list->items[i] = eval_expr(ctx, env, expr->children[i]);
            Value v = v_null();
            v.kind = VAL_LIST;
            v.as.list = list;
            return v;
        }
        case ND_UNARY: {
            Value r = eval_expr(ctx, env, expr->children[0]);
            return v_number(-r.as.number);
        }
        case ND_BINARY: {
            Value l = eval_expr(ctx, env, expr->children[0]);
            Value r = eval_expr(ctx, env, expr->children[1]);
            if (ctx->failed) return v_null();
            if (text_eq(expr->text, "+")) return v_number(l.as.number + r.as.number);
            if (text_eq(expr->text, "-")) return v_number(l.as.number - r.as.number);
            if (text_eq(expr->text, "*")) return v_number(l.as.number * r.as.number);
            if (text_eq(expr->text, "/")) {
                if (fabs(r.as.number) < 1e-12) {
                    runtime_error(ctx, expr->line, "division by zero");
                    return v_null();
                }
                return v_number(l.as.number / r.as.number);
            }
            if (text_eq(expr->text, "<")) return v_bool(l.as.number < r.as.number);
            if (text_eq(expr->text, "<=")) return v_bool(l.as.number <= r.as.number);
            if (text_eq(expr->text, ">")) return v_bool(l.as.number > r.as.number);
            if (text_eq(expr->text, ">=")) return v_bool(l.as.number >= r.as.number);
            if (text_eq(expr->text, "==")) {
                if (l.kind == VAL_NUMBER && r.kind == VAL_NUMBER) return v_bool(fabs(l.as.number - r.as.number) < 1e-9);
                if (l.kind == VAL_BOOL && r.kind == VAL_BOOL) return v_bool(l.as.boolean == r.as.boolean);
                if (l.kind == VAL_CHAR && r.kind == VAL_CHAR) return v_bool(l.as.ch == r.as.ch);
                if (l.kind == VAL_STRING && r.kind == VAL_STRING) return v_bool(strcmp(l.as.string, r.as.string) == 0);
                return v_bool(0);
            }
            if (text_eq(expr->text, "!=")) {
                if (l.kind == VAL_NUMBER && r.kind == VAL_NUMBER) return v_bool(fabs(l.as.number - r.as.number) >= 1e-9);
                if (l.kind == VAL_BOOL && r.kind == VAL_BOOL) return v_bool(l.as.boolean != r.as.boolean);
                if (l.kind == VAL_CHAR && r.kind == VAL_CHAR) return v_bool(l.as.ch != r.as.ch);
                if (l.kind == VAL_STRING && r.kind == VAL_STRING) return v_bool(strcmp(l.as.string, r.as.string) != 0);
                return v_bool(1);
            }
            return v_null();
        }
        case ND_MEMBER:
            return eval_member(ctx, env, expr);
        case ND_INDEX:
            return eval_index(ctx, env, expr);
        case ND_CALL:
            return call_function(ctx, env, expr);
        default:
            runtime_error(ctx, expr->line, "invalid expression at runtime");
            return v_null();
    }
}

static int assign_target(RtCtx *ctx, Env *env, const AstNode *lhs, Value value) {
    if (lhs->kind != ND_IDENT) {
        runtime_error(ctx, lhs->line, "only variables can be assigned in ANS Part 2");
        return 0;
    }
    if (!env_assign(env, lhs->text, value)) {
        char msg[160];
        snprintf(msg, sizeof(msg), "assignment to undefined name '%s'", lhs->text);
        runtime_error(ctx, lhs->line, msg);
        return 0;
    }
    return 1;
}

static void print_value(Value v) {
    switch (v.kind) {
        case VAL_NUMBER:   printf("%.10g\n", v.as.number); break;
        case VAL_BOOL:     printf("%s\n", v.as.boolean ? "true" : "false"); break;
        case VAL_STRING:   printf("%s\n", v.as.string); break;
        case VAL_CHAR:     printf("%c\n", v.as.ch); break;
        case VAL_ELEMENT:  printf("%s\n", v.as.element->symbol); break;
        case VAL_COMPOUND: printf("%s\n", v.as.compound->id); break;
        case VAL_QUANTITY: printf("%.10g %s\n", v.as.quantity.amount, v.as.quantity.unit); break;
        case VAL_NULL:     printf("null\n"); break;
        default:           printf("<%d>\n", v.kind); break;
    }
}

static ExecResult exec_ok(void) {
    ExecResult r;
    r.status = EX_OK;
    r.value = v_null();
    return r;
}

static ExecResult exec_return(Value v) {
    ExecResult r;
    r.status = EX_RETURN;
    r.value = v;
    return r;
}

static ExecResult exec_error(void) {
    ExecResult r;
    r.status = EX_ERROR;
    r.value = v_null();
    return r;
}

static ExecResult exec_stmt(RtCtx *ctx, Env *env, const AstNode *stmt) {
    if (ctx->failed) return exec_error();
    switch (stmt->kind) {
        case ND_VAR_DECL: {
            Value v = eval_expr(ctx, env, stmt->children[0]);
            env_define(env, stmt->text, v);
            return ctx->failed ? exec_error() : exec_ok();
        }
        case ND_ASSIGN: {
            Value v = eval_expr(ctx, env, stmt->children[1]);
            assign_target(ctx, env, stmt->children[0], v);
            return ctx->failed ? exec_error() : exec_ok();
        }
        case ND_AUG_ASSIGN: {
            if (stmt->children[0]->kind != ND_IDENT) {
                runtime_error(ctx, stmt->line, "augmented assignment target must be a variable");
                return exec_error();
            }
            Value *slot = env_lookup(env, stmt->children[0]->text);
            Value rhs = eval_expr(ctx, env, stmt->children[1]);
            if (!slot || slot->kind != VAL_NUMBER || rhs.kind != VAL_NUMBER) {
                runtime_error(ctx, stmt->line, "augmented assignment requires numbers");
                return exec_error();
            }
            if (text_eq(stmt->text, "+=")) slot->as.number += rhs.as.number;
            else if (text_eq(stmt->text, "-=")) slot->as.number -= rhs.as.number;
            else if (text_eq(stmt->text, "*=")) slot->as.number *= rhs.as.number;
            else if (text_eq(stmt->text, "/=")) {
                if (fabs(rhs.as.number) < 1e-12) {
                    runtime_error(ctx, stmt->line, "division by zero");
                    return exec_error();
                }
                slot->as.number /= rhs.as.number;
            }
            return exec_ok();
        }
        case ND_RETURN:
            return exec_return(stmt->n_children ? eval_expr(ctx, env, stmt->children[0]) : v_null());
        case ND_PRINT:
        {
            Value v = eval_expr(ctx, env, stmt->children[0]);
            if (ctx->failed) return exec_error();
            print_value(v);
            return ctx->failed ? exec_error() : exec_ok();
        }
        case ND_EXPR_STMT:
            (void)eval_expr(ctx, env, stmt->children[0]);
            return ctx->failed ? exec_error() : exec_ok();
        case ND_FOR: {
            Value iterable = eval_expr(ctx, env, stmt->children[1]);
            if (ctx->failed) return exec_error();
            if (iterable.kind == VAL_LIST) {
                for (int i = 0; i < iterable.as.list->count; i++) {
                    Env *loop = env_new(env);
                    env_define(loop, stmt->children[0]->text, iterable.as.list->items[i]);
                    ExecResult r = exec_block(ctx, loop, stmt->children[2]);
                    if (r.status != EX_OK) return r;
                }
            } else if (iterable.kind == VAL_FORMULA) {
                for (int i = 0; i < iterable.as.formula->count; i++) {
                    Env *loop = env_new(env);
                    Value v = v_null();
                    v.kind = VAL_ELEMENT;
                    v.as.element = iterable.as.formula->entries[i].element;
                    env_define(loop, stmt->children[0]->text, v);
                    ExecResult r = exec_block(ctx, loop, stmt->children[2]);
                    if (r.status != EX_OK) return r;
                }
            } else if (iterable.kind == VAL_REACTION_SIDE) {
                for (int i = 0; i < iterable.as.side->count; i++) {
                    Env *loop = env_new(env);
                    Value v = v_null();
                    v.kind = VAL_COMPOUND;
                    v.as.compound = iterable.as.side->species[i];
                    env_define(loop, stmt->children[0]->text, v);
                    ExecResult r = exec_block(ctx, loop, stmt->children[2]);
                    if (r.status != EX_OK) return r;
                }
            } else {
                runtime_error(ctx, stmt->line, "value is not iterable");
                return exec_error();
            }
            return exec_ok();
        }
        case ND_IF: {
            int i = 0;
            while (i + 1 < stmt->n_children) {
                Value cond = eval_expr(ctx, env, stmt->children[i]);
                if (value_truthy(cond)) return exec_block(ctx, env, stmt->children[i + 1]);
                i += 2;
            }
            if (i < stmt->n_children) return exec_block(ctx, env, stmt->children[i]);
            return exec_ok();
        }
        case ND_BLOCK:
            return exec_block(ctx, env, stmt);
        default:
            return exec_ok();
    }
}

static void exec_element_decl(RtCtx *ctx, const AstNode *decl) {
    ElementObj *e = calloc(1, sizeof(ElementObj));
    if (!e) { perror("calloc"); exit(1); }
    e->symbol = dup_cstr(decl->text);
    e->display = dup_cstr(decl->text);
    for (int i = 0; i < decl->n_children; i++) {
        const AstNode *f = decl->children[i];
        if (text_eq(f->text, "atomic_mass")) e->atomic_mass = f->children[0]->num;
        else if (text_eq(f->text, "valence")) e->valence = (int)f->children[0]->num;
        else if (text_eq(f->text, "symbol")) {
            free(e->display);
            e->display = unquote(f->children[0]->text);
        }
    }
    rt_add_element(ctx, e);
    Value v = v_null();
    v.kind = VAL_ELEMENT;
    v.as.element = e;
    env_define(ctx->global, e->symbol, v);
}

static void exec_compound_decl(RtCtx *ctx, const AstNode *decl) {
    CompoundObj *c = calloc(1, sizeof(CompoundObj));
    if (!c) { perror("calloc"); exit(1); }
    c->id = dup_cstr(decl->text);
    c->name = dup_cstr(decl->text);
    const AstNode *formula = field_child(decl, "formula");
    const AstNode *name = field_child(decl, "name");
    if (formula) c->formula = rt_formula_from_text(ctx, formula->text, formula->line);
    if (name) {
        free(c->name);
        c->name = unquote(name->text);
    }
    rt_add_compound(ctx, c);
    Value v = v_null();
    v.kind = VAL_COMPOUND;
    v.as.compound = c;
    env_define(ctx->global, c->id, v);
}

static CompoundObj *rt_species_to_compound(RtCtx *ctx, Env *env, const AstNode *node) {
    if (node->kind == ND_IDENT) {
        Value *v = env_lookup(env, node->text);
        if (!v || v->kind != VAL_COMPOUND) {
            runtime_error(ctx, node->line, "reaction species identifier is not a compound");
            return NULL;
        }
        return v->as.compound;
    }
    if (node->kind == ND_FORMULA) {
        CompoundObj *c = rt_find_compound_by_formula(ctx, node->text, node->line);
        if (!c) runtime_error(ctx, node->line, "reaction formula does not name a declared compound");
        return c;
    }
    runtime_error(ctx, node->line, "invalid reaction species");
    return NULL;
}

static ReactionSideObj *rt_side_from_ast(RtCtx *ctx, Env *env, const AstNode *rxn_expr) {
    ReactionSideObj *side = calloc(1, sizeof(ReactionSideObj));
    if (!side) { perror("calloc"); exit(1); }
    side->count = rxn_expr->n_children;
    side->species = calloc((size_t)side->count, sizeof(CompoundObj *));
    side->coeffs = calloc((size_t)side->count, sizeof(double));
    if ((!side->species || !side->coeffs) && side->count) { perror("calloc"); exit(1); }
    for (int i = 0; i < rxn_expr->n_children; i++) {
        const AstNode *term = rxn_expr->children[i];
        double coeff = 1.0;
        const AstNode *species = NULL;
        if (term->n_children == 1) species = term->children[0];
        else if (term->n_children >= 2) {
            coeff = term->children[0]->num;
            species = term->children[1];
        }
        side->coeffs[i] = coeff;
        side->species[i] = rt_species_to_compound(ctx, env, species);
    }
    return side;
}

static void exec_reaction_decl(RtCtx *ctx, const AstNode *decl) {
    ReactionObj *r = calloc(1, sizeof(ReactionObj));
    if (!r) { perror("calloc"); exit(1); }
    r->id = dup_cstr(decl->text);
    const AstNode *reactants = field_child(decl, "reactants");
    const AstNode *products = field_child(decl, "products");
    const AstNode *energy = field_child(decl, "energy");
    if (reactants) r->reactants = rt_side_from_ast(ctx, ctx->global, reactants);
    if (products) r->products = rt_side_from_ast(ctx, ctx->global, products);
    if (energy && energy->kind == ND_ENERGY_EXPR && energy->n_children > 0) {
        r->energy_kind = dup_cstr(energy->text ? energy->text : "");
        const AstNode *q = energy->children[0];
        if (q->kind == ND_QUANTITY) {
            r->energy_amount = q->children[0]->num;
            r->energy_unit = dup_cstr(q->text);
        }
    }
    Value v = v_null();
    v.kind = VAL_REACTION;
    v.as.reaction = r;
    env_define(ctx->global, r->id, v);
}

static void exec_function_decl(RtCtx *ctx, const AstNode *fn_node) {
    FunctionObj *fn = calloc(1, sizeof(FunctionObj));
    if (!fn) { perror("calloc"); exit(1); }
    fn->name = dup_cstr(fn_node->text);
    fn->node = fn_node;
    fn->arity = function_arity(fn_node);
    fn->param_names = calloc((size_t)fn->arity, sizeof(char *));
    fn->param_types = calloc((size_t)fn->arity, sizeof(Type));
    if ((!fn->param_names || !fn->param_types) && fn->arity) { perror("calloc"); exit(1); }
    int p = 0;
    for (int i = 0; i < fn_node->n_children; i++) {
        if (fn_node->children[i]->kind == ND_PARAM) {
            parse_param_text(fn_node->children[i]->text, &fn->param_types[p], &fn->param_names[p]);
            p++;
        }
    }
    fn->closure = ctx->global;
    Value v = v_null();
    v.kind = VAL_FUNCTION;
    v.as.function = fn;
    env_define(ctx->global, fn->name, v);
}

int ans_execute_program(const AstNode *program) {
    RtCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.global = env_new(NULL);

    for (int i = 0; i < program->n_children; i++) {
        if (program->children[i]->kind == ND_ELEMENT_DECL) exec_element_decl(&ctx, program->children[i]);
    }
    for (int i = 0; i < program->n_children; i++) {
        if (program->children[i]->kind == ND_COMPOUND_DECL) exec_compound_decl(&ctx, program->children[i]);
    }
    for (int i = 0; i < program->n_children; i++) {
        if (program->children[i]->kind == ND_REACTION_DECL) exec_reaction_decl(&ctx, program->children[i]);
    }
    for (int i = 0; i < program->n_children; i++) {
        if (program->children[i]->kind == ND_FUNCTION_DECL) exec_function_decl(&ctx, program->children[i]);
    }
    for (int i = 0; i < program->n_children; i++) {
        const AstNode *n = program->children[i];
        if (n->kind == ND_ELEMENT_DECL || n->kind == ND_COMPOUND_DECL ||
            n->kind == ND_REACTION_DECL || n->kind == ND_FUNCTION_DECL) {
            continue;
        }
        ExecResult r = exec_stmt(&ctx, ctx.global, n);
        if (r.status == EX_ERROR || ctx.failed) return 1;
    }
    return ctx.failed || g_had_error ? 1 : 0;
}
