# D3 — Example Programs & Test Report

**ANS — Atomic Notation Script**  
Yusuf Hamza Çelebi · Enes Özkan  
CSE 341 — Programming Languages, Gebze Technical University, Spring 2026

---

## Part 1 — Three Non-Trivial Example Programs

Each program parses successfully with the ANS parser (`./ans <file>`).  
Together they exercise every control structure, the structured chemistry types, function definition and call, and the domain-specific reaction construct.

---

### Program 1 — Molar Mass Calculator (`molar_mass_demo.ans`)

Exercises: `element` / `compound` declarations, `function` definition, `for` loop, `if` / `elif` / `else`, list literal, function call.

```ans
// Program 1 — Molar Mass Calculator
element H { atomic_mass = 1.008,  valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };
element C { atomic_mass = 12.011, valence = 4, symbol = "C" };
element N { atomic_mass = 14.007, valence = 3, symbol = "N" };

compound water   { formula = H2O, name = "Water"          };
compound methane { formula = CH4, name = "Methane"        };
compound co2     { formula = CO2, name = "Carbon dioxide" };
compound ammonia { formula = NH3, name = "Ammonia"        };

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

---

### Program 2 — Combustion Reaction Analysis (`combustion_analysis.ans`)

Exercises: `reaction` declaration with `check = balanced` and `exothermic` energy, typed parameters (`reaction r`, `compound c`), augmented assignment, nested function calls.

```ans
// Program 2 — Combustion Reaction Analysis
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
compute_yield(combustion, CH4);
molar_mass(water);
```

---

### Program 3 — Multi-Reaction Survey (`multi_reaction_survey.ans`)

Exercises: multiple `reaction` declarations using both formula-literal and identifier species, list of compounds iterated with `for`, nested `if` / `else` inside a loop, augmented assignment counters, member-access chains, character literal.

```ans
// Program 3 — Multi-Reaction Survey
element H { atomic_mass = 1.008,  valence = 1, symbol = "H" };
element O { atomic_mass = 15.999, valence = 2, symbol = "O" };
element C { atomic_mass = 12.011, valence = 4, symbol = "C" };
element N { atomic_mass = 14.007, valence = 3, symbol = "N" };

compound methane  { formula = CH4, name = "Methane"        };
compound oxygen   { formula = O2,  name = "Oxygen"         };
compound co2      { formula = CO2, name = "Carbon dioxide" };
compound water    { formula = H2O, name = "Water"          };
compound ammonia  { formula = NH3, name = "Ammonia"        };
compound nitrogen { formula = N2,  name = "Nitrogen"       };
compound hydrogen { formula = H2,  name = "Hydrogen"       };

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
let label  = "Survey complete";

total_reactant_mass(combustion);
total_reactant_mass(haber_bosch);
```

---

## Part 2 — Five Malformed Programs

Each program is fed to the ANS parser. The source, the error message produced, and the line number are shown for each.

---

### Error 1 — Missing semicolon after declaration

**Source (`err1_missing_semicolon.ans`):**
```ans
element H {
    atomic_mass = 1.008,
    valence = 1,
    symbol = "H"
}

let x = 5.0;
```

**Parser output:**
```
err1_missing_semicolon.ans:7:1: error: expected ';' but got let
```

The closing `}` on line 5 ends the element body, but the required `;` is absent. The parser sees `let` at line 7 where it expects the terminating semicolon.

---

### Error 2 — Unknown field in element body

**Source (`err2_unknown_field.ans`):**
```ans
element O {
    atomic_mass = 15.999,
    valence = 2,
    symbol = "O",
    protons = 8
};
```

**Parser output:**
```
err2_unknown_field.ans:5:5: error: unknown element field 'protons'
err2_unknown_field.ans:5:13: error: expected '}' but got =
```

`protons` is not a recognised field name (`atomic_mass`, `valence`, `symbol` are the only valid fields). The parser reports the unknown field at line 5 col 5 and then loses sync at the `=`.

---

### Error 3 — Illegal character in expression

**Source (`err3_bad_identifier.ans`):**
```ans
element H { atomic_mass = 1.008, valence = 1, symbol = "H" };

let result = 5.0 @ 2.0;
```

**Parser output:**
```
err3_bad_identifier.ans:3:18: error: unexpected character '@'
err3_bad_identifier.ans:3:18: error: expected ';' but got @
```

`@` is not part of the ANS character set. The lexer emits a `TOK_ERROR` at line 3 col 18; the parser then fails to find the expected semicolon.

---

### Error 4 — Missing closing brace in function body

**Source (`err4_unmatched_brace.ans`):**
```ans
element H { atomic_mass = 1.008, valence = 1, symbol = "H" };

function compute(compound c) {
    let mass = 0.0;
    for (let e in c.formula) {
        mass += e.atomic_mass;
    return mass;
};
```

**Parser output:**
```
err4_unmatched_brace.ans:8:2: error: unexpected token ; in expression
err4_unmatched_brace.ans:9:1: error: expected ';' but got
err4_unmatched_brace.ans:9:1: error: expected '}' but got
err4_unmatched_brace.ans:9:1: error: expected ';' but got
```

The `for` block is never closed — `return` appears where `}` was expected. Panic-mode recovery cascades to report three additional sync errors at EOF (line 9).

---

### Error 5 — Unterminated string literal

**Source (`err5_unterminated_string.ans`):**
```ans
element C { atomic_mass = 12.011, valence = 4, symbol = "C" };

compound co2 {
    formula = CO2,
    name = "Carbon dioxide
};
```

**Parser output:**
```
err5_unterminated_string.ans:5:13: error: unterminated string literal
err5_unterminated_string.ans:5:13: error: expected STRING but got "
err5_unterminated_string.ans:6:2: error: expected '}' but got ;
err5_unterminated_string.ans:7:1: error: expected ';' but got
```

The string on line 5 spans a newline, which is forbidden. The lexer reports the unterminated literal at line 5 col 13; the parser cannot find the closing `}` for the compound body and cascades further errors.
