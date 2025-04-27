#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "client_utils.h"

#define PORT 8080
#define BUFFER_SIZE 1024

void print_welcome() {
    printf("============================================================\n");
    printf("             WELCOME TO THE MULTI-THREADED FILE CLIENT      \n");
    printf("============================================================\n");
    printf("Type 'help' after login to see available commands.\n");
    printf("------------------------------------------------------------\n");
}

void print_help() {
    printf("\nAvailable Commands:\n");
    printf("------------------------------------------------------------\n");
    printf("upload <filename>   : Upload a file to the server\n");
    printf("download <filename> : Download a file from the server\n");
    printf("modify <filename>   : Modify a file on the server\n");
    printf("list                : List all available files on the server\n");
    printf("help                : Show this help message\n");
    printf("exit                : Exit the client\n");
    printf("------------------------------------------------------------\n");
}

int main() {
    int sock;
    char buffer[BUFFER_SIZE] = {0};

    print_welcome();

    // Connect to server
    sock = connect_to_server("172.24.9.252", PORT);
    if (sock < 0) {
        return -1;
    }

    // Authenticate
    while(1) {
        if(!authenticate(sock)) {
            close(sock);
            return 0;
        }
        else break;
    }

    printf("\nLogin successful. Type 'help' to see available commands.\n");

    // --- Main Loop ---
    while (1) {
        printf("\n> ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strcmp(buffer, "exit") == 0) {
            printf("Exiting... Goodbye!\n");
            close(sock);
            break;
        }
        else if (strcmp(buffer, "help") == 0) {
            print_help();
        }
        else if (strncmp(buffer, "upload ", 7) == 0) {
            char *filename = buffer + 7;
            printf("Uploading file: %s\n", filename);
            handle_upload(sock, filename);
        }
        else if (strncmp(buffer, "download ", 9) == 0) {
            char *filename = buffer + 9;
            printf("Downloading file: %s\n", filename);
            handle_download(sock, filename);
        }
        else if (strncmp(buffer, "modify ", 7) == 0) {
            char *filename = buffer + 7;
            printf("Modifying file: %s\n", filename);
            handle_modify(sock, filename);
        }
        else if (strcmp(buffer, "list") == 0) {
            printf("Listing files on server...\n");
            handle_list(sock); 
        }
        else {
            printf("Unknown command. Type 'help' to see available commands.\n");
        }
    }

    return 0;
}
