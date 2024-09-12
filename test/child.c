#include <stdio.h>
#include <string.h>

#include <process_bridge.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

void sleep_for_one_second()
{
#ifdef _WIN32
    Sleep(1000);
#else
    sleep(1);
#endif
}

int main()
{
    PB_process_t *parent = PB_create(PB_TYPE_PARENT);
    char buf[PB_STRING_SIZE_DEFAULT];

    strcpy(buf, "c1 ");
    PB_receive(parent, &(buf[strlen(buf)]), 50);
    strcat(buf, " ");
    PB_receive(parent, &(buf[strlen(buf)]), 50);
    strcat(buf, " ");
    PB_receive(parent, &(buf[strlen(buf)]), 50);

    PB_send(parent, buf);
    PB_send_err(parent, buf);
    PB_send(parent, "c2");
    PB_send_err(parent, "c2");
    PB_send(parent, "c3");
    PB_send_err(parent, "c3");

    sleep_for_one_second();

    return 12;
}