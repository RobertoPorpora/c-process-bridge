#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <process_bridge.h>

int main()
{
    bool success = true;
    char buf_in[200];
    char buf_in_err[200];
    char buf_out[200];
    PB_process_t *user = PB_create(PB_TYPE_PARENT);
    PB_process_t *child = PB_create(PB_TYPE_CHILD);

#ifdef _WIN32
    const char CHILD_COMMAND[] = "../bin/test_process_bridge_child.exe";
#else
    const char CHILD_COMMAND[] = "../bin/test_process_bridge_child";
#endif

    PB_spawn(child, CHILD_COMMAND);

    PB_send(child, "p1");
    PB_send(child, "p2");
    PB_send(child, "p3");

    PB_receive(child, &buf_in[0], 50);
    strcat(buf_in, " ");
    PB_receive_err(child, &buf_in_err[0], 50);
    strcat(buf_in_err, " ");
    PB_receive(child, &buf_in[strlen(buf_in)], 50);
    strcat(buf_in, " ");
    PB_receive_err(child, &buf_in_err[strlen(buf_in_err)], 50);
    strcat(buf_in_err, " ");
    PB_receive(child, &buf_in[strlen(buf_in)], 50);
    PB_receive_err(child, &buf_in_err[strlen(buf_in_err)], 50);

    PB_despawn(child);

    if (strncmp(buf_in, "c1 p1 p2 p3 c2 c3", 50))
    {
        PB_send(user, buf_in);
        PB_send(user, "ERROR: buf_in not as expected");
        success = false;
    }

    if (strncmp(buf_in_err, "c1 p1 p2 p3 c2 c3", 50))
    {
        PB_send(user, buf_in_err);
        PB_send(user, "ERROR: buf_in_err not as expected");
        success = false;
    }

    if (child->return_code != PB_DEFAULT_RETURN)
    {
        snprintf(buf_out, 50, "ERROR: return code = %d.", child->return_code);
        PB_send(user, buf_out);
        success = false;
    }

    //--------------------------------------------------------------------------

    PB_destroy(child);
    child = PB_create(PB_TYPE_CHILD);

    PB_spawn(child, CHILD_COMMAND);

    PB_send(child, "p1");
    PB_send(child, "p2");
    PB_send(child, "p3");

    PB_receive(child, &buf_in[0], 50);
    strcat(buf_in, " ");
    PB_receive_err(child, &buf_in_err[0], 50);
    strcat(buf_in_err, " ");
    PB_receive(child, &buf_in[strlen(buf_in)], 50);
    strcat(buf_in, " ");
    PB_receive_err(child, &buf_in_err[strlen(buf_in_err)], 50);
    strcat(buf_in_err, " ");
    PB_receive(child, &buf_in[strlen(buf_in)], 50);
    PB_receive_err(child, &buf_in_err[strlen(buf_in_err)], 50);

    PB_wait(child);

    if (strncmp(buf_in, "c1 p1 p2 p3 c2 c3", 50))
    {
        PB_send(user, buf_in);
        PB_send(user, "ERROR: buf_in not as expected");
        success = false;
    }

    if (strncmp(buf_in_err, "c1 p1 p2 p3 c2 c3", 50))
    {
        PB_send(user, buf_in_err);
        PB_send(user, "ERROR: buf_in_err not as expected");
        success = false;
    }

    if (child->return_code != 12)
    {
        snprintf(buf_out, 50, "ERROR: return code = %d.", child->return_code);
        PB_send(user, buf_out);
        success = false;
    }

    //--------------------------------------------------------------------------

    if (success)
    {
        PB_send(user, "All tests passed successfully.");
    }

    PB_destroy(user);
    PB_destroy(child);
}
