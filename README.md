# ANS - Alchemist's Notation Script

**ANS** is a small domain-specific language (DSL) designed for declaring chemical elements, compounds, and reactions. It verifies atomic balance at compile time and performs stoichiometric queries such as molar mass and theoretical yield. ANS is targeted at undergraduate chemistry students and instructors who want to express calculations naturally, just as they appear in textbooks.

## Key Features

- **Domain-Specific Syntax:** First-class constructs for chemistry terms (`element`, `compound`, `reaction`, `reactants`, `products`, `balanced`).
- **Compile-Time Validation:** The semantic checker ensures every element symbol exists and that reactions flagged with `check = balanced` have equal atom counts on both sides before any values are computed.
- **Unit Tracking:** Supports explicit unit literals (e.g., `mol`, `kJ/mol`) that are tracked during compilation, rather than silently discarded.
- **Stoichiometric Queries:** Built-in support for calculations like theoretical yield and molar mass directly from balanced equations.

## Sample Code

```ans
// Declare elements
element C { atomic_mass = 12.011, valence = 4, symbol = "C" };
element H { atomic_mass = 1.008, valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };

// Declare compounds
compound methane { formula = CH4, name = "Methane" };
compound oxygen  { formula = O2, name = "Oxygen" };
compound co2     { formula = CO2, name = "Carbon dioxide" };
compound water   { formula = H2O, name = "Water" };

// Declare a reaction and verify it is balanced at compile time
reaction combustion {
    reactants = methane + 2*oxygen,
    products = co2 + 2*water,
    check = balanced,
    energy = exothermic 890.3 kJ/mol
};

// Stoichiometric queries
let mass = molar_mass(water);
let yield_amt = compute_yield(combustion, methane, co2);

if (yield_amt > 0) {
    print "The yield of CH4 is positive, so combustion is possible.";
}
```

## Build & Execution

The current implementation (found in the `D2/` directory) includes a hand-written C lexer, recursive-descent parser, type checker, and interpreter.

To build the lexer and parser, you need a C11 compiler (`cc`). No external dependencies are required.

```bash
cd D2
make          # Optimised build
make debug    # Build with debug flags (-g -DDEBUG)
```

To run ANS on a source file:

```bash
./ans <file.ans>                  # Parse, type check, and execute
./ans --check <file.ans>          # Parse and type check only
./ans --parse-only <file.ans>     # Parser-only regression mode
./ans --dump-ast <file.ans>       # Parse and print the Abstract Syntax Tree (AST)
```

To run the test suite:

```bash
make test
```

## Language Design Philosophy

Following Sebesta's language-evaluation framework (Chapter 1), ANS intentionally prioritises **Readability**, **Writability**, and **Reliability** over general-purpose expressiveness.

- **Readability & Writability:** Syntax mirrors standard chemistry vocabulary. Common domain tasks (like balancing an equation or querying molar mass) are built-in constructs rather than complex user-coded library routines.
- **Reliability:** Extensive compile-time checks (e.g., formula parsing, unit handling, and balance checking) ensure equations are chemically sound before execution.
- **Trade-offs:** These benefits come at the cost of higher implementation complexity (for parsing formulas and tracking units) and a deliberate lack of generality outside the chemistry domain.

## Authors

- **Yusuf Hamza Çelebi**
- **Enes Özkan**

*Developed for CSE 341 — Programming Languages, Gebze Technical University, Spring 2026*
