// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "client_utils.h"

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock;
    char buffer[BUFFER_SIZE] = {0};

    // Connect to server
    sock = connect_to_server("127.0.0.1", PORT);
    if (sock < 0) {
        return -1;
    }

    // Authenticate
    while(1) {
        printf("Enter command (login): ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        if (strcmp(buffer, "login") == 0) {
            if(!authenticate(sock)) {
                close(sock);
                return 0;
            }
            break;
        }
        else {
            printf("Unknown command. Please login first using the 'login' command.\n");
        }
    }

    // --- Main Loop ---
    while (1) {
        printf("\nEnter command (upload <file> / download <file> / modify <file> / list /exit): ");
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
            printf("Uploading file: %s\n", filename);
            handle_upload(sock, filename);
        }

        // Download command
        else if (strncmp(buffer, "download ", 9) == 0) {
            char *filename = buffer + 9;
            handle_download(sock, filename);
        }

        else if(strncmp(buffer, "modify ", 7) == 0) {
            char *filename = buffer + 7;
            handle_modify(sock, filename);
        }

        else if (strcmp(buffer, "list") == 0) {
          send(sock, "list", strlen("list"), 0);
          int bytes_read;
          printf("Available files:\n");
          while ((bytes_read = recv(sock, buffer, BUFFER_SIZE-1, 0)) > 0) {
              buffer[bytes_read] = '\0';
              printf("%s\n", buffer);
              if(strstr(buffer, "__END__") != NULL) break;
          }   
        }

        // Unknown command
        else {
            printf("Unknown command.\n");
        }
    }

    return 0;
}
