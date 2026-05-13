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
4. [Lexical Structure](#lexical-structure)
5. [Grammar (EBNF Summary)](#grammar-ebnf-summary)
6. [Names, Scoping, and Lifetime](#names-scoping-and-lifetime)
7. [Implementation](#implementation)
8. [Build & Run](#build--run)
9. [Error Reporting](#error-reporting)
10. [Test Suite](#test-suite)
11. [Project Layout](#project-layout)

---

## Design Goals

ANS applies Sebesta's four evaluation criteria (Ch. 1) as follows:

| Criterion | Decision |
|---|---|
| **Readability** | Syntax mirrors chemistry vocabulary directly — `element`, `compound`, `reaction`, `reactants`, `products`, `balanced` — over a familiar C-like skeleton (`let`, braces, semicolons, `if`/`elif`/`else`). |
| **Writability** | Common domain tasks are first-class constructs: declaring an element, writing a balanced equation as `CH4 + 2*O2`, querying `molar_mass(water)`. No boilerplate library imports required. |
| **Reliability** | Formula literals (`H2O`, `CH4`) and unit literals (`mol`, `kJ/mol`) are distinct token categories. `check = balanced` reactions are verified for atomic equality at compile time. |
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

// Declare a reaction and verify it is balanced at compile time
reaction combustion {
    reactants = CH4 + 2*O2,
    products  = CO2 + 2*H2O,
    check     = balanced,
    energy    = exothermic 890.3 kJ/mol
};

// Function using typed parameters
function molar_mass(compound c) {
    let formula = c.formula;
    let mass = 0.0;
    for (let element in formula) {
        mass += element.atomic_mass * formula[element];
    }
    return mass;
};

// Stoichiometric queries
molar_mass(water);

if (molar_mass(water) > 18.0) {
    print "Heavier than expected.";
}
elif (molar_mass(water) == 18.0) {
    print "Exactly 18 g/mol.";
}
else {
    print "Lighter than expected.";
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

Declares a chemical reaction. Species in `reactants` and `products` may be formula literals (`CH4`, `O2`) or compound identifiers (`methane`, `water`). Stoichiometric coefficients are written `2*O2` — the `*` is required to distinguish coefficients from formula subscripts.

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
let items = [H, O, C];
let n = 6.02e23;
```

Variables are declared with `let`. Types are inferred at compile time. Augmented assignment operators `+=`, `-=`, `*=`, `/=` are supported.

### Functions

```ans
// Typed parameters
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

```ans
for (let element in formula) {
    mass += element.atomic_mass * formula[element];
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

Binary `+`, `-`, `*`, `/` are left-associative. A single comparison is allowed per expression (no chaining).

Member access and indexing:

```ans
c.formula          // property access
formula[element]   // index access
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
| List | `[H, O, C]`, `[1, "two", 3.45]` |

---

## Lexical Structure

ANS uses six token categories produced by a maximal-munch (longest-match) scanner followed by a reserved-word filter.

### Identifiers

```
[a-z_][A-Za-z0-9_]*
```

User-defined names always start with a lowercase letter or `_`. Any token starting with an uppercase letter is classified as a **formula literal**, not an identifier — this is how `water` and `H2O` are kept visually and grammatically separate without extra keyword rules.

### Formula Literals

```
([A-Z][a-z]?[0-9]*)+
```

Examples: `H`, `O`, `Na`, `H2O`, `CH4`, `CO2`, `NaCl`. The lexer produces a single `FORMULA` token for a whole molecular formula; no lookahead or statefulness is required.

### Unit Literals

Compound units containing `/` are scanned as a single token before the `/` operator is considered:

```
kg/mol | g/mol | kJ/mol
```

Single-word units (`mol`, `g`, `kg`, `kJ`) are emitted as identifiers by the scanner and promoted to `<unit>` role by the parser only when they immediately follow a number literal.

### Keywords

**Hard reserved words** (may never be used as identifiers):

```
let  function  return  for  in  if  elif  else  print
```

**Contextual keywords** (valid identifiers in all other positions; promoted by the parser based on grammar context):

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

**Case sensitivity.** ANS is fully case-sensitive. Tokens starting with uppercase are always formula literals; lowercase-starting tokens are identifiers. There is no ambiguity.

**Static (lexical) scoping.** Every name resolves to the nearest enclosing declaration in the source text, outward to the top-level program scope. This is required for compile-time formula validation and balance checking — the compiler must resolve names from source text alone without simulating execution.

**Compile-time vs. run-time bindings:**

| Binding | When |
|---|---|
| Operator symbol → semantics | Language-design time |
| Lexeme → token category | Compile time (lex) |
| Identifier → declaration | Compile time (static scope walk) |
| Variable → inferred type | Compile time |
| Parameter → declared type | Compile time |
| Formula validity | Compile time |
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
| `for` loop variable | Stack-dynamic — one allocation for the loop, rebound each iteration |
| List/compound/reaction values | Implicit heap-dynamic (runtime-managed, no explicit `new`/`delete`) |

ANS has **no explicit-heap-dynamic storage**, no pointers, and no aliasing, eliminating use-after-free and dangling-pointer errors by design.

---

## Implementation

The current implementation is a hand-written C11 front end (lexer + parser + AST) in `D2/`.

### Components

| File | Role |
|---|---|
| `src/lexer.c` | Maximal-munch scanner: whitespace/comment skipping, formula/unit/keyword scanning |
| `src/parser.c` | Recursive-descent parser — one function per EBNF rule |
| `src/ast.c` | AST node constructors and tree pretty-printer |
| `src/error.c` | `<file>:<line>:<col>: error:` formatting |
| `src/main.c` | CLI entry point: reads file, runs lexer + parser, optionally dumps AST |

### Parser strategy

The parser is a single-token lookahead recursive-descent parser matching the EBNF grammar exactly. Panic-mode error recovery skips to the next `;`, `}`, or EOF on any syntax error, allowing multiple errors to be reported per run.

**Contextual keyword disambiguation** happens entirely in the parser: `element`, `compound`, and `reaction` are contextual, resolved as declaration keywords vs. type annotations based on grammar position. The lexer emits plain `IDENT` for all three.

**Formula vs. identifier disambiguation** is done in the lexer by case: uppercase-leading → `FORMULA`, lowercase/`_`-leading → `IDENT`.

**Unit disambiguation**: compound units (`g/mol`, `kg/mol`, `kJ/mol`) are probed before consuming `/`, so they are emitted as `UNIT_LIT` without lexer statefulness. Single-word units (`mol`, `g`, `kg`, `kJ`) are always `IDENT`; the parser recognises them as `<unit>` only when they follow a number.

---

## Build & Run

Requires a C11 compiler (`cc`). No external dependencies.

```sh
cd D2

make          # optimised build → ./ans
make debug    # add -g -DDEBUG
make clean
```

```sh
./ans <file.ans>                # parse; exit 0 on success, 1 on error
./ans --dump-ast <file.ans>     # parse and print the AST to stdout
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

```
<file>:<line>:<col>: error: <message>
```

Examples:

```
tests/invalid/missing_semicolon.ans:7:1: error: expected ';' but got let
tests/invalid/unknown_field.ans:5:5: error: unknown element field 'protons'
tests/invalid/bad_char.ans:3:18: error: unexpected character '@'
```

The parser uses panic-mode recovery (skips to the next `;`, `}`, or EOF) so multiple errors are reported in a single run.

---

## Test Suite

```sh
cd D2
make test
```

Runs all programs in `tests/valid/` (each must exit 0) and all programs in `tests/invalid/` (each must exit 1).

| Category | Count | Description |
|---|---|---|
| `tests/valid/` | 4 | `elements.ans`, `control_flow.ans`, `functions.ans`, `main.ans` |
| `tests/invalid/` | varies | Missing semicolons, unknown fields, illegal characters, unmatched braces, unterminated strings |

Additional test programs covering three valid and five malformed cases are documented in `D3/`.

---

## Project Layout

```
ans/
├── D1/                        # Language design documents
│   ├── 4.1/                   # Language overview & Sebesta evaluation
│   ├── 4.2/                   # Lexical structure
│   ├── 4.3/EBNF/              # Complete EBNF grammar
│   ├── 4.6/                   # Names, binding, scope, lifetime
│   └── Final Report/          # D1 LaTeX submissions
├── D2/                        # Lexer + parser implementation (C11)
│   ├── include/               # lexer.h  parser.h  ast.h  error.h
│   ├── src/                   # lexer.c  parser.c  ast.c  error.c  main.c
│   ├── tests/
│   │   ├── valid/             # Programs that must be accepted
│   │   └── invalid/           # Programs that must be rejected
│   ├── Makefile
│   └── README.md
├── D3/                        # Example programs & test report (LaTeX)
├── D4/                        # AI usage journal
├── D5/                        # Retrospective & self-assessment
├── D6/                        # Contribution report
├── CSE341_Project_Guidance_Files/
└── Sebesta/                   # Concepts of Programming Languages (Sebesta)
```

---

## License

See [LICENSE](LICENSE).
