#include "server.h"
#include "file_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>

// Signal handler for graceful shutdown
void handle_signal(int signal) {
    printf("\nShutting down server...\n");
    file_handler_cleanup();
    exit(0);
}

int start_server() {
    int server_fd, *new_sock;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Load user database
    load_users("users");
    
    // Initialize file handler
    if (file_handler_init() != 0) {
        fprintf(stderr, "Failed to initialize file handler\n");
        exit(EXIT_FAILURE);
    }
    
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0); // TCP 
    if (server_fd == 0) {
        perror("socket failed");
        file_handler_cleanup();
        exit(EXIT_FAILURE);
    }

    // Configure socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        file_handler_cleanup();
        exit(EXIT_FAILURE);
    }

    // Start listening
    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        file_handler_cleanup();
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Accept client connections in a loop
    while (1) {
        int client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (client_socket < 0) {
            perror("accept failed");
            continue;
        }

        new_sock = (int *)malloc(sizeof(int));
        *new_sock = client_socket;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, new_sock);
        pthread_detach(tid);
    }

    close(server_fd);
    file_handler_cleanup();
    return 0;
}

int main() {
    return start_server();
} 