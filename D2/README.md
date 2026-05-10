# ANS - Full Interpreter (D2, Part 2)

A hand-written C11 implementation of ANS (Alchemist's Notation Script), a chemistry DSL for elements, compounds, reactions, formula traversal, and stoichiometric calculations.

## Build

```sh
make          # optimized build
make debug    # add -g -DDEBUG
```

Requires a C11 compiler (`cc`). No external dependencies.

## Run

```sh
./ans <file.ans>                  # parse, type check, then execute
./ans --check <file.ans>          # parse and type check only
./ans --parse-only <file.ans>     # parser-only mode for Part 1 regression tests
./ans --dump-ast <file.ans>       # parse and print the AST
```

## Test

```sh
make test
```

The test target runs:

- parser regressions in `tests/valid/` and `tests/invalid/`
- end-to-end execution for the three Part 2 programs in `../D3/programs/`
- type-checker negative tests in `tests/type_errors/`
- runtime-error tests in `tests/runtime_errors/`

## Error Format

```text
<file>:<line>:<col>: error: <message>
```

Examples:

```text
tests/type_errors/type_error_arg.ans:14:1: error: argument 1 of 'molar_mass' expects compound, got string
tests/runtime_errors/division_by_zero.ans:1:1: error: division by zero
```

## Implemented Part 2 Semantics

- Static lexical scoping with stack-dynamic function parameters and local variables.
- Strong type checking before execution for numbers, strings, chars, bools, elements, compounds, formulas, reactions, reaction sides, quantities, lists, and functions.
- Formula literals are parsed into atom-count maps and checked against declared elements.
- `check = balanced` reactions are validated before execution.
- Interpreter support for declarations, calls, `for`, `if`/`elif`/`else`, assignment, augmented assignment, `return`, and `print`.
- Runtime errors for division by zero, list index out of range, and reaction-side lookup of a missing compound.

## Project Layout

```text
D2/
├── Makefile
├── README.md
├── include/
│   ├── lexer.h
│   ├── parser.h
│   ├── ast.h
│   ├── semantics.h
│   └── error.h
├── src/
│   ├── main.c
│   ├── lexer.c
│   ├── parser.c
│   ├── ast.c
│   ├── semantics.c
│   └── error.c
└── tests/
    ├── valid/
    ├── invalid/
    ├── type_errors/
    └── runtime_errors/
```

## Authors

Yusuf Hamza Çelebi · Enes Özkan  
CSE 341 — Programming Languages, Gebze Technical University, Spring 2026
