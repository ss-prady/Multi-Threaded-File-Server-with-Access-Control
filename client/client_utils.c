#include "client_utils.h"
#include <netinet/tcp.h>

int connect_to_server(const char *server_ip, int port) {
    int sock;
    struct sockaddr_in serv_addr;

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Disable Nagle's algorithm
    int flag = 1;
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) < 0) {
        perror("Failed to disable Nagle's algorithm");
        close(sock);
        return -1;
    }

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("Invalid address / Address not supported\n");
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    printf("Connected to server.\n");
    return sock;
}

int authenticate(int sock) {
    char buffer[BUFFER_SIZE] = {0};
    char username[50], password[50];
    
    printf("Username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';

    printf("Password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';

    // Send username and password separately
    send(sock, username, strlen(username)+1, 0);
    size_t bytes_received = recv(sock,buffer,BUFFER_SIZE,0);
    memset(buffer, 0, BUFFER_SIZE);
    if(bytes_received<0){
        perror("failed to send the username");
        return -1;
    }
    send(sock, password, strlen(password)+1, 0);
    bytes_received = recv(sock,buffer,BUFFER_SIZE,0);
    if(bytes_received<0){
        perror("failed to send the password");
        return -1;
    }
    memset(buffer, 0, BUFFER_SIZE);
    bytes_received = recv(sock,buffer,BUFFER_SIZE,0);
    printf("Server: %s\n", buffer);

    if(strncmp(buffer,"AUTH_SUCCESS",9) == 0 ){
        return 1;
    }
    printf("Authentication failed. Closing connection.\n");
    return 0;
}

void handle_upload(int sock, const char *filename) {
    char buffer[BUFFER_SIZE] = {0};
    char command[BUFFER_SIZE + 7] = "upload ";
    
    strcat(command, filename);
    
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Client: File not found");
        return;
    }

    // Send command
    send(sock, command, strlen(command)+1, 0);

    // Wait for READY or error
    memset(buffer, 0, BUFFER_SIZE);
    recv(sock, buffer, BUFFER_SIZE, 0);
    if (strncmp(buffer, "READY", 5) != 0) {
        printf("Server: %s\n", buffer);
        fclose(fp);
        return;
    }

    // Send file
    char file_buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(file_buffer, 1, BUFFER_SIZE, fp)) > 0) {
        send(sock, file_buffer, bytes_read, 0);
    }
    fclose(fp);

    // Send EOF
    send(sock, "", 1, 0);

    // Wait for confirmation
    memset(buffer, 0, BUFFER_SIZE);
    recv(sock, buffer, BUFFER_SIZE, 0);
    printf("Server: %s\n", buffer);
}

void handle_download(int sock, const char *filename) {
    char buffer[BUFFER_SIZE] = {0};
    char command[BUFFER_SIZE + 10] = "download ";
    
    strcat(command, filename);
    
    // Send command to server
    send(sock, command, strlen(command)+1, 0);

    // Wait for server to respond
    memset(buffer, 0, BUFFER_SIZE);
    recv(sock, buffer, BUFFER_SIZE, 0);
    if (strncmp(buffer, "ERROR", 5) == 0 || strncmp(buffer, "Permission", 10) == 0 || strncmp(buffer, "File not", 9) == 0) {
        printf("Server: %s\n", buffer);
        return;
    }

    if (strncmp(buffer, "READY", 5) != 0) {
        printf("Unexpected response from server: %s\n", buffer);
        return;
    }

    // Create a temporary filename in case we can't use the original name
    char local_filename[256];
    strcpy(local_filename, filename);
    
    FILE *fp = fopen(local_filename, "wb");
    printf("Trying to open file: %s\n", local_filename);
    if (!fp) {
        // Try with modified permissions
        sprintf(local_filename, "downloaded_%s", filename);
        printf("Tried with modified permissions\n");
        fp = fopen(local_filename, "wb");
        
        if (!fp) {
            perror("Client: Could not open file to save");
            // Skip the incoming file data
            while (1) {
                memset(buffer, 0, BUFFER_SIZE);
                int bytes = recv(sock, buffer, BUFFER_SIZE, 0);
                if(buffer[bytes-1] == '\0'){
                    break;
                }
            }
            return;
        }
    }

    // Receive file data until EOF
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(sock, buffer, BUFFER_SIZE, 0);
        
        // Write file data to disk
        fwrite(buffer, 1, bytes - (buffer[bytes - 1] == '\0'), fp);
        
        if(buffer[bytes-1] == '\0'){
            break;
        }

    }

    fclose(fp);
    printf("Download complete. Saved as %s\n", local_filename);
} 