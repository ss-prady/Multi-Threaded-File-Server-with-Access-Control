#include "client_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>


void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE] = {0};

    // --- Authentication ---
    char username[50], password[50];
    int bytes_received;
    
    bytes_received = recv(client_socket, username, sizeof(username),0);
    if (bytes_received <= 0) {
        perror("Server: Failed to receive username");
        close(client_socket);
        return NULL;
    }
    // sending acknowledgement 
    char* ACK = "DATA_RECEIVED";
    size_t bytes_sent = send(client_socket,ACK,strlen(ACK),0);

    bytes_received = recv(client_socket, password, sizeof(password),0);
    if (bytes_received <= 0) {
        perror("Server: Failed to receive password");
        close(client_socket);
        return NULL;
    }
    // sending acknowledgement for password
    bytes_sent = send(client_socket,ACK,strlen(ACK),0);

    User *user = authenticate(username, password);
    if (!user) {
        char *fail_msg = "AUTH_FAIL";
        send(client_socket, fail_msg, strlen(fail_msg), 0);
        close(client_socket);
        return NULL;
    }

    char *success_msg = "AUTH_SUCCESS";
    send(client_socket, success_msg, strlen(success_msg), 0);

    // --- Command loop ---
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) break;

        buffer[bytes] = '\0';
        printf("[%s] Command: %s\n", user->username, buffer);

        if (strncmp(buffer, "upload ", 7) == 0) {
            if (strcmp(user->role, "write") != 0) {
                send(client_socket, "Permission denied", 18, 0);
                continue;
            }

            char *filename = buffer + 7;
            FILE *fp = fopen(filename, "wb");
            if (!fp) {
                perror("Server: Error creating file");
                break;
            }

            send(client_socket, "READY", 5, 0);

            while (1) {
                memset(buffer, 0, BUFFER_SIZE);
                int bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);
                if (bytes_read <= 0 || strcmp(buffer, "EOF") == 0) break;
                fwrite(buffer, sizeof(char), bytes_read, fp);
            }

            fclose(fp);
            printf("Upload complete.\n");
            send(client_socket, "Upload successful", 17, 0);

        } else if (strncmp(buffer, "download ", 9) == 0) {
            if (strcmp(user->role, "read") != 0 || strcmp(user->role, "write") != 0) {
                send(client_socket, "Permission denied", 18, 0);
                continue;
            }

            char *filename = buffer + 9;
            FILE *fp = fopen(filename, "rb");
        
            if (!fp) {
                perror("Server: File not found");
                send(client_socket, "ERROR: File not found", 22, 0);
                continue;
            }

            send(client_socket, "READY", 5, 0);
            sleep(1);  // Optional delay to avoid overlapping with file data

            // Send the file contents
            while ((bytes = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
                send(client_socket, buffer, bytes, 0);
            }
        
            fclose(fp);
        
            // Send EOF to signal end
            send(client_socket, "EOF", 3, 0);
            printf("Sent file %s to client.\n", filename);
        }
        else {
            char msg[BUFFER_SIZE];
            snprintf(msg, BUFFER_SIZE, "[%s]: I received: %s", user->username, buffer);
            send(client_socket, msg, strlen(msg), 0);
        }
    }

    close(client_socket);
    return NULL;
} 