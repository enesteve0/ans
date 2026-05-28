### File: D2/Makefile
```make
CC      = cc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -Iinclude
SRCS    = src/main.c src/lexer.c src/parser.c src/ast.c src/error.c \
          src/value.c src/checker.c src/interpreter.c
TARGET  = ans

D3DIR   = ../D3

.PHONY: all debug test clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

# -----------------------------------------------------------------------
# test: run all categories; exit 1 if any failure.
# D3 categories require ../D3/ to exist. Missing D3 assets are a failure
# because Part 2 requires end-to-end evidence for the D3 programs.
# -----------------------------------------------------------------------
test: $(TARGET)
	@PASS=0; FAIL=0; \
	\
	echo "=== Parser: valid programs (expect exit 0) ==="; \
	for f in tests/valid/*.ans; do \
	    if ./$(TARGET) "$$f" >/dev/null 2>&1; then \
	        echo "  PASS: $$f"; PASS=$$((PASS+1)); \
	    else \
	        echo "  FAIL: $$f"; FAIL=$$((FAIL+1)); \
	    fi; \
	done; \
	\
	echo "=== Parser: invalid programs (expect exit 1) ==="; \
	for f in tests/invalid/*.ans; do \
	    if ./$(TARGET) "$$f" >/dev/null 2>&1; then \
	        echo "  FAIL (accepted): $$f"; FAIL=$$((FAIL+1)); \
	    else \
	        echo "  PASS (rejected): $$f"; PASS=$$((PASS+1)); \
	    fi; \
	done; \
	\
	echo "=== Type checker: valid D2 programs (--check, expect exit 0) ==="; \
	for f in tests/check-valid/*.ans; do \
	    if ./$(TARGET) --check "$$f" >/dev/null 2>&1; then \
	        echo "  PASS: $$f"; PASS=$$((PASS+1)); \
	    else \
	        echo "  FAIL: $$f"; FAIL=$$((FAIL+1)); \
	    fi; \
	done; \
	\
	echo "=== Type checker: invalid D2 programs (--check, expect exit 1) ==="; \
	for f in tests/check-invalid/*.ans; do \
	    if ./$(TARGET) --check "$$f" >/dev/null 2>&1; then \
	        echo "  FAIL (accepted): $$f"; FAIL=$$((FAIL+1)); \
	    else \
	        echo "  PASS (rejected): $$f"; PASS=$$((PASS+1)); \
	    fi; \
	done; \
	\
	if [ -d "$(D3DIR)/programs" ]; then \
	    echo "=== D3 example programs (--run, expect exit 0) ==="; \
	    for f in $(D3DIR)/programs/*.ans; do \
	        if ./$(TARGET) --run "$$f" >/dev/null 2>&1; then \
	            echo "  PASS: $$f"; PASS=$$((PASS+1)); \
	        else \
	            echo "  FAIL: $$f"; FAIL=$$((FAIL+1)); \
	        fi; \
	    done; \
	else \
	    echo "=== D3 example programs: $(D3DIR)/programs not found — FAIL ==="; \
	    FAIL=$$((FAIL+1)); \
	fi; \
	\
	if [ -d "$(D3DIR)/malformed" ]; then \
	    echo "=== D3 malformed programs (parse, expect exit 1) ==="; \
	    for f in $(D3DIR)/malformed/*.ans; do \
	        if ./$(TARGET) "$$f" >/dev/null 2>&1; then \
	            echo "  FAIL (accepted): $$f"; FAIL=$$((FAIL+1)); \
	        else \
	            echo "  PASS (rejected): $$f"; PASS=$$((PASS+1)); \
	        fi; \
	    done; \
	else \
	    echo "=== D3 malformed programs: $(D3DIR)/malformed not found — FAIL ==="; \
	    FAIL=$$((FAIL+1)); \
	fi; \
	\
	if [ -d "$(D3DIR)/semantic" ]; then \
	    echo "=== Type-checker negative tests (--check, expect exit 1) ==="; \
	    for f in $(D3DIR)/semantic/sem*.ans; do \
	        if ./$(TARGET) --check "$$f" >/dev/null 2>&1; then \
	            echo "  FAIL (accepted): $$f"; FAIL=$$((FAIL+1)); \
	        else \
	            echo "  PASS (rejected): $$f"; PASS=$$((PASS+1)); \
	        fi; \
	    done; \
	    echo "=== Runtime-error tests (--run, expect exit 1) ==="; \
	    for f in $(D3DIR)/semantic/rt*.ans; do \
	        if ./$(TARGET) --run "$$f" >/dev/null 2>&1; then \
	            echo "  FAIL (no error raised): $$f"; FAIL=$$((FAIL+1)); \
	        else \
	            echo "  PASS (error raised): $$f"; PASS=$$((PASS+1)); \
	        fi; \
	    done; \
	else \
	    echo "=== Type-checker/runtime tests: $(D3DIR)/semantic not found — FAIL ==="; \
	    FAIL=$$((FAIL+1)); \
	fi; \
	\
	echo ""; \
	echo "Results: $$PASS passed, $$FAIL failed."; \
	[ "$$FAIL" -eq 0 ]

clean:
	rm -f $(TARGET)

```

### File: D2/README.md
```text
# ANS — Lexer, Parser, Type Checker & Interpreter (D2, Part 2)

A hand-written C11 implementation of the ANS (Alchemist's Notation Script) domain-specific language,
comprising a maximal-munch lexer, a recursive-descent parser, a two-pass type checker, and a
tree-walking interpreter with static (lexical) scoping.

---

## Build

```
make          # optimised build → ./ans
make debug    # add -g -DDEBUG
```

Requires a C11 compiler (`cc`).  No external dependencies.

---

## Run

```
./ans <file.ans>               # parse only; exit 0 on success, 1 on error
./ans --dump-ast <file.ans>    # parse and print the AST
./ans --check <file.ans>       # type-check; exit 0 if clean, 1 on semantic error
./ans --run <file.ans>         # type-check then interpret; exit 0 on success, 1 on error
```

### Exit codes

| Mode | Success | Failure |
|---|---|---|
| parse only | 0 | 1 (parse error) |
| `--check`  | 0 | 1 (semantic error) |
| `--run`    | 0 | 1 (semantic or runtime error) |

---

## Test

```
make test
```

Runs all test categories and reports a pass/fail count.  Exits **0** if every test passes,
**1** if any test fails.

### Categories

| Category | Location | Expected exit |
|---|---|---|
| Parser — valid programs | `tests/valid/` | 0 |
| Parser — invalid programs | `tests/invalid/` | 1 |
| Type checker — valid programs | `tests/check-valid/` | 0 |
| Type checker — invalid programs | `tests/check-invalid/` | 1 |
| D3 example programs (end-to-end) | `../D3/programs/` | 0 |
| D3 malformed parser programs | `../D3/malformed/` | 1 |
| Type-checker negative tests | `../D3/semantic/sem*.ans` | 1 |
| Runtime-error tests | `../D3/semantic/rt*.ans` | 1 |

The D3 folders are required for the Part 2 test target.  If they are missing,
`make test` reports a failure instead of silently skipping the end-to-end
evidence programs.

---

## Error format

```
<file>:<line>:<col>: error: <message>
```

Runtime errors omit the column:

```
<file>:<line>: runtime error: <message>
```

---

## Project layout

```
D2/
├── Makefile
├── README.md
├── include/
│   ├── ast.h           NodeKind enum, AstNode, ast_* API
│   ├── checker.h       two-pass type checker API
│   ├── error.h         error_at / fatal_at
│   ├── interpreter.h   tree-walking interpreter API
│   ├── lexer.h         token types, Token struct, Lexer API
│   ├── parser.h        Parser struct, parse_program
│   └── value.h         Value tagged union, Env, FormulaVal
├── src/
│   ├── main.c          CLI entry point (--check / --run flags)
│   ├── lexer.c         maximal-munch scanner
│   ├── parser.c        recursive-descent parser (one function per rule)
│   ├── ast.c           node constructors + tree pretty-printer
│   ├── error.c         error formatting and g_had_error flag
│   ├── value.c         Value constructors, formula parsing, environments
│   ├── checker.c       two-pass type checker
│   └── interpreter.c   tree-walking interpreter with static scoping
└── tests/
    ├── valid/          8 programs — must all be accepted (exit 0)
    ├── invalid/        4 programs — must all be rejected (exit 1)
    ├── check-valid/    semantic programs that must pass --check
    └── check-invalid/  semantic programs that must fail --check
```

---

## Authors

Yusuf Hamza Çelebi · Enes Özkan
CSE 341 — Programming Languages, Gebze Technical University, Spring 2026

```

### File: D2/include/ast.h
```c
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

```

### File: D2/include/checker.h
```c
#ifndef ANS_CHECKER_H
#define ANS_CHECKER_H

#include "ast.h"

/* Run the type checker over the program AST.
 * Reports errors via error_at() (sets g_had_error).
 * Does not stop on first error; continues to find more. */
void check_program(const AstNode *prog);

#endif /* ANS_CHECKER_H */

```

### File: D2/include/error.h
```c
#ifndef ANS_ERROR_H
#define ANS_ERROR_H

extern const char *g_filename;
extern int g_had_error;

void error_at(int line, int col, const char *fmt, ...);
void fatal_at(int line, int col, const char *fmt, ...);

#endif /* ANS_ERROR_H */

```

### File: D2/include/interpreter.h
```c
#ifndef ANS_INTERPRETER_H
#define ANS_INTERPRETER_H

#include "ast.h"
#include "value.h"

/* Execute the fully type-checked program AST.
 * Assumes check_program() has already been called and found no errors. */
void interpret_program(const AstNode *prog);

#endif /* ANS_INTERPRETER_H */

```

### File: D2/include/lexer.h
```c
#ifndef ANS_LEXER_H
#define ANS_LEXER_H

typedef enum {
    /* Literals */
    TOK_IDENT,
    TOK_NUMBER,
    TOK_STRING,
    TOK_CHAR,
    TOK_FORMULA,
    TOK_UNIT_LIT,       /* g/mol  kg/mol  kJ/mol — tried before '/' operator */

    /* Hard keywords */
    TOK_LET,
    TOK_FUNCTION,
    TOK_RETURN,
    TOK_FOR,
    TOK_IN,
    TOK_IF,
    TOK_ELIF,
    TOK_ELSE,
    TOK_PRINT,

    /* Operators */
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PLUS_EQ,
    TOK_MINUS_EQ,
    TOK_STAR_EQ,
    TOK_SLASH_EQ,
    TOK_EQ,
    TOK_EQ_EQ,
    TOK_BANG_EQ,
    TOK_LT,
    TOK_LT_EQ,
    TOK_GT,
    TOK_GT_EQ,

    /* Separators */
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_COMMA,
    TOK_SEMICOLON,
    TOK_DOT,

    TOK_EOF,
    TOK_ERROR
} TokenType;

typedef struct {
    TokenType   type;
    const char *start;   /* pointer into source buffer */
    int         length;
    int         line;
    int         col;
    double      num_val; /* filled for TOK_NUMBER */
} Token;

typedef struct {
    const char *src;
    int         pos;
    int         line;
    int         col;
} Lexer;

void  lexer_init(Lexer *l, const char *source);
Token lexer_next(Lexer *l);

const char *token_type_name(TokenType t);

#endif /* ANS_LEXER_H */

```

### File: D2/include/parser.h
```c
#ifndef ANS_PARSER_H
#define ANS_PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer  *lexer;
    Token   current;
    Token   previous;
} Parser;

void     parser_init(Parser *p, Lexer *l);
AstNode *parse_program(Parser *p);

#endif /* ANS_PARSER_H */

```

### File: D2/include/value.h
```c
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

```

### File: D2/src/ast.c
```c
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

```

### File: D2/src/checker.c
```c
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

static int type_is_quantity(const char *type) {
    if (!type) return 0;
    return strncmp(type, "quantity:", 9) == 0;
}

static char *make_quantity_type(const char *unit) {
    const char *u = unit ? unit : "unknown";
    size_t len = strlen(u) + 10; /* "quantity:" (9) + unit + '\0' (1) */
    char *out = malloc(len);
    snprintf(out, len, "quantity:%s", u);
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
            !type_is_quantity(arg_type) &&
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
        case ND_QUANTITY: return make_quantity_type(node->text);
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
            if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
                strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
                strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
                if (strcmp(ltype, "unknown") != 0 && strcmp(rtype, "unknown") != 0) {
                    if (strcmp(ltype, rtype) != 0) {
                        error_at(node->line, 0,
                            "comparison type mismatch: left is %s, right is %s",
                            ltype, rtype);
                    }
                }
                return "bool";
            }
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

```

### File: D2/src/error.c
```c
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "error.h"

const char *g_filename = "<input>";
int         g_had_error = 0;

void error_at(int line, int col, const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "%s:%d:%d: error: ", g_filename, line, col);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    g_had_error = 1;
}

void fatal_at(int line, int col, const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "%s:%d:%d: error: ", g_filename, line, col);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}

```

