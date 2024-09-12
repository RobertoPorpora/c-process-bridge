#pragma once

extern const char *NEWLINE;

void PB_strip_newlines(char *str);

void PB_clear_string(char *str);

int PB_extract_program(const char *command, char **program);
int PB_extract_argv(const char *command, char ***argv);
void PB_free_program_and_argv(char **program, char ***argv);

char *PB_string_clone(const char *s, size_t n);
char *PB_string_clone_with_newline(const char *s, size_t n);
