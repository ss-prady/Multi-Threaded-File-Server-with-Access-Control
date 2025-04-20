// threaded_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE] = {0};

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) break;

        buffer[bytes] = '\0'; // Ensure null termination
        printf("Received: %s\n", buffer);

        // Handle upload
        if (strncmp(buffer, "upload ", 7) == 0) {
            char *filename = buffer + 7;

            FILE *fp = fopen(filename, "wb");
            if (!fp) {
                perror("Server: Error creating file");
                break;
            }

            // Notify client to send file
            send(client_socket, "READY", 5, 0);

            // Receive file contents
            while (1) {
                memset(buffer, 0, BUFFER_SIZE);
                int bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);
            
                if (bytes_read <= 0)
                    break;
            
                // Check if this is the EOF signal
                if (bytes_read == 3 && memcmp(buffer, "EOF", 3) == 0)
                    break;
            
                fwrite(buffer, sizeof(char), bytes_read, fp);
            }

            fclose(fp);
            printf("Upload complete.\n");
            send(client_socket, "Upload successful", 17, 0);
        }
        //Handle download
        else if (strncmp(buffer, "download ", 9) == 0) {
            char *filename = buffer + 9;
            FILE *fp = fopen(filename, "rb");
        
            if (!fp) {
                perror("Server: File not found");
                send(client_socket, "ERROR: File not found", 22, 0);
                continue;
            }
        
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
            // If not a command, treat it as a normal message
            printf("Message: %s\n", buffer);

            // Echo a reply
            char response[BUFFER_SIZE];
            snprintf(response, BUFFER_SIZE, "I received: %s", buffer);
            send(client_socket, response, strlen(response), 0);
        }
    }

    close(client_socket);
    return NULL;
}



int main() {
    int server_fd, *new_sock;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Multi-threaded Server listening on port %d...\n", PORT);

    while (1) {
        int client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (client_socket < 0) {
            perror("accept failed");
            continue;
        }

        new_sock = malloc(sizeof(int));
        *new_sock = client_socket;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, new_sock);
        pthread_detach(tid);  // no need to join, auto-cleans thread resources
    }

    close(server_fd);
    return 0;
}