### File: D2/src/interpreter.c
```c
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
                formula_free(fv);
                runtime_error(line,
                    "reaction species formula '%s' does not match a declared compound",
                    species_node->text);
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
                if (left.type == VAL_CHAR && right.type == VAL_CHAR)
                    return make_bool(left.ch == right.ch);
                if (left.type == VAL_QUANTITY && right.type == VAL_QUANTITY) {
                    if (left.quantity.unit != right.quantity.unit) {
                        runtime_error(node->line, "cannot compare quantities with different units: %s vs %s",
                            unit_name(left.quantity.unit), unit_name(right.quantity.unit));
                    }
                    return make_bool(left.quantity.value == right.quantity.value);
                }
                return make_bool(0);
            }
            if (strcmp(op, "!=") == 0) {
                if (left.type == VAL_NUMBER && right.type == VAL_NUMBER)
                    return make_bool(left.number != right.number);
                if (left.type == VAL_BOOL && right.type == VAL_BOOL)
                    return make_bool(left.boolean != right.boolean);
                if (left.type == VAL_STRING && right.type == VAL_STRING)
                    return make_bool(strcmp(left.string, right.string) != 0);
                if (left.type == VAL_CHAR && right.type == VAL_CHAR)
                    return make_bool(left.ch != right.ch);
                if (left.type == VAL_QUANTITY && right.type == VAL_QUANTITY) {
                    if (left.quantity.unit != right.quantity.unit) {
                        runtime_error(node->line, "cannot compare quantities with different units: %s vs %s",
                            unit_name(left.quantity.unit), unit_name(right.quantity.unit));
                    }
                    return make_bool(left.quantity.value != right.quantity.value);
                }
                return make_bool(1);
            }
            if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                if (strcmp(op, "<")  == 0) return make_bool(left.number <  right.number);
                if (strcmp(op, "<=") == 0) return make_bool(left.number <= right.number);
                if (strcmp(op, ">")  == 0) return make_bool(left.number >  right.number);
                if (strcmp(op, ">=") == 0) return make_bool(left.number >= right.number);
            } else if (left.type == VAL_QUANTITY && right.type == VAL_QUANTITY) {
                if (left.quantity.unit != right.quantity.unit) {
                    runtime_error(node->line, "cannot compare quantities with different units: %s vs %s",
                        unit_name(left.quantity.unit), unit_name(right.quantity.unit));
                }
                if (strcmp(op, "<")  == 0) return make_bool(left.quantity.value <  right.quantity.value);
                if (strcmp(op, "<=") == 0) return make_bool(left.quantity.value <= right.quantity.value);
                if (strcmp(op, ">")  == 0) return make_bool(left.quantity.value >  right.quantity.value);
                if (strcmp(op, ">=") == 0) return make_bool(left.quantity.value >= right.quantity.value);
            } else {
                runtime_error(node->line, "operator '%s' requires numbers or quantities of the same unit", op);
            }
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

            Value cur;
            Value *store_slot = NULL;
            char *store_name = NULL;

            if (lhs_node->kind == ND_IDENT) {
                if (!env_lookup_opt(env, lhs_node->text, &cur))
                    runtime_error(node->line,
                        "augmented assignment to undeclared variable '%s'", lhs_node->text);
                store_name = lhs_node->text;
            } else if (lhs_node->kind == ND_INDEX) {
                Value obj   = eval_expr(lhs_node->children[0], env);
                Value index = eval_expr(lhs_node->children[1], env);
                if (obj.type != VAL_LIST || index.type != VAL_NUMBER)
                    runtime_error(node->line, "invalid augmented assignment target");
                int idx = (int)index.number;
                if (idx < 0 || idx >= obj.list.count)
                    runtime_error(node->line,
                        "list index %d out of range [0..%d]",
                        idx, obj.list.count - 1);
                store_slot = &obj.list.items[idx];
                cur = *store_slot;
            } else {
                runtime_error(node->line, "unsupported augmented assignment target");
            }

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
            if (store_slot) {
                *store_slot = make_number(result);
            } else {
                env_set(env, store_name, make_number(result));
            }
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

```

### File: D2/src/lexer.c
```c
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "lexer.h"
#include "error.h"

void lexer_init(Lexer *l, const char *source) {
    l->src  = source;
    l->pos  = 0;
    l->line = 1;
    l->col  = 1;
}

/* ------------------------------------------------------------------ helpers */

static char peek_c(const Lexer *l)          { return l->src[l->pos]; }
static char peek2_c(const Lexer *l)         { return l->src[l->pos] ? l->src[l->pos + 1] : '\0'; }

static char advance_c(Lexer *l) {
    char c = l->src[l->pos++];
    if (c == '\n') { l->line++; l->col = 1; }
    else            { l->col++; }
    return c;
}

static Token make_tok(TokenType t, const char *start, int length, int line, int col) {
    Token tok;
    tok.type    = t;
    tok.start   = start;
    tok.length  = length;
    tok.line    = line;
    tok.col     = col;
    tok.num_val = 0.0;
    return tok;
}

/* ------------------------------------------------------------------ keyword table */

static const struct { const char *word; TokenType type; } KEYWORDS[] = {
    { "let",      TOK_LET      },
    { "function", TOK_FUNCTION },
    { "return",   TOK_RETURN   },
    { "for",      TOK_FOR      },
    { "in",       TOK_IN       },
    { "if",       TOK_IF       },
    { "elif",     TOK_ELIF     },
    { "else",     TOK_ELSE     },
    { "print",    TOK_PRINT    },
    { NULL,       TOK_ERROR    },
};

static TokenType check_keyword(const char *start, int len) {
    for (int i = 0; KEYWORDS[i].word; i++) {
        if ((int)strlen(KEYWORDS[i].word) == len &&
            memcmp(KEYWORDS[i].word, start, (size_t)len) == 0) {
            return KEYWORDS[i].type;
        }
    }
    return TOK_IDENT;
}

/* ------------------------------------------------------------------ unit literal probe
   Returns the length (4, 5, or 6) if src[pos..] starts with a compound unit,
   0 otherwise.  Called before consuming '/'. */

static int probe_unit_literal(const Lexer *l) {
    const char *s = l->src + l->pos;
    if (strncmp(s, "g/mol",  5) == 0 && !isalpha((unsigned char)s[5])) return 5;
    if (strncmp(s, "kg/mol", 6) == 0 && !isalpha((unsigned char)s[6])) return 6;
    if (strncmp(s, "kJ/mol", 6) == 0 && !isalpha((unsigned char)s[6])) return 6;
    return 0;
}

/* ------------------------------------------------------------------ scan string */

static Token scan_string(Lexer *l) {
    int start_line = l->line, start_col = l->col;
    const char *start = l->src + l->pos - 1; /* opening '"' already consumed */
    while (peek_c(l) && peek_c(l) != '"' && peek_c(l) != '\n') {
        if (peek_c(l) == '\\') advance_c(l);
        advance_c(l);
    }
    if (peek_c(l) != '"') {
        error_at(start_line, start_col, "unterminated string literal");
        return make_tok(TOK_ERROR, start, 1, start_line, start_col);
    }
    advance_c(l); /* closing '"' */
    int len = (int)(l->src + l->pos - start);
    return make_tok(TOK_STRING, start, len, start_line, start_col);
}

/* ------------------------------------------------------------------ scan char */

static Token scan_char(Lexer *l) {
    int start_line = l->line, start_col = l->col;
    const char *start = l->src + l->pos - 1;
    if (peek_c(l) == '\\') advance_c(l);
    if (peek_c(l) && peek_c(l) != '\'') advance_c(l);
    if (peek_c(l) != '\'') {
        error_at(start_line, start_col, "unterminated character literal");
        return make_tok(TOK_ERROR, start, 1, start_line, start_col);
    }
    advance_c(l);
    int len = (int)(l->src + l->pos - start);
    return make_tok(TOK_CHAR, start, len, start_line, start_col);
}

/* ------------------------------------------------------------------ scan number */

static Token scan_number(Lexer *l, const char *start, int start_line, int start_col) {
    while (isdigit((unsigned char)peek_c(l))) advance_c(l);
    if (peek_c(l) == '.' && isdigit((unsigned char)peek2_c(l))) {
        advance_c(l);
        while (isdigit((unsigned char)peek_c(l))) advance_c(l);
    }
    if (peek_c(l) == 'e' || peek_c(l) == 'E') {
        advance_c(l);
        if (peek_c(l) == '+' || peek_c(l) == '-') advance_c(l);
        while (isdigit((unsigned char)peek_c(l))) advance_c(l);
    }
    int len = (int)(l->src + l->pos - start);
    Token tok = make_tok(TOK_NUMBER, start, len, start_line, start_col);
    tok.num_val = strtod(start, NULL);
    return tok;
}

/* ------------------------------------------------------------------ scan ident/keyword */

static Token scan_ident(Lexer *l, const char *start, int start_line, int start_col) {
    while (isalnum((unsigned char)peek_c(l)) || peek_c(l) == '_') advance_c(l);
    int len = (int)(l->src + l->pos - start);
    TokenType t = check_keyword(start, len);
    return make_tok(t, start, len, start_line, start_col);
}

/* ------------------------------------------------------------------ scan formula */

static Token scan_formula(Lexer *l, const char *start, int start_line, int start_col) {
    /* already consumed first uppercase letter; continue */
    if (islower((unsigned char)peek_c(l))) advance_c(l); /* optional lowercase */
    while (isdigit((unsigned char)peek_c(l))) advance_c(l); /* optional digits */
    /* additional element groups */
    while (isupper((unsigned char)peek_c(l))) {
        advance_c(l);
        if (islower((unsigned char)peek_c(l))) advance_c(l);
        while (isdigit((unsigned char)peek_c(l))) advance_c(l);
    }
    int len = (int)(l->src + l->pos - start);
    return make_tok(TOK_FORMULA, start, len, start_line, start_col);
}

/* ------------------------------------------------------------------ main entry */

Token lexer_next(Lexer *l) {
    /* skip whitespace and line comments */
    for (;;) {
        while (peek_c(l) && isspace((unsigned char)peek_c(l))) advance_c(l);
        if (peek_c(l) == '/' && peek2_c(l) == '/') {
            while (peek_c(l) && peek_c(l) != '\n') advance_c(l);
            continue;
        }
        break;
    }

    if (!peek_c(l)) {
        return make_tok(TOK_EOF, l->src + l->pos, 0, l->line, l->col);
    }

    int start_line = l->line, start_col = l->col;
    const char *start = l->src + l->pos;

    /* compound unit literals — must come before '/' operator */
    int unit_len = probe_unit_literal(l);
    if (unit_len) {
        for (int i = 0; i < unit_len; i++) advance_c(l);
        return make_tok(TOK_UNIT_LIT, start, unit_len, start_line, start_col);
    }

    char c = advance_c(l);

    /* strings and chars */
    if (c == '"') return scan_string(l);
    if (c == '\'') return scan_char(l);

    /* numbers */
    if (isdigit((unsigned char)c)) return scan_number(l, start, start_line, start_col);

    /* identifiers and hard keywords */
    if (c == '_' || islower((unsigned char)c)) return scan_ident(l, start, start_line, start_col);

    /* formula literals — uppercase start */
    if (isupper((unsigned char)c)) return scan_formula(l, start, start_line, start_col);

    /* two-character operators */
    char n = peek_c(l);
    if (c == '=' && n == '=') { advance_c(l); return make_tok(TOK_EQ_EQ,    start, 2, start_line, start_col); }
    if (c == '!' && n == '=') { advance_c(l); return make_tok(TOK_BANG_EQ,  start, 2, start_line, start_col); }
    if (c == '<' && n == '=') { advance_c(l); return make_tok(TOK_LT_EQ,    start, 2, start_line, start_col); }
    if (c == '>' && n == '=') { advance_c(l); return make_tok(TOK_GT_EQ,    start, 2, start_line, start_col); }
    if (c == '+' && n == '=') { advance_c(l); return make_tok(TOK_PLUS_EQ,  start, 2, start_line, start_col); }
    if (c == '-' && n == '=') { advance_c(l); return make_tok(TOK_MINUS_EQ, start, 2, start_line, start_col); }
    if (c == '*' && n == '=') { advance_c(l); return make_tok(TOK_STAR_EQ,  start, 2, start_line, start_col); }
    if (c == '/' && n == '=') { advance_c(l); return make_tok(TOK_SLASH_EQ, start, 2, start_line, start_col); }

    /* single-character */
    switch (c) {
        case '+': return make_tok(TOK_PLUS,      start, 1, start_line, start_col);
        case '-': return make_tok(TOK_MINUS,     start, 1, start_line, start_col);
        case '*': return make_tok(TOK_STAR,      start, 1, start_line, start_col);
        case '/': return make_tok(TOK_SLASH,     start, 1, start_line, start_col);
        case '=': return make_tok(TOK_EQ,        start, 1, start_line, start_col);
        case '<': return make_tok(TOK_LT,        start, 1, start_line, start_col);
        case '>': return make_tok(TOK_GT,        start, 1, start_line, start_col);
        case '{': return make_tok(TOK_LBRACE,    start, 1, start_line, start_col);
        case '}': return make_tok(TOK_RBRACE,    start, 1, start_line, start_col);
        case '(': return make_tok(TOK_LPAREN,    start, 1, start_line, start_col);
        case ')': return make_tok(TOK_RPAREN,    start, 1, start_line, start_col);
        case '[': return make_tok(TOK_LBRACKET,  start, 1, start_line, start_col);
        case ']': return make_tok(TOK_RBRACKET,  start, 1, start_line, start_col);
        case ',': return make_tok(TOK_COMMA,     start, 1, start_line, start_col);
        case ';': return make_tok(TOK_SEMICOLON, start, 1, start_line, start_col);
        case '.': return make_tok(TOK_DOT,       start, 1, start_line, start_col);
        default:
            error_at(start_line, start_col, "unexpected character '%c'", c);
            return make_tok(TOK_ERROR, start, 1, start_line, start_col);
    }
}

const char *token_type_name(TokenType t) {
    switch (t) {
        case TOK_IDENT:      return "IDENT";
        case TOK_NUMBER:     return "NUMBER";
        case TOK_STRING:     return "STRING";
        case TOK_CHAR:       return "CHAR";
        case TOK_FORMULA:    return "FORMULA";
        case TOK_UNIT_LIT:   return "UNIT_LIT";
        case TOK_LET:        return "'let'";
        case TOK_FUNCTION:   return "'function'";
        case TOK_RETURN:     return "'return'";
        case TOK_FOR:        return "'for'";
        case TOK_IN:         return "'in'";
        case TOK_IF:         return "'if'";
        case TOK_ELIF:       return "'elif'";
        case TOK_ELSE:       return "'else'";
        case TOK_PRINT:      return "'print'";
        case TOK_PLUS:       return "'+'";
        case TOK_MINUS:      return "'-'";
        case TOK_STAR:       return "'*'";
        case TOK_SLASH:      return "'/'";
        case TOK_PLUS_EQ:    return "'+='";
        case TOK_MINUS_EQ:   return "'-='";
        case TOK_STAR_EQ:    return "'*='";
        case TOK_SLASH_EQ:   return "'/='";
        case TOK_EQ:         return "'='";
        case TOK_EQ_EQ:      return "'=='";
        case TOK_BANG_EQ:    return "'!='";
        case TOK_LT:         return "'<'";
        case TOK_LT_EQ:      return "'<='";
        case TOK_GT:         return "'>'";
        case TOK_GT_EQ:      return "'>='";
        case TOK_LBRACE:     return "'{'";
        case TOK_RBRACE:     return "'}'";
        case TOK_LPAREN:     return "'('";
        case TOK_RPAREN:     return "')'";
        case TOK_LBRACKET:   return "'['";
        case TOK_RBRACKET:   return "']'";
        case TOK_COMMA:      return "','";
        case TOK_SEMICOLON:  return "';'";
        case TOK_DOT:        return "'.'";
        case TOK_EOF:        return "EOF";
        case TOK_ERROR:      return "ERROR";
        default:             return "?";
    }
}

```

