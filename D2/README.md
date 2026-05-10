# ANS - Lexer & Parser (D2, Part 1)

A hand-written C lexer and recursive-descent parser for the ANS (Alchemist's Notation Script) domain-specific language.

---

## Build

```
make          # optimised build
make debug    # add -g -DDEBUG
```

Requires a C11 compiler (`cc`).  No external dependencies.

---

## Run

```
./ans <file.ans>                  # parse; exit 0 on success, 1 on error
./ans --dump-ast <file.ans>       # parse and print the AST
```

---

## Test

```
make test
```

Runs all programs in `tests/valid/` (each must exit 0) and all programs in `tests/invalid/` (each must exit 1).

---

## Error format

```
<file>:<line>:<col>: error: <message>
```

Example:

```
tests/invalid/missing_semicolon.ans:8:1: error: expected ';' but got let
```

The parser uses panic-mode recovery (skips to the next `;`, `}`, or EOF) so multiple errors can be reported in a single run.

---

## AST dump example

```
$ ./ans --dump-ast tests/valid/elements.ans
Program
├── ElementDecl [H]
│   ├── ElementField [atomic_mass]
│   │   └── Number [1.008]
│   ├── ElementField [valence]
│   │   └── Number [1]
│   └── ElementField [symbol]
│       └── String ["H"]
...
```

---

## Project layout

```
D2/
├── Makefile
├── README.md
├── include/
│   ├── lexer.h     token types, Token struct, Lexer API
│   ├── parser.h    Parser struct, parse_program
│   ├── ast.h       NodeKind enum, AstNode, ast_* API
│   └── error.h     error_at / fatal_at
├── src/
│   ├── main.c      CLI entry point
│   ├── lexer.c     maximal-munch scanner
│   ├── parser.c    recursive-descent parser (one function per EBNF rule)
│   ├── ast.c       node constructors + tree pretty-printer
│   └── error.c     error formatting
└── tests/
    ├── valid/      9 programs — must all be accepted
    └── invalid/    11 programs — must all be rejected
```

---

## Authors

Yusuf Hamza Çelebi · Enes Özkan  
CSE 341 — Programming Languages, Gebze Technical University, Spring 2026
