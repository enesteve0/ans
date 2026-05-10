#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "lexer.h"
#include "error.h"

void lexer_init(Lexer *l, const char *source) {
    l->src  = source;
    l->pos  = 0;
    l->line = 1;
    l->col  = 1;
}

/* ------------------------------------------------------------------ helpers */

static char peek_c(const Lexer *l)          { return l->src[l->pos]; }
static char peek2_c(const Lexer *l)         { return l->src[l->pos] ? l->src[l->pos + 1] : '\0'; }

static char advance_c(Lexer *l) {
    char c = l->src[l->pos++];
    if (c == '\n') { l->line++; l->col = 1; }
    else            { l->col++; }
    return c;
}

static Token make_tok(TokenType t, const char *start, int length, int line, int col) {
    Token tok;
    tok.type    = t;
    tok.start   = start;
    tok.length  = length;
    tok.line    = line;
    tok.col     = col;
    tok.num_val = 0.0;
    return tok;
}

/* ------------------------------------------------------------------ keyword table */

static const struct { const char *word; TokenType type; } KEYWORDS[] = {
    { "let",      TOK_LET      },
    { "function", TOK_FUNCTION },
    { "return",   TOK_RETURN   },
    { "for",      TOK_FOR      },
    { "in",       TOK_IN       },
    { "if",       TOK_IF       },
    { "elif",     TOK_ELIF     },
    { "else",     TOK_ELSE     },
    { "print",    TOK_PRINT    },
    { NULL,       TOK_ERROR    },
};

static TokenType check_keyword(const char *start, int len) {
    for (int i = 0; KEYWORDS[i].word; i++) {
        if ((int)strlen(KEYWORDS[i].word) == len &&
            memcmp(KEYWORDS[i].word, start, (size_t)len) == 0) {
            return KEYWORDS[i].type;
        }
    }
    return TOK_IDENT;
}

/* ------------------------------------------------------------------ unit literal probe
   Returns the length (4, 5, or 6) if src[pos..] starts with a compound unit,
   0 otherwise.  Called before consuming '/'. */

static int probe_unit_literal(const Lexer *l) {
    const char *s = l->src + l->pos;
    if (strncmp(s, "g/mol",  5) == 0 && !isalpha((unsigned char)s[5])) return 5;
    if (strncmp(s, "kg/mol", 6) == 0 && !isalpha((unsigned char)s[6])) return 6;
    if (strncmp(s, "kJ/mol", 6) == 0 && !isalpha((unsigned char)s[6])) return 6;
    return 0;
}

/* ------------------------------------------------------------------ scan string */

static Token scan_string(Lexer *l) {
    int start_line = l->line, start_col = l->col;
    const char *start = l->src + l->pos - 1; /* opening '"' already consumed */
    while (peek_c(l) && peek_c(l) != '"' && peek_c(l) != '\n') {
        if (peek_c(l) == '\\') advance_c(l);
        advance_c(l);
    }
    if (peek_c(l) != '"') {
        error_at(start_line, start_col, "unterminated string literal");
        return make_tok(TOK_ERROR, start, 1, start_line, start_col);
    }
    advance_c(l); /* closing '"' */
    int len = (int)(l->src + l->pos - start);
    return make_tok(TOK_STRING, start, len, start_line, start_col);
}

/* ------------------------------------------------------------------ scan char */

static Token scan_char(Lexer *l) {
    int start_line = l->line, start_col = l->col;
    const char *start = l->src + l->pos - 1;
    if (peek_c(l) == '\'') {
        advance_c(l);
        error_at(start_line, start_col, "empty character literal");
        return make_tok(TOK_ERROR, start, (int)(l->src + l->pos - start), start_line, start_col);
    }
    if (peek_c(l) == '\\') {
        advance_c(l);
        if (!peek_c(l) || peek_c(l) == '\n') {
            error_at(start_line, start_col, "unterminated character literal");
            return make_tok(TOK_ERROR, start, (int)(l->src + l->pos - start), start_line, start_col);
        }
        advance_c(l);
    } else if (peek_c(l) && peek_c(l) != '\n' && peek_c(l) != '\'') {
        advance_c(l);
    }
    if (peek_c(l) != '\'') {
        error_at(start_line, start_col, "unterminated character literal");
        return make_tok(TOK_ERROR, start, 1, start_line, start_col);
    }
    advance_c(l);
    int len = (int)(l->src + l->pos - start);
    return make_tok(TOK_CHAR, start, len, start_line, start_col);
}

