# Multi-threaded HTTP Server in C

This project is a multi-threaded HTTP server written in C. It can serve static files (HTML, JS, images, etc.) from the `src/static/` directory and is designed for learning and experimentation with sockets, threading, and basic HTTP protocol handling.

## Features
- **Full HTTP Method Support**: GET, POST, PUT, DELETE with proper status codes
- Serves static files from `src/static/` (HTML, JS, images, etc.)
- Multi-threaded: uses a thread pool to handle multiple clients concurrently
- Graceful shutdown on SIGINT/SIGTERM
- Simple HTTP request parsing
- Proper HTTP status codes and error handling
- Extensible C codebase with clear separation of concerns

## Project Structure
```
http-server/
├── src/
│   ├── main.c            # Entry point, server loop, signal handling
│   ├── server.c          # Server socket setup
│   ├── client.c          # Client connection, thread pool, request handling
│   ├── parse_req.c       # HTTP request parsing
│   ├── http.c            # HTTP method handlers and response functions
│   ├── utils/
│   │   ├── server.h      # Server API
│   │   ├── client.h      # Client/thread pool API
│   │   ├── parse_req.h   # Request parsing API
│   │   └── http.h        # HTTP constants and response API
│   └── static/           # Static files served by the server
│       ├── index.html
│       ├── nav.html
│       ├── test.js
│       └── IMG_3851.JPG
├── Makefile              # Build configuration
├── README.md             # Project documentation
└── .gitignore
```

## Build Instructions

You need GCC and pthreads (standard on Linux). To build:

```sh
make
```

This will produce a `server` executable in the project root.

To clean up build artifacts:
```sh
make clean
```

## Run Instructions

From the project root, run:
```sh
./server
```

The server will start on port 3000. You can access it in your browser at:
```
http://localhost:3000/
```

## How It Works
- The server listens on port 3000.
- For each incoming connection, a client socket is created and handed off to a thread pool.
- Each worker thread parses the HTTP request and routes it to the appropriate handler based on the HTTP method:
  - **GET**: Serves static files from `src/static/` directory
  - **POST**: Creates new resources (returns 201 Created)
  - **PUT**: Updates existing resources (returns 200 OK)
  - **DELETE**: Removes resources (returns 204 No Content)
- Proper HTTP status codes are returned for all operations
- Content-Type is set based on file extension (supports HTML, CSS, JS, PNG, JPG, GIF, etc.)
- Unsupported methods return 405 Method Not Allowed

## API Overview

### Server API (`utils/server.h`)
- `int init_server(int port);`  
  Initializes and binds a server socket on the given port.

### Client/Thread Pool API (`utils/client.h`)
- `int create_client(int server_fd);`  
  Accepts a new client connection.
- `void handle_client_request(int client_fd);`  
  Handles a single HTTP request/response cycle.
- `thread_pool_t *create_thread_pool(int thread_count, int queue_size);`  
  Creates a thread pool for handling clients.
- `void destroy_thread_pool(thread_pool_t *pool);`  
  Cleans up the thread pool.
- `int add_client_to_pool(thread_pool_t *pool, int client_fd);`  
  Adds a client socket to the thread pool's queue.

### Request Parsing API (`utils/parse_req.h`)
- `int parse_request(const char *buffer, char *method, size_t msize, char *path, size_t psize);`  
  Parses the HTTP request line into method and path.

### HTTP API (`utils/http.h`)
- `void send_http_response(int client_fd, const char *status, const char *content_type, const char *body);`  
  Sends a complete HTTP response with headers and body.
- `void send_error_response(int client_fd, const char *status, const char *message);`  
  Sends an HTML error response.
- `int handle_get_request(int client_fd, const char *path);`  
  Handles GET requests for static files.
- `int handle_post_request(int client_fd, const char *path);`  
  Handles POST requests for creating resources.
- `int handle_put_request(int client_fd, const char *path);`  
  Handles PUT requests for updating resources.
- `int handle_delete_request(int client_fd, const char *path);`  
  Handles DELETE requests for removing resources.

## Static Files
Place your static files (HTML, JS, images, etc.) in `src/static/`. The server will serve these at the root URL path. For example:
- `src/static/index.html` → `http://localhost:3000/`
- `src/static/test.js` → `http://localhost:3000/test.js`
- `src/static/IMG_3851.JPG` → `http://localhost:3000/IMG_3851.JPG`
- `src/static/api-test.html` → `http://localhost:3000/api-test.html` (test HTTP methods)

## Testing HTTP Methods
Visit `http://localhost:3000/api-test.html` to test all supported HTTP methods (GET, POST, PUT, DELETE) with a web interface.

## License
MIT License (see LICENSE file if present)
