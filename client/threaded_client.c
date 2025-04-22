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
    if (!authenticate(sock)) {
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
            handle_upload(sock, filename);
        }

        // Download command
        else if (strncmp(buffer, "download ", 9) == 0) {
            char *filename = buffer + 9;
            handle_download(sock, filename);
        }

        // Unknown command
        else {
            printf("Unknown command.\n");
        }
    }

    return 0;
}