### File: D2/src/main.c
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "error.h"
#include "checker.h"
#include "interpreter.h"

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *buf = malloc((size_t)size + 1);
    if (!buf) { perror("malloc"); exit(1); }
    fread(buf, 1, (size_t)size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

int main(int argc, char **argv) {
    int dump_ast = 0;
    int do_check = 0;
    int do_run   = 0;
    const char *filepath = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dump-ast") == 0) {
            dump_ast = 1;
        } else if (strcmp(argv[i], "--check") == 0) {
            do_check = 1;
        } else if (strcmp(argv[i], "--run") == 0) {
            do_check = 1;
            do_run   = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "unknown option: %s\n", argv[i]);
            fprintf(stderr, "usage: ans [--dump-ast] [--check] [--run] <file.ans>\n");
            return 1;
        } else {
            filepath = argv[i];
        }
    }

    if (!filepath) {
        fprintf(stderr, "usage: ans [--dump-ast] [--check] [--run] <file.ans>\n");
        return 1;
    }

    g_filename = filepath;
    char *source = read_file(filepath);

    Lexer  lexer;
    Parser parser;
    lexer_init(&lexer, source);
    parser_init(&parser, &lexer);

    AstNode *tree = parse_program(&parser);

    if (dump_ast) ast_print(tree, 0);

    if (do_check && !g_had_error) {
        check_program(tree);
    }

    if (do_run && !g_had_error) {
        interpret_program(tree);
    }

    ast_free(tree);
    free(source);
    return g_had_error ? 1 : 0;
}

```

### File: D2/src/parser.c
```c
#include <string.h>
#include <stdio.h>
#include "parser.h"
#include "error.h"

/* ================================================================
   Helpers
   ================================================================ */

static int tok_is(const Token *t, TokenType type) {
    return t->type == type;
}

/* Test if current token is an IDENT with a specific lexeme. */
static int ident_is(const Token *t, const char *word) {
    if (t->type != TOK_IDENT) return 0;
    return (int)strlen(word) == t->length && memcmp(t->start, word, (size_t)t->length) == 0;
}

static void advance(Parser *p) {
    p->previous = p->current;
    p->current  = lexer_next(p->lexer);
}

/* Consume and return previous; crash if type doesn't match. */
static Token expect(Parser *p, TokenType type) {
    if (!tok_is(&p->current, type)) {
        error_at(p->current.line, p->current.col,
                 "expected %s but got %.*s",
                 token_type_name(type),
                 p->current.length, p->current.start);
        /* panic-mode recovery: skip to next sync point */
        while (!tok_is(&p->current, TOK_SEMICOLON) &&
               !tok_is(&p->current, TOK_RBRACE)    &&
               !tok_is(&p->current, TOK_EOF)) {
            advance(p);
        }
    }
    Token t = p->current;
    advance(p);
    return t;
}

/* Consume and return previous; crash if ident lexeme doesn't match. */
static Token expect_ident(Parser *p, const char *word) {
    if (!ident_is(&p->current, word)) {
        error_at(p->current.line, p->current.col,
                 "expected '%s' but got %.*s",
                 word, p->current.length, p->current.start);
    }
    Token t = p->current;
    advance(p);
    return t;
}

static int check(const Parser *p, TokenType type) {
    return tok_is(&p->current, type);
}

static int match(Parser *p, TokenType type) {
    if (!check(p, type)) return 0;
    advance(p);
    return 1;
}

/* ================================================================
   Contextual keyword helpers
   ================================================================ */

static int is_unit(const Parser *p) {
    return tok_is(&p->current, TOK_UNIT_LIT) ||
           ident_is(&p->current, "mol") ||
           ident_is(&p->current, "g")   ||
           ident_is(&p->current, "kg")  ||
           ident_is(&p->current, "kJ");
}

static int is_type_name(const Parser *p) {
    return ident_is(&p->current, "element")  ||
           ident_is(&p->current, "compound") ||
           ident_is(&p->current, "reaction");
}

/* ================================================================
   Forward declarations
   ================================================================ */

static AstNode *parse_statement(Parser *p);
static AstNode *parse_expression(Parser *p);
static AstNode *parse_block(Parser *p);

/* ================================================================
   §10  Expressions
   ================================================================ */

static AstNode *parse_primary(Parser *p) {
    Token t = p->current;

    /* number — may be followed by a unit → quantity */
    if (check(p, TOK_NUMBER)) {
        advance(p);
        AstNode *num = ast_new(ND_NUMBER, t.line);
        num->num = t.num_val;
        ast_set_text(num, t.start, t.length);

        if (is_unit(p)) {
            Token unit = p->current;
            advance(p);
            AstNode *q = ast_new(ND_QUANTITY, t.line);
            ast_set_text(q, unit.start, unit.length);
            ast_add_child(q, num);
            return q;
        }
        return num;
    }

    if (check(p, TOK_STRING)) {
        advance(p);
        AstNode *n = ast_new(ND_STRING, t.line);
        ast_set_text(n, t.start, t.length);
        return n;
    }

    if (check(p, TOK_CHAR)) {
        advance(p);
        AstNode *n = ast_new(ND_CHAR, t.line);
        ast_set_text(n, t.start, t.length);
        return n;
    }

    if (check(p, TOK_FORMULA)) {
        advance(p);
        AstNode *n = ast_new(ND_FORMULA, t.line);
        ast_set_text(n, t.start, t.length);
        return n;
    }

    /* list literal */
    if (check(p, TOK_LBRACKET)) {
        advance(p);
        AstNode *list = ast_new(ND_LIST, t.line);
        if (!check(p, TOK_RBRACKET)) {
            ast_add_child(list, parse_expression(p));
            while (match(p, TOK_COMMA)) {
                if (check(p, TOK_RBRACKET)) break;
                ast_add_child(list, parse_expression(p));
            }
        }
        expect(p, TOK_RBRACKET);
        return list;
    }

    /* parenthesised expression */
    if (check(p, TOK_LPAREN)) {
        advance(p);
        AstNode *inner = parse_expression(p);
        expect(p, TOK_RPAREN);
        return inner;
    }

    /* identifier */
    if (check(p, TOK_IDENT)) {
        advance(p);
        AstNode *n = ast_new(ND_IDENT, t.line);
        ast_set_text(n, t.start, t.length);
        return n;
    }

    error_at(t.line, t.col, "unexpected token %.*s in expression",
             t.length, t.start);
    advance(p);
    AstNode *err = ast_new(ND_IDENT, t.line);
    ast_set_text(err, "?", 1);
    return err;
}

static AstNode *parse_postfix(Parser *p) {
    AstNode *node = parse_primary(p);

    for (;;) {
        if (check(p, TOK_DOT)) {
            advance(p);
            Token member = p->current;
            expect(p, TOK_IDENT);
            AstNode *m = ast_new(ND_MEMBER, member.line);
            ast_set_text(m, member.start, member.length);
            ast_add_child(m, node);
            node = m;
        } else if (check(p, TOK_LBRACKET)) {
            advance(p);
            AstNode *idx = ast_new(ND_INDEX, p->current.line);
            ast_add_child(idx, node);
            ast_add_child(idx, parse_expression(p));
            expect(p, TOK_RBRACKET);
            node = idx;
        } else if (check(p, TOK_LPAREN)) {
            advance(p);
            AstNode *call = ast_new(ND_CALL, p->current.line);
            ast_add_child(call, node);
            if (!check(p, TOK_RPAREN)) {
                ast_add_child(call, parse_expression(p));
                while (match(p, TOK_COMMA)) {
                    ast_add_child(call, parse_expression(p));
                }
            }
            expect(p, TOK_RPAREN);
            node = call;
        } else {
            break;
        }
    }
    return node;
}

static AstNode *parse_unary(Parser *p) {
    if (check(p, TOK_MINUS)) {
        Token t = p->current;
        advance(p);
        AstNode *n = ast_new(ND_UNARY, t.line);
        ast_set_text(n, "-", 1);
        ast_add_child(n, parse_postfix(p));
        return n;
    }
    return parse_postfix(p);
}

static AstNode *parse_multiplicative(Parser *p) {
    AstNode *left = parse_unary(p);
    while (check(p, TOK_STAR) || check(p, TOK_SLASH)) {
        Token op = p->current;
        advance(p);
        AstNode *n = ast_new(ND_BINARY, op.line);
        ast_set_text(n, op.start, op.length);
        ast_add_child(n, left);
        ast_add_child(n, parse_unary(p));
        left = n;
    }
    return left;
}

static AstNode *parse_additive(Parser *p) {
    AstNode *left = parse_multiplicative(p);
    while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
        Token op = p->current;
        advance(p);
        AstNode *n = ast_new(ND_BINARY, op.line);
        ast_set_text(n, op.start, op.length);
        ast_add_child(n, left);
        ast_add_child(n, parse_multiplicative(p));
        left = n;
    }
    return left;
}

static AstNode *parse_comparison(Parser *p) {
    AstNode *left = parse_additive(p);
    if (check(p, TOK_EQ_EQ)   || check(p, TOK_BANG_EQ) ||
        check(p, TOK_LT_EQ)   || check(p, TOK_GT_EQ)   ||
        check(p, TOK_LT)      || check(p, TOK_GT)) {
        Token op = p->current;
        advance(p);
        AstNode *n = ast_new(ND_BINARY, op.line);
        ast_set_text(n, op.start, op.length);
        ast_add_child(n, left);
        ast_add_child(n, parse_additive(p));
        return n;
    }
    return left;
}

static AstNode *parse_expression(Parser *p) {
    return parse_comparison(p);
}

/* ================================================================
   §9  Reaction sub-grammar
   ================================================================ */

/* <reaction_term> → <species_ref> | <coefficient> "*" <species_ref> */
static AstNode *parse_reaction_term(Parser *p) {
    AstNode *term = ast_new(ND_REACTION_TERM, p->current.line);

    /* coefficient? */
    if (check(p, TOK_NUMBER)) {
        Token coeff = p->current;
        advance(p);
        AstNode *c = ast_new(ND_NUMBER, coeff.line);
        c->num = coeff.num_val;
        ast_set_text(c, coeff.start, coeff.length);

        if (match(p, TOK_STAR)) {
            /* species follows */
            AstNode *sp = ast_new(p->current.type == TOK_FORMULA ? ND_FORMULA : ND_IDENT,
                                  p->current.line);
            ast_set_text(sp, p->current.start, p->current.length);
            advance(p);
            ast_add_child(term, c);
            ast_add_child(term, sp);
        } else {
            /* bare number — treat as coefficient 1 with species = that number? */
            /* Actually this shouldn't happen per grammar; just return number */
            ast_add_child(term, c);
        }
        return term;
    }

    /* bare species_ref */
    AstNode *sp = ast_new(check(p, TOK_FORMULA) ? ND_FORMULA : ND_IDENT,
                          p->current.line);
    ast_set_text(sp, p->current.start, p->current.length);
    advance(p);
    ast_add_child(term, sp);
    return term;
}

/* <reaction_expression> → <reaction_term> { "+" <reaction_term> } */
static AstNode *parse_reaction_expression(Parser *p) {
    AstNode *rxn = ast_new(ND_REACTION_EXPR, p->current.line);
    ast_add_child(rxn, parse_reaction_term(p));
    while (check(p, TOK_PLUS)) {
        advance(p);
        ast_add_child(rxn, parse_reaction_term(p));
    }
    return rxn;
}

/* ================================================================
   §8  Reaction declaration
   ================================================================ */

static AstNode *parse_reaction_field(Parser *p) {
    AstNode *field = ast_new(ND_REACTION_FIELD, p->current.line);

    if (ident_is(&p->current, "reactants")) {
        Token key = p->current; advance(p);
        ast_set_text(field, key.start, key.length);
        expect(p, TOK_EQ);
        ast_add_child(field, parse_reaction_expression(p));
    } else if (ident_is(&p->current, "products")) {
        Token key = p->current; advance(p);
        ast_set_text(field, key.start, key.length);
        expect(p, TOK_EQ);
        ast_add_child(field, parse_reaction_expression(p));
    } else if (ident_is(&p->current, "check")) {
        Token key = p->current; advance(p);
        ast_set_text(field, key.start, key.length);
        expect(p, TOK_EQ);
        Token val = expect_ident(p, "balanced");
        AstNode *v = ast_new(ND_IDENT, val.line);
        ast_set_text(v, val.start, val.length);
        ast_add_child(field, v);
    } else if (ident_is(&p->current, "energy")) {
        Token key = p->current; advance(p);
        ast_set_text(field, key.start, key.length);
        expect(p, TOK_EQ);

        AstNode *energy = ast_new(ND_ENERGY_EXPR, p->current.line);
        if (ident_is(&p->current, "exothermic") || ident_is(&p->current, "endothermic")) {
            Token kind = p->current; advance(p);
            ast_set_text(energy, kind.start, kind.length);
        } else {
            error_at(p->current.line, p->current.col,
                     "expected 'exothermic' or 'endothermic'");
        }
        /* quantity: number + unit */
        Token num_tok = p->current;
        expect(p, TOK_NUMBER);
        AstNode *num = ast_new(ND_NUMBER, num_tok.line);
        num->num = num_tok.num_val;
        ast_set_text(num, num_tok.start, num_tok.length);

        if (is_unit(p)) {
            Token unit = p->current; advance(p);
            AstNode *q = ast_new(ND_QUANTITY, num_tok.line);
            ast_set_text(q, unit.start, unit.length);
            ast_add_child(q, num);
            ast_add_child(energy, q);
        } else {
            ast_add_child(energy, num);
        }
        ast_add_child(field, energy);
    } else {
        error_at(p->current.line, p->current.col,
                 "unknown reaction field '%.*s'",
                 p->current.length, p->current.start);
        advance(p);
    }
    return field;
}

static AstNode *parse_reaction_declaration(Parser *p) {
    Token kw = p->current;  /* "reaction" ident */
    advance(p);             /* consume "reaction" */
    AstNode *decl = ast_new(ND_REACTION_DECL, kw.line);

    Token name = p->current;
    expect(p, TOK_IDENT);
    ast_set_text(decl, name.start, name.length);

    expect(p, TOK_LBRACE);
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        ast_add_child(decl, parse_reaction_field(p));
        if (!match(p, TOK_COMMA)) break;
    }
    /* allow trailing comma */
    match(p, TOK_COMMA);
    expect(p, TOK_RBRACE);
    return decl;
}

