#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>

#define FILES_DIR "files"
#define MAX_FILES 100
#define MAX_FILENAME_LEN 256

// File access modes
typedef enum
{
    READ_MODE,
    WRITE_MODE,
    DOWNLOAD_MODE
} access_mode_t;

// Initialize the file handler system
int file_handler_init(void);

// Clean up resources when server shuts down
void file_handler_cleanup(void);

// Request access to a file, returns 0 on success, -1 on failure
int request_file_access(const char *filename, access_mode_t mode);

// Release file access when done
void release_file_access(const char *filename, access_mode_t mode);

// Check if a file exists in the files directory
bool file_exists(const char *filename);

// Get full path to a file in the files directory
char *get_file_path(const char *filename);

// Directory operations for upload mutual exclusion
void lock_directory_for_upload(void);
void unlock_directory_for_upload(void);

#endif // FILE_HANDLER_H