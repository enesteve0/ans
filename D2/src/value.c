#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "value.h"
#include "error.h"

/* ================================================================
   Units
   ================================================================ */

Unit parse_unit(const char *text) {
    if (strcmp(text, "mol")    == 0) return UNIT_MOL;
    if (strcmp(text, "g")      == 0) return UNIT_G;
    if (strcmp(text, "kg")     == 0) return UNIT_KG;
    if (strcmp(text, "kJ")     == 0) return UNIT_KJ;
    if (strcmp(text, "g/mol")  == 0) return UNIT_G_MOL;
    if (strcmp(text, "kg/mol") == 0) return UNIT_KG_MOL;
    if (strcmp(text, "kJ/mol") == 0) return UNIT_KJ_MOL;
    return UNIT_UNKNOWN;
}

const char *unit_name(Unit u) {
    switch (u) {
        case UNIT_MOL:    return "mol";
        case UNIT_G:      return "g";
        case UNIT_KG:     return "kg";
        case UNIT_KJ:     return "kJ";
        case UNIT_G_MOL:  return "g/mol";
        case UNIT_KG_MOL: return "kg/mol";
        case UNIT_KJ_MOL: return "kJ/mol";
        default:          return "?";
    }
}

/* ================================================================
   FormulaVal
   ================================================================ */

FormulaVal *formula_parse(const char *text) {
    int cap = 8;
    FormulaVal *f = calloc(1, sizeof(FormulaVal));
    f->symbols = malloc((size_t)cap * sizeof(char *));
    f->counts  = malloc((size_t)cap * sizeof(int));
    f->size    = 0;

    const char *p = text;
    while (*p) {
        if (!isupper((unsigned char)*p)) { p++; continue; }

        /* Read element symbol: one uppercase + optional lowercase */
        char sym[4] = {0, 0, 0, 0};
        sym[0] = *p++;
        if (*p && islower((unsigned char)*p)) sym[1] = *p++;

        /* Read optional count */
        int count = 0;
        while (*p && isdigit((unsigned char)*p)) count = count * 10 + (*p++ - '0');
        if (count == 0) count = 1;

        /* Check if symbol already in list (for deduplication) */
        int found = -1;
        for (int i = 0; i < f->size; i++) {
            if (strcmp(f->symbols[i], sym) == 0) { found = i; break; }
        }
        if (found >= 0) {
            f->counts[found] += count;
        } else {
            if (f->size >= cap) {
                cap *= 2;
                f->symbols = realloc(f->symbols, (size_t)cap * sizeof(char *));
                f->counts  = realloc(f->counts,  (size_t)cap * sizeof(int));
            }
            f->symbols[f->size] = strdup(sym);
            f->counts[f->size]  = count;
            f->size++;
        }
    }
    return f;
}

void formula_free(FormulaVal *f) {
    if (!f) return;
    for (int i = 0; i < f->size; i++) free(f->symbols[i]);
    free(f->symbols);
    free(f->counts);
    free(f);
}

int formula_equal(const FormulaVal *a, const FormulaVal *b) {
    if (a->size != b->size) return 0;
    for (int i = 0; i < a->size; i++) {
        int found = 0;
        for (int j = 0; j < b->size; j++) {
            if (strcmp(a->symbols[i], b->symbols[j]) == 0) {
                if (a->counts[i] != b->counts[j]) return 0;
                found = 1;
                break;
            }
        }
        if (!found) return 0;
    }
    return 1;
}

int formula_matches_text(const FormulaVal *f, const char *text) {
    FormulaVal *parsed = formula_parse(text);
    int eq = formula_equal(f, parsed);
    formula_free(parsed);
    return eq;
}

const char *formula_to_cstr(const FormulaVal *f) {
    static char buf[256];
    buf[0] = '\0';
    for (int i = 0; i < f->size; i++) {
        strncat(buf, f->symbols[i], sizeof(buf) - strlen(buf) - 1);
        if (f->counts[i] != 1) {
            char num[16];
            snprintf(num, sizeof(num), "%d", f->counts[i]);
            strncat(buf, num, sizeof(buf) - strlen(buf) - 1);
        }
    }
    return buf;
}

/* ================================================================
   Value constructors
   ================================================================ */

