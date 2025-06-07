# 42 School - Exam Rank 06

The last exam of 42 Common Core will require you to write a C program that implements a basic server for handling multiple clients. The server listens for incoming connections, manages connected clients, and allows them to send and receive messages.

## About the subject and initial files

Besides the subject, this exam provides a main.c file to serve as initial point for developing the server and test its functionality, as it includes some helper functions. However, at the same time this can be tricky, as there are many things on this file that are incorrect or not allowed for the final submission file.

This is a summary of what you **need to change from the main.c** file:

- `printf()` - The assignment only allows `write` for output

- `exit(0)` - Should be `exit(1)` for errors as per requirements

- The program prints custom messages (`"socket creation failed..."`) instead of `"Fatal error"`

- Doesn't write to `stderr` (uses stdout via printf)

- Unnecessary logs and outputs (`"Socket successfully created.."`), assignment requires silent operation except for error messages

And also, as summary of **incomplete Implementation** which you will need to develop:

- Only handles one client (no `select()` loop)

- No message passing between clients

- No client ID management

- No proper cleanup of resources

- Port is hardcoded to `8081` instead of being taken as argument

- IP address is hardcoded using numeric value `(2130706433)` instead of proper `127.0.0.1`

- No checks for `malloc/calloc` failures in the helper functions

- Doesn't properly `free` resources before exiting

> You should use the provided helper functions (`extract_message` and `str_join`) but need to rewrite the main server logic completely to meet all requirements.

## PoA

- Rename given `main.c` file into `mini_serv.c`
- Add more helper functions as needed: `void fatal_error()`, `void add_client(int fd)`, `void remove_client(int fd)`
- Add t_client struct for better organization and workflow
- Add global variables `int g_id = 0` and `t_client* clients = NULL`
- Rewrite the `main()` block to implement:
    - Make the main able to receive and check arguments
    - Setup socket (create-bind-listen)
    - Set the select() loop for non-blocking I/O
    - Accept connections and handle clients

