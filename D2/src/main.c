#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "error.h"
#include "semantics.h"

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *buf = malloc((size_t)size + 1);
    if (!buf) { perror("malloc"); exit(1); }
    fread(buf, 1, (size_t)size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

int main(int argc, char **argv) {
    int dump_ast   = 0;
    int parse_only = 0;
    int check_only = 0;
    const char *filepath = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dump-ast") == 0) {
            dump_ast = 1;
        } else if (strcmp(argv[i], "--parse-only") == 0) {
            parse_only = 1;
        } else if (strcmp(argv[i], "--check") == 0) {
            check_only = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "unknown option: %s\n", argv[i]);
            fprintf(stderr, "usage: ans [--dump-ast|--parse-only|--check] <file.ans>\n");
            return 1;
        } else {
            filepath = argv[i];
        }
    }

    if (!filepath) {
        fprintf(stderr, "usage: ans [--dump-ast|--parse-only|--check] <file.ans>\n");
        return 1;
    }

    g_filename = filepath;
    char *source = read_file(filepath);

    Lexer  lexer;
    Parser parser;
    lexer_init(&lexer, source);
    parser_init(&parser, &lexer);

    AstNode *tree = parse_program(&parser);

    if (!g_had_error) {
        if (dump_ast) {
            ast_print(tree, 0);
        } else if (!parse_only) {
            int check_result = ans_check_program(tree);
            if (!check_result && !check_only) {
                ans_execute_program(tree);
            }
        }
    }

    ast_free(tree);
    free(source);
    return g_had_error ? 1 : 0;
}
