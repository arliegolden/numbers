#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <asm-generic/socket.h>

#define PORT 8080
#define BUFFER_SIZE 8192
#define SMALL_BUFFER 1024

typedef struct {
    int client_socket;
    struct sockaddr_in client_addr;
} client_info;

// Structure to map file extensions to MIME types
typedef struct {
    const char *extension;
    const char *mime_type;
} mime_map;

// Expanded list of MIME types
mime_map mime_types[] = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"},
    {".ico", "image/x-icon"},
    {".txt", "text/plain"},
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".rar", "application/x-rar-compressed"},
    {".mp3", "audio/mpeg"},
    {".mp4", "video/mp4"},
    // Add more MIME types as needed
    {NULL, NULL} // Sentinel to mark end of array
};

// Function to determine MIME type based on file extension
const char* get_mime_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) {
        return "application/octet-stream"; // Default MIME type
    }

    for (int i = 0; mime_types[i].extension != NULL; i++) {
        if (strcasecmp(ext, mime_types[i].extension) == 0) {
            return mime_types[i].mime_type;
        }
    }
    return "application/octet-stream"; // Default MIME type
}

// Function to send HTTP responses with a message body
void send_response(int client_socket, const char *status, const char *content_type, const void *body, size_t body_length) {
    char header[SMALL_BUFFER];
    int header_length = snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        status, content_type, body_length);

    send(client_socket, header, header_length, 0);
    send(client_socket, body, body_length, 0);
}

// Function to send HTTP responses without a message body
void send_simple_response(int client_socket, const char *status, const char *content_type) {
    char header[SMALL_BUFFER];
    int header_length = snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n\r\n",
        status, content_type);

    send(client_socket, header, header_length, 0);
}

// Function to serve static files
void serve_static_file(int client_socket, const char *path) {
    // Prevent directory traversal
    if (strstr(path, "..")) {
        send_simple_response(client_socket, "400 Bad Request", "text/plain");
        return;
    }

    // Handle root path
    char file_path[SMALL_BUFFER] = ".";
    strcat(file_path, path);

    // Open the requested file
    int file_fd = open(file_path, O_RDONLY);
    if (file_fd == -1) {
        // File not found
        send_simple_response(client_socket, "404 Not Found", "text/plain");
        return;
    }

    // Get file size
    struct stat st;
    if (fstat(file_fd, &st) == -1) {
        perror("fstat failed");
        send_simple_response(client_socket, "500 Internal Server Error", "text/plain");
        close(file_fd);
        return;
    }
    long file_size = st.st_size;

    // Determine MIME type
    const char *mime_type = get_mime_type(file_path);

    // Create response headers
    char header[SMALL_BUFFER];
    int header_length = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n\r\n",
        mime_type, file_size);

    // Send headers
    send(client_socket, header, header_length, 0);

    // Send file content
    ssize_t bytes;
    char buffer[BUFFER_SIZE];
    while ((bytes = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
        send(client_socket, buffer, bytes, 0);
    }

    close(file_fd);
}

// Function to handle POST /ping
void handle_post_ping(int client_socket, const char *body, size_t body_length, struct timeval start_time) {
    (void)body; // Unused in this handler

    // Create JSON response
    char response_body[SMALL_BUFFER];
    int response_length = snprintf(response_body, sizeof(response_body),
        "{ \"response\": \"pong\" }");

    // Send JSON response
    send_response(client_socket, "200 OK", "application/json", response_body, response_length);
}

// Function to handle client requests
void *handle_client(void *arg) {
    client_info *cinfo = (client_info *)arg;
    int client_socket = cinfo->client_socket;
    char buffer[BUFFER_SIZE];
    ssize_t received;

    // Start timing
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // Receive data
    received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (received < 0) {
        perror("recv failed");
        close(client_socket);
        free(cinfo);
        pthread_exit(NULL);
    }
    buffer[received] = '\0';

    // Parse the request line
    char method[SMALL_BUFFER];
    char path[SMALL_BUFFER];
    char protocol[SMALL_BUFFER];
    sscanf(buffer, "%s %s %s", method, path, protocol);

    printf("Received request: %s %s %s\n", method, path, protocol);

    // Route the request
    if (strcasecmp(method, "GET") == 0) {
        serve_static_file(client_socket, path);
    }
    else if (strcasecmp(method, "POST") == 0 && strcmp(path, "/ping") == 0) {
        // Find the start of the body
        char *body = strstr(buffer, "\r\n\r\n");
        if (body) {
            body += 4; // Move past the "\r\n\r\n"
            size_t body_length = received - (body - buffer);
            handle_post_ping(client_socket, body, body_length, start_time);
        }
        else {
            send_simple_response(client_socket, "400 Bad Request", "text/plain");
        }
    }
    else {
        // Method not supported
        send_simple_response(client_socket, "501 Not Implemented", "text/plain");
    }

    close(client_socket);
    free(cinfo);
    pthread_exit(NULL);
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    // Create socket file descriptor (IPv4, TCP)
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to allow reuse of address and port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Define the server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    address.sin_port = htons(PORT);

    // Bind the socket to the address and port
    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_fd, 16) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("HTTP Server is running on port %d\n", PORT);

    // Main loop to accept incoming connections
    while (1) {
        client_info *cinfo = malloc(sizeof(client_info));
        if (!cinfo) {
            perror("malloc failed");
            continue;
        }
        socklen_t addr_len = sizeof(cinfo->client_addr);
        cinfo->client_socket = accept(server_fd, (struct sockaddr *)&cinfo->client_addr, &addr_len);
        if (cinfo->client_socket < 0) {
            perror("accept failed");
            free(cinfo);
            continue;
        }

        // Create a new thread to handle the client
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, (void *)cinfo) != 0) {
            perror("pthread_create failed");
            close(cinfo->client_socket);
            free(cinfo);
            continue;
        }

        // Detach the thread so that resources are freed upon completion
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}
