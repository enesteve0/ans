#ifndef ANS_LEXER_H
#define ANS_LEXER_H

typedef enum {
    /* Literals */
    TOK_IDENT,
    TOK_NUMBER,
    TOK_STRING,
    TOK_CHAR,
    TOK_FORMULA,
    TOK_UNIT_LIT,       /* g/mol  kg/mol  kJ/mol — tried before '/' operator */

    /* Hard keywords */
    TOK_LET,
    TOK_FUNCTION,
    TOK_RETURN,
    TOK_FOR,
    TOK_IN,
    TOK_IF,
    TOK_ELIF,
    TOK_ELSE,
    TOK_PRINT,

    /* Operators */
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PLUS_EQ,
    TOK_MINUS_EQ,
    TOK_STAR_EQ,
    TOK_SLASH_EQ,
    TOK_EQ,
    TOK_EQ_EQ,
    TOK_BANG_EQ,
    TOK_LT,
    TOK_LT_EQ,
    TOK_GT,
    TOK_GT_EQ,

    /* Separators */
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_COMMA,
    TOK_SEMICOLON,
    TOK_DOT,

    TOK_EOF,
    TOK_ERROR
} TokenType;

typedef struct {
    TokenType   type;
    const char *start;   /* pointer into source buffer */
    int         length;
    int         line;
    int         col;
    double      num_val; /* filled for TOK_NUMBER */
} Token;

typedef struct {
    const char *src;
    int         pos;
    int         line;
    int         col;
} Lexer;

void  lexer_init(Lexer *l, const char *source);
Token lexer_next(Lexer *l);

const char *token_type_name(TokenType t);

#endif /* ANS_LEXER_H */
