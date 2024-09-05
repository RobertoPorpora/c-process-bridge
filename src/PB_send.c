#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "PB_generic_functions.h"
#include "process_bridge.h"

//------------------------------------------------------------------------------

static PB_status_t send_dispatcher(PB_process_t *process, const char *message, bool is_err);

static PB_status_t send_to_parent(PB_process_t *process, const char *message, bool is_err);
static PB_status_t send_to_child(PB_process_t *process, const char *message);

//------------------------------------------------------------------------------

PB_status_t PB_send(PB_process_t *process, const char *message)
{
    return send_dispatcher(process, message, false);
}

PB_status_t PB_send_err(PB_process_t *process, const char *message)
{
    return send_dispatcher(process, message, true);
}

//------------------------------------------------------------------------------

static PB_status_t send_dispatcher(PB_process_t *process, const char *message, bool is_err)
{
    if (NULL == process)
    {
        return PB_STATUS_USAGE_ERROR;
    }
    switch (process->type)
    {
    case PB_TYPE_PARENT:
        return send_to_parent(process, message, is_err);
        break;
    case PB_TYPE_CHILD:
        return send_to_child(process, message);
        break;
    default:
        snprintf(process->error, PB_STRING_SIZE_DEFAULT, "Not implemented");
        process->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
        break;
    }
    return PB_STATUS_USAGE_ERROR;
}

//------------------------------------------------------------------------------

static PB_status_t send_to_parent(PB_process_t *process, const char *message, bool is_err)
{
    if (NULL == process)
    {
        return PB_STATUS_USAGE_ERROR;
    }

    if (NULL == message)
    {
        snprintf(process->error, PB_STRING_SIZE_DEFAULT, "Message argument is NULL in send_to_parent call");
        process->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    if (is_err)
    {
        if (fprintf(stderr, "%s%s", message, NEWLINE))
        {
            strncpy(process->error, "Could not print to stderr", PB_STRING_SIZE_DEFAULT);
            process->status = PB_STATUS_GENERIC_ERROR;
            return PB_STATUS_GENERIC_ERROR;
        }
    }
    else
    {
        if (fprintf(stdout, "%s%s", message, NEWLINE))
        {
            strncpy(process->error, "Could not print to stdout", PB_STRING_SIZE_DEFAULT);
            process->status = PB_STATUS_GENERIC_ERROR;
            return PB_STATUS_GENERIC_ERROR;
        }
    }

    return PB_STATUS_OK;
}

//------------------------------------------------------------------------------

#ifdef _WIN32

#include <windows.h>
#include <io.h>
#include <fcntl.h>

static PB_status_t send_to_child(PB_process_t *process, const char *message)
{
    if (NULL == process)
    {
        return PB_STATUS_USAGE_ERROR;
    }

    if (NULL == message)
    {
        snprintf(process->error, PB_STRING_SIZE_DEFAULT, "Message argument is NULL in send_to_child call");
        process->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    char *msg_copy = PB_string_clone_with_newline(message, strlen(message));
    if (msg_copy == NULL)
    {
        snprintf(process->error, PB_STRING_SIZE_DEFAULT, "Memory allocation error");
        process->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    fputs(msg_copy, process->stdin_f);
    fflush(process->stdin_f);

    // DWORD bytes_written = 0;
    // HANDLE handle = (HANDLE)_get_osfhandle(_fileno(process->stdin_f));
    // if (!WriteFile(handle, msg_copy, (DWORD)strlen(msg_copy), &bytes_written, NULL))
    // {
    //     free(msg_copy);
    //     strncpy(process->error, "Couldn't write to child's stdin", PB_STRING_SIZE_DEFAULT);
    //     process->status = PB_STATUS_GENERIC_ERROR;
    //     return PB_STATUS_GENERIC_ERROR;
    // }

    // if (!FlushFileBuffers(handle))
    // {
    //     free(msg_copy);
    //     strncpy(process->error, "Couldn't flush child's stdin", PB_STRING_SIZE_DEFAULT);
    //     process->status = PB_STATUS_GENERIC_ERROR;
    //     return PB_STATUS_GENERIC_ERROR;
    // }

    free(msg_copy);
    return PB_STATUS_OK;
}

#else // Unix

#include <unistd.h>

static PB_status_t send_to_child(PB_process_t *process, const char *message)
{
    if (NULL == process)
    {
        return PB_STATUS_USAGE_ERROR;
    }

    if (NULL == message)
    {
        snprintf(process->error, PB_STRING_SIZE_DEFAULT, "Message argument is NULL in send_to_child call");
        process->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    char *msg_copy = strdup(message);
    strcat(msg_copy, NEWLINE);

    ssize_t bytes_written = write(process->stdin_fd, msg_copy, strlen(msg_copy));
    if (bytes_written == -1)
    {
        free(msg_copy);
        strncpy(process->error, "Couldn't write to child's stdin", PB_STRING_SIZE_DEFAULT);
        process->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    if (fsync(process->stdin_fd))
    {
        free(msg_copy);
        strncpy(process->error, "Couldn't fsync child's stdin", PB_STRING_SIZE_DEFAULT);
        process->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    free(msg_copy);
    return PB_STATUS_OK;
}

#endif
