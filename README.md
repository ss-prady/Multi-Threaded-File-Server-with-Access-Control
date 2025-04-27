# Multi-Threaded-File-Server-with-Access-Control
A project in CSC-204Operating Systems

Before starting client, update the IP address of server in main.c

## To start client: 

chmod +x run_client.sh<br>
./run_client.sh

## To start server:

chmod +x run_server.sh<br>
./run_server.sh

## User Authentication System

1. This project implements a simple user authentication system in C with the following features:
    -   User management (add, list, delete users) via command-line options.
    -   Password security using salted SHA-256 hashing (OpenSSL library used).
    -   User data (username, password hash, salt, role) is stored in a users file.
    -   Roles supported: read, write, and modify.

2. Command-line options:
    -a <username> <password> <role> : Add a new user
    -l : List all users
    -d <username> : Delete an existing user
    -h : Show usage/help

3. Key implementation details:
    -   Passwords are hashed after salting with a random 16-byte salt.
    -   OpenSSL's RAND_bytes and SHA256 functions are used.
    -   User database supports up to 100 users.
    -   All changes (add/delete) are persisted to the users file.
    -  Functions are modularized for clarity (user_auth.h, logger.h used internally).


## Operations

1. **Read/Download** - Multiple clients can download a file simultaneously
2. **Write/Upload** - A client can upload a new file, which requires a unique filename
3. **Modify** - A client can modify a file, which locks the file for exclusive access
4. **List** - A client can list all files available on server

## Implementation Details

The server uses a file_handler module that manages file access through proper mutual exclusion:

- The file_handler uses semaphores to ensure thread-safe file operations
- It implements a reader-writer lock pattern where:
  - Multiple readers can access a file simultaneously
  - Multiple downloaders can access a file simultaneously
  - Writers get exclusive access (no readers, downloaders, or other writers)
  - Each file has its own set of semaphores and locks
  - Uploads use minimal directory-level locking only for file creation

## File Locking Rules

1. If a file is being read by one or more clients, new read and download requests are allowed
2. If a file is being downloaded by one or more clients, new read and download requests are allowed
3. If a file is being read or downloaded, write/modify requests must wait
4. If a file is being written/modified, all other access requests must wait
5. File uploads require a unique filename and use briefly-held directory-level locks just for the file creation

## Semaphore Implementation

The implementation uses a per-file locking mechanism:

1. Each file has:
   - A read semaphore (`read_sem`) - Controls access when writers are active
   - A write semaphore (`write_sem`) - Ensures mutual exclusion for writers
   - A mutex for updating counters safely

2. For read operations:
   - Wait on the read semaphore if a writer is active
   - Increment the reader count
   - First reader acquires the write semaphore (blocks writers)
   - Last reader releases the write semaphore

3. For download operations (similar to read):
   - Wait on the read semaphore if a writer is active
   - Increment the downloader count
   - First concurrent reader/downloader acquires write semaphore
   - Last concurrent reader/downloader releases write semaphore

4. For write/modify operations:
   - Mark writer as active (blocks new readers/downloaders)
   - Acquire write semaphore (waits for all readers/downloaders to finish)
   - Upon completion, release write semaphore and signal read semaphore

5. For upload operations:
   - Check file existence without locking (allowing concurrent existence checks)
   - Lock directory briefly only during file creation
   - Check existence again after locking to prevent race conditions
   - Release lock immediately after file creation

This approach ensures data consistency while maximizing concurrent access where appropriate. 