/* ------------------------------------------------------------------ scan number */

static Token scan_number(Lexer *l, const char *start, int start_line, int start_col) {
    while (isdigit((unsigned char)peek_c(l))) advance_c(l);
    if (peek_c(l) == '.' && isdigit((unsigned char)peek2_c(l))) {
        advance_c(l);
        while (isdigit((unsigned char)peek_c(l))) advance_c(l);
    }
    if (peek_c(l) == 'e' || peek_c(l) == 'E') {
        advance_c(l);
        if (peek_c(l) == '+' || peek_c(l) == '-') advance_c(l);
        if (!isdigit((unsigned char)peek_c(l))) {
            error_at(start_line, start_col, "malformed exponent in number literal");
            return make_tok(TOK_ERROR, start, (int)(l->src + l->pos - start),
                            start_line, start_col);
        }
        while (isdigit((unsigned char)peek_c(l))) advance_c(l);
    }
    int len = (int)(l->src + l->pos - start);
    Token tok = make_tok(TOK_NUMBER, start, len, start_line, start_col);
    tok.num_val = strtod(start, NULL);
    return tok;
}

/* ------------------------------------------------------------------ scan ident/keyword */

static Token scan_ident(Lexer *l, const char *start, int start_line, int start_col) {
    while (isalnum((unsigned char)peek_c(l)) || peek_c(l) == '_') advance_c(l);
    int len = (int)(l->src + l->pos - start);
    TokenType t = check_keyword(start, len);
    return make_tok(t, start, len, start_line, start_col);
}

/* ------------------------------------------------------------------ scan formula */

static Token scan_formula(Lexer *l, const char *start, int start_line, int start_col) {
    /* already consumed first uppercase letter; continue */
    if (islower((unsigned char)peek_c(l))) advance_c(l); /* optional lowercase */
    while (isdigit((unsigned char)peek_c(l))) advance_c(l); /* optional digits */
    /* additional element groups */
    while (isupper((unsigned char)peek_c(l))) {
        advance_c(l);
        if (islower((unsigned char)peek_c(l))) advance_c(l);
        while (isdigit((unsigned char)peek_c(l))) advance_c(l);
    }
    int len = (int)(l->src + l->pos - start);
    return make_tok(TOK_FORMULA, start, len, start_line, start_col);
}

/* ------------------------------------------------------------------ main entry */

