#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#endif

// -----------------------------------------------------------------------------
// Types
// -----------------------------------------------------------------------------

#define PB_STRING_SIZE_DEFAULT 200

typedef enum
{
    PB_TYPE_PARENT = 0,
    PB_TYPE_CHILD = 1,
} PB_type_t;

typedef enum
{
    PB_STATUS_OK = 0,
    PB_STATUS_NOT_SPAWNED,
    PB_STATUS_COMPLETED,
    PB_STATUS_TERMINATED,
    PB_STATUS_GENERIC_ERROR,
    PB_STATUS_USAGE_ERROR,
} PB_status_t;

#ifdef _WIN32
typedef DWORD PB_return_t;
#else
typedef uint8_t PB_return_t;
#endif

static const PB_return_t PB_DEFAULT_RETURN = 0xFF;

typedef struct PB_process_t
{
    PB_type_t type;
    PB_status_t status;
    char error[PB_STRING_SIZE_DEFAULT];
    PB_return_t return_code;
#ifdef _WIN32
    HANDLE process_h;
    HANDLE stdin_h;
    HANDLE stdout_h;
    HANDLE stderr_h;
#else
    pid_t pid;
    int stdin_fd;
    int stdout_fd;
    int stderr_fd;
#endif
} PB_process_t;

// -----------------------------------------------------------------------------
// Create / Destroy functions
// -----------------------------------------------------------------------------

PB_process_t *PB_create(PB_type_t);
void PB_destroy(PB_process_t *);

// -----------------------------------------------------------------------------
// Process life management
// -----------------------------------------------------------------------------

PB_status_t PB_spawn(PB_process_t *, const char *);
PB_status_t PB_despawn(PB_process_t *);
PB_status_t PB_wait(PB_process_t *);

// -----------------------------------------------------------------------------
// Process communications
// -----------------------------------------------------------------------------

PB_status_t PB_send(PB_process_t *, const char *message);
PB_status_t PB_send_err(PB_process_t *, const char *message);
PB_status_t PB_receive(PB_process_t *, char *mailbox, size_t);
PB_status_t PB_receive_err(PB_process_t *, char *mailbox, size_t);

// -----------------------------------------------------------------------------
// Errors management
// -----------------------------------------------------------------------------

void PB_clear_error(PB_process_t *);