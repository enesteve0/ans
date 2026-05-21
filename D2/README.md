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
