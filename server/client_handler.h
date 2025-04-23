#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "user_auth.h"

// Buffer size for receiving data from clients
#define BUFFER_SIZE 1024

// Thread function to handle client connections
void *handle_client(void *arg);
size_t recv_line(int sock, char *buffer, size_t max_len);
#endif // CLIENT_HANDLER_H 