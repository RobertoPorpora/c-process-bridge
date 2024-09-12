#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "PB_generic_functions.h"
#include "process_bridge.h"

PB_process_t *PB_create(PB_type_t type)
{
    PB_process_t *process = (PB_process_t *)malloc(sizeof(PB_process_t));
    if (NULL == process)
    {
        return NULL;
    }
    process->type = type;
    process->return_code = PB_DEFAULT_RETURN;

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
        PB_clear_string(process->error);
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
    if (NULL != process)
    {
        free(process);
    }
}

#ifdef _WIN32

#include <windows.h>

PB_status_t PB_spawn(PB_process_t *child, const char *command)
{
    STARTUPINFOA startInfo;
    memset(&startInfo, 0, sizeof(startInfo));
    startInfo.cb = sizeof(startInfo);
    startInfo.dwFlags = STARTF_USESTDHANDLES;

    PROCESS_INFORMATION processInfo;
    memset(&processInfo, 0, sizeof(processInfo));

    HANDLE read_h = NULL;
    HANDLE write_h = NULL;
    SECURITY_ATTRIBUTES security_attributes;
    security_attributes.bInheritHandle = TRUE;
    security_attributes.lpSecurityDescriptor = NULL;
    security_attributes.nLength = sizeof(security_attributes);

    // Create pipe for stdin
    if (!CreatePipe(&read_h, &write_h, &security_attributes, 0))
    {
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while creating child's stdin pipe. Error code: %lu", GetLastError());
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }
    startInfo.hStdInput = read_h;
    child->stdin_h = write_h;

    // Create pipe for stdout
    if (!CreatePipe(&read_h, &write_h, &security_attributes, 0))
    {
        CloseHandle(startInfo.hStdInput);
        CloseHandle(child->stdin_h);
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while creating child's stdout pipe. Error code: %lu", GetLastError());
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }
    child->stdout_h = read_h;
    startInfo.hStdOutput = write_h;

    // Create pipe for stderr
    if (!CreatePipe(&read_h, &write_h, &security_attributes, 0))
    {
        CloseHandle(startInfo.hStdInput);
        CloseHandle(child->stdin_h);
        CloseHandle(startInfo.hStdOutput);
        CloseHandle(child->stdout_h);
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while creating child's stderr pipe. Error code: %lu", GetLastError());
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }
    child->stderr_h = read_h;
    startInfo.hStdError = write_h;

    // Spawn command
    if (!CreateProcessA(
            NULL,           // lpApplicationName
            (LPSTR)command, // lpCommandLine
            NULL,           // lpProcessAttributes
            NULL,           // lpThreadAttributes
            TRUE,           // bInheritHandles
            0,              // dwCreationFlags
            NULL,           // lpEnvironment
            NULL,           // lpCurrentDirectory
            &startInfo,     // lpStartupInfo
            &processInfo    // lpProcessInformation
            ))
    {
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while creating process. Error code: %lu", GetLastError());
        CloseHandle(startInfo.hStdInput);
        CloseHandle(child->stdin_h);
        CloseHandle(startInfo.hStdOutput);
        CloseHandle(child->stdout_h);
        CloseHandle(startInfo.hStdError);
        CloseHandle(child->stderr_h);
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    // Store process handle and close unused handles
    child->process_h = processInfo.hProcess;
    CloseHandle(processInfo.hThread);

    // Close handles that are not needed in the parent process
    CloseHandle(startInfo.hStdInput);
    CloseHandle(startInfo.hStdOutput);
    CloseHandle(startInfo.hStdError);

    PB_clear_string(child->error);
    child->status = PB_STATUS_OK;
    return PB_STATUS_OK;
}

PB_status_t PB_despawn(PB_process_t *child)
{
    PB_status_t return_value = PB_STATUS_OK;
    if (!TerminateProcess(child->process_h, PB_DEFAULT_RETURN))
    {
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while terminating process.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return_value = PB_STATUS_GENERIC_ERROR;
    }
    PB_wait(child);

    if (child->process_h)
    {
        CloseHandle(child->process_h);
        child->process_h = NULL;
    }
    if (child->stdin_h)
    {
        CloseHandle(child->stdin_h);
        child->stdin_h = NULL;
    }
    return return_value;
}

PB_status_t PB_wait(PB_process_t *child)
{
    if (NULL == child)
    {
        return PB_STATUS_USAGE_ERROR;
    }

    if (child->stdin_h)
    {
        CloseHandle(child->stdin_h);
        child->stdin_h = NULL;
    }

    const DWORD INFINITE_TIME = 0xFFFFFFFF;
    WaitForSingleObject(child->process_h, INFINITE_TIME);

    if (!GetExitCodeProcess(child->process_h, &(child->return_code)))
    {
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while getting the process' exit code.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    child->status = PB_STATUS_OK;
    return PB_STATUS_OK;
}

#else // Unix

#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <signal.h>
extern char **environ;

PB_status_t PB_spawn(PB_process_t *child, const char *command)
{
    // Command line setup
    char *program = NULL;
    char **argv = NULL;
    if (PB_extract_program(command, &program))
    {
        PB_free_program_and_argv(&program, &argv);
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while extracting program from command string.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }
    if (PB_extract_argv(command, &argv))
    {
        PB_free_program_and_argv(&program, &argv);
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while extracting arguments from command string.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    // Pipes setup
    const size_t NUMBER_OF_SIDES = 2;
    const size_t WRITE_SIDE = 1;
    const size_t READ_SIDE = 0;
    int stdin_pipe[NUMBER_OF_SIDES];
    int stdout_pipe[NUMBER_OF_SIDES];
    int stderr_pipe[NUMBER_OF_SIDES];

    bool stdin_pipe_error = -1 == pipe(stdin_pipe);
    bool stdout_pipe_error = -1 == pipe(stdout_pipe);
    bool stderr_pipe_error = -1 == pipe(stderr_pipe);

    if (stdin_pipe_error || stdout_pipe_error || stderr_pipe_error)
    {
        PB_free_program_and_argv(&program, &argv);
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while creating pipes.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    // Actions setup...
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    // ...make duplicates of pipes for child.
    posix_spawn_file_actions_adddup2(&actions, stdin_pipe[READ_SIDE], STDIN_FILENO);
    posix_spawn_file_actions_adddup2(&actions, stdout_pipe[WRITE_SIDE], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&actions, stderr_pipe[WRITE_SIDE], STDERR_FILENO);
    // ...tell child to close its unused pipe sides.
    posix_spawn_file_actions_addclose(&actions, stdin_pipe[WRITE_SIDE]);
    posix_spawn_file_actions_addclose(&actions, stdout_pipe[READ_SIDE]);
    posix_spawn_file_actions_addclose(&actions, stderr_pipe[READ_SIDE]);

    // Spawn command
    if (posix_spawn(&child->pid, program, &actions, NULL, argv, environ))
    {
        PB_free_program_and_argv(&program, &argv);
        posix_spawn_file_actions_destroy(&actions);
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while extracting arguments from command string.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    // Free memory that is not needed anymore
    PB_free_program_and_argv(&program, &argv);
    posix_spawn_file_actions_destroy(&actions);

    // Close pipe sides that are unused by parent
    close(stdin_pipe[READ_SIDE]);
    close(stdout_pipe[WRITE_SIDE]);
    close(stderr_pipe[WRITE_SIDE]);

    // Save pipe sides that are used by parent
    child->stdin_fd = stdin_pipe[WRITE_SIDE];
    child->stdout_fd = stdout_pipe[READ_SIDE];
    child->stderr_fd = stderr_pipe[READ_SIDE];

    PB_clear_string(child->error);
    child->status = PB_STATUS_OK;
    return PB_STATUS_OK;
}

PB_status_t PB_despawn(PB_process_t *child)
{
    if (NULL == child)
    {
        return PB_STATUS_USAGE_ERROR;
    }

    bool close_stdin_error = -1 == close(child->stdin_fd);
    bool close_stdout_error = -1 == close(child->stdout_fd);
    bool close_stderr_error = -1 == close(child->stderr_fd);

    if (close_stdin_error || close_stdout_error || close_stderr_error)
    {
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while closing file descriptors.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    if (-1 == kill(child->pid, SIGKILL))
    {
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while killing process");
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    return PB_STATUS_OK;
}

PB_status_t PB_wait(PB_process_t *child)
{
    if (NULL == child)
    {
        return PB_STATUS_USAGE_ERROR;
    }

    int status;
    if (-1 == waitpid(child->pid, &status, 0))
    {
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while waiting for process.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    if (WIFEXITED(status))
    {
        child->return_code = WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status))
    {
        child->return_code = PB_DEFAULT_RETURN;
    }
    else
    {
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Unknown error after waiting for process.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    return PB_STATUS_OK;
}

#endif

void PB_clear_error(PB_process_t *process)
{
    if (process)
    {
        PB_clear_string(process->error);
        process->status = PB_STATUS_OK;
    }
}