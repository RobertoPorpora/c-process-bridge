#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "PB_generic_functions.h"

#ifdef _WIN32
const char *NEWLINE = "\r\n";
const size_t NEWLINE_LEN = 2;
#else // Unix
const char *NEWLINE = "\n";
const size_t NEWLINE_LEN = 1;
#endif

char *PB_string_clone(const char *s, size_t n)
{
    if (NULL == s)
    {
        return NULL;
    }
    size_t len = strnlen(s, n);
    char *clone = (char *)malloc(len + 1);
    if (NULL == clone)
    {
        return NULL;
    }
    strncpy(clone, s, len);
    clone[len] = '\0';
    return clone;
}

char *PB_string_clone_with_newline(const char *s, size_t n)
{
    if (NULL == s)
    {
        return NULL;
    }
    size_t len = strnlen(s, n);
    char *clone = (char *)malloc(len + NEWLINE_LEN + 1);
    if (NULL == clone)
    {
        return NULL;
    }
    strncpy(clone, s, len);
    clone[len] = '\0';
    strcat(clone, NEWLINE);
    return clone;
}

void PB_strip_newlines(char *str)
{
    size_t len = strlen(str);
    if (len > 0)
    {
        if (str[len - 1] == '\n')
        {
            str[len - 1] = '\0';
        }
        if (len > 1)
        {
            if (str[len - 2] == '\r')
            {
                str[len - 2] = '\0';
            }
        }
    }
}

void PB_clear_string(char *str)
{
    memset(str, 0, strlen(str));
}

int PB_extract_program(const char *command, char **program)
{
    while (*command == ' ')
        command++;
    const char *start = command;
    while (*command && *command != ' ')
        command++;

    size_t len = command - start;
    *program = PB_string_clone(start, len);
    if (*program == NULL)
    {
        return 1;
    }
    return 0;
}

int PB_extract_argv(const char *command, char ***argv)
{
    *argv = NULL;
    size_t argc = 0;
    char *cmd_copy = strdup(command);
    if (cmd_copy == NULL)
    {
        return 1;
    }

    char *ptr = cmd_copy;
    char *token;

    while (*ptr)
    {
        while (*ptr == ' ')
            ptr++;

        if (*ptr == '"')
        {
            ptr++;
            token = ptr;
            while (*ptr && *ptr != '"')
                ptr++;
        }
        else
        {
            token = ptr;
            while (*ptr && *ptr != ' ')
                ptr++;
        }

        if (*ptr)
        {
            *ptr++ = '\0';
        }

        char **new_argv = realloc(*argv, (argc + 2) * sizeof(char *));
        if (new_argv == NULL)
        {
            free(cmd_copy);
            return 1;
        }
        *argv = new_argv;

        (*argv)[argc] = strdup(token);
        if ((*argv)[argc] == NULL)
        {
            free(cmd_copy);
            return 1;
        }
        argc++;
        (*argv)[argc] = NULL;
    }

    free(cmd_copy);
    return 0;
}

void PB_free_program_and_argv(char **program, char ***argv)
{
    if (*program)
        free(*program);
    for (size_t i = 0; (*argv)[i]; i++)
    {
        free((*argv)[i]);
    }
    if (*argv)
        free(*argv);
}