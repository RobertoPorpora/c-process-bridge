#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// -----------------------------------------------------------------------------
// Types
// -----------------------------------------------------------------------------

#define PB_DEFAULT_BUFSIZE 200

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

typedef uint8_t PB_return_t;

typedef struct PB_process_t
{
    PB_type_t type;
    PB_status_t status;
    char error[PB_DEFAULT_BUFSIZE];
    PB_return_t return_code;
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