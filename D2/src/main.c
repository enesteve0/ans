#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "error.h"
#include "checker.h"
#include "interpreter.h"

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
    int dump_ast = 0;
    int do_check = 0;
    int do_run   = 0;
    const char *filepath = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dump-ast") == 0) {
            dump_ast = 1;
        } else if (strcmp(argv[i], "--check") == 0) {
            do_check = 1;
        } else if (strcmp(argv[i], "--run") == 0) {
            do_check = 1;
            do_run   = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "unknown option: %s\n", argv[i]);
            fprintf(stderr, "usage: ans [--dump-ast] [--check] [--run] <file.ans>\n");
            return 1;
        } else {
            filepath = argv[i];
        }
    }

    if (!filepath) {
        fprintf(stderr, "usage: ans [--dump-ast] [--check] [--run] <file.ans>\n");
        return 1;
    }

    g_filename = filepath;
    char *source = read_file(filepath);

    Lexer  lexer;
    Parser parser;
    lexer_init(&lexer, source);
    parser_init(&parser, &lexer);

    AstNode *tree = parse_program(&parser);

    if (dump_ast) ast_print(tree, 0);

    if (do_check && !g_had_error) {
        check_program(tree);
    }

    if (do_run && !g_had_error) {
        interpret_program(tree);
    }

    ast_free(tree);
    free(source);
    return g_had_error ? 1 : 0;
}
