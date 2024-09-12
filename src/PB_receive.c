
#include <stdio.h>
#include <string.h>

#include "PB_generic_functions.h"
#include "process_bridge.h"

static PB_status_t receive_dispatcher(PB_process_t *process, char *message, size_t size, bool is_err);

static PB_status_t receive_from_parent(PB_process_t *process, char *message, size_t size);
static PB_status_t receive_from_child(PB_process_t *process, char *message, size_t size, bool is_err);

//------------------------------------------------------------------------------

PB_status_t PB_receive(PB_process_t *process, char *mailbox, size_t size)
{
    return receive_dispatcher(process, mailbox, size, false);
}

PB_status_t PB_receive_err(PB_process_t *process, char *mailbox, size_t size)
{
    return receive_dispatcher(process, mailbox, size, true);
}

//------------------------------------------------------------------------------

static PB_status_t receive_dispatcher(PB_process_t *process, char *mailbox, size_t size, bool is_err)
{
    if (NULL == process)
    {
        return PB_STATUS_USAGE_ERROR;
    }
    switch (process->type)
    {
    case PB_TYPE_PARENT:
        return receive_from_parent(process, mailbox, size);
        break;
    case PB_TYPE_CHILD:
        return receive_from_child(process, mailbox, size, is_err);
        break;
    default:
        snprintf(process->error, PB_STRING_SIZE_DEFAULT, "Not implemented");
        process->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
        break;
    }
    return PB_STATUS_USAGE_ERROR;
}

PB_status_t receive_from_parent(PB_process_t *process, char *mailbox, size_t size)
{
    if (NULL == process)
    {
        return PB_STATUS_USAGE_ERROR;
    }

    if (NULL == mailbox)
    {
        snprintf(process->error, PB_STRING_SIZE_DEFAULT, "Mailbox argument is NULL in receive_from_parent call");
        process->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    char *ptr = mailbox;

    if (NULL == fgets(mailbox, PB_STRING_SIZE_DEFAULT, stdin))
    {
        snprintf(process->error, PB_STRING_SIZE_DEFAULT, "fgets() had an error or reached EOF");
        process->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    PB_strip_newlines(mailbox);

    return PB_STATUS_OK;
}

#ifdef _WIN32

#include <windows.h>
#include <io.h>
#include <fcntl.h>

static PB_status_t receive_from_child(PB_process_t *process, char mailbox[PB_STRING_SIZE_DEFAULT], size_t size, bool is_err)
{
    if (NULL == process)
    {
        return PB_STATUS_USAGE_ERROR;
    }

    if (NULL == mailbox)
    {
        strncpy(process->error, "Mailbox is NULL in receive_from_child call", PB_STRING_SIZE_DEFAULT);
        process->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    char buf[2];
    size_t index = 0;

    HANDLE handle;
    if (is_err)
    {
        handle = process->stderr_h;
    }
    else
    {
        handle = process->stdout_h;
    }

    while (true)
    {
        DWORD bytes_read;
        BOOL result = ReadFile(handle, buf, 1, &bytes_read, NULL);
        if (!result)
        {
            DWORD error = GetLastError();
            snprintf(process->error, PB_STRING_SIZE_DEFAULT, "Error while reading from child's %s.", is_err ? "stderr" : "stdout");
            process->status = PB_STATUS_GENERIC_ERROR;
            return PB_STATUS_GENERIC_ERROR;
        }
        else if (bytes_read != 0)
        {
            mailbox[index] = buf[0];
            index++;
            if (index >= size - 1 || buf[0] == '\n')
            {
                mailbox[index] = '\0';
                break;
            }
        }
    }

    PB_strip_newlines(mailbox);

    return PB_STATUS_OK;
}

#else // Unix

#include <unistd.h>

static PB_status_t receive_from_child(PB_process_t *process, char mailbox[PB_STRING_SIZE_DEFAULT], size_t size, bool is_err)
{
    if (NULL == process)
    {
        return PB_STATUS_USAGE_ERROR;
    }

    if (NULL == mailbox)
    {
        strncpy(process->error, "Mailbox is NULL in receive_from_child call", PB_STRING_SIZE_DEFAULT);
        process->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    char buf[2];
    char index = 0;

    int fd = is_err ? process->stderr_fd : process->stdout_fd;

    while (true)
    {
        ssize_t bytes_read = read(fd, buf, 1);
        if (-1 == bytes_read)
        {
            snprintf(process->error, PB_STRING_SIZE_DEFAULT, "Error while reading from child's %s.", is_err ? "stderr" : "stdout");
            process->status = PB_STATUS_GENERIC_ERROR;
            return PB_STATUS_GENERIC_ERROR;
        }
        else if (0 != bytes_read)
        {
            mailbox[index] = buf[0];
            index++;
            if ('\n' == buf[0])
            {
                mailbox[index] = 0;
                break;
            }
        }
    }

    PB_strip_newlines(mailbox);

    return PB_STATUS_OK;
}

#endif