/* ================================================================
   §7  Compound declaration
   ================================================================ */

static AstNode *parse_compound_declaration(Parser *p) {
    Token kw = p->current; advance(p);
    AstNode *decl = ast_new(ND_COMPOUND_DECL, kw.line);

    Token name = p->current;
    expect(p, TOK_IDENT);
    ast_set_text(decl, name.start, name.length);

    expect(p, TOK_LBRACE);
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        AstNode *field = ast_new(ND_COMPOUND_FIELD, p->current.line);
        if (ident_is(&p->current, "formula")) {
            Token key = p->current; advance(p);
            ast_set_text(field, key.start, key.length);
            expect(p, TOK_EQ);
            Token val = p->current;
            if (check(p, TOK_FORMULA)) {
                advance(p);
                AstNode *v = ast_new(ND_FORMULA, val.line);
                ast_set_text(v, val.start, val.length);
                ast_add_child(field, v);
            } else {
                error_at(val.line, val.col, "expected formula literal after 'formula ='");
                advance(p);
            }
        } else if (ident_is(&p->current, "name")) {
            Token key = p->current; advance(p);
            ast_set_text(field, key.start, key.length);
            expect(p, TOK_EQ);
            Token val = p->current;
            expect(p, TOK_STRING);
            AstNode *v = ast_new(ND_STRING, val.line);
            ast_set_text(v, val.start, val.length);
            ast_add_child(field, v);
        } else {
            error_at(p->current.line, p->current.col,
                     "unknown compound field '%.*s'",
                     p->current.length, p->current.start);
            advance(p);
        }
        ast_add_child(decl, field);
        if (!match(p, TOK_COMMA)) break;
    }
    match(p, TOK_COMMA);
    expect(p, TOK_RBRACE);
    return decl;
}

/* ================================================================
   §6  Element declaration
   ================================================================ */

static AstNode *parse_element_declaration(Parser *p) {
    Token kw = p->current; advance(p);
    AstNode *decl = ast_new(ND_ELEMENT_DECL, kw.line);

    Token sym = p->current;
    /* element symbol is a FORMULA_LITERAL, e.g. H, O, C */
    if (!check(p, TOK_FORMULA)) {
        error_at(sym.line, sym.col,
                 "expected element symbol (formula literal) after 'element'");
    } else {
        advance(p);
    }
    ast_set_text(decl, sym.start, sym.length);

    expect(p, TOK_LBRACE);
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        AstNode *field = ast_new(ND_ELEMENT_FIELD, p->current.line);
        if (ident_is(&p->current, "atomic_mass")) {
            Token key = p->current; advance(p);
            ast_set_text(field, key.start, key.length);
            expect(p, TOK_EQ);
            Token val = p->current;
            expect(p, TOK_NUMBER);
            AstNode *v = ast_new(ND_NUMBER, val.line);
            v->num = val.num_val;
            ast_set_text(v, val.start, val.length);
            ast_add_child(field, v);
        } else if (ident_is(&p->current, "valence")) {
            Token key = p->current; advance(p);
            ast_set_text(field, key.start, key.length);
            expect(p, TOK_EQ);
            Token val = p->current;
            expect(p, TOK_NUMBER);
            AstNode *v = ast_new(ND_NUMBER, val.line);
            v->num = val.num_val;
            ast_set_text(v, val.start, val.length);
            ast_add_child(field, v);
        } else if (ident_is(&p->current, "symbol")) {
            Token key = p->current; advance(p);
            ast_set_text(field, key.start, key.length);
            expect(p, TOK_EQ);
            Token val = p->current;
            expect(p, TOK_STRING);
            AstNode *v = ast_new(ND_STRING, val.line);
            ast_set_text(v, val.start, val.length);
            ast_add_child(field, v);
        } else {
            error_at(p->current.line, p->current.col,
                     "unknown element field '%.*s'",
                     p->current.length, p->current.start);
            advance(p);
        }
        ast_add_child(decl, field);
        if (!match(p, TOK_COMMA)) break;
    }
    match(p, TOK_COMMA);
    expect(p, TOK_RBRACE);
    return decl;
}

/* ================================================================
   §3  Function declaration
   ================================================================ */

static AstNode *parse_parameter(Parser *p) {
    AstNode *param = ast_new(ND_PARAM, p->current.line);

    /* optional type annotation: element/compound/reaction before an ident */
    if (is_type_name(p) && p->current.type == TOK_IDENT) {
        /* look ahead: if next is IDENT, consume as type annotation */
        Token type_tok = p->current;
        advance(p);
        if (check(p, TOK_IDENT)) {
            /* type annotation confirmed */
            char type_str[64];
            int len = type_tok.length < 63 ? type_tok.length : 63;
            memcpy(type_str, type_tok.start, (size_t)len);
            type_str[len] = '\0';

            Token name_tok = p->current;
            advance(p);

            char full[128];
            snprintf(full, sizeof(full), "%s %.*s", type_str, name_tok.length, name_tok.start);
            ast_set_text(param, full, (int)strlen(full));
        } else {
            /* just an identifier (no type annotation) — type_tok was the param name */
            ast_set_text(param, type_tok.start, type_tok.length);
        }
    } else {
        Token name_tok = p->current;
        expect(p, TOK_IDENT);
        ast_set_text(param, name_tok.start, name_tok.length);
    }
    return param;
}

static AstNode *parse_function_declaration(Parser *p) {
    Token kw = p->current; advance(p); /* consume 'function' */
    AstNode *decl = ast_new(ND_FUNCTION_DECL, kw.line);

    Token name = p->current;
    expect(p, TOK_IDENT);
    ast_set_text(decl, name.start, name.length);

    expect(p, TOK_LPAREN);
    if (!check(p, TOK_RPAREN)) {
        ast_add_child(decl, parse_parameter(p));
        while (match(p, TOK_COMMA)) {
            ast_add_child(decl, parse_parameter(p));
        }
    }
    expect(p, TOK_RPAREN);
    ast_add_child(decl, parse_block(p));
    return decl;
}

/* ================================================================
   §5  If / elif / else
   ================================================================ */

static AstNode *parse_if_statement(Parser *p) {
    Token kw = p->current; advance(p); /* 'if' */
    AstNode *ifn = ast_new(ND_IF, kw.line);

    expect(p, TOK_LPAREN);
    ast_add_child(ifn, parse_expression(p));
    expect(p, TOK_RPAREN);
    ast_add_child(ifn, parse_block(p));

    while (check(p, TOK_ELIF)) {
        advance(p);
        expect(p, TOK_LPAREN);
        ast_add_child(ifn, parse_expression(p));
        expect(p, TOK_RPAREN);
        ast_add_child(ifn, parse_block(p));
    }

    if (check(p, TOK_ELSE)) {
        advance(p);
        ast_add_child(ifn, parse_block(p));
    }
    return ifn;
}

/* ================================================================
   §4  For loop
   ================================================================ */

static AstNode *parse_for_statement(Parser *p) {
    Token kw = p->current; advance(p); /* 'for' */
    AstNode *forn = ast_new(ND_FOR, kw.line);

    expect(p, TOK_LPAREN);
    expect(p, TOK_LET);
    Token var = p->current;
    expect(p, TOK_IDENT);
    AstNode *var_node = ast_new(ND_IDENT, var.line);
    ast_set_text(var_node, var.start, var.length);
    ast_add_child(forn, var_node);

    expect(p, TOK_IN);
    ast_add_child(forn, parse_expression(p));
    expect(p, TOK_RPAREN);
    ast_add_child(forn, parse_block(p));
    return forn;
}

/* ================================================================
   §2  Block and Statements
   ================================================================ */

static AstNode *parse_block(Parser *p) {
    Token lbrace = p->current;
    expect(p, TOK_LBRACE);
    AstNode *block = ast_new(ND_BLOCK, lbrace.line);
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        AstNode *stmt = parse_statement(p);
        if (stmt) ast_add_child(block, stmt);
    }
    expect(p, TOK_RBRACE);
    return block;
}

static AstNode *parse_statement(Parser *p) {
    /* variable declaration */
    if (check(p, TOK_LET)) {
        advance(p);
        Token name = p->current;
        expect(p, TOK_IDENT);
        expect(p, TOK_EQ);
        AstNode *decl = ast_new(ND_VAR_DECL, name.line);
        ast_set_text(decl, name.start, name.length);
        ast_add_child(decl, parse_expression(p));
        expect(p, TOK_SEMICOLON);
        return decl;
    }

    /* return */
    if (check(p, TOK_RETURN)) {
        Token kw = p->current; advance(p);
        AstNode *ret = ast_new(ND_RETURN, kw.line);
        if (!check(p, TOK_SEMICOLON)) ast_add_child(ret, parse_expression(p));
        expect(p, TOK_SEMICOLON);
        return ret;
    }

    /* print */
    if (check(p, TOK_PRINT)) {
        Token kw = p->current; advance(p);
        AstNode *pr = ast_new(ND_PRINT, kw.line);
        ast_add_child(pr, parse_expression(p));
        expect(p, TOK_SEMICOLON);
        return pr;
    }

    /* for */
    if (check(p, TOK_FOR)) return parse_for_statement(p);

    /* if */
    if (check(p, TOK_IF))  return parse_if_statement(p);

    /* assignment or expression statement
       We parse an expression, then check for assignment operators. */
    {
        AstNode *lhs = parse_postfix(p);

        if (check(p, TOK_EQ)) {
            Token op = p->current; advance(p);
            AstNode *asgn = ast_new(ND_ASSIGN, op.line);
            ast_add_child(asgn, lhs);
            ast_add_child(asgn, parse_expression(p));
            expect(p, TOK_SEMICOLON);
            return asgn;
        }

        if (check(p, TOK_PLUS_EQ)  || check(p, TOK_MINUS_EQ) ||
            check(p, TOK_STAR_EQ)  || check(p, TOK_SLASH_EQ)) {
            Token op = p->current; advance(p);
            AstNode *aug = ast_new(ND_AUG_ASSIGN, op.line);
            ast_set_text(aug, op.start, op.length);
            ast_add_child(aug, lhs);
            ast_add_child(aug, parse_expression(p));
            expect(p, TOK_SEMICOLON);
            return aug;
        }

        /* expression statement — rebuild full precedence from the already-consumed postfix */
        AstNode *expr = lhs;

        /* multiplicative (higher precedence first) */
        while (check(p, TOK_STAR) || check(p, TOK_SLASH)) {
            Token op = p->current; advance(p);
            AstNode *bin = ast_new(ND_BINARY, op.line);
            ast_set_text(bin, op.start, op.length);
            ast_add_child(bin, expr);
            ast_add_child(bin, parse_unary(p));
            expr = bin;
        }
        /* additive — right operand via parse_multiplicative for correct precedence */
        while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
            Token op = p->current; advance(p);
            AstNode *bin = ast_new(ND_BINARY, op.line);
            ast_set_text(bin, op.start, op.length);
            ast_add_child(bin, expr);
            ast_add_child(bin, parse_multiplicative(p));
            expr = bin;
        }
        /* comparison */
        if (check(p, TOK_EQ_EQ) || check(p, TOK_BANG_EQ) ||
            check(p, TOK_LT_EQ) || check(p, TOK_GT_EQ)   ||
            check(p, TOK_LT)    || check(p, TOK_GT)) {
            Token op = p->current; advance(p);
            AstNode *cmp = ast_new(ND_BINARY, op.line);
            ast_set_text(cmp, op.start, op.length);
            ast_add_child(cmp, expr);
            ast_add_child(cmp, parse_additive(p));
            expr = cmp;
        }

        AstNode *stmt = ast_new(ND_EXPR_STMT, expr->line);
        ast_add_child(stmt, expr);
        expect(p, TOK_SEMICOLON);
        return stmt;
    }
}

/* ================================================================
   §1  Top-level statements and program
   ================================================================ */

static AstNode *parse_top_level(Parser *p) {
    /* element declaration */
    if (ident_is(&p->current, "element")) {
        AstNode *n = parse_element_declaration(p);
        expect(p, TOK_SEMICOLON);
        return n;
    }

    /* compound declaration */
    if (ident_is(&p->current, "compound")) {
        AstNode *n = parse_compound_declaration(p);
        expect(p, TOK_SEMICOLON);
        return n;
    }

    /* reaction declaration */
    if (ident_is(&p->current, "reaction")) {
        AstNode *n = parse_reaction_declaration(p);
        expect(p, TOK_SEMICOLON);
        return n;
    }

    /* function declaration */
    if (check(p, TOK_FUNCTION)) {
        AstNode *n = parse_function_declaration(p);
        expect(p, TOK_SEMICOLON);
        return n;
    }

    /* variable declaration */
    if (check(p, TOK_LET)) {
        advance(p);
        Token name = p->current;
        expect(p, TOK_IDENT);
        expect(p, TOK_EQ);
        AstNode *decl = ast_new(ND_VAR_DECL, name.line);
        ast_set_text(decl, name.start, name.length);
        ast_add_child(decl, parse_expression(p));
        expect(p, TOK_SEMICOLON);
        return decl;
    }

    /* print */
    if (check(p, TOK_PRINT)) {
        Token kw = p->current; advance(p);
        AstNode *pr = ast_new(ND_PRINT, kw.line);
        ast_add_child(pr, parse_expression(p));
        expect(p, TOK_SEMICOLON);
        return pr;
    }

    /* for statement — no trailing semicolon at top level (`;` optional after `}`) */
    if (check(p, TOK_FOR)) {
        AstNode *n = parse_for_statement(p);
        match(p, TOK_SEMICOLON); /* accept optional trailing `;` */
        return n;
    }

    /* if statement — no trailing semicolon */
    if (check(p, TOK_IF)) return parse_if_statement(p);

    /* expression or assignment statement */
    {
        AstNode *lhs = parse_postfix(p);

        if (check(p, TOK_EQ)) {
            Token op = p->current; advance(p);
            AstNode *asgn = ast_new(ND_ASSIGN, op.line);
            ast_add_child(asgn, lhs);
            ast_add_child(asgn, parse_expression(p));
            expect(p, TOK_SEMICOLON);
            return asgn;
        }

        if (check(p, TOK_PLUS_EQ)  || check(p, TOK_MINUS_EQ) ||
            check(p, TOK_STAR_EQ)  || check(p, TOK_SLASH_EQ)) {
            Token op = p->current; advance(p);
            AstNode *aug = ast_new(ND_AUG_ASSIGN, op.line);
            ast_set_text(aug, op.start, op.length);
            ast_add_child(aug, lhs);
            ast_add_child(aug, parse_expression(p));
            expect(p, TOK_SEMICOLON);
            return aug;
        }

        /* expression statement — rebuild full precedence from the already-consumed postfix */
        AstNode *expr = lhs;
        /* multiplicative (higher precedence first) */
        while (check(p, TOK_STAR) || check(p, TOK_SLASH)) {
            Token op = p->current; advance(p);
            AstNode *bin = ast_new(ND_BINARY, op.line);
            ast_set_text(bin, op.start, op.length);
            ast_add_child(bin, expr);
            ast_add_child(bin, parse_unary(p));
            expr = bin;
        }
        /* additive — right operand via parse_multiplicative for correct precedence */
        while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
            Token op = p->current; advance(p);
            AstNode *bin = ast_new(ND_BINARY, op.line);
            ast_set_text(bin, op.start, op.length);
            ast_add_child(bin, expr);
            ast_add_child(bin, parse_multiplicative(p));
            expr = bin;
        }
        if (check(p, TOK_EQ_EQ) || check(p, TOK_BANG_EQ) ||
            check(p, TOK_LT_EQ) || check(p, TOK_GT_EQ)   ||
            check(p, TOK_LT)    || check(p, TOK_GT)) {
            Token op = p->current; advance(p);
            AstNode *cmp = ast_new(ND_BINARY, op.line);
            ast_set_text(cmp, op.start, op.length);
            ast_add_child(cmp, expr);
            ast_add_child(cmp, parse_additive(p));
            expr = cmp;
        }

        AstNode *stmt = ast_new(ND_EXPR_STMT, expr->line);
        ast_add_child(stmt, expr);
        expect(p, TOK_SEMICOLON);
        return stmt;
    }
}

