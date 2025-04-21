// threaded_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

typedef struct {
    char username[50];
    char password[50];
    char role[10]; // "read" or "write"
} User;

User users[100];
int user_count = 0;

void load_users(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Could not open users file");
        exit(EXIT_FAILURE);
    }

    char line[150];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        sscanf(line, "%[^:]:%[^:]:%s", users[user_count].username, users[user_count].password, users[user_count].role);
        user_count++;
    }
    fclose(file);
}

User *authenticate(const char *username, const char *password) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            return &users[i];
        }
    }
    return NULL;
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE] = {0};

    // --- Authentication ---
    char username[50], password[50];
    recv(client_socket, username, sizeof(username), 0);
    recv(client_socket, password, sizeof(password), 0);

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
            if (strcmp(user->role, "read") != 0) {
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

int main() {
    int server_fd, *new_sock;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    load_users("users.txt");

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

    printf("Server listening on port %d...\n", PORT);

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
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}
