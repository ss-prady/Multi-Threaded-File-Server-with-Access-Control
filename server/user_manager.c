#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "user_auth.h"
#include <openssl/evp.h>

void print_usage(const char *program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -a <username> <password> <role>    Add a new user\n");
    printf("  -l                                 List all users\n");
    printf("  -d <username>                      Delete a user\n");
    printf("  -h                                 Show this help message\n");
    printf("\nRoles must be either 'read', 'write' or 'modify'\n");
}

int main(int argc, char *argv[]) {
    const char *users_file = "users";
    
    // Initialize OpenSSL
    OpenSSL_add_all_algorithms();
    
    // Load existing users
    load_users(users_file);
    
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "-a") == 0) {
        // Add a user
        if (argc < 5) {
            printf("Error: Missing arguments for adding a user\n");
            print_usage(argv[0]);
            return 1;
        }
        
        const char *username = argv[2];
        const char *password = argv[3];
        const char *role = argv[4];
        
        // Validate role
        if (strcmp(role, "read") != 0 && strcmp(role, "write") != 0 && strcmp(role, "modify") != 0) {
            printf("Error: Role must be either 'read', 'write' or 'modify'\n");
            return 1;
        }
        
        int result = create_user(username, password, role);
        if (result == 1) {
            printf("User '%s' added successfully with role '%s'\n", username, role);
            save_users(users_file);
        } else if (result == 0) {
            printf("Error: User '%s' already exists\n", username);
            return 1;
        } else {
            printf("Error: User database is full\n");
            return 1;
        }
    }
    else if (strcmp(argv[1], "-l") == 0) {
        // List users
        printf("Users (%d total):\n", get_user_count());
        printf("--------------------\n");
        printf("Username               Role\n");
        printf("--------------------   ------\n");
        
        for (int i = 0; i < get_user_count(); i++) {
            User *user = authenticate_by_index(i);
            if (user) {
                printf("%-20s   %s\n", user->username, user->role);
            }
        }
    }
    else if (strcmp(argv[1], "-d") == 0) {
        // Delete a user
        if (argc < 3) {
            printf("Error: Missing username for deleting a user\n");
            print_usage(argv[0]);
            return 1;
        }
        
        const char *username = argv[2];
        if (delete_user(username)) {
            printf("User '%s' deleted successfully\n", username);
            save_users(users_file);
        } else {
            printf("Error: User '%s' not found\n", username);
            return 1;
        }
    }
    else if (strcmp(argv[1], "-h") == 0) {
        // Show help
        print_usage(argv[0]);
    }
    else {
        printf("Error: Unknown option '%s'\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }
    
    return 0;
} 