/* ================================================================
   Public API
   ================================================================ */

void parser_init(Parser *p, Lexer *l) {
    p->lexer = l;
    advance(p); /* prime the lookahead */
}

AstNode *parse_program(Parser *p) {
    AstNode *prog = ast_new(ND_PROGRAM, 1);
    while (!check(p, TOK_EOF)) {
        AstNode *node = parse_top_level(p);
        if (node) ast_add_child(prog, node);
        else {
            /* stuck — skip one token to avoid infinite loop */
            if (!check(p, TOK_EOF)) advance(p);
        }
    }
    return prog;
}

```

### File: D2/src/value.c
```c
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

```

### File: D2/tests/check-invalid/empty_list.ans
```text
let values = [];

```

### File: D2/tests/check-invalid/formula_index_assignment.ans
```text
element H { atomic_mass = 1.008, valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };

compound water { formula = H2O, name = "Water" };

let formula = water.formula;
for (let element in formula) {
    formula[element] = 2;
};

```

### File: D2/tests/check-invalid/function_call_assignment.ans
```text
function value() {
    return 1;
};

value() = 2;

```

### File: D2/tests/check-invalid/heterogeneous_list.ans
```text
element H { atomic_mass = 1.008, valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };
compound water { formula = H2O, name = "Water" };

let mixed = [1, "one", water, H2O];

```

### File: D2/tests/check-invalid/list_index_bad_member.ans
```text
let numbers = [1, 2];
let bad = numbers[0].atomic_mass;

```

### File: D2/tests/check-invalid/mol_bad_argument.ans
```text
let bad = mol("not a quantity");

```

### File: D2/tests/check-invalid/quantity_comparison_mismatch.ans
```text
let amount = 5.0 mol;
let wrong = 10.0 g/mol;
let eq = amount == wrong;

```

### File: D2/tests/check-invalid/quantity_list_mismatch.ans
```text
let mixed = [5.0 mol, 10.0 g/mol, 2.0 mol];

```

### File: D2/tests/check-invalid/quantity_unit_mismatch.ans
```text
let amount = 5.0 mol;
let wrong = 10.0 g/mol;
amount = wrong;

```

### File: D2/tests/check-invalid/reaction_formula_without_compound.ans
```text
element H { atomic_mass = 1.008, valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };

reaction water_synthesis {
    reactants = 2*H2 + O2,
    products  = 2*H2O,
    check     = balanced,
    energy    = exothermic 571.6 kJ
};

```

### File: D2/tests/check-invalid/record_field_assignment.ans
```text
element H { atomic_mass = 1.008, valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };

compound water { formula = H2O, name = "Water" };

water.formula = H2O;

```

### File: D2/tests/check-valid/list_for_compound_parameter.ans
```text
element H { atomic_mass = 1.008, valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };
element C { atomic_mass = 12.011, valence = 4, symbol = "C" };

compound water   { formula = H2O, name = "Water" };
compound methane { formula = CH4, name = "Methane" };

function molar_mass(compound c) {
    let formula = c.formula;
    let mass = 0.0;
    for (let element in formula) {
        mass += element.atomic_mass * formula[element];
    }
    return mass;
};

let compounds = [water, methane];
for (let c in compounds) {
    molar_mass(c);
};

```

### File: D2/tests/check-valid/list_index_compound_member.ans
```text
element H { atomic_mass = 1.008, valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };

compound water { formula = H2O, name = "Water" };

let compounds = [water];
let formula = compounds[0].formula;
let name = compounds[0].name;

```

### File: D2/tests/check-valid/list_index_number_assignment.ans
```text
let numbers = [1, 2, 3];
numbers[0] = 10;
numbers[1] += 5;

```

### File: D2/tests/check-valid/quantity_same_unit.ans
```text
let amount1 = 5.0 mol;
let amount2 = 10.0 mol;
let list = [amount1, amount2];
let eq = amount1 == amount2;
let lt = amount1 < amount2;

```

### File: D2/tests/invalid/bad_element_field.ans
```text
// Unknown field name inside element body
element H {
    atomic_mass = 1.008,
    valence = 1,
    symbol = "H",
    protons = 1
};

```

### File: D2/tests/invalid/missing_semicolon.ans
```text
// Missing semicolon after variable declaration
element H {
    atomic_mass = 1.008,
    valence = 1,
    symbol = "H"
}
// ^ missing ';' after the closing brace
let x = 5.0;

```

### File: D2/tests/invalid/unknown_token.ans
```text
// Illegal character in source
let x = 5.0;
let y = x @ 2.0;

