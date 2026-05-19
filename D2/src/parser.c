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
static AstNode *parse_expr_or_assignment_statement(Parser *p);

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
            if (!check(p, TOK_FORMULA) && !check(p, TOK_IDENT)) {
                error_at(p->current.line, p->current.col,
                         "expected reaction species after coefficient '*'");
                advance(p);
                ast_add_child(term, c);
                return term;
            }
            AstNode *sp = ast_new(p->current.type == TOK_FORMULA ? ND_FORMULA : ND_IDENT,
                                  p->current.line);
            ast_set_text(sp, p->current.start, p->current.length);
            advance(p);
            ast_add_child(term, c);
            ast_add_child(term, sp);
        } else {
            error_at(coeff.line, coeff.col,
                     "reaction coefficient must be followed by '*' and a species");
            ast_add_child(term, c);
        }
        return term;
    }

    /* bare species_ref */
    if (!check(p, TOK_FORMULA) && !check(p, TOK_IDENT)) {
        error_at(p->current.line, p->current.col,
                 "expected reaction species but got %.*s",
                 p->current.length, p->current.start);
        advance(p);
        return term;
    }
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

    return parse_expr_or_assignment_statement(p);
}

/* ================================================================
   §1  Top-level statements and program
   ================================================================ */

static AstNode *parse_expr_or_assignment_statement(Parser *p) {
    AstNode *lhs = parse_expression(p);

    if (check(p, TOK_EQ)) {
        Token op = p->current; advance(p);
        if (lhs->kind != ND_IDENT) {
            error_at(lhs->line, 1, "assignment target must be a variable name");
        }
        AstNode *asgn = ast_new(ND_ASSIGN, op.line);
        ast_add_child(asgn, lhs);
        ast_add_child(asgn, parse_expression(p));
        expect(p, TOK_SEMICOLON);
        return asgn;
    }

    if (check(p, TOK_PLUS_EQ)  || check(p, TOK_MINUS_EQ) ||
        check(p, TOK_STAR_EQ)  || check(p, TOK_SLASH_EQ)) {
        Token op = p->current; advance(p);
        if (lhs->kind != ND_IDENT) {
            error_at(lhs->line, 1, "augmented assignment target must be a variable name");
        }
        AstNode *aug = ast_new(ND_AUG_ASSIGN, op.line);
        ast_set_text(aug, op.start, op.length);
        ast_add_child(aug, lhs);
        ast_add_child(aug, parse_expression(p));
        expect(p, TOK_SEMICOLON);
        return aug;
    }

    AstNode *stmt = ast_new(ND_EXPR_STMT, lhs->line);
    ast_add_child(stmt, lhs);
    expect(p, TOK_SEMICOLON);
    return stmt;
}

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

    return parse_expr_or_assignment_statement(p);
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
