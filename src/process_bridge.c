#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include "process_bridge.h"

#ifdef _WIN32
const char *NEWLINE = "\r\n";
#else
const char *NEWLINE = "\n";
#endif

// -----------------------------------------------------------------------------
// Create / Destroy functions
// -----------------------------------------------------------------------------

PB_process_t *PB_create(PB_type_t type)
{
    PB_process_t *process = (PB_process_t *)malloc(sizeof(PB_process_t));
    if (process == NULL)
    {
        return NULL;
    }
    process->type = type;

    switch (type)
    {
    case PB_TYPE_CHILD:
        process->status = PB_STATUS_NOT_SPAWNED;
        snprintf(process->error, PB_STRING_SIZE_DEFAULT, "Child not spawned");
        break;
    case PB_TYPE_PARENT:
        setvbuf(stdin, NULL, _IONBF, 0);
        setvbuf(stdout, NULL, _IONBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);
        snprintf(process->error, PB_STRING_SIZE_DEFAULT, "");
        process->status = PB_STATUS_OK;
        break;
    default:
        process->status = PB_STATUS_USAGE_ERROR;
        snprintf(process->error, PB_STRING_SIZE_DEFAULT, "Unsupported process type.");
        break;
    }

    return process;
}

void PB_destroy(PB_process_t *process)
{
    if (process != NULL)
    {
        free(process);
    }
}

// -----------------------------------------------------------------------------
// Process life management
// -----------------------------------------------------------------------------

PB_status_t PB_spawn(PB_process_t *child, const char *command)
{
    snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Not implemented.");
    child->status = PB_STATUS_GENERIC_ERROR;
    return PB_STATUS_GENERIC_ERROR;
}

PB_status_t PB_despawn(PB_process_t *child)
{
    snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Not implemented.");
    child->status = PB_STATUS_GENERIC_ERROR;
    return PB_STATUS_GENERIC_ERROR;
}

PB_status_t PB_wait(PB_process_t *child)
{
    snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Not implemented.");
    child->status = PB_STATUS_GENERIC_ERROR;
    return PB_STATUS_GENERIC_ERROR;
}

// -----------------------------------------------------------------------------
// Process communications
// -----------------------------------------------------------------------------

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
    if (process == NULL)
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
        snprintf(process->error, PB_STRING_SIZE_DEFAULT, "Not implemented.");
        process->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
        break;
    }
    return PB_STATUS_USAGE_ERROR;
}

static PB_status_t send_to_parent(PB_process_t *process, const char *message, bool is_err)
{
    if (process == NULL)
    {
        return PB_STATUS_USAGE_ERROR;
    }

    if (message == NULL)
    {
        snprintf(process->error, PB_STRING_SIZE_DEFAULT, "Message argument is NULL");
        process->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    int result;
    if (is_err)
    {
        result = fprintf(stderr, "%s%s", message, NEWLINE);
    }
    else
    {
        result = fprintf(stdout, "%s%s", message, NEWLINE);
    }

    if (result < 0)
    {
        snprintf(process->error, PB_STRING_SIZE_DEFAULT, strerror(errno));
        process->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    snprintf(process->error, PB_STRING_SIZE_DEFAULT, "");
    process->status = PB_STATUS_OK;
    return PB_STATUS_OK;
}

static PB_status_t send_to_child(PB_process_t *process, const char *message)
{
    if (process == NULL)
    {
        return PB_STATUS_USAGE_ERROR;
    }
    snprintf(process->error, PB_STRING_SIZE_DEFAULT, "Not implemented.");
    process->status = PB_STATUS_GENERIC_ERROR;
    return PB_STATUS_GENERIC_ERROR;
}

//------------------------------------------------------------------------------

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
    if (process == NULL)
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
        snprintf(process->error, PB_STRING_SIZE_DEFAULT, "Not implemented.");
        process->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
        break;
    }
    return PB_STATUS_USAGE_ERROR;
}

PB_status_t receive_from_parent(PB_process_t *process, char *mailbox, size_t size)
{
    if (process == NULL)
    {
        return PB_STATUS_USAGE_ERROR;
    }

    if (mailbox == NULL)
    {
        snprintf(process->error, PB_STRING_SIZE_DEFAULT, "Mailbox argument is NULL");
        process->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    if (fgets(mailbox, size, stdin) == NULL)
    {
        if (feof(stdin))
        {
            snprintf(process->error, PB_STRING_SIZE_DEFAULT, "End of file reached");
        }
        else if (ferror(stdin))
        {
            snprintf(process->error, PB_STRING_SIZE_DEFAULT, strerror(errno));
        }
        else
        {
            snprintf(process->error, PB_STRING_SIZE_DEFAULT, "Unknown error");
        }
        process->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    size_t len = strlen(mailbox);
    if (len > 0)
    {
        if (mailbox[len - 1] == '\n')
        {
            mailbox[len - 1] = '\0';
        }
        else if (len > 1 && mailbox[len - 2] == '\r' && mailbox[len - 1] == '\n')
        {
            mailbox[len - 1] = '\0';
            mailbox[len - 2] = '\0';
        }
    }

    snprintf(process->error, PB_STRING_SIZE_DEFAULT, "");
    process->status = PB_STATUS_OK;
    return PB_STATUS_OK;
}

static PB_status_t receive_from_child(PB_process_t *process, char *mailbox, size_t size, bool is_err)
{
    if (process == NULL)
    {
        return PB_STATUS_USAGE_ERROR;
    }
    snprintf(process->error, PB_STRING_SIZE_DEFAULT, "Not implemented.");
    process->status = PB_STATUS_GENERIC_ERROR;
    return PB_STATUS_GENERIC_ERROR;
}
