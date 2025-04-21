// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
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

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    printf("Connected to server.\n");

    // --- Authentication ---
    char username[50], password[50];
    printf("Username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';

    printf("Password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';

    // Send username and password separately
    send(sock, username, strlen(username), 0);
    // sleep(1); // slight delay to prevent merging
    send(sock, password, strlen(password), 0);

    // Wait for authentication response
    memset(buffer, 0, BUFFER_SIZE);
    recv(sock, buffer, BUFFER_SIZE, 0);
    printf("Server: %s\n", buffer);

    if (strncmp(buffer, "AUTH_FAIL", 9) == 0) {
        printf("Authentication failed. Closing connection.\n");
        close(sock);
        return 0;
    }

    // --- Main Loop ---
    while (1) {
        printf("\nEnter command (upload <file> / download <file> / exit): ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strcmp(buffer, "exit") == 0) {
            printf("Exiting...\n");
            close(sock);
            break;
        }

        // Upload command
        if (strncmp(buffer, "upload ", 7) == 0) {
            char *filename = buffer + 7;
            FILE *fp = fopen(filename, "rb");
            if (!fp) {
                perror("Client: File not found");
                continue;
            }

            // Send command
            send(sock, buffer, strlen(buffer), 0);

            // Wait for READY or error
            memset(buffer, 0, BUFFER_SIZE);
            recv(sock, buffer, BUFFER_SIZE, 0);
            if (strncmp(buffer, "READY", 5) != 0) {
                printf("Server: %s\n", buffer);
                fclose(fp);
                continue;
            }

            // Send file
            char file_buffer[BUFFER_SIZE];
            size_t bytes_read;
            while ((bytes_read = fread(file_buffer, 1, BUFFER_SIZE, fp)) > 0) {
                send(sock, file_buffer, bytes_read, 0);
            }
            fclose(fp);

            // Send EOF
            send(sock, "EOF", 3, 0);

            // Wait for confirmation
            memset(buffer, 0, BUFFER_SIZE);
            recv(sock, buffer, BUFFER_SIZE, 0);
            printf("Server: %s\n", buffer);
        }

        // Download command
        // Client: Download file handling part
        else if (strncmp(buffer, "download ", 9) == 0) {
            char *filename = buffer + 9;
        
            // Send command to server
            send(sock, buffer, strlen(buffer), 0);
        
            // Wait for server to respond
            memset(buffer, 0, BUFFER_SIZE);
            recv(sock, buffer, BUFFER_SIZE, 0);
            if (strncmp(buffer, "ERROR", 5) == 0 || strncmp(buffer, "Permission", 10) == 0 || strncmp(buffer, "File not", 9) == 0) {
                printf("Server: %s\n", buffer);
                continue;
            }
        
            if (strncmp(buffer, "READY", 5) != 0) {
                printf("Unexpected response from server: %s\n", buffer);
                continue;
            }
        
            FILE *fp = fopen(filename, "wb");
            if (!fp) {
                // fallback name
                strcpy(filename, "downloaded_file.txt");
                fp = fopen(filename, "wb");
                if (!fp) {
                    perror("Client: Could not open file to save");
                    continue;
                }
            }

        
            // Receive file data until EOF
            while (1) {
                memset(buffer, 0, BUFFER_SIZE);
                int bytes = recv(sock, buffer, BUFFER_SIZE, 0);
                if (bytes <= 0) break;
            
                // Handle case where "EOF" may be part of the buffer
                if (bytes == 3 && strncmp(buffer, "EOF", 3) == 0) {
                    break;  // Stop receiving when EOF is detected
                }
            
                // Write file data to disk
                fwrite(buffer, 1, bytes, fp);
            }
        
            fclose(fp);
            printf("Download complete.\n");
        }

        // Unknown command
        else {
            printf("Unknown command.\n");
        }
    }

    return 0;
}
