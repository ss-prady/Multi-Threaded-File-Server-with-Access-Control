#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Socket connection functions
int connect_to_server(const char *server_ip, int port);

// Authentication function
int authenticate(int sock);

// Command handling functions
void handle_upload(int sock, const char *filename);
void handle_download(int sock, const char *filename);
void handle_modify(int sock, const char *filename);

#endif // CLIENT_UTILS_H