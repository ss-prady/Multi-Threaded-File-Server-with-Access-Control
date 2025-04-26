#include "server.h"
#include "file_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <ifaddrs.h>  

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

    //Print all available IP addresses
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on:\n");
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;
        if (ifa->ifa_addr->sa_family == AF_INET) { // IPv4 addresses
            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
            char *ip = inet_ntoa(sa->sin_addr);

            // Avoid printing "127.0.0.1" multiple times unless you want localhost
            if (strcmp(ip, "127.0.0.1") != 0) {
                printf("  %s:%d\n", ip, PORT);
            }
        }
    }
    freeifaddrs(ifaddr);

    printf("  127.0.0.1:%d\n", PORT); // Also show localhost explicitly

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