# 🚀 Multi-Threaded HTTP Server in C

A high-performance, feature-rich HTTP server implemented in C with modern backend engineering practices. This project demonstrates advanced systems programming concepts and is perfect for showcasing backend development skills.

## ✨ Features

### 🏗️ **Core Architecture**
- **Multi-threaded design** with thread pool for concurrent request handling
- **Non-blocking I/O** with efficient socket management
- **Graceful shutdown** with signal handling (SIGINT, SIGTERM)
- **Memory-safe** with proper resource management and cleanup

### 🌐 **HTTP Protocol Support**
- **Full HTTP/1.1** request/response handling
- **All major HTTP methods**: GET, POST, PUT, DELETE, OPTIONS
- **Request parsing** with headers, body, and query string support
- **Proper HTTP status codes** and error handling
- **Content-Type detection** for various file types
- **CORS support** for cross-origin requests

### 🔧 **RESTful API**
- **Complete CRUD operations** for user management
- **JSON request/response** handling
- **RESTful URL patterns** (`/api/users`, `/api/users/{id}`)
- **Proper HTTP status codes** (200, 201, 204, 400, 404, 405, 500)
- **In-memory database** with thread-safe operations

### 📊 **Monitoring & Observability**
- **Health check endpoint** (`/health`) with uptime information
- **Real-time metrics** (`/metrics`) with request statistics
- **Performance monitoring** (total requests, success rate, error tracking)
- **Request logging** with timestamps and method/path tracking

### 🛡️ **Security Features**
- **Path traversal protection** with `realpath()` validation
- **Input sanitization** and validation
- **Directory access restrictions** (static files only)
- **Request size limits** and buffer overflow protection

### 📁 **Static File Serving**
- **Efficient file serving** with proper MIME type detection
- **Large file support** with streaming
- **Directory traversal protection**
- **Support for**: HTML, CSS, JS, JSON, images (PNG, JPG, GIF)

### 🎯 **Developer Experience**
- **Clean, modular codebase** with separation of concerns
- **Comprehensive error handling** and logging
- **Easy-to-use Makefile** for building and development
- **Interactive test page** for API demonstration

## 🚀 Quick Start

### Prerequisites
- GCC compiler
- Make
- Linux/Unix environment (uses POSIX threads and sockets)

### Build & Run
```bash
# Clone and navigate to project
cd http-server

# Build the server
make

# Run the server (starts on port 3000)
./server

# Or build and run in one command
make && ./server
```

### Test the Server
```bash
# Health check
curl http://localhost:3000/health

# Get metrics
curl http://localhost:3000/metrics

# API endpoints
curl http://localhost:3000/api/users
curl -X POST http://localhost:3000/api/users
curl http://localhost:3000/api/users/1
curl -X PUT http://localhost:3000/api/users/1
curl -X DELETE http://localhost:3000/api/users/1

# Static files
curl http://localhost:3000/
curl http://localhost:3000/api-test.html
```

## 📋 API Documentation

### Health Check
```http
GET /health
```
Returns server health status and uptime.

**Response:**
```json
{
  "status": "healthy",
  "uptime": 1234,
  "timestamp": "Wed Dec 13 10:30:00 2023"
}
```

### Metrics
```http
GET /metrics
```
Returns server performance metrics.

**Response:**
```json
{
  "total_requests": 150,
  "successful_requests": 145,
  "error_requests": 5,
  "uptime_seconds": 3600,
  "success_rate": 96.67
}
```

### Users API

#### List All Users
```http
GET /api/users
```

#### Get Specific User
```http
GET /api/users/{id}
```

#### Create User
```http
POST /api/users
```

#### Update User
```http
PUT /api/users/{id}
```

#### Delete User
```http
DELETE /api/users/{id}
```

## 🏗️ Architecture

```
src/
├── main.c          # Server entry point and signal handling
├── server.c        # Socket initialization and binding
├── client.c        # Client handling and thread pool
├── http.c          # HTTP protocol implementation
├── parse_req.c     # Request parsing and validation
├── api.c           # RESTful API endpoints
└── utils/          # Header files
    ├── server.h
    ├── client.h
    ├── http.h
    └── parse_req.h
```

### Key Components

1. **Thread Pool**: Manages worker threads for concurrent request handling
2. **Request Parser**: Parses HTTP requests with headers and body
3. **HTTP Handler**: Implements HTTP protocol and response generation
4. **API Layer**: RESTful endpoints with JSON handling
5. **Static File Server**: Efficient file serving with security
6. **Metrics System**: Real-time performance monitoring

## 🔧 Development

### Building
```bash
make          # Build the server
make clean    # Clean build artifacts
```

### Adding New Features
1. **New API endpoints**: Add to `api.c`
2. **HTTP methods**: Extend `http.c`
3. **Request parsing**: Modify `parse_req.c`
4. **Threading**: Update `client.c`

### Code Style
- Consistent indentation and naming
- Comprehensive error handling
- Memory safety and resource cleanup
- Clear separation of concerns

## 🎯 Why This Project Stands Out

### For Backend Engineering Internships

1. **Systems Programming**: Demonstrates low-level C programming skills
2. **Concurrency**: Multi-threading with proper synchronization
3. **Network Programming**: Socket programming and HTTP protocol
4. **Performance**: Efficient resource management and optimization
5. **Security**: Input validation and attack prevention
6. **Monitoring**: Observability and metrics collection
7. **API Design**: RESTful principles and JSON handling
8. **Production-Ready**: Error handling, logging, and graceful shutdown

### Technical Highlights

- **Thread-safe operations** with mutex protection
- **Memory-efficient** with proper allocation/deallocation
- **Scalable architecture** with thread pool design
- **Security-conscious** with input validation
- **Developer-friendly** with comprehensive documentation
- **Production features** like health checks and metrics

## 🚀 Future Enhancements

- [ ] **Database Integration** (SQLite/PostgreSQL)
- [ ] **Authentication & Authorization** (JWT tokens)
- [ ] **Rate Limiting** and DDoS protection
- [ ] **HTTPS Support** (SSL/TLS)
- [ ] **WebSocket Support** for real-time communication
- [ ] **Configuration Management** (JSON/YAML config files)
- [ ] **Logging System** with different levels
- [ ] **Caching Layer** (Redis-like in-memory cache)
- [ ] **Load Balancing** support
- [ ] **Docker Containerization**

## 📝 License

This project is open source and available under the MIT License.

---

**Built with ❤️ for demonstrating backend engineering skills**
