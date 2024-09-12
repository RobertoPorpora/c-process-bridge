# Process Bridge

**Process Bridge** is a library designed to facilitate the creation and management of long-lived child processes, enabling bidirectional communication through pipes.  
It provides a simple interface for spawning, despawning, sending and receiving data between parent and child processes, streamlining the integration and control of parallel processes.  
Ideal for applications requiring multiple persistent and collaborative processes.  

## How to use it

### Linking the Library

To use the `process_bridge` library in your project, you can use one these methods:
Method 1: CMake (Recommended):
1. Copy the whole folder in your project (or import it as a submodule).
2. In your CMakeLists.txt script:
    - Call the CMakeLists.txt script of this folder with `add_subdirectory()` .
    - Link "process_bridge" library with `target_link_libraries()`.
Method 2: File copy:
1. Copy the .c and .h files from 'src' and 'include' folders in your sources folder.
2. Configure your toolchain to compile also the imported .c files

...and then include the header file where you want to use it:
```c
#include <process_bridge.h> // if you are using CMake
// or
#include "process_bridge.h" // if you copied the files
```

### Example 1: Spawning and communicating with a long living child process.
> This part of the library is currently not available, but planned for the future.

```c
#include <stdio.h>
#include <string.h>

#include "process_bridge.h"

int main()
{
    PB_status_t status;

    // Create a child process and allocate its memory
    PB_process_t *child = PB_create(PB_TYPE_CHILD);
    if (child == NULL)
    {
        printf("Failed to create child process\n");
        return 1;
    }

    // Spawn the child process
    status = PB_spawn(child, "path/to/your/executable");
    if (PB_STATUS_OK != status)
    {
        printf("Failed to spawn child process: %s\n", child->error);
        return 1;
    }

    // Send a message to child from parent
    status = PB_send(child, "Hello from parent!");
    if (PB_STATUS_OK != status)
    {
        printf("Failed to send response to parent: %s\n", child->error);
    }

    // Receive a message back from the child process
    char mailbox[PB_STRING_SIZE_DEFAULT];
    status = PB_receive(child, mailbox, PB_STRING_SIZE_DEFAULT);
    if (PB_STATUS_OK == status)
    {
        printf("Received a message from child: %s\n", mailbox);
    }
    else
    {
        printf("Failed to receive message: %s\n", child->error);
    }

    // ...do something with the message...
    // ...eventually send and or receive other messages...

    // kill the process
    PB_despawn(child);

    // free memory
    PB_destroy(child);

    return 0;
}
```

### Example 2: Spawned child process communicating with its parent process.
```c
#include <stdio.h>
#include <string.h>

#include "process_bridge.h"

int main()
{
    // Initialize a bridge to the parent process.
    PB_process_t *parent = PB_create(PB_TYPE_PARENT);
    
    if (parent == NULL) {
        // Failed to create parent process bridge.
        return 1;
    }

    // Receive a message from parent
    char mailbox[PB_STRING_SIZE_DEFAULT]
    PB_receive(parent, mailbox, PB_STRING_SIZE_DEFAULT);

    // ...do something with the message...

    // Send a message back to the parent process.
    const char *message = "Hello from child!";
    PB_send(parent, message);

    // ...eventually send and or receive other messages...

    // free memory
    PB_destroy(parent);
}
```

## Contributing

Contributions are welcome!  
Please submit pull requests with clear descriptions and ensure that your code passes existing tests.  
Any improvements would be greatly appreciated.

## License

This project is licensed under the MIT License. See the `LICENSE` file for more details.

---

For any issues or questions, please open an issue on the project's repository.