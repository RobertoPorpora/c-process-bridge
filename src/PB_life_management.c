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

#include <Windows.h>
#include <io.h>
#include <fcntl.h>

PB_status_t PB_spawn(PB_process_t *child, const char *command)
{
    // Process creation setup
    STARTUPINFOA startInfo;
    PROCESS_INFORMATION processInfo;
    memset(&startInfo, 0, sizeof(startInfo));
    startInfo.cb = sizeof(startInfo);
    startInfo.dwFlags = STARTF_USESTDHANDLES;

    memset(&processInfo, 0, sizeof(processInfo));

    // Setup pipes

    int fd;
    HANDLE rd, wr;

    if (!CreatePipe(&rd, &wr, NULL, 0))
    {
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while creating child's stdin / stdout pipe.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    // Setup child's stdin
    fd = _open_osfhandle((intptr_t)wr, 0);
    if (-1 == fd)
    {
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while opening child's stdin handle.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }
    child->stdin_f = _fdopen(fd, "wb");
    if (NULL == child->stdin_f)
    {
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while opening child's stdin file.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }
    startInfo.hStdInput = rd;

    // Setup child's stdout
    fd = _open_osfhandle((intptr_t)rd, 0);
    if (-1 == fd)
    {
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while opening child's stdout handle.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }
    child->stdout_f = _fdopen(fd, "rb");
    if (NULL == child->stdout_f)
    {
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while opening child's stdout file.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }
    startInfo.hStdOutput = wr;

    // Setup child's stderr
    if (!CreatePipe(&rd, &wr, NULL, 0))
    {
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while creating child's stderr pipe.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }
    fd = _open_osfhandle((intptr_t)rd, 0);
    if (-1 == fd)
    {
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while opening child's stdout handle.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }
    child->stderr_f = _fdopen(fd, "rb");
    if (NULL == child->stderr_f)
    {
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while opening child's stderr file.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }
    startInfo.hStdError = wr;

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
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while creating process.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
    }

    child->process_h = processInfo.hProcess;
    child->stdin_h = startInfo.hStdInput;

    // CloseHandle(processInfo.hThread);

    if (NULL != startInfo.hStdOutput)
    {
        CloseHandle(startInfo.hStdOutput);
    }
    if (NULL != startInfo.hStdError)
    {
        CloseHandle(startInfo.hStdError);
    }

    PB_clear_string(child->error);
    child->status = PB_STATUS_OK;
    return PB_STATUS_OK;
}

PB_status_t PB_despawn(PB_process_t *child)
{
    PB_status_t return_value = PB_STATUS_OK;
    if (!TerminateProcess(child->process_h, 99))
    {
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Error while terminating process.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return_value = PB_STATUS_GENERIC_ERROR;
    }
    PB_wait(child);
    if (child->stdin_f)
    {
        fclose(child->stdin_f);
        child->stdin_f = NULL;
    }
    if (child->stdout_f)
    {
        fclose(child->stdout_f);
        child->stdout_f = NULL;
    }
    if (child->stderr_f)
    {
        fclose(child->stderr_f);
        child->stderr_f = NULL;
    }
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

    if (child->stdin_f)
    {
        fclose(child->stdin_f);
        child->stdin_f = NULL;
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
        snprintf(child->error, PB_STRING_SIZE_DEFAULT, "Process terminated by signal.");
        child->status = PB_STATUS_GENERIC_ERROR;
        return PB_STATUS_GENERIC_ERROR;
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