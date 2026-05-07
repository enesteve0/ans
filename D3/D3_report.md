---
title: "D3 - Example Programs & Test Report"
subtitle: "ANS - Atomic Notation Script"
author: "Yusuf Hamza Çelebi (220104004926) and Enes Özkan (220104004931)"
date: "CSE 341 - Programming Languages, Spring 2026"
geometry: margin=0.65in
fontsize: 9.5pt
---

# Part 1 Test Report

This report documents the D3 tests for the Part 1 ANS lexer and parser. The parser under test is the C implementation in `D2/`, run as `D2/ans <file>`. A valid program must parse with exit code `0`; a malformed program must be rejected with a useful line-numbered diagnostic.

## Three Valid Programs

Full sources are stored in `D3/programs/`. The parser accepts all three programs silently, which is the expected success behavior for `D2/ans <file>`.

- `molar_mass_demo.ans` (40 lines): exercises `element` and `compound` declarations, functions, typed parameters, list literals, `for`, `if` / `elif` / `else`, member access, index access, and function calls. Result: accepted, exit `0`.
- `combustion_analysis.ans` (52 lines): exercises a `reaction` declaration, formula literals, `check = balanced`, exothermic energy quantity, typed `reaction` / `compound` parameters, nested calls, and augmented assignment. Result: accepted, exit `0`.
- `multi_reaction_survey.ans` (70 lines): exercises multiple reactions, formula-literal and identifier species, compound lists, loop counters, relational expressions, character and string literals, and repeated function calls. Result: accepted, exit `0`.

Verification run:

```text
D2/ans D3/programs/combustion_analysis.ans     OK
D2/ans D3/programs/molar_mass_demo.ans         OK
D2/ans D3/programs/multi_reaction_survey.ans   OK
```

Together, these examples cover the required DSL surface: primitive literals (`number`, `string`, `char`, boolean-like contextual words), structured chemistry declarations (`element`, `compound`, `reaction`), functions with parameters, expression precedence, control flow, list traversal, and the domain-specific reaction syntax.

## Five Malformed Programs

Full sources are stored in `D3/malformed/`. Each malformed file is intentionally small so the reported line number points directly to the syntax or lexical problem.

- `err1_missing_semicolon.ans`: missing `;` after an `element` declaration. The parser reports the next token (`let`) at line 7 where the semicolon was required.
- `err2_unknown_field.ans`: invalid field name `protons` inside an `element` body. The parser reports the unknown field at line 5, then a recovery error at the following `=`.
- `err3_bad_identifier.ans`: illegal character `@` in an expression. The lexer reports the illegal character with line/column, then the parser reports the missing semicolon.
- `err4_unmatched_brace.ans`: missing closing brace for a `for` block inside a function. The parser reports a syntax error near the premature `;`, followed by EOF recovery diagnostics.
- `err5_unterminated_string.ans`: string literal reaches a newline before a closing quote. The lexer reports the unterminated string at line 5, then parser recovery reports the broken compound body.

Actual parser output:

\footnotesize
```text
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
```
\normalsize

## Conclusion

The valid examples demonstrate that the Part 1 parser accepts non-trivial ANS programs using declarations, expressions, control structures, functions, and the chemistry-specific reaction construct. The malformed examples show that lexical errors, missing separators, invalid declaration fields, unmatched braces, and unterminated strings are rejected with file names, line numbers, columns, and actionable messages.
