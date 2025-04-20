// threaded_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int sock;

void *send_thread(void *arg) {
    char message[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];

    while (1) {
        printf("Enter message: ");
        fgets(message, BUFFER_SIZE, stdin);

        // Remove newline
        message[strcspn(message, "\n")] = '\0';

        if (strcmp(message, "exit") == 0) {
            printf("Exiting...\n");
            close(sock);
            exit(0);
        }

        if (strncmp(message, "upload ", 7) == 0) {
            char *filename = message + 7;
            FILE *fp = fopen(filename, "rb");
            if (!fp) {
                perror("Client: File not found");
                continue;
            }

            // Send upload command
            send(sock, message, strlen(message), 0);

            // Wait for server response
            memset(buffer, 0, BUFFER_SIZE);
            int bytes = recv(sock, buffer, BUFFER_SIZE, 0);
            if (bytes <= 0 || strncmp(buffer, "READY", 5) != 0) {
                printf("Server refused upload.\n");
                fclose(fp);
                continue;
            }

            // Send file data
            while ((bytes = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
                send(sock, buffer, bytes, 0);
            }
            fclose(fp);

            // Signal EOF
            send(sock, "EOF", 3, 0);

            // Get server confirmation
            memset(buffer, 0, BUFFER_SIZE);
            recv(sock, buffer, BUFFER_SIZE, 0);
            printf("Server: %s\n", buffer);
        } 
        
        else if (strncmp(message, "download ", 9) == 0) {
            char *filename = message + 9;
        
            // Send download command
            send(sock, message, strlen(message), 0);
        
            // Create file locally to save the contents
            FILE *fp = fopen(filename, "wb");
            if (!fp) {
                perror("Client: Error creating file");
                continue;
            }
        
            // Receive data and write to file
            while (1) {
                memset(buffer, 0, BUFFER_SIZE);
                int bytes = recv(sock, buffer, BUFFER_SIZE, 0);
                if (bytes <= 0 || (bytes == 3 && strncmp(buffer, "EOF", 3) == 0))
                    break;
        
                fwrite(buffer, 1, bytes, fp);
            }
        
            fclose(fp);
            printf("Download complete: %s\n", filename);
        }
        
        
        else {
            // Regular message
            send(sock, message, strlen(message), 0);

            // Receive response
            memset(buffer, 0, BUFFER_SIZE);
            int bytes = recv(sock, buffer, BUFFER_SIZE, 0);
            if (bytes <= 0) {
                printf("Disconnected from server.\n");
                close(sock);
                exit(0);
            }
            printf("Server: %s\n", buffer);
        }
    }

    return NULL;
}

int main() {
    struct sockaddr_in serv_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address / Address not supported\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    printf("Connected to server.\n");

    pthread_t send_tid;
    pthread_create(&send_tid, NULL, send_thread, NULL);
    pthread_join(send_tid, NULL);

    return 0;
}
