# CSE 341 — AI Usage Journal
## Part 1 (Due 8 May 2026)
**Student:** Yusuf Hamza Çelebi
**Student Number:** 220104004926
**Language:** ANS (Alchemist's Notation Script)





---


## Entry 1

| Field | Value |
|---|---|
| **Entry #** | 1 of 21 |
| **Date** | 2026-05-05 |
| **Phase** | Design / Writing |
| **AI Tool** | Claude Opus 4.7 (Anthropic) |
| **Mandatory Experiment** | — |

**Goal**
Set up an AI assistant workflow for the CSE 341 project with auto-drafted journal entries.

**Prompt (verbatim)**
> "Act as a programming and language design assistant for my CSE 341 course project. For every prompt I give you, you must follow a two-step process: Step 1: Fulfill my request. Step 2: Immediately below your response, generate a draft for my AI Usage Journal using the exact template below. You will fill in the objective data based on our interaction and leave clear placeholders for the subjective evaluations I must write myself. [template followed]"

**Response — Key Part**
Claude agreed to the workflow but proposed a modification: it would auto-fill only the objective metadata fields (Date, Phase, AI tool, Goal, Prompt, Response summary) and leave the four graded subjective fields (Accepted, Rejected/Modified, Errors caught, Reflection) as placeholders. It cited the handout §6 rule that "the rationale sections and the reflection parts of the journal must be in your own voice" and §9's warning that AI-generated rationale undefendable in the exam is treated as exam failure. It flagged that Experiments E1–E3 specifically require documented cases of the AI being wrong, suggesting outputs be treated as drafts to interrogate rather than accept.

**Accepted**
The modified workflow — objective fields auto-filled, subjective fields left as placeholders — was accepted as the right approach. Claude's point about E1–E3 requiring documented AI errors was taken seriously; subsequent sessions were approached more adversarially as a result.

**Rejected / Modified**
The original template asked Claude to fill in all fields including the subjective reflection and evaluation. This was rejected after Claude explained the academic integrity and exam-performance implications — those fields are exactly what the exam probes and must be in the student's own voice.

**Errors You Caught**
No factual errors in this setup turn. Claude asked whether we had read Sebesta Ch. 1–5 as a scoping question — at this point D1 was already drafted, so the question was misaligned with where we actually were. Minor assumption error about starting point.

**Reflection**
Setting up a structured workflow before diving into content was a good instinct — it meant every subsequent AI interaction produced archivable material immediately. The distinction between objective and subjective journal fields maps directly onto what the exam tests: recording what an AI said is easy, explaining why you accepted or rejected it is the skill being assessed.


---


## Entry 2

| Field | Value |
|---|---|
| **Entry #** | 2 of 21 |
| **Date** | 2026-05-05 |
| **Phase** | Design |
| **AI Tool** | Claude Opus 4.7 (Anthropic) |
| **Mandatory Experiment** | — |

**Goal**
Provide project context (chemistry domain, pair, C implementation) and request a thorough review of the completed D1 specification PDF.

**Prompt (verbatim)**
> "Q1 Our domain will be chemical reactions. Q2 We are a pair. We submitted the pairing form. Q3 We want to implement it in C. Q4 Actually we worked out D1 and prepared a pdf about it but I want to check diligently to the design that we made with my friend."

**Response — Key Part**
Claude gave initial reactions: chemistry is a strong original domain (not in §2 list) with natural Sebesta vocabulary fit around conservation of mass and units. C is allowed but adds ~30% implementation time — recommended an arena allocator ("bump-allocate nodes into a big buffer and free the whole thing at the end") and warned "left recursion is fatal for hand-written recursive descent in C." Suggested pair split: lexer/parser for Part 1, type checker/interpreter for Part 2. Noted D1 PDF was not attached and provided a six-point pre-flight checklist.

**Accepted**
The arena allocator recommendation was accepted and implemented in D2. The pair split suggestion was adopted as the working arrangement. The pre-flight checklist was used for self-review before sending the PDF in the next turn.

**Rejected / Modified**
The suggestion to avoid ANTLR-C was accepted by default since we were already committed to hand-written C. The kJ_per_mol underscore suggestion (raised later) was ultimately rejected in favour of keeping kJ/mol notation.

**Errors You Caught**
Claude did not see the D1 PDF in this turn but proceeded with a detailed checklist as if it had full context — this produced somewhat generic guidance rather than specific feedback. The next turn with the actual PDF was more useful.

**Reflection**
Getting domain and implementation choices validated early prevented wasted effort. The arena allocator tip was the single most practically useful piece of implementation advice from this session — it is the kind of C-specific knowledge that AI excels at providing and would have been painful to discover the need for mid-implementation.


---


## Entry 3

| Field | Value |
|---|---|
| **Entry #** | 3 of 21 |
| **Date** | 2026-05-05 |
| **Phase** | Design / Writing |
| **AI Tool** | Claude Opus 4.7 (Anthropic) |
| **Mandatory Experiment** | — |

**Goal**
Get a thorough critical review of the completed D1 design specification (§4.1, §4.2, §4.3, §4.6) for the ANS chemistry DSL.

**Prompt (verbatim)**
> "[D1 PDF attached] Here is the pdf let's discuss on it thoroughly. Btw the code sample is deprecated. I will provide you the new sample: [updated sample program with element/compound/reaction declarations, compute_yield, molar_mass, two reactions, if/elif/else block]"

**Response — Key Part**
Claude identified 5 critical issues: (1) `&&`/`||` in grammar but missing from §4.2 operator list — "the lexer cannot produce them, so the parser productions are unreachable"; (2) `UNIT_LITERAL` as a lexer category incompatible with contextual-keyword treatment of `mol` — "the lexer cannot decide locally"; (3) static-scoping example in §4.6 internally contradictory — `let water = methane` violates no-redeclaration rule when `compound water` is already declared; (4) reaction arrow `->` in §4.1 prose but absent from grammar; (5) no `true`/`false` literals and no `!` operator. Also flagged: heterogeneous list literals conflicting with static typing, `reactants[compound].mol` relying on undescribed runtime model, LL(1) conflict between assignment and expression statements, and "implicit heap-dynamic" as the correct Sebesta §5.4.3 term.

**Accepted**
Issues 1 (&&/|| missing), 3 (scoping example contradiction), 4 (reaction arrow in prose), and the "implicit heap-dynamic" terminology were all corrected. The LL(1) conflict was documented in grammar annotations. The heterogeneous list was resolved by adopting `list[any]` and documenting the explicit `any` type in §4.5.

**Rejected / Modified**
Claude recommended kJ_per_mol underscore notation as the cleanest unit fix — rejected in favour of keeping kJ/mol, which required more parser work but preserved chemistry readability. The boolean literals issue was resolved differently: rather than adding `true`/`false` keywords, comparison expressions are documented as producing runtime boolean values with no literal syntax, which is a valid simplification for a DSL.

**Errors You Caught**
The static-scoping counterexample (point 3) was described as a "bug in the spec" — and it was — but needed careful reading to separate "the example is wrong" from "the explanation of static scoping is wrong." The concept was sound; only the example needed fixing. Accepting the critique wholesale without this distinction would have rewritten a correct explanation.

**Reflection**
A critical review against a concrete spec is far more useful than general advice. Every issue Claude flagged could be traced to a specific line in the EBNF or §4.6 — that precision is what makes feedback actionable. The five "critical" issues were genuinely critical; the "significant" items were genuinely deferrable to Part 2, suggesting Claude's triage was well-calibrated in this session.


---


## Entry 4

| Field | Value |
|---|---|
| **Entry #** | 4 of 21 |
| **Date** | 2026-05-05 |
| **Phase** | Design / Lexer |
| **AI Tool** | Claude Opus 4.7 (Anthropic) |
| **Mandatory Experiment** | — |

**Goal**
Understand the proposed "drop UNIT_LITERAL" lexer fix in detail and learn what the dangling-else ambiguity is.

**Prompt (verbatim)**
> "I didnt understand this solution can you explain it much more detailed? What is dangling else error? [quoting the '(Cheap) Drop UNIT_LITERAL as a lexer category' excerpt]"

**Response — Key Part**
On the unit fix: "stop trying to decide at lexing time whether mol is a unit. Always emit IDENTIFIER. Let the parser figure it out from context." Gave a C sketch with `is_unit_name()` in `parse_primary` and walked through four concrete cases: `5.0 mol` (quantity), `function f(reaction, mol, compound)` (parameter), `mol / reactants[compound].mol` (variable use), `let mol = 7` (declaration) — showing no conflicts in any case. For compound units recommended kJ_per_mol (underscores) over parser-level kJ/mol handling.

On dangling-else: explained the classical ambiguity with a nested-if showing two parse trees with different runtime semantics. Listed three resolutions; confirmed ANS uses mandatory braces, meaning "the dangling-else problem cannot arise," and recommended citing Sebesta §3.4.5 for rubric credit.

**Accepted**
The parser-level unit promotion via `is_unit_name()` after consuming a number was accepted and implemented exactly as described in D2. The dangling-else explanation was accepted fully — §4.3 now explicitly cites Sebesta §3.4.5 for the mandatory-braces decision.

**Rejected / Modified**
The kJ_per_mol underscore recommendation was rejected — kJ/mol preserves chemistry readability, which §4.1 prioritises. The compound unit was handled at the parser level as a two-token sequence. Added ~15 lines of parser code but preserved the intended notation.

**Errors You Caught**
Claude's C sketch used `p->cur.text` to check the unit name, but in a lookahead-based parser the current token would already have been consumed before the check. The correct approach is to peek before consuming the number. This is a subtle ordering bug that would have caused the parser to lose the number token when forming a quantity node. Caught during code review before implementation.

**Reflection**
The dangling-else explanation clarified why mandatory braces are not stylistic but architecturally motivated — a genuinely new insight. The four-case walkthrough method (trace specific inputs through the mechanism) is a useful technique to request whenever a lexer/parser rule feels abstract; it forces specificity that surface-level explanations skip.


---


## Entry 5

| Field | Value |
|---|---|
| **Entry #** | 5 of 21 |
| **Date** | 2026-05-05 |
| **Phase** | Design |
| **AI Tool** | Claude Opus 4.7 (Anthropic) |
| **Mandatory Experiment** | — |

**Goal**
Get feedback on a proposed redesign of `compute_yield` using typed parameters and `r.mol` instead of a separate `mol` parameter.

**Prompt (verbatim)**
> "# Function declaration for computing yield of a compound in a reaction
> function compute_yield(reaction r, compound c) {
>     let products = r.products;
>     let reactants = r.reactants;
>     let reactant_mol = r.mol / r.reactants[c].mol;
>     let product_mol = reactant_mol * r.products[c].mol;
>     return product_mol;
> };
> How about doing it like this?"

**Response — Key Part**
Claude separated the two changes. On typed parameters: "a strong move" that solves several issues at once — required changes to §4.2, §4.3 `<typed_parameter>` rule, and §4.6 bindings table to "declared type · Compile time (Sebesta §5.3)." Recommended mandatory on parameters, inferred on `let`.

On `r.mol`: rejected on two grounds — (1) `r.mol` is not in the `<reaction_field>` schema so the function does not type-check against its own language; (2) "a reaction is a stoichiometric equation that doesn't have a starting amount — the amount is caller-supplied." Also surfaced a quiet bug: indexing both `reactants[c]` and `products[c]` with the same compound when the limiting reactant and desired product are generally different. Offered three fixes; recommended Option B (mixture construct) for originality rubric credit.

**Accepted**
The typed parameter change was accepted in full — `<typed_parameter> → <type_name> <identifier>` is in the final EBNF, mandatory-on-parameters + inferred-on-let hybrid was adopted, and §4.6 bindings table updated accordingly. The `r.mol` schema violation was accepted as a real bug. The quiet `reactants[c]`/`products[c]` same-compound bug was also accepted and fixed in the sample code.

**Rejected / Modified**
Option B (mixture construct) was rejected — adding a half-designed new construct three days before submission would create a D1/D2 inconsistency without time to implement it. Option A was adopted instead: four-parameter `compute_yield(reaction r, quantity amount, compound limiting, compound desired)`. Less DSL-feeling than mixture but implementable and defensible. Claude's framing of Option B as an "originality rubric" argument was also questioned — originality rewards genuine design choices, not post-hoc additions.

**Errors You Caught**
Claude's Option B recommendation was partly motivated by the rubric's originality dimension, but this framing was subtly misleading. A partially-implemented construct added under deadline pressure would score lower on correctness and consistency than a well-implemented simpler design. Recognising this distinction prevented a poor strategic decision.

**Reflection**
The two-bugs-in-one-proposal pattern is common when revising code quickly. AI review that separates changes and evaluates each independently is more useful than a summary verdict. The `reactants[c]`/`products[c]` same-compound bug was the most subtle catch — it would not be caught by syntax or type checking, only by thinking about the chemistry. This is the kind of domain-specific correctness check where having a knowledgeable reviewer matters most.

---


---


## Entry 6 — Mandatory Experiment E1

| Field | Value |
|---|---|
| **Entry #** | 6 of 21 |
| **Date** | 2026-05-07 |
| **Phase** | Design / Parser |
| **AI Tool** | Claude Code (Claude Sonnet, CLI) |
| **Mandatory Experiment** | **E1 — EBNF grammar generation** |

**Goal**
Ask the AI to generate/rewrite an EBNF grammar for ANS from the existing sample code and grammar file, then compare the result with my own grammar.

**Prompt (verbatim)**
> "Can you rewrite the ebnf with the sebesta style"
>
> (Second iteration, after updating sample code:)
> "can you rewrite the ebnf again I made some changes to the sample code scrutinize it"

**Response — Key Part**
First pass — Claude identified that the file already claimed "Sebesta-style" but used `::=` instead of Sebesta's `→` arrow notation. It rewrote the grammar with the following structural changes:

1. `→` instead of `::=` — the defining characteristic of Sebesta's EBNF notation.
2. `(* ... *)` comments — replacing C-style `/* */`.
3. Inline grouping — alternatives written as `( "+" | "-" )` inline rather than broken into separate productions.
4. Collapsed alternatives — e.g. `<augmented_assignment_operator>` shortened to one line.

Second pass — after the sample code was updated, Claude cross-read `main.ans` against the grammar and identified four mismatches:
1. `import std` — no `<import_statement>` rule in the grammar.
2. `//` line comments — lexer still defined only `#[^\r\n]*`.
3. Typed parameters — `function compute_yield(reaction r, compound c)` contradicts `<parameter_list> → <identifier> { "," <identifier> }` which allows only bare identifiers.
4. Parenthesized if/elif conditions — `if (expr) {` — the sample wraps every condition in `()`; the grammar had bare `"if" <expression>`.

**Accepted**
The Sebesta notation conversion (`→`, `(* *)`, inline grouping) was fully accepted — these are mechanical and correct. The four mismatch detections in the second pass were all verified against the source and accepted. The `<parameter>` rule extension (`<parameter> → [ <type_name> ] <identifier>`) and the `<type_name>` production were accepted as the correct fix for typed parameters.

**Rejected / Modified**
The `import` statement addition was later reverted entirely — rather than adding a module system, the decision was made to drop `import std` from the sample and declare `mol` as a pre-declared built-in. The grammar Claude generated for `import` (`<import_statement> → "import" <identifier>`) was therefore discarded.

The parenthesized if/elif condition fix (`"if" "(" <expression> ")"`) was accepted syntactically but reviewed carefully — making parentheses mandatory is a design decision (reliability over writability) that needed to be explicitly documented in D1, not just silently applied.

**Errors You Caught**
Claude did not spontaneously note the architectural contradiction in the unit tokenization rule (`kJ/mol` claimed to be resolved by "longest match" while `mol` was claimed to require previous-token context). This was caught in a separate review session and required a structural rethink (Option C). If the generated grammar had been submitted as-is, the lexer section would have contained an internal contradiction that would likely have been flagged in the exam.

Additionally, Claude's first-pass grammar introduced `<import_statement>` as a fix, which added complexity that was later unnecessary. Accepting it uncritically would have created a module system that was never designed or implemented.

**Reflection**
AI-generated EBNF is reliable for mechanical transformations (notation style, collapsing alternatives) but requires cross-checking against the actual sample code to catch semantic mismatches. The second pass was significantly more valuable than the first because it compared grammar against code rather than just reformatting. The operator precedence hierarchy (`<comparison_expression>` → `<additive_expression>` → `<multiplicative_expression>` → `<unary_expression>` → `<postfix_expression>` → `<primary_expression>`) was preserved correctly — this is the part most likely to go wrong, and it did not.

---


---


## Entry 7

| Field | Value |
|---|---|
| **Entry #** | 7 of 21 |
| **Date** | 2026-05-07 |
| **Phase** | Writing |
| **AI Tool** | Claude Code (Claude Sonnet, CLI) |
| **Mandatory Experiment** | — |

**Goal**
Write D1 §4.1 Language Overview as a markdown file by reading all files in the repo and synthesising them — not copying from the LaTeX draft.

**Prompt (verbatim)**
> "Can you write the [screenshot of §4.1 requirements] these in to a markdown file in 4.1 in D1 in sebesta style check all the files in the repo write it accordingly"

**Response — Key Part**
Claude read `main.ans`, `D1_P1.tex` (the LaTeX draft), and the EBNF before writing. It produced `4.1_language_overview.md` with the following source mapping:

| Source | Contribution |
|---|---|
| `main.ans` | Sample program — used current syntax: `import std`, `//` comments, typed params, parenthesized `if` |
| `D1_P1.tex` §4.1 | Prose — domain description, Sebesta justification paragraph, four criteria and trade-offs |
| EBNF | Confirmed `import`, `<type_name>`, parenthesized if/elif as first-class grammar constructs |

The sample program in the output was condensed from `main.ans` to ~25 lines showing five key features: element/compound declarations, typed function with a `for` loop, a reaction with `check = balanced`, and a parenthesized `if`.

**Accepted**
The overall structure was accepted. The four-criteria framework (readability, writability, reliability, cost — Sebesta §1.3) was correctly applied and the trade-off analysis was accurate for a chemistry DSL.

**Rejected / Modified**
The sample program in the output still contained `import std` — which was later removed from the language entirely. This meant §4.1 needed a minor revision after the `import` decision was made. The lesson: writing documentation before finalising language features creates downstream inconsistencies.

**Errors You Caught**
Claude included `import std` in the §4.1 sample program at a point in the session when the design was still in flux. This was not an AI error per se — the sample code itself still had `import` at that timestamp — but it illustrates that documentation written against an unstable spec needs version-tracking.

**Reflection**
Having the AI cross-reference multiple source files (LaTeX + EBNF + sample code) before writing produced more consistent output than asking it to write from a single source. The explicit source-mapping table in the response was useful for verifying that nothing was fabricated.


---


## Entry 8

| Field | Value |
|---|---|
| **Entry #** | 8 of 21 |
| **Date** | 2026-05-07 |
| **Phase** | Writing |
| **AI Tool** | Claude Code (Claude Sonnet, CLI) |
| **Mandatory Experiment** | — |

**Goal**
Write D1 §4.6 Names, Binding, Scope, and Lifetime from scratch — not lifted from the LaTeX draft — deriving every answer directly from the EBNF and sample code.

**Prompt (verbatim)**
> "can you prepare a md file in 4.6 do all the statements"
>
> (After Claude started reading the LaTeX draft, interrupted:)
> "dont late it from the latex write from zero"

**Response — Key Part**
Claude restarted and derived every claim from the EBNF (`IDENTIFIER` pattern, sections 12–13 for keywords) and `main.ans` (actual variable names, actual function signatures). The output included:

- Legal identifier pattern derived from EBNF §12
- Compile-time vs run-time bindings table with every row tied to a specific line in `main.ans`
- Static scoping argument built around `check = balanced` and formula validation — both of which require compile-time name resolution
- Dynamic scoping counterexample using the actual `molar_mass`/`compute_yield` pair
- Lifetime table with every row mapped to a concrete declaration in `main.ans`

**Accepted**
The approach of deriving from EBNF + sample code rather than the LaTeX was fully accepted and produced a more honest, verifiable document. The dynamic scoping counterexample was particularly strong because it used real code rather than a hypothetical.

**Rejected / Modified**
The first attempt (before interruption) was drawing from the LaTeX draft — this was rejected. The LaTeX was written earlier and contained some stale framing that didn't match the current EBNF state. Writing from the primary sources (EBNF + code) produced a more accurate and consistent document.

Additionally, the first version Claude produced described the loop variable as "destroyed at the end of each iteration body" — this was corrected to "one stack-dynamic allocation for the `for` block, rebound each iteration" after cross-checking with Sebesta §5.4.3.

**Errors You Caught**
"Implicit heap" — Claude used this informal phrase instead of Sebesta's exact term "implicit heap-dynamic (§5.4.3)." This is the kind of terminology imprecision that costs rubric points. The exact term was substituted before submission.

**Reflection**
The instruction "write from zero" — not from the LaTeX — was the most important design decision in this session. A document derived from the actual grammar and code is checkable; one copied from a draft is opaque. For Part 2, the same principle applies: write §4.4 (Semantics) and §4.5 (Type System) by reasoning from the interpreter, not from prior notes.


---


## Entry 9

| Field | Value |
|---|---|
| **Entry #** | 9 of 21 |
| **Date** | 2026-05-07 |
| **Phase** | Lexer / Parser |
| **AI Tool** | Claude Code (Claude Sonnet, CLI) |
| **Mandatory Experiment** | — |

**Goal**
Build a professional C lexer and parser for ANS following the EBNF, with Makefile, header files, `--dump-ast` flag, and useful error messages including line numbers.

**Prompt (verbatim)**
> "I want you to build a lexer and parser according to our ebnf and ANS's lexical structure. Follow the best practices for c project file structures, language design projects in c. use header files and make files. Keep it professional"

**Response — Key Part**
Claude produced a complete D2 implementation in C with the following file structure:

| File | Purpose |
|---|---|
| `include/lexer.h` | `TokenType` enum (35 types), `Token` struct, `Lexer` API |
| `include/parser.h` | `Parser` struct, `parse_program` declaration |
| `include/ast.h` | `NodeKind` enum (35 node types), `AstNode` with dynamic children |
| `include/error.h` | `error_at` / `fatal_at` with `<file>:<line>:<col>: error:` format |
| `src/lexer.c` | Maximal-munch scanner — unit literals tried before `/`, reserved-word filter promotes hard keywords, contextual keywords stay as `TOK_IDENT` |
| `src/parser.c` | One function per EBNF rule, panic-mode recovery, contextual keyword dispatch |
| `src/ast.c` | Node constructors + box-drawing tree printer (`├──` / `└──`) |
| `src/main.c` | CLI: `./ans [--dump-ast] <file.ans>` |
| `Makefile` | `all` / `debug` / `test` / `clean` targets |

The build was clean and all 12 tests passed (8 valid programs accepted, 4 malformed programs rejected with correct line numbers) after one small fix: `for` loops at the top level were not handled in `parse_top_level` and had to be added.

**Accepted**
The overall architecture — one function per EBNF rule in the parser, separate header files per module, panic-mode recovery — was accepted entirely. This is textbook recursive-descent parser structure matching Sebesta Ch. 4. The `--dump-ast` flag with box-drawing output was a clean implementation of the D2 requirement.

**Rejected / Modified**
The initial parser did not handle `for` at the top level — only inside `<statement>`. The fix was a one-line addition to `parse_top_level`. More importantly, the `D3/` directory was used initially for the implementation; this was moved to `D2/` after realising the handout assigns the implementation to D2 and the test report to D3.

**Errors You Caught**
The `for` loop omission at the top level is a direct grammar inconsistency — `<top_level_statement>` in the EBNF does not include `<for_statement>`. This exposed a gap in the EBNF itself: should top-level `for` loops be valid? The current implementation accepts them; the EBNF should be updated to match, or the implementation should reject them. This needs to be resolved before Part 2.

**Reflection**
AI-generated C parsers are structurally sound but need grammar-specific verification for every edge case. The "one function per EBNF rule" principle made the code directly auditable against the grammar — every parser function could be checked against its corresponding EBNF production. This is the right architecture for an exam where questions target specific grammar rules.


---


## Entry 10

| Field | Value |
|---|---|
| **Entry #** | 10 of 21 |
| **Date** | 2026-05-07 |
| **Phase** | Testing |
| **AI Tool** | Claude Code (Claude Sonnet, CLI) |
| **Mandatory Experiment** | — |

**Goal**
Build the D3 test report — three non-trivial valid programs and five malformed programs with real parser error output captured from the built binary.

**Prompt (verbatim)**
> [Screenshot of D3 requirements from handout]
> (No additional text — requirements image sent directly)

**Response — Key Part**
Claude built three valid programs exercising different language features, ran all three through the parser to confirm they accepted cleanly, then created five distinct malformed programs and captured real error output. The session was interrupted by a usage limit before the D3 markdown report was completed.

The valid programs parsed successfully. The five malformed programs produced error messages with file, line, and column numbers in the format `<file>:<line>:<col>: error: <message>` — matching the D2 requirement for "useful error message including line number."

**Accepted**
The error message format (`<file>:<line>:<col>: error:`) is correct and matches the handout requirement. Running programs through the actual binary to capture real output is the right approach — fabricated error messages would be detectable in the exam.

**Rejected / Modified**
The D3 report markdown was not completed — the session hit the Claude Code usage limit before the file was written. The programs and error output were generated but the written report (PDF, 2–3 pages) still needs to be assembled manually. This is a partial completion: the raw material exists, the deliverable does not.

**Errors You Caught**
The session ended mid-task due to a usage limit. This is an operational failure of AI-assisted workflow: a task that should have been broken into smaller checkpointed steps was run as one long session. For Part 2, large generation tasks (especially the D3 report and interpreter) should be broken into explicit checkpoints to avoid losing work at context limits.

**Reflection**
Running test programs through the actual compiled binary and capturing real output is non-negotiable for credibility — the grader can run the same programs. Any discrepancy between reported output and actual output would be immediately visible. The usage limit interruption is a reminder that AI tools have operational constraints that must be planned around, especially on deadline days.

---


---


## Entry 11

| Field | Value |
|---|---|
| **Entry #** | 11 of 21 |
| **Date** | 2026-05-07 |
| **Phase** | Parser |
| **AI Tool** | Claude Code (Claude Sonnet, CLI) |
| **Mandatory Experiment** | — |

**Goal**
Resolve a diverged git branch and remove a tracked `.DS_Store` file from the repository.

**Prompt (verbatim)**
> "resolve the git error"

**Response — Key Part**
Claude ran `git status` and identified two problems: the local `master` had 1 commit that diverged from `origin/master` which had 4 different commits, and `.DS_Store` was still being tracked despite being in `.gitignore`. It resolved both: rebased the local commit ("changed syntax in the sample code") cleanly on top of the 4 remote commits, and untracked `.DS_Store` with `git rm --cached`. Final state: branch 1 commit ahead of origin, clean working tree.

**Accepted**
The rebase strategy over merge was the right call — it keeps the commit history linear and makes the "changed syntax in the sample code" commit clearly attributable. The `.DS_Store` untracking was necessary; it had been committed earlier before the `.gitignore` was added.

**Rejected / Modified**
Claude suggested `git merge` as an alternative to rebase. Merge was rejected — it would have created an extra merge commit, making the history harder to read for a grader inspecting the project. Rebase was chosen to keep the commit log linear and each commit clearly attributable to its author.

**Errors You Caught**
No factual errors in the fix itself. However, Claude acted on a vague prompt ('resolve the git error') without asking for clarification — it read `git status` directly and assumed the diverged branch was the problem. That assumption was correct here, but in a more complex repo it could have misidentified the issue. A more specific prompt ('I have a diverged branch — my partner pushed 4 commits while I had 1 local commit') would have been safer.

**Reflection**
Diverged branches happen in pair projects when one partner pushes without the other pulling first — a foreseeable issue given the pair structure of this project. Knowing to rebase rather than merge keeps history linear and readable. For future sessions, providing Claude with the actual `git log --oneline --all` output rather than just 'resolve the error' produces faster and more targeted diagnosis.


---


## Entry 12

| Field | Value |
|---|---|
| **Entry #** | 12 of 21 |
| **Date** | 2026-05-08 |
| **Phase** | Writing |
| **AI Tool** | Claude Sonnet 4.6 |
| **Mandatory Experiment** | — |

**Goal**
Get a reusable AI Usage Journal entry template that Claude pre-fills at the bottom of every response.

**Prompt (verbatim)**
> "Can you prepare the ai journal entry template in every response"
> followed by: "I meant you write it at every answer and output"

**Response — Key Part**
Claude initially misunderstood the request as wanting a one-time interactive HTML widget. After clarification, Claude confirmed it will append a pre-filled journal entry based on each interaction at the bottom of every response going forward.

**Accepted**
The workflow of having the journal auto-filled per response — reduces friction and ensures entries are dated accurately.

**Rejected / Modified**
The earlier HTML widget approach was useful as a standalone tool but is not the format needed here; plain text per response is more practical for the journal PDF.

**Errors You Caught**
Claude initially misunderstood the request as wanting a one-time interactive template rather than a recurring inline entry.

**Reflection**
Having the AI generate journal entries about its own outputs is meta but legitimate — the key is that I (the student) must still verify, edit, and reflect on whether what is written here accurately represents my own understanding and decisions, not just accept it verbatim.


---


## Entry 13

| Field | Value |
|---|---|
| **Entry #** | 13 of 21 |
| **Date** | 2026-05-08 |
| **Phase** | Design / Parser |
| **AI Tool** | Claude Sonnet 4.6 |
| **Mandatory Experiment** | — |

**Goal**
Check the complete EBNF grammar for left recursion, ambiguity, and consistency with the sample program.

**Prompt (verbatim)**
> "Can you see any left recursion or ambiguity or error in the ebnf? Is there any problem? I checked myself but couldnt see anything"
> (Files uploaded: ANS_complete_EBNF.ebnf, main.ans)

**Response — Key Part**
Claude identified 3 critical inconsistencies between main.ans and the EBNF:
1. `import std` on line 1 has no grammar production.
2. `//` comments are used throughout but only `#` comments are defined in the lexer section.
3. Typed parameter syntax `reaction r, compound c` contradicts `<parameter_list>` which allows only bare identifiers.

Additionally flagged an LL(1) conflict between `<assignment_statement>` and `<expression_statement>`, and underspecified unit tokenization.

**Accepted**
All three critical bugs are genuine — verified by cross-reading the grammar rules and sample code manually. The LL(1) conflict analysis is also correct: both `<assignable>` and `<expression>` start with IDENTIFIER.

**Rejected / Modified**
The claim that there is "no formal ambiguity" needs verification: confirmed that `=` does not appear as an operator inside `<expression>`, so the grammar is formally unambiguous — just LL(1)-conflicted. The notes on missing boolean literals and logical operators are design decisions, not errors; will document them in D1 §4.5–4.7 as intentional.

**Errors You Caught**
Claude noted the LL(1) conflict but it is NOT a grammar ambiguity in the formal sense — worth distinguishing carefully in D1 and in the exam.

**Reflection**
Checking grammar against sample code simultaneously is essential — the EBNF alone looked clean, but the sample program exposed three real contradictions. The typed parameter issue is the most impactful since it affects every function declaration in the language.


---


## Entry 14

| Field | Value |
|---|---|
| **Entry #** | 14 of 21 |
| **Date** | 2026-05-08 |
| **Phase** | Design |
| **AI Tool** | Claude Sonnet 4.6 |
| **Mandatory Experiment** | — |

**Goal**
Get a design review of the ANS sample program (main.ans) — syntax quality and overall DSL assessment.

**Prompt (verbatim)**
> "Ohh ok I sent you the old ebnf before sending you the new one what do you think about the syntax of the sample code? What do you think about the DSL?"

**Response — Key Part**
Claude praised the domain choice as genuinely domain-specific (reaction declarations, formula literals, `check = balanced`, energy expressions). Identified 5 issues:
1. Inconsistent semicolon-after-block style
2. `compute_yield` called with 3 arguments but declared with 2 parameters
3. `mol()` used but never defined (`import std`?)
4. Heterogeneous list contradicting strong typing claim
5. `char` used as variable name (shadows type name)

**Accepted**
The arity bug on `compute_yield` is real — verified by counting arguments manually. The heterogeneous list concern is valid and needs a type system answer. The semicolon inconsistency is a genuine design decision I need to document explicitly in D1.

**Rejected / Modified**
The praise for `check = balanced` is accurate but I should not copy this wording into my D1 — I need to write the design rationale in my own voice. The suggestion on homogeneous lists may not apply if I document ANS lists as dynamically typed containers.

**Errors You Caught**
None in this session — all five issues were verified correct by cross-checking the source code manually.

**Reflection**
Having a second reader (AI) look at the sample code from a pure syntax/design perspective caught the arity bug that I had overlooked entirely. The heterogeneous list point is the one that could hurt most in the exam — a committed answer needs to be ready, not a vague one.


---


## Entry 15

| Field | Value |
|---|---|
| **Entry #** | 15 of 21 |
| **Date** | 2026-05-08 |
| **Phase** | Design |
| **AI Tool** | Claude Sonnet 4.6 |
| **Mandatory Experiment** | — |

**Goal**
Clarify and defend four design decisions in the ANS sample program after Claude's review.

**Prompt (verbatim)**
> "1. I wanted to end every statement with semicolon to prevent any ambiguity with scoping.
> 2. Ok saw the problem thx I will resolve it.
> 3. Its imported from std
> 4. I wanted to copy the list concept from python"

**Response — Key Part**
Claude confirmed decision 1 is valid but noted the EBNF is inconsistent: `<if_statement>` and `<for_statement>` lack trailing `";"` in `<statement>` and `<top_level_statement>`. For std imports, Claude recommended either formalising import in the grammar or dropping it and treating builtins as pre-declared. For heterogeneous lists, Claude identified a conflict with the strong typing claim and offered two resolution paths: `list[any]` or explicit `any` type.

**Accepted**
The EBNF inconsistency on semicolons is real — will add `";"` after if/for in the grammar rules. The import vs builtin framing is useful — will drop import and declare `mol` as a builtin in D1. The `list[any]` framing gives a way to keep heterogeneous lists without abandoning strong typing.

**Rejected / Modified**
Claude's suggestion to use typed `list[T]` may be overengineered for this DSL scope — going with `list[any]` as it is simpler to implement and document.

**Errors You Caught**
No errors in Claude's analysis this session. The semicolon grammar inconsistency was a real oversight not noticed before.

**Reflection**
Design decisions made informally in code need to be grounded in the grammar and type system sections of D1 — a mismatch between the two is exactly what the exam targets. Resolving the import question early prevents a much harder question during the exam.


---


## Entry 16

| Field | Value |
|---|---|
| **Entry #** | 16 of 21 |
| **Date** | 2026-05-08 |
| **Phase** | Design |
| **AI Tool** | Claude Sonnet 4.6 |
| **Mandatory Experiment** | — |

**Goal**
Confirm consequences of removing `import std` from the language design.

**Prompt (verbatim)**
> "I removed import"

**Response — Key Part**
Claude confirmed the fix and identified two follow-up actions: remove the line from main.ans, and document `mol` as a pre-declared built-in with static lifetime in D1 §4.6.

**Accepted**
Both follow-up actions are correct. Placing `mol` in §4.6 under binding/lifetime is the right location — it answers the static lifetime question cleanly.

**Rejected / Modified**
Nothing to reject — response was minimal and accurate.

**Errors You Caught**
None.

**Reflection**
Removing a language feature (import) has cascading documentation consequences. The fix is not just deleting a line from the sample — it requires the spec to account for where the removed dependency (`mol`) now comes from.


---


## Entry 17

| Field | Value |
|---|---|
| **Entry #** | 17 of 21 |
| **Date** | 2026-05-08 |
| **Phase** | Lexer |
| **AI Tool** | Claude Sonnet 4.6 |
| **Mandatory Experiment** | — |

**Goal**
Review D1 §4.2 Lexical Structure for consistency with main.ans and the EBNF.

**Prompt (verbatim)**
> [Pasted full §4.2 markdown document]
> "what do you think about our lexical structure is it consistent with our sample code and ebnf?"

**Response — Key Part**
Claude confirmed the overall structure is sound. Flagged: (1) `import` still unaddressed, (2) `kJ/mol` needs explicit note that `/` doesn't split it as an operator, (3) `H1` is lexically valid per the formula pattern, (4) `element` appears in two contextual positions without a stated disambiguation rule, (5) markdown escaping artifact in the string literal regex, (6) no boolean literal tokens.

**Accepted**
The `kJ/mol` single-token issue is real and not handled by "longer match first" alone since `/` is an operator token — one explicit sentence needed. The markdown escaping issue needs to be fixed before PDF export. The `import` gap remains open — will remove `import std` and declare `mol` as a builtin.

**Rejected / Modified**
The `H1` point is a real edge case but acceptable — will document it as known lexical behaviour rather than fixing it. Boolean literals may remain absent if comparisons return int (0/1) like C — a design decision to commit to in §4.5.

**Errors You Caught**
None in Claude's analysis. All flagged issues verified manually against the source files.

**Reflection**
Lexical structure looks clean at first glance but compound tokens like `kJ/mol` that cross operator characters are a subtle implementation trap. Documenting the disambiguation rule explicitly is more important than it seems — it's exactly the kind of detail the exam asks about.


---


## Entry 18

| Field | Value |
|---|---|
| **Entry #** | 18 of 21 |
| **Date** | 2026-05-08 |
| **Phase** | Lexer |
| **AI Tool** | Claude Sonnet 4.6 |
| **Mandatory Experiment** | — |

**Goal**
Final consistency check on revised §4.2 after adopting Option C for unit tokenization.

**Prompt (verbatim)**
> [Pasted full revised §4.2 markdown]
> "What about now u see anything wrong?"

**Response — Key Part**
Claude confirmed §4.2 is architecturally clean. Two remaining issues: (1) the EBNF file still has the old `<unit_literal>` rules and needs updating to the new `<unit>` production; (2) the grammar snippet uses quoted strings `"mol"` / `"g"` etc. which in Sebesta-style EBNF implies separate terminal token types, but these are actually IDENTIFIER tokens matched by lexeme value — needs a clarifying note.

**Accepted**
Both issues are real. The EBNF inconsistency is the more important one — D1 and the EBNF file must be consistent for the grader. The notation clarification is a minor but exam-worthy point.

**Rejected / Modified**
Nothing to reject — response was precise and minimal.

**Errors You Caught**
None — both issues verified by cross-reading the EBNF file and the new §4.2 side by side.

**Reflection**
A design change in one document (§4.2 lexical structure) always propagates to at least one other document (the EBNF). Keeping both in sync is as important as the correctness of each individually.


---


## Entry 19

| Field | Value |
|---|---|
| **Entry #** | 19 of 21 |
| **Date** | 2026-05-08 |
| **Phase** | Design |
| **AI Tool** | Claude Sonnet 4.6 |
| **Mandatory Experiment** | — |

**Goal**
Review D1 §4.6 Names, Binding, Scope, and Lifetime for errors and Sebesta consistency. Then confirm all issues are resolved in the revised version.

**Prompt (verbatim)**
> [Pasted full §4.6 markdown]
> "I prepared 4.6 as this. Do you see anything wrong or errors?"
> followed by: [Pasted revised §4.6]
> "I fixed the issues now how is it?"

**Response — Key Part**
First review found four issues: (1) semantic checker warning claim is a Part 2 commitment not yet implemented; (2) loop variable lifetime described as "destroyed per iteration" but the correct model is "one allocation, rebound per iteration"; (3) heap storage called "Implicit heap" instead of Sebesta's term "implicit heap-dynamic §5.4.3"; (4) `mol` builtin missing from both binding and lifetime tables despite earlier agreement. Second review confirmed all four issues correctly resolved. No new problems found.

**Accepted**
All four issues were valid and all are now correctly addressed. The implicit heap-dynamic terminology fix is the most important for rubric scoring. Exam prep note accepted: be ready to justify stack-dynamic locals — answer is: supports recursion, avoids memory waste, prevents concurrent-call corruption of shared locals.

**Rejected / Modified**
Nothing to reject in either review round.

**Errors You Caught**
None — all four issues verified against the text and Sebesta §5.4.3 directly.

**Reflection**
Strong technical writing can still miss terminology precision. Sebesta's exact terms (implicit heap-dynamic, stack-dynamic) are what the rubric checks — paraphrasing them correctly in meaning but not in name costs points unnecessarily. Iterative review works best when each round has a specific list of issues to resolve.

---


---


## Entry 20

| Field | Value |
|---|---|
| **Entry #** | 20 of 21 |
| **Date** | 2026-05-08 |
| **Phase** | Parser |
| **AI Tool** | Claude Code (Claude Sonnet, CLI) |
| **Mandatory Experiment** | — |

**Goal**
Diagnose `make: Nothing to be done for 'all'` and get the ANS parser running for the first time.

**Prompt (verbatim)**
> "I cannot run my parser and lexer when I write make to the terminal I get the output make: Nothing to be done for 'all'."
>
> (Follow-up after fix:) `./ans --dump-ast tests/valid/compounds.ans`

**Response — Key Part**
Claude diagnosed the issue: the `ans` binary already existed and was newer than the source files, so `make` correctly saw nothing to rebuild. Fix: `make clean && make` from the `D2` directory. After the rebuild, running `./ans --dump-ast tests/valid/compounds.ans` produced the first successful AST dump:

```
Program
├── ElementDecl [H]
│   ├── ElementField [atomic_mass]
│   │   └── Number [1.008]
│   ├── ElementField [valence]
│   │   └── Number [1]
│   └── ElementField [symbol]
│       └── String ["H"]
├── CompoundDecl [water]
│   ├── CompoundField [formula]
│   │   └── Formula [H2O]
...
```

Claude confirmed: "Your parser and lexer are working perfectly. The AST is clean and well-structured."

**Accepted**
The diagnosis was correct — verified by checking that the binary timestamp was newer than the source files. The `make clean && make` fix is the standard approach. The `--dump-ast` flag working on the first real `.ans` file confirms the lexer, parser, and AST printer are all functioning end-to-end.

**Rejected / Modified**
Claude suggested running `touch src/*.c && make` as an alternative to force a rebuild without cleaning — this was not used. `make clean && make` is cleaner and more explicit about what is happening.

**Errors You Caught**
Claude suggested `touch src/*.c && make` as an alternative to `make clean && make`. This is not technically wrong but it is a bad habit — forcing a rebuild via `touch` creates false timestamps in version control and masks the real state of the build. If this had been used in CI or shared with a partner, the repo timestamps would be misleading. Caught by thinking through what `touch` actually does to file modification times.

**Reflection**
Understanding that `make` uses file timestamps to decide what to rebuild is fundamental for C projects — 'nothing to be done' is make working correctly, not an error. Seeing the `--dump-ast` output on the first real `.ans` file was the first end-to-end validation that the entire pipeline (lexer → parser → AST printer) was wired together correctly. This is a key milestone: a passing test suite proves individual functions work, but a clean AST dump proves the modules communicate correctly.


---


## Entry 21

| Field | Value |
|---|---|
| **Entry #** | 21 of 21 |
| **Date** | 2026-05-08 |
| **Phase** | Parser |
| **AI Tool** | Claude Code (Claude Sonnet, CLI) |
| **Mandatory Experiment** | — |

**Goal**
Understand the purpose and architecture of the C header files in the `include/` directory of the ANS implementation.

**Prompt (verbatim)**
> "What does the source files in include does?"

**Response — Key Part**
Claude explained that `include/` holds the public interface of each module — declarations without implementation — so that separate `.c` files can use each other's types and functions without seeing their internals. Key contents per file:

- **`lexer.h`**: `TokenType` enum (every token kind), `Token` struct (type + position + value), `Lexer` struct (current position, line, column), declares `lexer_init()` and `lexer_next()`
- **`parser.h`**: `Parser` struct (lexer pointer + current/previous token lookahead window), declares `parser_init()` and `parse_program()`
- **`ast.h`**: `NodeKind` enum (every node type), `AstNode` struct (kind, line number, text/numeric payload, dynamic children array), declares `ast_new()`, `ast_add_child()`, `ast_print()`, `ast_free()`
- **`error.h`**: `g_filename`, `g_had_error` globals, `error_at()` (reports and continues), `fatal_at()` (reports and aborts)

**Accepted**
The explanation is accurate and matches the actual file contents. The separation of concerns — one header per module — is standard C practice and makes the architecture directly auditable against the EBNF (each grammar rule level has a corresponding module). This maps cleanly onto the exam question "describe the architecture of your D2 implementation."

**Rejected / Modified**
Nothing was rejected — this was a conceptual explanation session with no code output. The explanations were verified by opening each header file and checking that the described types and function signatures were actually present. This cross-checking step is important: accepting an explanation of code you have not read is exactly the kind of thing the exam can expose.

**Errors You Caught**
Claude described `g_filename` and `g_had_error` in `error.h` as 'global variables.' More precisely, a header file should contain `extern` declarations — announcements that a variable exists — not definitions that allocate storage. The actual storage lives in `error.c`. If the header contained the definition without `extern` and was included in multiple `.c` files, the linker would report a duplicate-symbol error. This is a subtle but real C distinction worth knowing for the exam if asked about the implementation architecture.

**Reflection**
C header files enforce the separation between interface and implementation that the rubric calls 'clean architecture' — each header corresponds directly to one grammar level or concern (lexer, parser, AST, errors), which means the code structure mirrors the language design structure. This makes D2 directly auditable against D1: a grader can ask 'how does your parser communicate with your AST?' and the answer is traceable through `parser.h` and `ast.h` without reading implementation code. Understanding this connection before the exam is worth more than memorising function signatures.



*Journal continues in Part 2 (entries dated 9–22 May 2026)*