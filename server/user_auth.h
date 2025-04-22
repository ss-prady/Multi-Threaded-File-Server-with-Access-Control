#ifndef USER_AUTH_H
#define USER_AUTH_H

#include <stddef.h> // For size_t

// User authentication and management functions

typedef struct
{
    char username[50];
    char password_hash[65]; // SHA-256 hash in hex (64 chars + null terminator)
    char salt[33];          // 16 bytes of salt as hex string (32 chars + null terminator)
    char role[10];          // "read" or "write"
} User;

// Load users from a file
void load_users(const char *filename);

// Authenticate a user based on username and password
User *authenticate(const char *username, const char *password);

// Get user by index (for listing users)
User *authenticate_by_index(int index);

// Get number of users
int get_user_count();

// Hash a password with a given salt
void hash_password(const char *password, const char *salt, char *hash_output);

// Generate a random salt
void generate_salt(char *salt, size_t length);

// Create a new user (for user management)
int create_user(const char *username, const char *password, const char *role);

// Delete a user by username
int delete_user(const char *username);

// Save the current user database to file
int save_users(const char *filename);

#endif // USER_AUTH_H