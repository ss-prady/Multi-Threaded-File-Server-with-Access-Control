#include "client_handler.h"
#include "file_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>     
#include <sys/types.h>  


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

    printf("%s\n", username);

    // sending acknowledgement 
    char* ACK = "DATA_RECEIVED";
    size_t bytes_sent = send(client_socket,ACK,strlen(ACK),0);

    bytes_received = recv(client_socket, password, sizeof(password),0);
    if (bytes_received <= 0) {
        perror("Server: Failed to receive password");
        close(client_socket);
        return NULL;
    }

    printf("%s\n", password);

    // sending acknowledgement for password
    bytes_sent = send(client_socket,ACK,strlen(ACK),0);

    User *user = authenticate(username, password);
    if (!user) {
        char *fail_msg = "AUTH_FAIL";
        send(client_socket, fail_msg, strlen(fail_msg) + 1, 0);
        close(client_socket);
        return NULL;
    }

    char *success_msg = "AUTH_SUCCESS";
    send(client_socket, success_msg, strlen(success_msg) + 1, 0);

    // --- Command loop ---
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) break;

        buffer[bytes] = '\0';
        printf("[%s] Command: %s\n", user->username, buffer);

        if (strncmp(buffer, "upload ", 7) == 0) {
            if (strcmp(user->role, "modify") != 0 && strcmp(user->role, "write") != 0) {
                send(client_socket, "Permission denied", 18, 0);
                continue;
            }

            // char *filename = buffer + 7;
            char filename[strlen(buffer) - 6];
            strcpy(filename, buffer + 7);
            filename[strlen(buffer) - 6] = '\0';
            
            // Check if file already exists (no need to lock for this)
            if (file_exists(filename)) {
                send(client_socket, "ERROR: File with the same name already exists", 46, 0);
                continue;
            }
            
            // Get full path to file
            char *filepath = get_file_path(filename);
            if (!filepath) {
                send(client_socket, "ERROR: Internal server error", 29, 0);
                continue;
            }
            
            printf("%s", filepath);
            
            // Lock directory for file creation only
            lock_directory_for_upload();
            
            // Check again after locking to prevent race conditions
            if (file_exists(filename)) {
                unlock_directory_for_upload();
                free(filepath);
                send(client_socket, "ERROR: File with the same name already exists", 46, 0);
                continue;
            }
            
            FILE *fp = fopen(filepath, "wb");
            
            // We can unlock the directory as soon as the file is created
            unlock_directory_for_upload();
            

            free(filepath);
            
            if (!fp) {
                perror("Server: Error creating file");
                send(client_socket, "ERROR: Could not create file", 29, 0);
                continue;
            }

            // We can now proceed with the upload
            send(client_socket, "READY", 5, 0);

            while (1) {
                memset(buffer, 0, BUFFER_SIZE);
                int bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);
                fwrite(buffer, sizeof(char), bytes_read - (buffer[bytes_read - 1] == 0), fp);
                // if (bytes_read <= 0 || strcmp(buffer, "EOF") == 0) break;
                if(buffer[bytes_read - 1] == '\0') break;
            }

            fclose(fp);
            printf("Upload complete.\n");
            send(client_socket, "Upload successful", 18, 0);

        } else if (strncmp(buffer, "download ", 9) == 0) {
            if (strcmp(user->role, "read") != 0 && strcmp(user->role, "write") != 0 && strcmp(user->role, "modify") != 0) {
                send(client_socket, "Permission denied", 18, 0);
                continue;
            }

            // char *filename = buffer + 9;

            char filename[strlen(buffer) - 8];
            strcpy(filename, buffer + 9);
            filename[strlen(buffer) - 8] = '\0';

            // Check if file exists
            if (!file_exists(filename)) {
                send(client_socket, "ERROR: File not found", 22, 0);
                continue;
            }

            // Request download access to the file
            if (request_file_access(filename, DOWNLOAD_MODE) != 0) {
                send(client_socket, "ERROR: Cannot access file for downloading", 41, 0);
                continue;
            }

            // Get full path to file
            char *filepath = get_file_path(filename);
            if (!filepath) {
                release_file_access(filename, DOWNLOAD_MODE);
                send(client_socket, "ERROR: Internal server error", 28, 0);
                continue;
            }

            FILE *fp = fopen(filepath, "rb");
            free(filepath);
            if (!fp) {
                perror("Server: Error opening file");
                release_file_access(filename, DOWNLOAD_MODE);
                send(client_socket, "ERROR: Cannot open file", 24, 0);
                continue;
            }
            
            send(client_socket, "READY", 5, 0);
            
            // Send the file contents
            while ((bytes = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
                send(client_socket, buffer, bytes, 0);
            }
        
            fclose(fp);
            release_file_access(filename, DOWNLOAD_MODE);
            buffer[0] = '\0';
            // Send EOF to signal end
            send(client_socket, buffer, 1, 0);
            printf("Sent file %s to client for download.\n", filename);
        } else if (strncmp(buffer, "modify ", 7) == 0) {
            if (strcmp(user->role, "modify") != 0) {
                send(client_socket, "Permission denied", 18, 0);
                continue;
            }
            
            // char *filename = buffer + 7;
            char filename[strlen(buffer) - 6];
            strcpy(filename, buffer + 7);
            filename[strlen(buffer) - 6] = '\0';
            
            // Check if file exists
            if (!file_exists(filename)) {
                send(client_socket, "ERROR: File not found", 22, 0);
                continue;
            }
            // Request write access to the file
            if (request_file_access(filename, WRITE_MODE) != 0) {
                send(client_socket, "ERROR: Cannot access file for modification", 43, 0);
                continue;
            }
            
            // Get full path to file
            char *filepath = get_file_path(filename);
            if (!filepath) {
                release_file_access(filename, WRITE_MODE);
                send(client_socket, "ERROR: Internal server error", 28, 0);
                continue;
            }
            
            // Send file for modification
            FILE *fp = fopen(filepath, "rb");
            if (!fp) {
                perror("Server: Error opening file");
                free(filepath);
                release_file_access(filename, WRITE_MODE);
                send(client_socket, "ERROR: Cannot open file", 24, 0);
                continue;
            }
            
            send(client_socket, "READY", 5, 0);
            
            // Send the file contents
            while ((bytes = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
                send(client_socket, buffer, bytes, 0);
            }
            
            // Send EOF to signal end of file content
            send(client_socket, "", 1, 0);
            fclose(fp);
            
            // Wait for modified file
            fp = fopen(filepath, "wb");
            free(filepath);
            
            if (!fp) {
                perror("Server: Error opening file for writing");
                release_file_access(filename, WRITE_MODE);
                send(client_socket, "ERROR: Cannot save modifications", 32, 0);
                continue;
            }
            
            // Receive modified file
            while (1) {
                memset(buffer, 0, BUFFER_SIZE);
                int bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);
                // if (bytes_read <= 0 || strcmp(buffer, "EOF") == 0) break;
                fwrite(buffer, sizeof(char), bytes_read - (buffer[bytes_read - 1] == 0), fp);
                if(buffer[bytes_read - 1] == '\0') break;
            }
            
            fclose(fp);
            release_file_access(filename, WRITE_MODE);
            send(client_socket, "Modification saved", 18, 0);
            printf("File %s modified by client %s.\n", filename, user->username);
        } 
        else if(strncmp(buffer, "list", 4) == 0) {
            DIR *dir;
            struct dirent *entry;
            char filepath[BUFFER_SIZE];
        
            dir = opendir("./files");
            if (dir == NULL) {
                perror("Failed to open directory");
                send(client_socket, "ERROR: Internal server error", 28, 0);
                continue;
            }
        
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }
        
                snprintf(filepath, sizeof(filepath), "%s\n", entry->d_name);
                send(client_socket, filepath, strlen(filepath), 0);
            }
        
            // Send null byte to signal EOF
            send(client_socket, "", 1, 0); // <-- this is important
        
            closedir(dir);
        }
        
        else {
            char msg[BUFFER_SIZE];
            snprintf(msg, BUFFER_SIZE, "[%s]: I received: %s", user->username, buffer);
            send(client_socket, msg, strlen(msg) + 1, 0);
        }
    }

    close(client_socket);
    return NULL;
} 