Token lexer_next(Lexer *l) {
    /* skip whitespace and line comments */
    for (;;) {
        while (peek_c(l) && isspace((unsigned char)peek_c(l))) advance_c(l);
        if (peek_c(l) == '/' && peek2_c(l) == '/') {
            while (peek_c(l) && peek_c(l) != '\n') advance_c(l);
            continue;
        }
        break;
    }

    if (!peek_c(l)) {
        return make_tok(TOK_EOF, l->src + l->pos, 0, l->line, l->col);
    }

    int start_line = l->line, start_col = l->col;
    const char *start = l->src + l->pos;

    /* compound unit literals — must come before '/' operator */
    int unit_len = probe_unit_literal(l);
    if (unit_len) {
        for (int i = 0; i < unit_len; i++) advance_c(l);
        return make_tok(TOK_UNIT_LIT, start, unit_len, start_line, start_col);
    }

    char c = advance_c(l);

    /* strings and chars */
    if (c == '"') return scan_string(l);
    if (c == '\'') return scan_char(l);

    /* numbers */
    if (isdigit((unsigned char)c)) return scan_number(l, start, start_line, start_col);

    /* identifiers and hard keywords */
    if (c == '_' || islower((unsigned char)c)) return scan_ident(l, start, start_line, start_col);

    /* formula literals — uppercase start */
    if (isupper((unsigned char)c)) return scan_formula(l, start, start_line, start_col);

    /* two-character operators */
    char n = peek_c(l);
    if (c == '=' && n == '=') { advance_c(l); return make_tok(TOK_EQ_EQ,    start, 2, start_line, start_col); }
    if (c == '!' && n == '=') { advance_c(l); return make_tok(TOK_BANG_EQ,  start, 2, start_line, start_col); }
    if (c == '<' && n == '=') { advance_c(l); return make_tok(TOK_LT_EQ,    start, 2, start_line, start_col); }
    if (c == '>' && n == '=') { advance_c(l); return make_tok(TOK_GT_EQ,    start, 2, start_line, start_col); }
    if (c == '+' && n == '=') { advance_c(l); return make_tok(TOK_PLUS_EQ,  start, 2, start_line, start_col); }
    if (c == '-' && n == '=') { advance_c(l); return make_tok(TOK_MINUS_EQ, start, 2, start_line, start_col); }
    if (c == '*' && n == '=') { advance_c(l); return make_tok(TOK_STAR_EQ,  start, 2, start_line, start_col); }
    if (c == '/' && n == '=') { advance_c(l); return make_tok(TOK_SLASH_EQ, start, 2, start_line, start_col); }

    /* single-character */
    switch (c) {
        case '+': return make_tok(TOK_PLUS,      start, 1, start_line, start_col);
        case '-': return make_tok(TOK_MINUS,     start, 1, start_line, start_col);
        case '*': return make_tok(TOK_STAR,      start, 1, start_line, start_col);
        case '/': return make_tok(TOK_SLASH,     start, 1, start_line, start_col);
        case '=': return make_tok(TOK_EQ,        start, 1, start_line, start_col);
        case '<': return make_tok(TOK_LT,        start, 1, start_line, start_col);
        case '>': return make_tok(TOK_GT,        start, 1, start_line, start_col);
        case '{': return make_tok(TOK_LBRACE,    start, 1, start_line, start_col);
        case '}': return make_tok(TOK_RBRACE,    start, 1, start_line, start_col);
        case '(': return make_tok(TOK_LPAREN,    start, 1, start_line, start_col);
        case ')': return make_tok(TOK_RPAREN,    start, 1, start_line, start_col);
        case '[': return make_tok(TOK_LBRACKET,  start, 1, start_line, start_col);
        case ']': return make_tok(TOK_RBRACKET,  start, 1, start_line, start_col);
        case ',': return make_tok(TOK_COMMA,     start, 1, start_line, start_col);
        case ';': return make_tok(TOK_SEMICOLON, start, 1, start_line, start_col);
        case '.': return make_tok(TOK_DOT,       start, 1, start_line, start_col);
        default:
            error_at(start_line, start_col, "unexpected character '%c'", c);
            return make_tok(TOK_ERROR, start, 1, start_line, start_col);
    }
}

const char *token_type_name(TokenType t) {
    switch (t) {
        case TOK_IDENT:      return "IDENT";
        case TOK_NUMBER:     return "NUMBER";
        case TOK_STRING:     return "STRING";
        case TOK_CHAR:       return "CHAR";
        case TOK_FORMULA:    return "FORMULA";
        case TOK_UNIT_LIT:   return "UNIT_LIT";
        case TOK_LET:        return "'let'";
        case TOK_FUNCTION:   return "'function'";
        case TOK_RETURN:     return "'return'";
        case TOK_FOR:        return "'for'";
        case TOK_IN:         return "'in'";
        case TOK_IF:         return "'if'";
        case TOK_ELIF:       return "'elif'";
        case TOK_ELSE:       return "'else'";
        case TOK_PRINT:      return "'print'";
        case TOK_PLUS:       return "'+'";
        case TOK_MINUS:      return "'-'";
        case TOK_STAR:       return "'*'";
        case TOK_SLASH:      return "'/'";
        case TOK_PLUS_EQ:    return "'+='";
        case TOK_MINUS_EQ:   return "'-='";
        case TOK_STAR_EQ:    return "'*='";
        case TOK_SLASH_EQ:   return "'/='";
        case TOK_EQ:         return "'='";
        case TOK_EQ_EQ:      return "'=='";
        case TOK_BANG_EQ:    return "'!='";
        case TOK_LT:         return "'<'";
        case TOK_LT_EQ:      return "'<='";
        case TOK_GT:         return "'>'";
        case TOK_GT_EQ:      return "'>='";
        case TOK_LBRACE:     return "'{'";
        case TOK_RBRACE:     return "'}'";
        case TOK_LPAREN:     return "'('";
        case TOK_RPAREN:     return "')'";
        case TOK_LBRACKET:   return "'['";
        case TOK_RBRACKET:   return "']'";
        case TOK_COMMA:      return "','";
        case TOK_SEMICOLON:  return "';'";
        case TOK_DOT:        return "'.'";
        case TOK_EOF:        return "EOF";
        case TOK_ERROR:      return "ERROR";
        default:             return "?";
    }
}
