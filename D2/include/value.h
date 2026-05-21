#ifndef ANS_VALUE_H
#define ANS_VALUE_H

/* ---- Units ---- */
typedef enum {
    UNIT_MOL, UNIT_G, UNIT_KG, UNIT_KJ,
    UNIT_G_MOL, UNIT_KG_MOL, UNIT_KJ_MOL,
    UNIT_UNKNOWN
} Unit;

Unit        parse_unit(const char *text);
const char *unit_name(Unit u);

/* ---- FormulaVal: ordered element->count map ---- */
typedef struct {
    char **symbols;   /* element symbols in first-appearance order */
    int  *counts;     /* atom counts */
    int   size;
} FormulaVal;

FormulaVal *formula_parse(const char *text);
void        formula_free(FormulaVal *f);
int         formula_equal(const FormulaVal *a, const FormulaVal *b);
int         formula_matches_text(const FormulaVal *f, const char *text);
/* Returns a statically allocated string valid until the next call. */
const char *formula_to_cstr(const FormulaVal *f);

/* ---- Value type tag ---- */
typedef enum {
    VAL_VOID,
    VAL_NUMBER,
    VAL_STRING,
    VAL_CHAR,
    VAL_BOOL,
    VAL_QUANTITY,
    VAL_FORMULA,
    VAL_LIST,
    VAL_ELEMENT,
    VAL_COMPOUND,
    VAL_REACTION,
    VAL_REACTION_SIDE,
    VAL_RETURN
} ValueType;

/* Forward declarations */
typedef struct ElementVal   ElementVal;
typedef struct CompoundVal  CompoundVal;
typedef struct ReactionVal  ReactionVal;
typedef struct ReactionSideVal ReactionSideVal;
typedef struct Value        Value;

/* ---- Record types ---- */
struct ElementVal {
    char  *id;           /* declaration symbol, e.g. "H" */
    double atomic_mass;
    int    valence;
    char  *symbol;       /* the symbol field string, e.g. "H" */
};

struct CompoundVal {
    char       *id;      /* declaration name, e.g. "water" */
    FormulaVal *formula;
    char       *name;    /* the name field string, e.g. "Water" */
};

typedef struct {
    double  coefficient;
    Value  *species;     /* heap-allocated Value (VAL_COMPOUND or VAL_FORMULA) */
} ReactionTerm;

struct ReactionSideVal {
    ReactionTerm *terms;
    int           count;
};

struct ReactionVal {
    char          *id;
    ReactionSideVal reactants;
    ReactionSideVal products;
    char          *direction;     /* "exothermic" or "endothermic" */
    double         energy_value;
    Unit           energy_unit;
};

/* ---- Tagged-union value ---- */
struct Value {
    ValueType type;
    union {
        double  number;
        char   *string;   /* owned, heap-allocated */
        char    ch;
        int     boolean;
        struct { double value; Unit unit; } quantity;
        FormulaVal      *formula;
        struct { Value *items; int count; } list;
        ElementVal      *element;
        CompoundVal     *compound;
        ReactionVal     *reaction;
        ReactionSideVal *reaction_side;
        Value           *return_val;  /* VAL_RETURN: heap-allocated inner value */
    };
};

/* ---- Constructors ---- */
Value make_void(void);
Value make_number(double n);
Value make_string_own(char *s);   /* takes ownership of malloc'd string */
Value make_char(char c);
Value make_bool(int b);
Value make_quantity(double v, Unit u);
Value make_formula(FormulaVal *f);
Value make_list(Value *items, int count);
Value make_element(ElementVal *e);
Value make_compound(CompoundVal *c);
Value make_reaction(ReactionVal *r);
Value make_reaction_side(ReactionSideVal *rs);
Value make_return_val(Value inner);

void value_print(const Value *v);

/* ---- Lexical environment (linked-list scope frames) ---- */
#define ENV_SLOTS 256

typedef struct Env {
    struct Env *parent;
    char  *names[ENV_SLOTS];
    Value  values[ENV_SLOTS];
    int    count;
} Env;

Env  *env_new(Env *parent);
void  env_free(Env *e);            /* frees only this frame, not parent */
Value env_lookup(Env *e, const char *name);          /* exits if not found */
int   env_lookup_opt(Env *e, const char *name, Value *out); /* 0 = not found */
void  env_bind(Env *e, const char *name, Value v);
int   env_set(Env *e, const char *name, Value v);    /* 0 = not found */

#endif /* ANS_VALUE_H */
