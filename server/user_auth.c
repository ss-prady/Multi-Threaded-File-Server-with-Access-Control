#include "user_auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <time.h>

// Global user database
static User users[100];
static int user_count = 0;

// Helper function to convert binary data to hex string
void bin2hex(const unsigned char *bin, size_t bin_len, char *hex) {
    for (size_t i = 0; i < bin_len; i++) {
        sprintf(hex + (i * 2), "%02x", bin[i]);
    }
    hex[bin_len * 2] = '\0';
}

// Helper function to convert hex string to binary data
void hex2bin(const char *hex, unsigned char *bin, size_t bin_len) {
    for (size_t i = 0; i < bin_len; i++) {
        sscanf(hex + (i * 2), "%2hhx", &bin[i]);
    }
}

void generate_salt(char *hex_salt, size_t hex_len) {
    size_t bin_len = hex_len / 2;
    unsigned char bin_salt[bin_len];
    
    // Generate random bytes
    if (RAND_bytes(bin_salt, bin_len) != 1) {
        // Fallback to a less secure but available method if OpenSSL random fails
        srand(time(NULL));
        for (size_t i = 0; i < bin_len; i++) {
            bin_salt[i] = rand() % 256;
        }
    }
    
    // Convert to hex string
    bin2hex(bin_salt, bin_len, hex_salt);
}

void hash_password(const char *password, const char *hex_salt, char *hash_output) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    char salted_pw[256];
    unsigned char bin_salt[16];
    
    // Convert hex salt to binary
    hex2bin(hex_salt, bin_salt, 16);
    
    // Combine password and salt
    snprintf(salted_pw, sizeof(salted_pw), "%s", password);
    memcpy(salted_pw + strlen(password), bin_salt, 16);
    
    // Generate hash
    SHA256((unsigned char*)salted_pw, strlen(password) + 16, hash);
    
    // Convert to hex string
    bin2hex(hash, SHA256_DIGEST_LENGTH, hash_output);
}

void load_users(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Could not open users file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    user_count = 0;
    
    while (fgets(line, sizeof(line), file) && user_count < 100) {
        line[strcspn(line, "\n")] = 0;
        
        // Parse the line: username:password_hash:salt:role
        char *token = strtok(line, ":");
        if (!token) continue;
        strncpy(users[user_count].username, token, sizeof(users[user_count].username) - 1);
        
        token = strtok(NULL, ":");
        if (!token) continue;
        strncpy(users[user_count].password_hash, token, sizeof(users[user_count].password_hash) - 1);
        
        token = strtok(NULL, ":");
        if (!token) continue;
        strncpy(users[user_count].salt, token, sizeof(users[user_count].salt) - 1);
        
        token = strtok(NULL, ":");
        if (!token) continue;
        strncpy(users[user_count].role, token, sizeof(users[user_count].role) - 1);
        
        user_count++;
    }
    
    fclose(file);
    printf("Loaded %d users from %s\n", user_count, filename);
}

User *authenticate(const char *username, const char *password) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            // Compute hash of the provided password with stored salt
            char computed_hash[65];
            hash_password(password, users[i].salt, computed_hash);
            
            // Compare with stored hash
            if (strcmp(computed_hash, users[i].password_hash) == 0) {
                return &users[i];
            }
            break; // Username found but password didn't match
        }
    }
    return NULL;
}

User *authenticate_by_index(int index) {
    if (index >= 0 && index < user_count) {
        return &users[index];
    }
    return NULL;
}

int create_user(const char *username, const char *password, const char *role) {
    // Check if username already exists
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return 0; // User already exists
        }
    }
    
    // Check if we have room for another user
    if (user_count >= 100) {
        return -1; // No more room
    }
    
    // Create new user
    strncpy(users[user_count].username, username, sizeof(users[user_count].username) - 1);
    
    // Generate salt
    generate_salt(users[user_count].salt, 32);
    
    // Hash password
    hash_password(password, users[user_count].salt, users[user_count].password_hash);
    
    // Set role
    strncpy(users[user_count].role, role, sizeof(users[user_count].role) - 1);
    
    user_count++;
    return 1; // Success
}

int delete_user(const char *username) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            // Found the user, remove by shifting the rest down
            for (int j = i; j < user_count - 1; j++) {
                // Copy the next user to the current position
                memcpy(&users[j], &users[j + 1], sizeof(User));
            }
            user_count--;
            return 1; // Success
        }
    }
    return 0; // User not found
}

int save_users(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Could not open users file for writing");
        return 0;
    }
    
    for (int i = 0; i < user_count; i++) {
        fprintf(file, "%s:%s:%s:%s\n", 
                users[i].username, 
                users[i].password_hash, 
                users[i].salt, 
                users[i].role);
    }
    
    fclose(file);
    return 1;
}

int get_user_count() {
    return user_count;
} 