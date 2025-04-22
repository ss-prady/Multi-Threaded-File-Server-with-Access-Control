# Multi-Threaded File Server

This is a multi-threaded file server with secure user authentication and access control.

## Features

- Secure user authentication with password hashing and salting (SHA-256)
- Role-based access control (read/write permissions)
- File upload and download functionality
- Multi-threaded client handling
****
## Building the Server

To build the server, you'll need CMake (version 3.10 or higher), a C compiler, and OpenSSL development libraries.

### Install Dependencies

On Ubuntu/Debian:
```bash
sudo apt-get install build-essential cmake libssl-dev
```

On CentOS/RHEL:
```bash
sudo yum install gcc gcc-c++ make cmake openssl-devel
```

### Build

```bash
# Create a build directory
mkdir build
cd build

# Configure and build
cmake ..
make

# Run the server
./file_server
```

## User Management

The server comes with a user management utility for creating and managing user accounts:

```bash
# Add a new user with read access
./user_manager -a username password read

# Add a new user with write access
./user_manager -a username password write

# List all users
./user_manager -l

# Delete a user
./user_manager -d username
```

## Authentication Security

The system uses a secure authentication scheme:
- Passwords are never stored in plaintext
- Each password is hashed using SHA-256 with a unique random salt
- Salt values prevent rainbow table attacks
- User data is stored in the format: `username:password_hash:salt:role`

## Client Commands

- `upload <filename>` - Upload a file to the server (requires write permission)
- `download <filename>` - Download a file from the server (requires read permission) 