```

### File: D2/tests/invalid/unmatched_brace.ans
```text
// Unmatched opening brace — block never closed
function broken() {
    let x = 1.0;
    if (x > 0) {
        print "positive";
    // missing closing brace for function body

```

### File: D2/tests/valid/compounds.ans
```text
// Compound declarations
element H { atomic_mass = 1.008, valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };
element C { atomic_mass = 12.011, valence = 4, symbol = "C" };
element N { atomic_mass = 14.007, valence = 3, symbol = "N" };

compound water    { formula = H2O,  name = "Water" };
compound peroxide { formula = H2O2, name = "Hydrogen peroxide" };
compound co2      { formula = CO2,  name = "Carbon dioxide" };
compound ammonia  { formula = NH3,  name = "Ammonia" };

```

### File: D2/tests/valid/control_flow.ans
```text
// Control flow — for loop, if/elif/else
element H { atomic_mass = 1.008, valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };
element C { atomic_mass = 12.011, valence = 4, symbol = "C" };

compound water   { formula = H2O, name = "Water" };
compound methane { formula = CH4, name = "Methane" };
compound co2     { formula = CO2, name = "Carbon dioxide" };
compound oxygen  { formula = O2,  name = "Oxygen" };

reaction combustion {
    reactants = CH4 + 2*O2,
    products  = CO2 + 2*H2O,
    check     = balanced,
    energy    = exothermic 890.3 kJ/mol
};

function molar_mass(compound c) {
    let formula = c.formula;
    let mass = 0.0;
    for (let element in formula) {
        mass += element.atomic_mass * formula[element];
    }
    return mass;
};

function compute_yield(reaction r, compound c) {
    let products = r.products;
    let reactants = r.reactants;
    let reactant_mol = 1.0 / mol(reactants[c]);
    let product_mol = reactant_mol * mol(products[c]);
    return product_mol;
};

let mass = molar_mass(water);

if (mass > 18.0) {
    print "Molar mass is above 18.";
}
elif (mass == 18.0) {
    print "Molar mass is exactly 18.";
}
else {
    print "Molar mass is below 18.";
}

// Nested if inside for
let compounds = [water, methane, co2];
for (let c in compounds) {
    if (molar_mass(c) > 30.0) {
        print "Heavy compound.";
    }
    else {
        print "Light compound.";
    }
};

```

### File: D2/tests/valid/elements.ans
```text
// Element declarations only
element H {
    atomic_mass = 1.008,
    valence = 1,
    symbol = "H"
};
element O {
    atomic_mass = 15.999,
    valence = 2,
    symbol = "O"
};
element C {
    atomic_mass = 12.011,
    valence = 4,
    symbol = "C"
};
element N {
    atomic_mass = 14.007,
    valence = 3,
    symbol = "N"
};

```

### File: D2/tests/valid/expressions.ans
```text
// Expression precedence, postfix chains, quantity literals
element H { atomic_mass = 1.008, valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };

compound water { formula = H2O, name = "Water" };

// Arithmetic precedence
let a = 1.0 + 2.0 * 3.0;       // 7.0
let b = (1.0 + 2.0) * 3.0;     // 9.0
let c = 10.0 / 2.0 - 1.0;      // 4.0
let d = -3.14 * 2.0;            // negative unary

// Comparison
let eq = 1.0 == 1.0;
let ne = 2.0 != 3.0;
let lt = 1.0 < 2.0;
let le = 1.0 <= 1.0;
let gt = 5.0 > 3.0;
let ge = 4.0 >= 4.0;

// Postfix chains
let sym = water.name;
let fm  = water.formula;

// Quantity literals
let mass_g    = 18.015 g/mol;
let mass_kg   = 0.018  kg/mol;
let energy_kj = 890.3  kJ/mol;
let amount    = 5.0    mol;
let grams     = 100.0  g;
let kilograms = 1.0    kg;
let kilojoules = 285.0 kJ;

// Scientific notation
let avogadro = 6.02e23;
let tiny     = 1.0e-10;

// Augmented assignment
let x = 10.0;
x += 5.0;
x -= 2.0;
x *= 3.0;
x /= 2.0;

```

### File: D2/tests/valid/functions.ans
```text
// Function declarations — typed and untyped parameters, return
element H { atomic_mass = 1.008, valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };
element C { atomic_mass = 12.011, valence = 4, symbol = "C" };

compound water   { formula = H2O,  name = "Water" };
compound methane { formula = CH4,  name = "Methane" };
compound co2     { formula = CO2,  name = "Carbon dioxide" };
compound oxygen  { formula = O2,   name = "Oxygen" };

reaction combustion {
    reactants = CH4 + 2*O2,
    products  = CO2 + 2*H2O,
    check     = balanced,
    energy    = exothermic 890.3 kJ/mol
};

// Typed parameters
function molar_mass(compound c) {
    let formula = c.formula;
    let mass = 0.0;
    for (let element in formula) {
        mass += element.atomic_mass * formula[element];
    }
    return mass;
};

function compute_yield(reaction r, compound c) {
    let products = r.products;
    let reactants = r.reactants;
    let reactant_mol = 1.0 / mol(reactants[c]);
    let product_mol = reactant_mol * mol(products[c]);
    return product_mol;
};

// Untyped parameter
function double_val(x) {
    return x * 2.0;
};

// No parameters
function get_avogadro() {
    return 6.02e23;
};

molar_mass(water);
compute_yield(combustion, CH4);

```

### File: D2/tests/valid/lists.ans
```text
// Parser-only list syntax coverage. The semantic checker rejects mixed and
// empty lists in tests/check-invalid/.
element H { atomic_mass = 1.008, valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };
element C { atomic_mass = 12.011, valence = 4, symbol = "C" };

compound water   { formula = H2O, name = "Water" };
compound methane { formula = CH4, name = "Methane" };
compound co2     { formula = CO2, name = "Carbon dioxide" };

// Mixed-type list
let list = [1, "two", 3.45, 4, 5, water, methane.formula, co2.name, H];

// List of elements
let elements = [H, O, C];

// List of compounds
let compounds = [water, methane, co2];

// Empty list
let empty = [];

// Nested access
let first_name = compounds[0].name;

```

### File: D2/tests/valid/main.ans
```text
// Declare elements
element H {
    atomic_mass = 1.008,
    valence = 1,
    symbol = "H"
};
element O {
    atomic_mass = 15.999,
    valence = 2,
    symbol = "O"
};
element C {
    atomic_mass = 12.011,
    valence = 4,
    symbol = "C"
};

// Declare compounds
compound water {
    formula = H2O,
    name = "Water"
};
compound methane {
    formula = CH4,
    name = "Methane"
};
compound co2 {
    formula = CO2,
    name = "Carbon dioxide"
};
compound oxygen {
    formula = O2,
    name = "Oxygen"
};

// Declare variables
let num = 20;
let name = "Hydrogen";
let char = 'H';
let elements = [H, O, C];
let n = 21.2;
let list = [1, "two", 3.45, 4, 5, water, methane.formula, co2.name, H];

// Function declaration for computing yield of a compound in a reaction
function compute_yield(reaction r, compound c) {
    let products = r.products;
    let reactants = r.reactants;
    let reactant_mol = 1.0 / mol(reactants[c]);
    let product_mol = reactant_mol * mol(products[c]);
    return product_mol;
};

// Function declaration for computing molar mass of a compound
function molar_mass(compound c) {
    let formula = c.formula;
    let mass = 0.0;
    for (let element in formula) {
        mass += element.atomic_mass * formula[element];
    }
    return mass;
};

// Declare a reaction and verify it is balanced at compile time
reaction combustion {
    reactants = CH4 + 2*O2,
    products = CO2 + 2*H2O,
    check = balanced,
    energy = exothermic 890.3 kJ/mol
};

reaction combustion2 {
    reactants = methane + 2*oxygen,
    products = co2 + 2*water,
    check = balanced,
    energy = exothermic 890.3 kJ/mol
};

// Stoichiometric query
molar_mass(water);
compute_yield(combustion, CH4);

// if statements
if (compute_yield(combustion, 5.0 mol, CH4) > 0) {
    print "The yield of CH4 is positive, so combustion is possible.";
}
elif (compute_yield(combustion, 5.0 mol, CH4) == 0) {
    print "The yield of CH4 is zero, so combustion is not possible.";
}
else {
    print "The yield of CH4 is negative, so combustion is not possible.";
}

```

### File: D2/tests/valid/reactions.ans
```text
// Reaction declarations — formula-literal form and identifier form
element H { atomic_mass = 1.008, valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };
element C { atomic_mass = 12.011, valence = 4, symbol = "C" };

compound methane  { formula = CH4, name = "Methane" };
compound oxygen   { formula = O2,  name = "Oxygen" };
compound co2      { formula = CO2, name = "Carbon dioxide" };
compound water    { formula = H2O, name = "Water" };

// Formula-literal species
reaction combustion {
    reactants = CH4 + 2*O2,
    products  = CO2 + 2*H2O,
    check     = balanced,
    energy    = exothermic 890.3 kJ/mol
};

// Identifier species
reaction combustion2 {
    reactants = methane + 2*oxygen,
    products  = co2 + 2*water,
    check     = balanced,
    energy    = exothermic 890.3 kJ/mol
};

// Endothermic example
reaction photosynthesis {
    reactants = CO2 + H2O,
    products  = O2,
    check     = balanced,
    energy    = endothermic 2800.0 kJ/mol
};

```

### File: D3/P1/220104004926-220104004931_D3_P1.tex
```tex
% =====================================================================
% CSE 341 -- Concepts of Programming Languages
% Project D3 (Part 1) -- Example Programs & Test Report for ANS
% Students: 220104004926 & 220104004931
% Compile with: tectonic 220104004926-220104004931_D3_P1.tex
% =====================================================================
\documentclass[11pt,a4paper]{article}

% ---------- Encoding, fonts, language ----------
\usepackage[utf8]{inputenc}
\usepackage[T1]{fontenc}
\usepackage{mathpazo}                    % Palatino serif, matching D1
\usepackage[scaled=0.92]{helvet}         % Helvetica sans
\usepackage[varqu,varl]{zi4}             % Inconsolata monospace
\usepackage[english]{babel}
\usepackage{microtype}

% ---------- Page geometry ----------
\usepackage[a4paper,margin=2.25cm]{geometry}

% ---------- Colour, graphics, hyperlinks ----------
\usepackage[table]{xcolor}
\definecolor{ansblue}{HTML}{1E3A8A}
\definecolor{anscodebg}{HTML}{F9FAFB}
\definecolor{ansborder}{HTML}{E5E7EB}
\definecolor{anscaption}{HTML}{6B7280}
\definecolor{anskeyword}{HTML}{2563EB}
\definecolor{anssuccess}{HTML}{047857}
\definecolor{ansdanger}{HTML}{B91C1C}
\usepackage[colorlinks=true, linkcolor=ansblue, urlcolor=ansblue,
            citecolor=ansblue, pdfborder={0 0 0}]{hyperref}

% ---------- Typography ----------
\usepackage[explicit]{titlesec}
\titleformat{\section}
  {\Large\bfseries\sffamily\color{black}}
  {\thesection}{0.8em}{#1}
\titleformat{\subsection}
  {\normalsize\bfseries\sffamily\color{black}}
  {\thesubsection}{0.8em}{#1}
\titlespacing*{\section}{0pt}{16pt}{7pt}
\titlespacing*{\subsection}{0pt}{10pt}{4pt}
\setlength{\parskip}{5pt plus 2pt minus 1pt}
\setlength{\parindent}{0pt}
\linespread{1.08}
\setcounter{secnumdepth}{0}

% ---------- Tables ----------
\usepackage{array}
\usepackage{tabularx}
\usepackage{booktabs}
\usepackage{ragged2e}
\usepackage{makecell}
\newcolumntype{Y}{>{\RaggedRight\arraybackslash}X}
\newcolumntype{L}[1]{>{\RaggedRight\arraybackslash}p{#1}}
\renewcommand{\arraystretch}{1.18}

% ---------- Lists ----------
\usepackage{enumitem}
\setlist[itemize]{leftmargin=*, itemsep=2pt, topsep=2pt}

% ---------- Code listings ----------
\usepackage{upquote}
\usepackage{listings}
\lstdefinelanguage{ANS}{
  morekeywords={let,function,return,for,in,if,elif,else,print,
                element,compound,reaction,reactants,products,
                formula,name,atomic_mass,valence,symbol,
                check,balanced,energy,exothermic,endothermic,
                molar_mass,compute_yield,mol},
  sensitive=true,
  morecomment=[l]{//},
  morestring=[b]",
  morestring=[b]'
}

\lstdefinestyle{ans}{
  language=ANS,
  basicstyle=\ttfamily\small,
  keywordstyle=\bfseries\color{anskeyword},
  commentstyle=\itshape\color{anscaption},
  stringstyle=\color{anskeyword!80!black},
  numbers=none,
  frame=none,
  backgroundcolor=\color{anscodebg},
  framesep=6pt,
  xleftmargin=6pt,
  xrightmargin=6pt,
  aboveskip=7pt,
  belowskip=7pt,
  showstringspaces=false,
  columns=fullflexible,
  keepspaces=true,
  breaklines=true,
  breakatwhitespace=false,
  upquote=true
}

\lstdefinestyle{terminal}{
  language={},
  basicstyle=\ttfamily\scriptsize,
  keywordstyle={},
  commentstyle={},
  stringstyle={},
  numbers=none,
  frame=none,
  backgroundcolor=\color{anscodebg},
  framesep=6pt,
  xleftmargin=6pt,
  xrightmargin=6pt,
  aboveskip=7pt,
  belowskip=7pt,
  showstringspaces=false,
  columns=fullflexible,
  keepspaces=true,
  breaklines=true,
  breakatwhitespace=false,
  upquote=true
}
\lstset{style=ans}

% ---------- Header / footer ----------
\usepackage{fancyhdr}
\pagestyle{fancy}
\fancyhf{}
\renewcommand{\headrulewidth}{0pt}
\fancyfoot[L]{\footnotesize\color{anscaption}%
  CSE 341 --- ANS Test Report (D3, Part 1) $\cdot$
  Students 220104004926 \& 220104004931}
\fancyfoot[R]{\footnotesize\color{anscaption}Page \thepage}

% ---------- PDF metadata ----------
\hypersetup{
  pdftitle  = {ANS -- Example Programs and Test Report (D3, Part 1)},
  pdfauthor = {220104004926 and 220104004931},
  pdfsubject= {CSE 341 Project Part 1},
  pdfkeywords={DSL, chemistry, lexer, parser, tests, ANS}
}

% ---------- Helpers ----------
\newcommand{\code}[1]{\texttt{#1}}
\newcommand{\file}[1]{{\footnotesize\path{#1}}}
\newcommand{\statusok}{\textcolor{anssuccess}{\textbf{Accepted}}}
\newcommand{\statuserr}{\textcolor{ansdanger}{\textbf{Rejected}}}
\newcommand{\sectionrule}{\vspace{-3pt}\textcolor{ansborder}{\rule{\textwidth}{0.6pt}}\vspace{2pt}}

\begin{document}

% ---------------- Title block ----------------
\begin{center}
  {\footnotesize\color{anscaption}
    CSE 341 --- Concepts of Programming Languages\par}
  \vspace{4pt}
  {\Huge\bfseries\sffamily
    ANS: Alchemist's Notation Script\par}
  \vspace{6pt}
  {\Large Example Programs \& Test Report (D3) --- Part 1\par}
  \vspace{8pt}
  {\footnotesize\color{anscaption}
    Students: 220104004926 \& 220104004931 \quad$\cdot$\quad
    Spring 2026 \quad$\cdot$\quad
    Gebze Technical University\par}
\end{center}
\vspace{8pt}

\section{Purpose and Parser Under Test}
\sectionrule

This report documents the Part 1 parser tests for ANS. The parser under test is the C implementation in \code{D2/}, run as \code{D2/ans <file>}. A valid program must parse with exit code \code{0}; a malformed program must be rejected with a useful diagnostic containing the file name, line number, column, and message.

\begin{table}[h]
\centering
\rowcolors{2}{anscodebg}{white}
\begin{tabularx}{\textwidth}{@{} L{3.1cm} Y @{}}
\toprule
\rowcolor{ansblue}
\textcolor{white}{\textbf{Evidence item}} & \textcolor{white}{\textbf{D3 coverage}} \\
\midrule
Valid programs & Three non-trivial ANS programs in \code{D3/programs/}; all accepted by \code{D2/ans}. \\
Malformed programs & Five negative parser tests in \code{D3/malformed/}; all rejected with line-numbered diagnostics. \\
Domain coverage & Elements, compounds, reactions, formula literals, energy quantities, typed functions, control flow, lists, calls, member access, and index access. \\
\bottomrule
\end{tabularx}
\end{table}

% =====================================================================
\section{Three Valid Programs}
\sectionrule

The full valid source files are submitted in \code{D3/programs/}. The parser accepts all three programs silently, which is the expected success behavior for \code{D2/ans <file>}.

\begin{table}[h]
\centering
\rowcolors{2}{anscodebg}{white}
\begin{tabularx}{\textwidth}{@{} L{3.35cm} L{1.45cm} Y L{2.0cm} @{}}
\toprule
\rowcolor{ansblue}
\textcolor{white}{\textbf{Program}} &
\textcolor{white}{\textbf{Size}} &
\textcolor{white}{\textbf{Language features exercised}} &
\textcolor{white}{\textbf{Result}} \\
\midrule
\file{molar_mass_demo.ans} &
40 lines &
\code{element} and \code{compound} declarations, typed functions, list literals, \code{for}, \code{if}/\code{elif}/\code{else}, member access, index access, and function calls. &
\statusok, exit \code{0} \\
\file{combustion_analysis.ans} &
52 lines &
\code{reaction} declaration, formula literals, \code{check = balanced}, exothermic energy quantity, typed \code{reaction}/\code{compound} parameters, nested calls, and augmented assignment. &
\statusok, exit \code{0} \\
\file{multi_reaction_survey.ans} &
70 lines &
Multiple reactions, formula-literal and identifier species, compound lists, loop counters, relational expressions, character and string literals, and repeated function calls. &
\statusok, exit \code{0} \\
\bottomrule
\end{tabularx}
\end{table}

\textbf{Verification run.}
\begin{lstlisting}[style=terminal]
D2/ans D3/programs/combustion_analysis.ans     OK
D2/ans D3/programs/molar_mass_demo.ans         OK
D2/ans D3/programs/multi_reaction_survey.ans   OK
\end{lstlisting}

Together, these examples cover the required DSL surface: primitive literals (\code{number}, \code{string}, \code{char}, contextual words), structured chemistry declarations (\code{element}, \code{compound}, \code{reaction}), functions with parameters, expression precedence, control flow, list traversal, and the chemistry-specific reaction syntax.

% =====================================================================
\section{Five Malformed Programs}
\sectionrule

The full malformed source files are submitted in \code{D3/malformed/}. Each malformed file is intentionally small so the reported line number points directly to the syntax or lexical problem.

\begin{table}[h]
\centering
\rowcolors{2}{anscodebg}{white}
\begin{tabularx}{\textwidth}{@{} L{3.85cm} Y Y L{1.9cm} @{}}
\toprule
\rowcolor{ansblue}
\textcolor{white}{\textbf{Malformed file}} &
\textcolor{white}{\textbf{Defect}} &
\textcolor{white}{\textbf{Diagnostic behavior}} &
\textcolor{white}{\textbf{Result}} \\
\midrule
\file{err1_missing_semicolon.ans} &
Missing \code{;} after an \code{element} declaration. &
Reports token \code{let} at line 7 where the semicolon was required. &
\statuserr \\
\file{err2_unknown_field.ans} &
Invalid field name \code{protons} inside an \code{element} body. &
Reports the unknown field at line 5, then a recovery error at the following \code{=}. &
\statuserr \\
\file{err3_bad_identifier.ans} &
Illegal character \code{@} in an expression. &
Lexer reports the illegal character with line and column; parser then reports the missing semicolon. &
\statuserr \\
\file{err4_unmatched_brace.ans} &
Missing closing brace for a \code{for} block inside a function. &
Reports a syntax error near the premature \code{;}, followed by EOF recovery diagnostics. &
\statuserr \\
\file{err5_unterminated_string.ans} &
String literal reaches a newline before a closing quote. &
Lexer reports the unterminated string at line 5; parser recovery reports the broken compound body. &
\statuserr \\
\bottomrule
\end{tabularx}
\end{table}

\newpage
\textbf{Actual parser output.}
\begin{lstlisting}[style=terminal]
=== D3/malformed/err1_missing_semicolon.ans ===
D3/malformed/err1_missing_semicolon.ans:7:1: error: expected ';' but got let

=== D3/malformed/err2_unknown_field.ans ===
D3/malformed/err2_unknown_field.ans:5:5: error: unknown element field 'protons'
D3/malformed/err2_unknown_field.ans:5:13: error: expected '}' but got =

=== D3/malformed/err3_bad_identifier.ans ===
D3/malformed/err3_bad_identifier.ans:3:18: error: unexpected character '@'
D3/malformed/err3_bad_identifier.ans:3:18: error: expected ';' but got @

=== D3/malformed/err4_unmatched_brace.ans ===
D3/malformed/err4_unmatched_brace.ans:8:2: error: unexpected token ; in expression
D3/malformed/err4_unmatched_brace.ans:9:1: error: expected ';' but got
D3/malformed/err4_unmatched_brace.ans:9:1: error: expected '}' but got
D3/malformed/err4_unmatched_brace.ans:9:1: error: expected ';' but got

=== D3/malformed/err5_unterminated_string.ans ===
D3/malformed/err5_unterminated_string.ans:5:13: error: unterminated string literal
D3/malformed/err5_unterminated_string.ans:5:13: error: expected STRING but got "
D3/malformed/err5_unterminated_string.ans:6:2: error: expected '}' but got ;
D3/malformed/err5_unterminated_string.ans:7:1: error: expected ';' but got
\end{lstlisting}

\section{Conclusion}
\sectionrule

The valid examples demonstrate that the Part 1 parser accepts non-trivial ANS programs using declarations, expressions, control structures, functions, and the chemistry-specific reaction construct. The malformed examples show that lexical errors, missing separators, invalid declaration fields, unmatched braces, and unterminated strings are rejected with line-numbered, actionable diagnostics.

\end{document}

```

### File: D3/P2/220104004926-220104004931_D3_P2.tex
```tex
% =====================================================================
% CSE 341 -- Concepts of Programming Languages
% Project D3 (Part 2) -- Interpreter & Type-Checker Test Report for ANS
% Students: 220104004926 & 220104004931
% Compile with: pdflatex 220104004926-220104004931_D3_P2.tex
% =====================================================================
\documentclass[11pt,a4paper]{article}

% ---------- Encoding, fonts, language ----------
\usepackage[utf8]{inputenc}
\usepackage[T1]{fontenc}
\usepackage{lmodern}                     % Latin Modern (T1-complete; Palatino/Inconsolata unavailable)
\usepackage[english]{babel}
\usepackage{microtype}

% ---------- Page geometry ----------
\usepackage[a4paper,margin=2.25cm]{geometry}

% ---------- Colour, graphics, hyperlinks ----------
\usepackage[table]{xcolor}
\definecolor{ansblue}{HTML}{1E3A8A}
\definecolor{anscodebg}{HTML}{F9FAFB}
\definecolor{ansborder}{HTML}{E5E7EB}
\definecolor{anscaption}{HTML}{6B7280}
\definecolor{anskeyword}{HTML}{2563EB}
\definecolor{anssuccess}{HTML}{047857}
\definecolor{ansdanger}{HTML}{B91C1C}
\usepackage[colorlinks=true, linkcolor=ansblue, urlcolor=ansblue,
            citecolor=ansblue, pdfborder={0 0 0}]{hyperref}

% ---------- Typography ----------
\usepackage[explicit]{titlesec}
\titleformat{\section}
  {\Large\bfseries\sffamily\color{black}}
  {\thesection}{0.8em}{#1}
\titleformat{\subsection}
  {\normalsize\bfseries\sffamily\color{black}}
  {\thesubsection}{0.8em}{#1}
\titlespacing*{\section}{0pt}{16pt}{7pt}
\titlespacing*{\subsection}{0pt}{10pt}{4pt}
\setlength{\parskip}{5pt plus 2pt minus 1pt}
\setlength{\parindent}{0pt}
\linespread{1.08}
\setcounter{secnumdepth}{0}

% ---------- Tables ----------
\usepackage{array}
\usepackage{tabularx}
\usepackage{booktabs}
\usepackage{ragged2e}
\newcolumntype{Y}{>{\RaggedRight\arraybackslash}X}
\newcolumntype{L}[1]{>{\RaggedRight\arraybackslash}p{#1}}
\renewcommand{\arraystretch}{1.18}
\setlength{\emergencystretch}{3em}

% ---------- Lists ----------
\usepackage{enumitem}
\setlist[itemize]{leftmargin=*, itemsep=2pt, topsep=2pt}

% ---------- Code listings ----------
\usepackage{upquote}
\usepackage{listings}

\lstdefinestyle{terminal}{
  language={},
  basicstyle=\ttfamily\scriptsize,
  keywordstyle={},
  commentstyle={},
  stringstyle={},
  numbers=none,
  frame=none,
  backgroundcolor=\color{anscodebg},
  framesep=6pt,
  xleftmargin=6pt,
  xrightmargin=6pt,
  aboveskip=7pt,
  belowskip=7pt,
  showstringspaces=false,
  columns=fullflexible,
  keepspaces=true,
  breaklines=true,
  breakatwhitespace=false,
  upquote=true
}

% ---------- Header / footer ----------
\usepackage{fancyhdr}
\pagestyle{fancy}
\fancyhf{}
\renewcommand{\headrulewidth}{0pt}
\fancyfoot[L]{\footnotesize\color{anscaption}%
  CSE 341 --- ANS Interpreter \& Type-Checker Report (D3, Part 2) $\cdot$
  Students 220104004926 \& 220104004931}
\fancyfoot[R]{\footnotesize\color{anscaption}Page \thepage}

% ---------- PDF metadata ----------
\hypersetup{
  pdftitle  = {ANS -- Interpreter and Type-Checker Test Report (D3, Part 2)},
  pdfauthor = {220104004926 and 220104004931},
  pdfsubject= {CSE 341 Project D3 Part 2},
  pdfkeywords={DSL, chemistry, interpreter, type-checker, ANS}
}

% ---------- Helpers ----------
\newcommand{\code}[1]{\texttt{#1}}
\newcommand{\file}[1]{{\footnotesize\texttt{#1}}}
\newcommand{\statusok}{\textcolor{anssuccess}{\textbf{Pass}}}
\newcommand{\statuserr}{\textcolor{ansdanger}{\textbf{Rejected}}}
\newcommand{\sectionrule}{\vspace{-3pt}\textcolor{ansborder}{\rule{\textwidth}{0.6pt}}\vspace{2pt}}

\begin{document}

% =====================================================================
% Title
% =====================================================================
\begin{center}
  {\footnotesize\color{anscaption}
    CSE 341 --- Concepts of Programming Languages\par}
  \vspace{4pt}
  {\Huge\bfseries\sffamily
    ANS: Alchemist's Notation Script\par}
  \vspace{6pt}
  {\Large Interpreter \& Type-Checker Test Report (D3) --- Part 2\par}
  \vspace{8pt}
  {\footnotesize\color{anscaption}
    Students: 220104004926 \& 220104004931 \quad$\cdot$\quad
    Spring 2026 \quad$\cdot$\quad
    Gebze Technical University\par}
\end{center}
\vspace{8pt}

% =====================================================================
\section{Purpose and Implementation Under Test}
\sectionrule

This report documents D3 Part 2: end-to-end execution of the three ANS example
programs, type-checker validation of semantically invalid programs, and runtime
error handling. The implementation under test is the C11 type-checker and
tree-walking interpreter added in D2 Part 2, invoked via two new flags:
\code{--check} (type-check only, exit 0/1) and \code{--run} (type-check then
interpret, exit 0/1). All tests were run against \code{D2/ans} built from the
submitted sources.

\begin{table}[h]
\vspace{2pt}
\centering\small
\rowcolors{2}{anscodebg}{white}
\begin{tabularx}{\textwidth}{@{} L{3.2cm} Y @{}}
\toprule
\rowcolor{ansblue}
\textcolor{white}{\textbf{Evidence category}} &
\textcolor{white}{\textbf{Coverage}} \\
\midrule
Valid programs (end-to-end) &
  3 programs in \code{D3/programs/} execute to completion with correct
  \code{stdout} output; exit 0. \\
Semantic error detection &
  4 negative tests in \code{D3/semantic/}; each rejected with a precise,
  line-numbered diagnostic; exit 1. \\
Runtime error handling &
  2 runtime-error tests in \code{D3/semantic/}; each aborts with an
  informative message; exit 1. \\
Parser error evidence (D3 P1) &
  5 malformed programs in \code{D3/malformed/} rejected by the parser;
  summarised here for completeness. \\
\bottomrule
\end{tabularx}
\end{table}

% =====================================================================
\section{End-to-End Execution of Three Valid Programs}
\sectionrule

All programs are run as \code{D2/ans --run <file>}. The type checker executes
first; the interpreter runs only if no semantic errors are found.

% ----- Program 1 -----
\subsection{Program 1 \textemdash{} Molar Mass Classifier
  (\file{D3/programs/molar\_mass\_demo.ans})}

\textbf{Features exercised:} \code{element} and \code{compound} declarations;
typed function parameter (\code{compound c}); \code{for}-in over a Formula
(§4.4.1, yields element objects in first-appearance order); member access and
formula indexing; augmented assignment; \code{if}/\code{elif}/\code{else};
\code{for}-in over a \code{List<Compound>}.

\smallskip
\textbf{Trace.} \code{molar\_mass} accumulates
$\textstyle\sum(\textit{atomic\_mass}\times\textit{count})$ per formula element:

\begin{table}[h]
\vspace{2pt}
\centering\small
\rowcolors{2}{anscodebg}{white}
\begin{tabularx}{\textwidth}{@{} L{2.0cm} L{1.8cm} L{5.2cm} L{1.6cm} L{1.8cm} @{}}
\toprule
\rowcolor{ansblue}
\textcolor{white}{\textbf{Compound}} &
\textcolor{white}{\textbf{Formula}} &
\textcolor{white}{\textbf{Calculation}} &
\textcolor{white}{\textbf{g/mol}} &
\textcolor{white}{\textbf{Class}} \\
\midrule
\code{water}   & H\textsubscript{2}O  & $2{\times}1.008 + 1{\times}15.999$ & 18.015 & Light \\
\code{methane} & CH\textsubscript{4}  & $1{\times}12.011 + 4{\times}1.008$ & 16.043 & Light \\
\code{co2}     & CO\textsubscript{2}  & $1{\times}12.011 + 2{\times}15.999$ & 44.009 & Medium \\
\code{ammonia} & NH\textsubscript{3}  & $1{\times}14.007 + 3{\times}1.008$ & 17.031 & Light \\
\bottomrule
\end{tabularx}
\end{table}

\begin{lstlisting}[style=terminal]
$ D2/ans --run D3/programs/molar_mass_demo.ans
Light molecule.
Light molecule.
Medium molecule.
Light molecule.
\end{lstlisting}
Result: \statusok, exit \code{0}.

% ----- Program 2 -----
\subsection{Program 2 \textemdash{} Combustion Reaction Analysis
  (\file{D3/programs/combustion\_analysis.ans})}

\textbf{Features exercised:} \code{reaction} declaration with
\code{check = balanced} and exothermic energy quantity; typed parameters
\code{reaction r} and \code{compound c}; \code{.reactants}/\code{.products}
member access; ReactionSide indexing (\code{reactants[c]} returns the
stoichiometric coefficient); built-in \code{mol()}; arithmetic; \code{if}/\code{else}.

\smallskip
\textbf{Trace of \code{compute\_yield(combustion, methane)}.}
\code{reactants[methane]} $\Rightarrow$ \textbf{1.0} (CH\textsubscript{4}
coefficient in CH\textsubscript{4}\,+\,2\,O\textsubscript{2}).
\code{products[methane]} $\Rightarrow$ \textbf{0.0} (CH\textsubscript{4} not
in CO\textsubscript{2}\,+\,2\,H\textsubscript{2}O).
\code{product\_mol} $= 1.0 \times 0.0 = \mathbf{0.0}$.
Condition \code{yield\_val > 0.0} is false $\Rightarrow$ \textit{No product formed.}
(Chemically correct: methane is a reactant, not a product, of its own combustion.)
Reaction balance also verified at interpretation time:
both sides $\{C{:}1, H{:}4, O{:}4\}$.

\begin{lstlisting}[style=terminal]
$ D2/ans --run D3/programs/combustion_analysis.ans
No product formed.
\end{lstlisting}
Result: \statusok, exit \code{0}.

% ----- Program 3 -----
\subsection{Program 3 \textemdash{} Multi-Reaction Survey
  (\file{D3/programs/multi\_reaction\_survey.ans})}

\textbf{Features exercised:} Multiple \code{reaction} declarations with
formula-literal species; \code{for}-in over a \code{List<Compound>}; augmented
assignment to a top-level variable (\code{heavy\_count}, \code{light\_count})
across loop iterations; \code{character} literal; \code{string} literal;
\code{for}-in over a ReactionSide in \code{total\_reactant\_mass} (§4.4 rule:
iterates each term's resolved compound in declaration order).

\smallskip
\textbf{Trace.} After the loop over 7 compounds:

\begin{table}[h]
\vspace{2pt}
\centering\small
\rowcolors{2}{anscodebg}{white}
\begin{tabular}{@{} l l r l @{}}
\toprule
\rowcolor{ansblue}
\textcolor{white}{\textbf{Compound}} &
\textcolor{white}{\textbf{Formula}} &
\textcolor{white}{\textbf{g/mol}} &
\textcolor{white}{\textbf{Bucket}} \\
\midrule
\code{methane}  & CH\textsubscript{4} & 16.043 & light \\
\code{oxygen}   & O\textsubscript{2}  & 31.998 & heavy \\
\code{co2}      & CO\textsubscript{2} & 44.009 & heavy \\
\code{water}    & H\textsubscript{2}O & 18.015 & light \\
\code{ammonia}  & NH\textsubscript{3} & 17.031 & light \\
\code{nitrogen} & N\textsubscript{2}  & 28.014 & light \\
\code{hydrogen} & H\textsubscript{2}  &  2.016 & light \\
\bottomrule
\end{tabular}
\end{table}

Final state: \code{heavy\_count = 2}, \code{light\_count = 5}.
\code{total\_reactant\_mass} is called twice (returns values used as expression
statements, not printed). No \code{print} statement is ever reached, so the
program exits silently --- designed to verify correct scoping and counter
persistence without producing output.

\begin{lstlisting}[style=terminal]
$ D2/ans --run D3/programs/multi_reaction_survey.ans
(no output -- exit 0)
\end{lstlisting}
Result: \statusok, exit \code{0}.

% =====================================================================
\section{Parser Error Evidence (D3 Part 1 Summary)}
\sectionrule

The five programs below were submitted with D3 Part 1 and are reproduced here
for completeness. Each is syntactically malformed; the parser rejects it before
type checking or interpretation begins.

\begin{table}[h]
\vspace{2pt}
\centering\small
\rowcolors{2}{anscodebg}{white}
\begin{tabularx}{\textwidth}{@{} Y Y L{1.5cm} @{}}
\toprule
\rowcolor{ansblue}
\textcolor{white}{\textbf{File}} &
\textcolor{white}{\textbf{First diagnostic emitted}} &
\textcolor{white}{\textbf{Result}} \\
\midrule
\file{err1\_missing\_semicolon.ans} &
\code{7:1: error: expected ';' but got let} &
\statuserr \\
\file{err2\_unknown\_field.ans} &
\code{5:5: error: unknown element field 'protons'} &
\statuserr \\
\file{err3\_bad\_identifier.ans} &
\code{3:18: error: unexpected character '@'} &
\statuserr \\
\file{err4\_unmatched\_brace.ans} &
\code{8:2: error: unexpected token ; in expression} &
\statuserr \\
\file{err5\_unterminated\_string.ans} &
\code{5:13: error: unterminated string literal} &
\statuserr \\
\bottomrule
\end{tabularx}
\end{table}

% =====================================================================
\section{Type-Checker Semantic Error Detection}
\sectionrule

The following four programs in \code{D3/semantic/} are syntactically valid
(the parser accepts them) but semantically invalid. Each is rejected by
\code{D2/ans --check} with a line-numbered diagnostic, exercising the
strong-typing, formula-validity, and reaction-balance requirements from D1 §4.5
and §4.4.

\begin{table}[h]
\vspace{2pt}
\centering\small
\rowcolors{2}{anscodebg}{white}
\begin{tabularx}{\textwidth}{@{} Y Y L{1.5cm} @{}}
\toprule
\rowcolor{ansblue}
\textcolor{white}{\textbf{File}} &
\textcolor{white}{\textbf{Defect \& diagnostic}} &
\textcolor{white}{\textbf{Result}} \\
\midrule
\file{sem1\_unbalanced\_reaction.ans} &
Reaction \code{bad\_water}: H\textsubscript{2}\,+\,O\textsubscript{2}
$\to$ H\textsubscript{2}O; reactant O count = 2, product O count = 1.
Rejected with ``reaction is not balanced'' at declaration line. &
\statuserr \\
\file{sem2\_undeclared\_element.ans} &
Formula \code{NH3} uses element symbol \code{N}, which was never declared.
Rejected with ``formula uses undeclared element'' at compound line. &
\statuserr \\
\file{sem3\_missing\_field.ans} &
\code{compound water} omits the required \code{name} field.
Rejected with ``missing name'' at compound declaration line. &
\statuserr \\
\file{sem4\_wrong\_arg\_count.ans} &
\code{molar\_mass()} invoked with 0 arguments; function declares 1 parameter.
Rejected with ``expects 1 argument(s), got 0'' at call site. &
\statuserr \\
\bottomrule
\end{tabularx}
\end{table}

\textbf{Actual checker output:}
\begin{lstlisting}[style=terminal]
$ D2/ans --check D3/semantic/sem1_unbalanced_reaction.ans
D3/semantic/sem1_unbalanced_reaction.ans:9:0: error: reaction 'bad_water' is not balanced (atom counts differ)

$ D2/ans --check D3/semantic/sem2_undeclared_element.ans
D3/semantic/sem2_undeclared_element.ans:7:0: error: formula 'NH3' uses undeclared element 'N'

$ D2/ans --check D3/semantic/sem3_missing_field.ans
D3/semantic/sem3_missing_field.ans:6:0: error: compound 'water' missing name

$ D2/ans --check D3/semantic/sem4_wrong_arg_count.ans
D3/semantic/sem4_wrong_arg_count.ans:18:0: error: function 'molar_mass' expects 1 argument(s), got 0
\end{lstlisting}

% =====================================================================
\section{Runtime Error Handling}
\sectionrule

Two programs in \code{D3/semantic/} pass the type checker but trigger fatal
runtime errors. Both abort with exit code \code{1} and a diagnostic carrying
file name and line number. A third class of runtime error (formula uses an
element not found at run time) is caught statically by the type checker for
well-formed programs; the interpreter also guards against it defensively.

\begin{table}[h]
\vspace{2pt}
\centering\small
\rowcolors{2}{anscodebg}{white}
\begin{tabularx}{\textwidth}{@{} Y Y L{1.5cm} @{}}
\toprule
\rowcolor{ansblue}
\textcolor{white}{\textbf{File}} &
\textcolor{white}{\textbf{Defect \& diagnostic}} &
\textcolor{white}{\textbf{Result}} \\
\midrule
\file{rt1\_division\_by\_zero.ans} &
\code{42.0 / denom} where \code{denom = 0.0}; detected when the
\code{/} operator evaluates. Aborts with ``division by zero'' at line 6. &
\statuserr, exit 1 \\
\file{rt2\_index\_out\_of\_range.ans} &
\code{xs[5]} where \code{xs} has length 2; detected during index evaluation.
Aborts with index and valid range \code{[0..1]} at line 12. &
\statuserr, exit 1 \\
\bottomrule
\end{tabularx}
\end{table}

\textbf{Actual runtime output:}
\begin{lstlisting}[style=terminal]
$ D2/ans --run D3/semantic/rt1_division_by_zero.ans
D3/semantic/rt1_division_by_zero.ans:6: runtime error: division by zero

$ D2/ans --run D3/semantic/rt2_index_out_of_range.ans
D3/semantic/rt2_index_out_of_range.ans:12: runtime error: list index 5 out of range [0..1]
\end{lstlisting}

% =====================================================================
\section{D1 Consistency Checklist}
\sectionrule

\vspace{2pt}
\small
\rowcolors{2}{anscodebg}{white}
\begin{tabularx}{\textwidth}{@{} L{3.8cm} L{1.0cm} Y @{}}
\toprule
\rowcolor{ansblue}
\textcolor{white}{\textbf{D1 property}} &
\textcolor{white}{\textbf{§}} &
\textcolor{white}{\textbf{Demonstrated by}} \\
\midrule
Static (lexical) scoping &
§4.6 &
Nested calls (\code{classify\_mass} $\to$ \code{molar\_mass}) resolve names in
declaration scope, not caller scope. Loop scopes destroyed per iteration. \\
Call-by-value &
§4.6 &
Compound arguments are copied into each function frame; no aliasing across calls. \\
Strong typing, no coercion &
§4.5 &
\code{sem4}: wrong arg count; \code{sem2}: undeclared element; type mismatches
reported, not coerced. \\
Kind-based type checking for domain types &
§4.5 &
All \code{compound} values share the type \code{compound}; typed parameters accept
any compound regardless of formula. Formula equality is not type equality. \\
Reaction balance verification &
§4.4 &
\code{sem1} rejected at check time; \code{combustion} balance verified at
interpretation time (both sides $\{C{:}1,H{:}4,O{:}4\}$). \\
\code{for}-in over Formula &
§4.4 &
\code{molar\_mass}: elements in first-appearance order; correct sums verified
by molar mass table. \\
\code{for}-in over ReactionSide &
§4.4 &
\code{total\_reactant\_mass}: iterates each term's species in declaration order;
yields resolved compounds. \\
\code{for}-in over List &
§4.4 &
P1: \code{classify\_mass} called once per compound. P3: counter loop over 7
compounds. \\
Division-by-zero error &
§4.4 &
\code{rt1}: detected at run time; file, line, message. Exit 1. \\
Index-out-of-range error &
§4.5 &
\code{rt2}: detected at run time; file, line, index, valid range. Exit 1. \\
\bottomrule
\end{tabularx}
\normalsize

% =====================================================================
\section{Conclusion}
\sectionrule

The ANS interpreter and type checker pass all required tests: three valid programs
execute end-to-end with correct output; four semantic errors and two runtime errors
are each detected and reported with precise, line-numbered diagnostics. Every
property declared in D1 --- static scoping, call-by-value, strong typing, no
implicit coercion, reaction-balance verification, and all three forms of
\code{for}-in iteration --- is exercised by at least one test. The implementation
is consistent with the D1 specification, including the §4.4 rule for
\code{for}-in over a ReactionSide.

\end{document}

```

### File: D3/malformed/err1_missing_semicolon.ans
```text
element H {
    atomic_mass = 1.008,
    valence = 1,
    symbol = "H"
}

let x = 5.0;

```

### File: D3/malformed/err2_unknown_field.ans
```text
element O {
    atomic_mass = 15.999,
    valence = 2,
    symbol = "O",
    protons = 8
};

```

### File: D3/malformed/err3_bad_identifier.ans
```text
element H { atomic_mass = 1.008, valence = 1, symbol = "H" };

let result = 5.0 @ 2.0;

```

### File: D3/malformed/err4_unmatched_brace.ans
```text
element H { atomic_mass = 1.008, valence = 1, symbol = "H" };

function compute(compound c) {
    let mass = 0.0;
    for (let e in c.formula) {
        mass += e.atomic_mass;
    return mass;
};

```

### File: D3/malformed/err5_unterminated_string.ans
```text
element C { atomic_mass = 12.011, valence = 4, symbol = "C" };

compound co2 {
    formula = CO2,
    name = "Carbon dioxide
};

```

### File: D3/programs/combustion_analysis.ans
```text
// Program 2 — Combustion Reaction Analysis
// Exercises: reaction declaration (check = balanced, exothermic energy),
//             typed parameters (reaction r, compound c),
//             function definition and call, augmented assignment

element H { atomic_mass = 1.008,  valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };
element C { atomic_mass = 12.011, valence = 4, symbol = "C" };

compound methane { formula = CH4, name = "Methane"        };
compound oxygen  { formula = O2,  name = "Oxygen"         };
compound co2     { formula = CO2, name = "Carbon dioxide" };
compound water   { formula = H2O, name = "Water"          };

reaction combustion {
    reactants = CH4 + 2*O2,
    products  = CO2 + 2*H2O,
    check     = balanced,
    energy    = exothermic 890.3 kJ/mol
};

function molar_mass(compound c) {
    let formula = c.formula;
    let mass = 0.0;
    for (let element in formula) {
        mass += element.atomic_mass * formula[element];
    }
    return mass;
};

function compute_yield(reaction r, compound c) {
    let products  = r.products;
    let reactants = r.reactants;
    let reactant_mol = 1.0 / mol(reactants[c]);
    let product_mol  = reactant_mol * mol(products[c]);
    return product_mol;
};

function print_reaction_summary(reaction r, compound fuel, compound product) {
    let yield_val = compute_yield(r, fuel);
    let mass      = molar_mass(product);
    if (yield_val > 0.0) {
        print "Reaction produces output.";
    }
    else {
        print "No product formed.";
    }
};

print_reaction_summary(combustion, methane, co2);
compute_yield(combustion, methane);
molar_mass(water);

```

### File: D3/programs/molar_mass_demo.ans
```text
// Program 1 — Molar Mass Calculator
// Exercises: element/compound declarations, function definition,
//             for loop, if/elif/else, function call, list literal

element H { atomic_mass = 1.008,  valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };
element C { atomic_mass = 12.011, valence = 4, symbol = "C" };
element N { atomic_mass = 14.007, valence = 3, symbol = "N" };

compound water    { formula = H2O,  name = "Water"            };
compound methane  { formula = CH4,  name = "Methane"          };
compound co2      { formula = CO2,  name = "Carbon dioxide"   };
compound ammonia  { formula = NH3,  name = "Ammonia"          };

function molar_mass(compound c) {
    let formula = c.formula;
    let mass = 0.0;
    for (let element in formula) {
        mass += element.atomic_mass * formula[element];
    }
    return mass;
};

function classify_mass(compound c) {
    let mass = molar_mass(c);
    if (mass < 20.0) {
        print "Light molecule.";
    }
    elif (mass < 50.0) {
        print "Medium molecule.";
    }
    else {
        print "Heavy molecule.";
    }
};

let compounds = [water, methane, co2, ammonia];
for (let c in compounds) {
    classify_mass(c);
};

```

### File: D3/programs/multi_reaction_survey.ans
```text
// Program 3 — Multi-Reaction Survey
// Exercises: multiple reactions (formula-literal and identifier species),
//             list of compounds iterated with for loop,
//             nested if/else inside for loop, augmented assignment,
//             member access chain, character and string literals

element H { atomic_mass = 1.008,  valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };
element C { atomic_mass = 12.011, valence = 4, symbol = "C" };
element N { atomic_mass = 14.007, valence = 3, symbol = "N" };

compound methane  { formula = CH4, name = "Methane"          };
compound oxygen   { formula = O2,  name = "Oxygen"           };
compound co2      { formula = CO2, name = "Carbon dioxide"   };
compound water    { formula = H2O, name = "Water"            };
compound ammonia  { formula = NH3, name = "Ammonia"          };
compound nitrogen { formula = N2,  name = "Nitrogen"         };
compound hydrogen { formula = H2,  name = "Hydrogen"         };

reaction combustion {
    reactants = CH4 + 2*O2,
    products  = CO2 + 2*H2O,
    check     = balanced,
    energy    = exothermic 890.3 kJ/mol
};

reaction haber_bosch {
    reactants = nitrogen + 3*hydrogen,
    products  = 2*ammonia,
    check     = balanced,
    energy    = exothermic 92.0 kJ/mol
};

function molar_mass(compound c) {
    let formula = c.formula;
    let mass = 0.0;
    for (let element in formula) {
        mass += element.atomic_mass * formula[element];
    }
    return mass;
};

function total_reactant_mass(reaction r) {
    let reactants = r.reactants;
    let total = 0.0;
    for (let species in reactants) {
        total += molar_mass(species);
    }
    return total;
};

let heavy_count = 0;
let light_count = 0;
let compounds = [methane, oxygen, co2, water, ammonia, nitrogen, hydrogen];

for (let c in compounds) {
    let mass = molar_mass(c);
    if (mass >= 30.0) {
        heavy_count += 1;
    }
    else {
        light_count += 1;
    }
};

let sym_h = 'H';
let label = "Survey complete";

total_reactant_mass(combustion);
total_reactant_mass(haber_bosch);

```

### File: D3/semantic/rt1_division_by_zero.ans
```text
// Runtime error 1: division by zero.

element H { atomic_mass = 1.008, valence = 1, symbol = "H" };

let denom = 0.0;
let result = 42.0 / denom;

```

### File: D3/semantic/rt2_index_out_of_range.ans
```text
// Runtime error 2: list index out of range.

element H { atomic_mass = 1.008,  valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };

compound water   { formula = H2O, name = "Water"   };
compound methane { formula = CH4, name = "Methane"  };

element C { atomic_mass = 12.011, valence = 4, symbol = "C" };

let xs = [water, methane];
let bad = xs[5];

```

### File: D3/semantic/sem1_unbalanced_reaction.ans
```text
// Semantic error 1: unbalanced reaction.
// Reactants: H2 + O2  → { H:2, O:2 }
// Products:  H2O      → { H:2, O:1 }
// O counts differ → checker must reject.

element H { atomic_mass = 1.008,  valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };

reaction bad_water {
    reactants = H2 + O2,
    products  = H2O,
    check     = balanced,
    energy    = exothermic 0 kJ
};

```

### File: D3/semantic/sem2_undeclared_element.ans
```text
// Semantic error 2: formula references an undeclared element.
// Element 'N' is never declared, so NH3 is invalid.

element H { atomic_mass = 1.008, valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };

compound ammonia { formula = NH3, name = "Ammonia" };

```

### File: D3/semantic/sem3_missing_field.ans
```text
// Semantic error 3: compound declaration is missing the required 'name' field.

element H { atomic_mass = 1.008,  valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };

compound water { formula = H2O };

```

### File: D3/semantic/sem4_wrong_arg_count.ans
```text
// Semantic error 4: function called with wrong number of arguments.
// molar_mass expects 1 argument (compound c), called with 0.

element H { atomic_mass = 1.008,  valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };

compound water { formula = H2O, name = "Water" };

function molar_mass(compound c) {
    let formula = c.formula;
    let mass = 0.0;
    for (let element in formula) {
        mass += element.atomic_mass * formula[element];
    }
    return mass;
};

molar_mass();

```