Value make_void(void) {
    Value v; v.type = VAL_VOID; return v;
}
Value make_number(double n) {
    Value v; v.type = VAL_NUMBER; v.number = n; return v;
}
Value make_string_own(char *s) {
    Value v; v.type = VAL_STRING; v.string = s; return v;
}
Value make_char(char c) {
    Value v; v.type = VAL_CHAR; v.ch = c; return v;
}
Value make_bool(int b) {
    Value v; v.type = VAL_BOOL; v.boolean = b; return v;
}
Value make_quantity(double val, Unit u) {
    Value v; v.type = VAL_QUANTITY; v.quantity.value = val; v.quantity.unit = u; return v;
}
Value make_formula(FormulaVal *f) {
    Value v; v.type = VAL_FORMULA; v.formula = f; return v;
}
Value make_element(ElementVal *e) {
    Value v; v.type = VAL_ELEMENT; v.element = e; return v;
}
Value make_compound(CompoundVal *c) {
    Value v; v.type = VAL_COMPOUND; v.compound = c; return v;
}
Value make_reaction(ReactionVal *r) {
    Value v; v.type = VAL_REACTION; v.reaction = r; return v;
}
Value make_reaction_side(ReactionSideVal *rs) {
    Value v; v.type = VAL_REACTION_SIDE; v.reaction_side = rs; return v;
}
Value make_list(Value *items, int count) {
    Value v; v.type = VAL_LIST;
    v.list.count = count;
    v.list.items = malloc((size_t)(count > 0 ? count : 1) * sizeof(Value));
    for (int i = 0; i < count; i++) v.list.items[i] = items[i];
    return v;
}
Value make_return_val(Value inner) {
    Value v; v.type = VAL_RETURN;
    v.return_val = malloc(sizeof(Value));
    *v.return_val = inner;
    return v;
}

/* ================================================================
   value_print
   ================================================================ */

void value_print(const Value *v) {
    switch (v->type) {
        case VAL_NUMBER: {
            double n = v->number;
            if (n == (long long)n && n > -1e15 && n < 1e15)
                printf("%.6g", n);
            else
                printf("%g", n);
            break;
        }
        case VAL_STRING:   printf("%s", v->string);  break;
        case VAL_CHAR:     printf("%c", v->ch);       break;
        case VAL_BOOL:     printf("%s", v->boolean ? "true" : "false"); break;
        case VAL_QUANTITY: printf("%g %s", v->quantity.value, unit_name(v->quantity.unit)); break;
        case VAL_FORMULA:  if (v->formula) printf("%s", formula_to_cstr(v->formula)); break;
        case VAL_ELEMENT:  if (v->element) printf("<element %s>", v->element->id); break;
        case VAL_COMPOUND: if (v->compound) printf("<compound %s>", v->compound->id); break;
        case VAL_REACTION: if (v->reaction) printf("<reaction %s>", v->reaction->id); break;
        default: break;
    }
}

/* ================================================================
   Environment
   ================================================================ */

Env *env_new(Env *parent) {
    Env *e = calloc(1, sizeof(Env));
    e->parent = parent;
    e->count  = 0;
    return e;
}

void env_free(Env *e) {
    if (!e) return;
    for (int i = 0; i < e->count; i++) free(e->names[i]);
    free(e);
}

int env_lookup_opt(Env *e, const char *name, Value *out) {
    for (Env *cur = e; cur; cur = cur->parent) {
        for (int i = cur->count - 1; i >= 0; i--) {
            if (strcmp(cur->names[i], name) == 0) {
                if (out) *out = cur->values[i];
                return 1;
            }
        }
    }
    return 0;
}

Value env_lookup(Env *e, const char *name) {
    Value v;
    if (!env_lookup_opt(e, name, &v)) {
        fprintf(stderr, "%s: runtime error: undefined name '%s'\n", g_filename, name);
        exit(1);
    }
    return v;
}

void env_bind(Env *e, const char *name, Value v) {
    if (e->count >= ENV_SLOTS) {
        fprintf(stderr, "internal error: environment overflow\n");
        exit(1);
    }
    e->names[e->count]  = strdup(name);
    e->values[e->count] = v;
    e->count++;
}

int env_set(Env *e, const char *name, Value v) {
    for (Env *cur = e; cur; cur = cur->parent) {
        for (int i = cur->count - 1; i >= 0; i--) {
            if (strcmp(cur->names[i], name) == 0) {
                cur->values[i] = v;
                return 1;
            }
        }
    }
    return 0;
}
