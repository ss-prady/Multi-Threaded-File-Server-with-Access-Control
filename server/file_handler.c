#include "file_handler.h"
#include <dirent.h> // For directory operations

// Structure to track file access statistics and semaphores
typedef struct {
    char filename[MAX_FILENAME_LEN];
    sem_t read_sem;         // Counting semaphore for readers
    sem_t write_sem;        // Binary semaphore for writers
    pthread_mutex_t mutex;  // Mutex for updating reader/writer counts
    int reader_count;       // Number of active readers
    int writer_active;      // Whether a writer is active
    bool initialized;       // Whether this file entry is initialized
} file_entry_t;

// Array to store file entries
static file_entry_t file_entries[MAX_FILES];

// Mutex to protect the file entries array
static pthread_mutex_t entries_mutex = PTHREAD_MUTEX_INITIALIZER;

// Mutex for directory-level operations (upload)
static pthread_mutex_t directory_mutex = PTHREAD_MUTEX_INITIALIZER;

// Find or create a file entry
static file_entry_t* get_file_entry(const char* filename) {
    pthread_mutex_lock(&entries_mutex);
    
    // First, try to find an existing entry
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_entries[i].initialized && strcmp(file_entries[i].filename, filename) == 0) {
            pthread_mutex_unlock(&entries_mutex);
            return &file_entries[i];
        }
    }
    
    // If not found, create a new entry
    for (int i = 0; i < MAX_FILES; i++) {
        if (!file_entries[i].initialized) {
            strncpy(file_entries[i].filename, filename, MAX_FILENAME_LEN - 1);
            file_entries[i].filename[MAX_FILENAME_LEN - 1] = '\0';
            
            sem_init(&file_entries[i].read_sem, 0, 1);
            sem_init(&file_entries[i].write_sem, 0, 1);
            pthread_mutex_init(&file_entries[i].mutex, NULL);
            
            file_entries[i].reader_count = 0;
            file_entries[i].writer_active = 0;
            file_entries[i].initialized = true;
            
            pthread_mutex_unlock(&entries_mutex);
            return &file_entries[i];
        }
    }
    
    pthread_mutex_unlock(&entries_mutex);
    return NULL; // No free slots available
}

int file_handler_init(void) {
    // Create the files directory if it doesn't exist
    struct stat st = {0};
    if (stat(FILES_DIR, &st) == -1) {
        if (mkdir(FILES_DIR, 0777) == -1) {
            perror("Failed to create files directory");
            return -1;
        }
    }
    
    // Initialize file entries
    for (int i = 0; i < MAX_FILES; i++) {
        file_entries[i].initialized = false;
    }

    // Scan the files directory and initialize file entries
    DIR *dir = opendir(FILES_DIR);
    if (dir) {
        struct dirent *entry;
        int index = 0;

        while ((entry = readdir(dir)) != NULL && index < MAX_FILES) {
            struct stat file_stat;
            char full_path[MAX_FILENAME_LEN + sizeof(FILES_DIR) + 1];
            snprintf(full_path, sizeof(full_path), "%s/%s", FILES_DIR, entry->d_name);
            if (stat(full_path, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) { // Regular file
                strncpy(file_entries[index].filename, entry->d_name, MAX_FILENAME_LEN - 1);
                file_entries[index].filename[MAX_FILENAME_LEN - 1] = '\0';

                sem_init(&file_entries[index].read_sem, 0, 1);
                sem_init(&file_entries[index].write_sem, 0, 1);
                pthread_mutex_init(&file_entries[index].mutex, NULL);

                file_entries[index].reader_count = 0;
                file_entries[index].writer_active = 0;
                file_entries[index].initialized = true;

                index++;
            }
        }

        closedir(dir);
    } else {
        perror("Failed to open files directory");
    }
    
    return 0;
}

void file_handler_cleanup(void) {
    pthread_mutex_lock(&entries_mutex);
    
    // Clean up all initialized entries
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_entries[i].initialized) {
            sem_destroy(&file_entries[i].read_sem);
            sem_destroy(&file_entries[i].write_sem);
            pthread_mutex_destroy(&file_entries[i].mutex);
            file_entries[i].initialized = false;
        }
    }
    
    pthread_mutex_unlock(&entries_mutex);
    pthread_mutex_destroy(&entries_mutex);
    pthread_mutex_destroy(&directory_mutex);
}

int request_file_access(const char *filename, access_mode_t mode) {
    file_entry_t *entry = get_file_entry(filename);
    if (!entry) {
        fprintf(stderr, "Failed to get file entry for %s\n", filename);
        return -1;
    }
    
    switch (mode) {
        case READ_MODE:
            // Get read access
            pthread_mutex_lock(&entry->mutex);
            
            // If a writer is waiting or active, wait for read semaphore
            if (entry->writer_active) {
                pthread_mutex_unlock(&entry->mutex);
                sem_wait(&entry->read_sem);
                pthread_mutex_lock(&entry->mutex);
            }
            
            // Increment reader count
            entry->reader_count++;
            pthread_mutex_unlock(&entry->mutex);
            // If this is the first reader, lock the write semaphore
            if (entry->reader_count == 1) {
                sem_wait(&entry->write_sem);
            }
            
            return 0;
            
        case WRITE_MODE:
            // Mark that a writer wants access
            pthread_mutex_lock(&entry->mutex);
            entry->writer_active = 1;
            pthread_mutex_unlock(&entry->mutex);
            // Wait for exclusive access (no readers, no writers, no downloaders)
            printf("Waiting for write access to %s...\n", filename);
            sem_wait(&entry->write_sem);
            return 0;
    }
    
    return -1; // Invalid mode
}

void release_file_access(const char *filename, access_mode_t mode) {
    file_entry_t *entry = get_file_entry(filename);
    if (!entry) {
        fprintf(stderr, "Failed to get file entry for %s\n", filename);
        return;
    }
    
    switch (mode) {
        case READ_MODE:
            pthread_mutex_lock(&entry->mutex);
            
            // Decrement reader count
            entry->reader_count--;
            pthread_mutex_unlock(&entry->mutex);
            // If there are no more readers or downloaders, release the write semaphore
            if (entry->reader_count == 0) {
                sem_post(&entry->write_sem);
            }
            
            break;
            
        case WRITE_MODE:
            // Writer is done, release the semaphore
            pthread_mutex_lock(&entry->mutex);
            entry->writer_active = 0;
            pthread_mutex_unlock(&entry->mutex);
            
            // Signal that writing is done
            sem_post(&entry->write_sem);
            break;
    }
}

bool file_exists(const char *filename) {
    char path[MAX_FILENAME_LEN + sizeof(FILES_DIR) + 1];
    snprintf(path, sizeof(path), "%s/%s", FILES_DIR, filename);
    
    struct stat st;
    return (stat(path, &st) == 0);
}

char *get_file_path(const char *filename) {
    char *path = malloc(MAX_FILENAME_LEN + sizeof(FILES_DIR) + 1);
    if (!path) {
        return NULL;
    }
    
    snprintf(path, MAX_FILENAME_LEN + sizeof(FILES_DIR) + 1, "%s/%s", FILES_DIR, filename);
    return path;
}

void lock_directory_for_upload(void) {
    pthread_mutex_lock(&directory_mutex);
}

void unlock_directory_for_upload(void) {
    pthread_mutex_unlock(&directory_mutex);
} 