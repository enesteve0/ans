# ANS — Alchemist's Notation Script

ANS is a domain-specific language for declaring chemical elements, compounds, and reactions, verifying atomic balance at compile time, and performing stoichiometric queries such as molar mass and theoretical yield. It is designed so that programs read the way chemistry appears in textbooks rather than as bare numerical code.

Developed as the term project for **CSE 341 — Concepts of Programming Languages** at Gebze Technical University, Spring 2026.

**Authors:** Yusuf Hamza Çelebi (220104004926) · Enes Özkan (220104004931)

---

## Table of Contents

1. [Design Goals](#design-goals)
2. [Quick Example](#quick-example)
3. [Language Reference](#language-reference)
   - [Declarations](#declarations)
   - [Variables](#variables)
   - [Functions](#functions)
   - [Control Flow](#control-flow)
   - [Expressions](#expressions)
   - [Literals](#literals)
4. [Type System](#type-system)
5. [Lexical Structure](#lexical-structure)
6. [Grammar (EBNF Summary)](#grammar-ebnf-summary)
7. [Names, Scoping, and Lifetime](#names-scoping-and-lifetime)
8. [Implementation](#implementation)
9. [Build & Run](#build--run)
10. [Error Reporting](#error-reporting)
11. [Test Suite](#test-suite)
12. [Project Layout](#project-layout)

---

## Design Goals

ANS applies Sebesta's four evaluation criteria (Ch. 1) as follows:

| Criterion | Decision |
|---|---|
| **Readability** | Syntax mirrors chemistry vocabulary directly — `element`, `compound`, `reaction`, `reactants`, `products`, `balanced` — over a familiar C-like skeleton (`let`, braces, semicolons, `if`/`elif`/`else`). |
| **Writability** | Common domain tasks are first-class constructs: declaring an element, writing a balanced equation as `CH4 + 2*O2`, querying `molar_mass(water)`. No boilerplate library imports required. |
| **Reliability** | Formula literals (`H2O`, `CH4`) and unit literals (`mol`, `kJ/mol`) are distinct token categories. `check = balanced` reactions are verified for atomic equality at compile time. Every type error is caught before execution. |
| **Cost / Trade-offs** | Higher implementation cost for formula parsing, unit handling, and balance checking. Reduced generality (unsuited to non-chemistry tasks) and reduced orthogonality (fixed domain forms replace freely-combinable primitives). |

---

## Quick Example

```ans
// Declare elements
element H { atomic_mass = 1.008, valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };
element C { atomic_mass = 12.011, valence = 4, symbol = "C" };

// Declare compounds
compound water   { formula = H2O, name = "Water" };
compound methane { formula = CH4, name = "Methane" };
compound co2     { formula = CO2, name = "Carbon dioxide" };
compound oxygen  { formula = O2,  name = "Oxygen" };

// Declare a reaction — verified balanced at compile time
reaction combustion {
    reactants = CH4 + 2*O2,
    products  = CO2 + 2*H2O,
    check     = balanced,
    energy    = exothermic 890.3 kJ/mol
};

// Function with typed parameter
function molar_mass(compound c) {
    let formula = c.formula;
    let mass = 0.0;
    for (let element in formula) {
        mass += element.atomic_mass * formula[element];
    }
    return mass;
};

// Stoichiometric query
molar_mass(water);

if (molar_mass(water) > 18.0) {
    print "Heavier than expected.";
}
elif (molar_mass(water) == 18.0) {
    print "Exactly 18 g/mol.";
}
else {
    print "Light.";
}
```

---

## Language Reference

### Declarations

All declarations live at the top level and are terminated with `;`.

#### `element`

Declares a chemical element. The name must be a **formula literal** (uppercase-leading, e.g. `H`, `O`, `Na`).

```ans
element H {
    atomic_mass = 1.008,
    valence     = 1,
    symbol      = "H"
};
```

Required fields: `atomic_mass` (number), `valence` (number), `symbol` (string). Order is flexible; a trailing comma is allowed.

#### `compound`

Declares a named chemical compound. The identifier is a lowercase program name; `formula` holds the chemical formula as a formula literal.

```ans
compound water {
    formula = H2O,
    name    = "Water"
};
```

Required fields: `formula` (formula literal), `name` (string).

#### `reaction`

Declares a chemical reaction. Species in `reactants` and `products` may be formula literals (`CH4`, `O2`) or compound identifiers (`methane`, `water`). Stoichiometric coefficients are written `2*O2` — the `*` is required.

```ans
reaction combustion {
    reactants = CH4 + 2*O2,
    products  = CO2 + 2*H2O,
    check     = balanced,
    energy    = exothermic 890.3 kJ/mol
};
```

Fields:
- `reactants` / `products` — reaction expressions
- `check = balanced` — requests compile-time atom-count verification
- `energy = exothermic <qty>` or `energy = endothermic <qty>` — energy annotation

### Variables

```ans
let x = 42;
let name = "Hydrogen";
let ch = 'H';
let compounds = [water, methane, co2, oxygen];  // List<Compound>
let n = 6.02e23;
```

Variables are declared with `let`. Types are inferred from the right-hand side at compile time. Lists must be homogeneous (`List<T>`). Augmented assignment operators `+=`, `-=`, `*=`, `/=` are supported.

### Functions

```ans
// Typed parameter
function molar_mass(compound c) {
    let mass = 0.0;
    return mass;
};

// Untyped parameter
function double_val(x) {
    return x * 2.0;
};

// No parameters
function get_avogadro() {
    return 6.02e23;
};
```

- Parameters may optionally carry a type annotation: `element`, `compound`, or `reaction`.
- Untyped parameters accept any value.
- `return` with no expression returns from a void function.

### Control Flow

#### `for` — iterator loop

Iterates over a **Formula** (yields each Element in first-appearance order) or a **ReactionSide** (yields each species as a Compound):

```ans
// Iterate over formula elements
for (let element in formula) {
    mass += element.atomic_mass * formula[element];
}

// Iterate over reaction species
for (let species in combustion.reactants) {
    print species.name;
}
```

#### `if` / `elif` / `else`

```ans
if (mass > 18.0) {
    print "Heavy.";
}
elif (mass == 18.0) {
    print "Exactly 18.";
}
else {
    print "Light.";
}
```

All branch bodies are braced blocks — there is no dangling-else ambiguity.

#### `print`

```ans
print "The yield is positive.";
print molar_mass(water);
```

### Expressions

Operator precedence (lowest to highest):

| Level | Operators |
|---|---|
| Comparison | `==` `!=` `>=` `<=` `>` `<` |
| Additive | `+` `-` |
| Multiplicative | `*` `/` |
| Unary | `-` |
| Postfix | call `()`, index `[]`, member `.` |
| Primary | literals, identifiers, `(expr)` |

Binary `+`, `-`, `*`, `/` are left-associative. A single comparison per expression — no chaining. Operands are evaluated left to right. Assignment is a statement, not an expression.

Member access and indexing:

```ans
c.formula          // compound field access → Formula
formula[element]   // formula indexing → Number (atom count)
list[0]            // list indexing → element type T
molar_mass(water)  // function call
```

### Literals

| Kind | Examples |
|---|---|
| Number | `20`, `1.008`, `21.2`, `890.3`, `6.02e23` |
| String | `"Water"`, `"Carbon dioxide"` |
| Character | `'H'`, `'\n'` |
| Formula | `H`, `O2`, `H2O`, `CH4`, `CO2` |
| Quantity | `5.0 mol`, `890.3 kJ/mol`, `18.0 g/mol` |
| List | `[water, methane, co2]`, `[1.008, 15.999, 12.011]` |

---

## Type System

ANS is **statically and strongly typed**. All types are determined at compile time; there are no implicit coercions.

### Primitive types

| Type | Value set |
|---|---|
| `Number` | IEEE-754 double. `20` and `20.0` are the same value — no separate integer type. |
| `String` | Unicode sequence with escapes `\"` `\\` `\n` `\r` `\t`. |
| `Character` | Single Unicode character, same escapes. |
| `Boolean` | Produced by comparison operators; consumed by `if`/`elif`. No `true`/`false` literals. |

### Domain types

| Type | Description |
|---|---|
| `Quantity` | A (Number, unit) pair. Units: `mol`, `g`, `kg`, `kJ`, `g/mol`, `kg/mol`, `kJ/mol`. |
| `Formula` | Ordered map from element symbol to atom count. Iterable; indexable by Element. |
| `List<T>` | Homogeneous sequence, 0-indexed, runtime bounds-checked. Element type `T` is inferred from the literal. Empty list `[]` is a semantic error (no context to fix `T`). |
| `Element` | Record: `atomic_mass`, `valence`, `symbol`. Read-only after declaration. |
| `Compound` | Record: `formula`, `name`. Read-only after declaration. |
| `Reaction` | Record: `reactants`, `products`, energy fields. Read-only after declaration. |
| `ReactionSide` | The type of `.reactants` and `.products`. Iterable over species. |

### Type equivalence

- **Name/kind equivalence** for primitive and domain types — `compound` matches `compound` regardless of formula.
- **Unit-tag equality** for Quantities — `5 mol` and `7 mol` are compatible; `5 mol` and `7 g/mol` are not.
- **Structural equivalence** for `List<T>` — `List<Number>` ≡ `List<Number>` when element types match.

---

## Lexical Structure

ANS uses six token categories produced by a maximal-munch (longest-match) scanner followed by a reserved-word filter.

### Identifiers

```
[a-z_][A-Za-z0-9_]*
```

User-defined names always start with a lowercase letter or `_`. Any token starting with an uppercase letter is classified as a **formula literal**, not an identifier.

### Formula Literals

```
([A-Z][a-z]?[0-9]*)+
```

Examples: `H`, `O`, `Na`, `H2O`, `CH4`, `CO2`, `NaCl`. The lexer produces a single `FORMULA` token for a whole molecular formula.

### Unit Literals

Compound units containing `/` are scanned as a single token before the `/` operator is considered:

```
kg/mol | g/mol | kJ/mol
```

Single-word units (`mol`, `g`, `kg`, `kJ`) are emitted as identifiers and promoted to `<unit>` by the parser only when they immediately follow a number literal.

### Keywords

**Hard reserved words** (may never be used as identifiers):

```
let  function  return  for  in  if  elif  else  print
```

**Contextual keywords** (valid identifiers elsewhere; promoted by the parser):

| Position | Words |
|---|---|
| Declaration head | `element` `compound` `reaction` |
| Declaration field names | `atomic_mass` `valence` `symbol` `formula` `name` `reactants` `products` `check` `energy` |
| Reaction field values | `balanced` `exothermic` `endothermic` |
| Parameter type annotations | `element` `compound` `reaction` |

### Operators

```
==  !=  <=  >=  +=  -=  *=  /=  =  +  -  *  /  <  >
```

### Separators

```
{  }  (  )  [  ]  ,  ;  .
```

### Comments

Line comments only, discarded before parsing:

```
// This is a comment
```

---

## Grammar (EBNF Summary)

```ebnf
<program>             → { <top_level_statement> } EOF

<top_level_statement> → <element_declaration> ";"
                      | <compound_declaration> ";"
                      | <reaction_declaration> ";"
                      | <function_declaration> ";"
                      | <variable_declaration> ";"
                      | <print_statement> ";"
                      | <expression_statement> ";"
                      | <if_statement>
                      | <for_statement>

<element_declaration> → "element" <formula_literal> "{" <element_field_list> "}"
<element_field>       → "atomic_mass" "=" <number_literal>
                      | "valence"     "=" <number_literal>
                      | "symbol"      "=" <string_literal>

<compound_declaration>→ "compound" <identifier> "{" <compound_field_list> "}"
<compound_field>      → "formula" "=" <formula_literal>
                      | "name"    "=" <string_literal>

<reaction_declaration>→ "reaction" <identifier> "{" <reaction_field_list> "}"
<reaction_field>      → "reactants" "=" <reaction_expression>
                      | "products"  "=" <reaction_expression>
                      | "check"     "=" "balanced"
                      | "energy"    "=" ( "exothermic" | "endothermic" ) <quantity_literal>

<reaction_expression> → <reaction_term> { "+" <reaction_term> }
<reaction_term>       → <species_ref> | <number_literal> "*" <species_ref>
<species_ref>         → <formula_literal> | <identifier>

<function_declaration>→ "function" <identifier> "(" [ <parameter_list> ] ")" <block>
<parameter>           → [ <type_name> ] <identifier>
<type_name>           → "element" | "compound" | "reaction"

<for_statement>       → "for" "(" "let" <identifier> "in" <expression> ")" <block>
<if_statement>        → "if" "(" <expression> ")" <block>
                        { "elif" "(" <expression> ")" <block> }
                        [ "else" <block> ]

<expression>          → <comparison_expression>
<comparison_expression> → <additive_expression>
                          [ ( "==" | "!=" | ">=" | "<=" | ">" | "<" ) <additive_expression> ]
<additive_expression> → <multiplicative_expression> { ( "+" | "-" ) <multiplicative_expression> }
<multiplicative_expression> → <unary_expression> { ( "*" | "/" ) <unary_expression> }
<unary_expression>    → [ "-" ] <postfix_expression>
<postfix_expression>  → <primary_expression> { "." <identifier> | "[" <expression> "]" | "(" [ <argument_list> ] ")" }
<primary_expression>  → <number_or_quantity_literal> | <string_literal> | <char_literal>
                      | <formula_literal> | <list_literal> | <identifier>
                      | "(" <expression> ")"

<number_or_quantity_literal> → <number_literal> [ <unit> ]
<unit>                → UNIT_LITERAL | "mol" | "g" | "kg" | "kJ"
<list_literal>        → "[" [ <argument_list> ] "]"
```

The full EBNF is in `D1/4.3/EBNF/ANS_complete_EBNF.ebnf`.

---

## Names, Scoping, and Lifetime

**Case sensitivity.** ANS is fully case-sensitive. Tokens starting with uppercase are always formula literals; lowercase-starting tokens are identifiers.

**Static (lexical) scoping.** Every name resolves to the nearest enclosing declaration in the source text, outward to the top-level program scope. This is required for compile-time formula validation and balance checking.

**Compile-time vs. run-time bindings:**

| Binding | When |
|---|---|
| Lexeme → token category | Compile time (lex) |
| Identifier → declaration | Compile time (static scope walk) |
| Variable → inferred type | Compile time |
| Parameter → declared type | Compile time |
| Formula symbol validity | Compile time |
| `check = balanced` | Compile time |
| Parameter → argument value | Run time (call) |
| Local variable → storage cell | Run time (block entry) |
| Loop variable → current element | Run time (each iteration) |

**Storage lifetimes:**

| Entity | Lifetime |
|---|---|
| `element`, `compound`, `reaction` declarations | Static — allocated once, live for the whole program |
| Top-level `let` variables | Static |
| Function parameters | Stack-dynamic — allocated per call, supports recursion |
| Local `let` inside a function/block | Stack-dynamic — freed on block exit |
| `for` loop variable | Stack-dynamic — rebound each iteration |
| List/compound/reaction values | Implicit heap-dynamic (no explicit `new`/`delete`) |

ANS has **no explicit-heap-dynamic storage**, no pointers, and no aliasing.

---

## Implementation

The implementation is a hand-written C11 pipeline in `D2/`: lexer → parser → type checker → interpreter.

### Components

| File | Role |
|---|---|
| `src/lexer.c` | Maximal-munch scanner: whitespace/comment skipping, formula/unit/keyword scanning |
| `src/parser.c` | Recursive-descent parser — one function per EBNF rule; produces AST |
| `src/ast.c` | AST node constructors, tree pretty-printer |
| `src/error.c` | `<file>:<line>:<col>: error:` formatting |
| `src/value.c` | Runtime value types: FormulaVal, ReactionSideVal, Env (lexical scope frames) |
| `src/checker.c` | Two-pass static type checker: collect declarations, then validate all expressions and statements |
| `src/interpreter.c` | Tree-walk interpreter: eval\_expr, exec\_stmt, for-in dispatch, call\_function |
| `src/main.c` | CLI entry point: `--dump-ast`, `--check`, `--run` |

### Pipeline

```
source file
    │
    ▼
  lexer.c        → token stream
    │
    ▼
  parser.c       → AST
    │
    ▼
  checker.c      → type errors (compile-time; halts before interpreter if any)
    │
    ▼
  interpreter.c  → program output / runtime errors
```

### Key design notes

- **Contextual keyword disambiguation** happens in the parser. `element`, `compound`, and `reaction` are plain `IDENT` tokens; the parser promotes them based on grammar position.
- **Formula vs. identifier** disambiguation is done in the lexer by case: uppercase-leading → `FORMULA`, lowercase/`_`-leading → `IDENT`.
- **Unit disambiguation**: compound units (`g/mol`, `kJ/mol`) are probed before consuming `/`; single-word units are `IDENT` tokens promoted by the parser.
- **Static scoping**: function frames are created with the global environment as parent — functions see top-level declarations but not each other's locals.
- **Balance check**: the checker accumulates atom maps for both sides of a reaction and reports an error if they differ.

---

## Build & Run

Requires a C11 compiler (`cc`). No external dependencies.

```sh
cd D2

make          # optimised build → ./ans
make debug    # add -g -DDEBUG
make clean
make test     # run all 35 parse + semantic tests
```

### Commands

```sh
./ans --dump-ast <file.ans>   # parse and print AST; exit 0 on success
./ans --check   <file.ans>    # parse + type-check; exit 0 if clean, 1 on errors
./ans --run     <file.ans>    # parse + check + execute; prints program output
```

### Examples

```sh
# Run the combustion analysis example
./ans --run ../D3/programs/combustion_analysis.ans

# Check a program for type errors without running it
./ans --check ../D1/4.1/code\ sample/main.ans

# Inspect the AST
./ans --dump-ast tests/valid/functions.ans
```

### AST dump example

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

## Error Reporting

### Parse errors

```
<file>:<line>:<col>: error: <message>
```

```
tests/invalid/missing_semicolon.ans:7:1: error: expected ';' but got let
tests/invalid/bad_char.ans:3:18: error: unexpected character '@'
```

The parser uses panic-mode recovery (skips to the next `;`, `}`, or EOF) so multiple errors are reported per run.

### Type errors (`--check`)

```
main.ans:45:5: error: argument 1 to 'compute_yield': expected compound, got number
main.ans:64:5: error: reaction 'combustion' is not balanced: left={C:1,H:4,O:4} right={C:1,H:4,O:3}
```

### Runtime errors (`--run`)

```
<file>:<line>: runtime error: division by zero
<file>:<line>: runtime error: list index 5 out of range (length 3)
<file>:<line>: runtime error: formula element 'Xx' is not declared
```

---

## Test Suite

```sh
cd D2
make test
```

Runs all programs in `tests/valid/` (each must exit 0) and all programs in `tests/invalid/` (each must exit 1). **35 tests total, all passing.**

| Category | Count | Description |
|---|---|---|
| `tests/valid/` | 8 | `elements.ans`, `compounds.ans`, `reactions.ans`, `control_flow.ans`, `functions.ans`, `main.ans`, `for_loop.ans`, `expressions.ans` |
| `tests/invalid/` | 27 | Missing semicolons, unknown fields, illegal characters, unmatched braces, unterminated strings, and more |

The three D3 example programs also serve as end-to-end integration tests and are run as part of verification:

```sh
./ans --run ../D3/programs/molar_mass_demo.ans
./ans --run ../D3/programs/combustion_analysis.ans
./ans --run ../D3/programs/multi_reaction_survey.ans
```

---

## Project Layout

```
ans/
├── D1/                              # Language design specification
│   ├── 4.1/                         # Language overview & sample program
│   ├── 4.2/                         # Lexical structure
│   ├── 4.3/EBNF/                    # Complete EBNF grammar
│   ├── 4.4/                         # Operational semantics
│   ├── 4.5/                         # Type system
│   ├── 4.6/                         # Names, binding, scope, lifetime
│   ├── 4.7/                         # Expressions & assignment
│   ├── 4.8/                         # Design rationale
│   └── Final Report/
│       ├── v1/                      # D1 P1 PDFs (220104004926, 220104004931)
│       └── v2/                      # D1 P2 PDFs (220104004926, 220104004931)
├── D2/                              # Full interpreter implementation (C11)
│   ├── include/                     # lexer.h  parser.h  ast.h  error.h
│   │                                # value.h  checker.h  interpreter.h
│   ├── src/                         # lexer.c  parser.c  ast.c  error.c  main.c
│   │                                # value.c  checker.c  interpreter.c
│   ├── tests/
│   │   ├── valid/                   # Programs that must be accepted (8)
│   │   └── invalid/                 # Programs that must be rejected (27)
│   ├── ans                          # Compiled binary (not committed)
│   ├── Makefile
│   └── README.md
├── D3/                              # Example programs & test report
│   ├── P1/                          # D3 P1 PDF + LaTeX
│   ├── P2/                          # D3 P2 PDF + LaTeX
│   ├── programs/                    # molar_mass_demo.ans  combustion_analysis.ans
│   │                                # multi_reaction_survey.ans
│   ├── malformed/                   # err1…err5 parser-error programs
│   └── semantic/                    # sem1…sem4 type-error programs, rt1…rt2 runtime-error programs
├── D4/                              # AI usage journal (per student)
│   ├── 220104004926/
│   │   ├── P1/                      # D4 P1 PDF + LaTeX
│   │   └── P2/                      # D4 P2 PDF + LaTeX
│   └── 220104004931/
│       └── P1/                      # D4 P1 PDF + LaTeX
├── D5/                              # Retrospective & self-assessment (per student)
│   ├── 220104004926/
│   │   ├── P1/                      # D5 P1 PDF + LaTeX
│   │   └── P2/                      # D5 P2 PDF + LaTeX
│   └── 220104004931/
│       └── P1/                      # D5 P1 PDF + LaTeX
├── D6/                              # Contribution report (pair)
│   ├── P1/                          # D6 P1 PDF + LaTeX
│   └── P2/                          # D6 P2 PDF + LaTeX
├── Submission/
│   ├── Part 1/                      # Ready-to-upload Part 1 files
│   │   ├── 220104004926/
│   │   └── 220104004931/
│   └── Part 2/                      # Ready-to-upload Part 2 files
│       ├── 220104004926/
│       └── 220104004931/
├── Archive/                         # Claude Code session JSONL logs
├── CSE341_Project_Guidance_Files/   # Submission guide & project handout
└── Sebesta_Course_Book/             # Concepts of Programming Languages (Sebesta)
```

---

## License

See [LICENSE](LICENSE).
