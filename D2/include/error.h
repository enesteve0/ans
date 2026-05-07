#ifndef ANS_ERROR_H
#define ANS_ERROR_H

extern const char *g_filename;
extern int g_had_error;

void error_at(int line, int col, const char *fmt, ...);
void fatal_at(int line, int col, const char *fmt, ...);

#endif /* ANS_ERROR_H */
