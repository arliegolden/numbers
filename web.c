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
#include <dirent.h>

#define PORT 8080
#define BUFFER_SIZE 8192
#define SMALL_BUFFER 1024
#define ROOT_DIR "./www" // Define the root directory

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
        printf("No extension found for path: %s. Defaulting to application/octet-stream\n", path);
        return "application/octet-stream"; // Default MIME type
    }

    for (int i = 0; mime_types[i].extension != NULL; i++) {
        if (strcasecmp(ext, mime_types[i].extension) == 0) {
            printf("MIME type for %s: %s\n", path, mime_types[i].mime_type);
            return mime_types[i].mime_type;
        }
    }
    printf("Unknown extension '%s' for path: %s. Defaulting to application/octet-stream\n", ext, path);
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

    if (send(client_socket, header, header_length, 0) == -1) {
        perror("send header failed");
        return;
    }
    if (send(client_socket, body, body_length, 0) == -1) {
        perror("send body failed");
        return;
    }
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

    if (send(client_socket, header, header_length, 0) == -1) {
        perror("send simple response failed");
    }
}

// Function to generate HTML for directory listing
void generate_directory_listing(const char *dir_path, char *output, size_t output_size) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char full_path[SMALL_BUFFER];
    char time_str[80];
    
    // Start the HTML content
    int written = snprintf(output, output_size,
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<title>Directory listing</title>\n"
        "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/jquery/3.7.1/jquery.min.js\"></script>\n"
        "</head>\n"
        "<body>\n"
        "<div id=\"listing\">\n"
        "<h1>Directory listing for %s</h1>\n"
        "<table id=\"files\">\n"
        "<thead><tr><th>Name</th><th>Size</th><th>Last Modified</th></tr></thead>\n"
        "<tbody>\n", dir_path);

    output += written;
    output_size -= written;

    dir = opendir(dir_path);
    if (dir) {
        // Add parent directory link if not in root
        if (strcmp(dir_path, ROOT_DIR) != 0) {
            written = snprintf(output, output_size,
                "<tr><td><a href=\"..\">..</a></td><td>-</td><td>-</td></tr>\n");
            output += written;
            output_size -= written;
        }

        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0) continue;
            if (strcmp(entry->d_name, "..") == 0 && strcmp(dir_path, ROOT_DIR) == 0) continue;

            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
            
            if (stat(full_path, &file_stat) == 0) {
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&file_stat.st_mtime));
                
                const char *size_str;
                char size_buf[20];
                
                if (S_ISDIR(file_stat.st_mode)) {
                    size_str = "-";
                    written = snprintf(output, output_size,
                        "<tr><td><a href=\"%s/\">%s/</a></td><td>%s</td><td>%s</td></tr>\n",
                        entry->d_name, entry->d_name, size_str, time_str);
                } else {
                    if (file_stat.st_size < 1024)
                        snprintf(size_buf, sizeof(size_buf), "%ld B", file_stat.st_size);
                    else if (file_stat.st_size < 1024 * 1024)
                        snprintf(size_buf, sizeof(size_buf), "%.1f KB", file_stat.st_size / 1024.0);
                    else if (file_stat.st_size < 1024 * 1024 * 1024)
                        snprintf(size_buf, sizeof(size_buf), "%.1f MB", file_stat.st_size / (1024.0 * 1024.0));
                    else
                        snprintf(size_buf, sizeof(size_buf), "%.1f GB", file_stat.st_size / (1024.0 * 1024.0 * 1024.0));
                    
                    written = snprintf(output, output_size,
                        "<tr><td><a href=\"%s\">%s</a></td><td>%s</td><td>%s</td></tr>\n",
                        entry->d_name, entry->d_name, size_buf, time_str);
                }
                output += written;
                output_size -= written;
            }
        }
        closedir(dir);
    }

    // Add the JavaScript for sorting and the closing tags
    written = snprintf(output, output_size,
        "</tbody>\n"
        "</table>\n"
        "<script>\n"
        "$(document).ready(function() {\n"
        "  let sortOrder = 1;\n"
        "  $('th').click(function() {\n"
        "    const table = $(this).parents('table').eq(0);\n"
        "    const rows = table.find('tr:gt(0)').toArray().sort(compare($(this).index()));\n"
        "    sortOrder = -sortOrder;\n"
        "    $.each(rows, function(index, row) {\n"
        "      table.children('tbody').append(row);\n"
        "    });\n"
        "  });\n"
        "  function compare(index) {\n"
        "    return function(a, b) {\n"
        "      const valA = getCellValue(a, index);\n"
        "      const valB = getCellValue(b, index);\n"
        "      return $.isNumeric(valA) && $.isNumeric(valB) ?\n"
        "        sortOrder * (valA - valB) :\n"
        "        sortOrder * valA.localeCompare(valB);\n"
        "    };\n"
        "  }\n"
        "  function getCellValue(row, index) {\n"
        "    return $(row).children('td').eq(index).text();\n"
        "  }\n"
        "});\n"
        "</script>\n"
        "</div>\n"
        "</body>\n"
        "</html>");

    output += written;
}

// Function to serve static files and directory listings
void serve_static_file(int client_socket, const char *path) {
    // Prevent directory traversal
    if (strstr(path, "..")) {
        send_simple_response(client_socket, "400 Bad Request", "text/plain");
        printf("Directory traversal attempt detected: %s\n", path);
        return;
    }

    char file_path[SMALL_BUFFER] = ROOT_DIR;
    strcat(file_path, path);

    // Remove trailing slash for stat
    size_t len = strlen(file_path);
    if (len > 1 && file_path[len - 1] == '/') {
        file_path[len - 1] = '\0';
    }

    struct stat path_stat;
    if (stat(file_path, &path_stat) == -1) {
        send_simple_response(client_socket, "404 Not Found", "text/plain");
        printf("File not found: %s\n", file_path);
        return;
    }

    // Handle directory
    if (S_ISDIR(path_stat.st_mode)) {
        // Check for index.html
        char index_path[SMALL_BUFFER];
        snprintf(index_path, sizeof(index_path), "%s/index.html", file_path);
        
        if (access(index_path, F_OK) != -1) {
            // Serve index.html
            strcat(strcpy(file_path, path), "index.html");
            serve_static_file(client_socket, file_path);
            return;
        }

        // Generate directory listing
        char *listing = malloc(BUFFER_SIZE * 4); // Allocate larger buffer for directory listing
        if (!listing) {
            send_simple_response(client_socket, "500 Internal Server Error", "text/plain");
            return;
        }

        generate_directory_listing(file_path, listing, BUFFER_SIZE * 4);
        send_response(client_socket, "200 OK", "text/html", listing, strlen(listing));
        free(listing);
        return;
    }

    // Handle regular file
    int file_fd = open(file_path, O_RDONLY);
    if (file_fd == -1) {
        send_simple_response(client_socket, "404 Not Found", "text/plain");
        printf("File not found: %s\n", file_path);
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
    printf("Serving file: %s (Size: %ld bytes)\n", file_path, file_size);

    // Determine MIME type
    const char *mime_type = get_mime_type(file_path);
    printf("MIME type: %s\n", mime_type);

    // Create response headers
    char header[SMALL_BUFFER];
    int header_length = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n\r\n",
        mime_type, file_size);

    // Send headers
    if (send(client_socket, header, header_length, 0) == -1) {
        perror("send header failed");
        close(file_fd);
        return;
    }

    // Send file content with proper send handling
    ssize_t bytes;
    char buffer[BUFFER_SIZE];
    while ((bytes = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
        ssize_t total_sent = 0;
        while (total_sent < bytes) {
            ssize_t sent = send(client_socket, buffer + total_sent, bytes - total_sent, 0);
            if (sent == -1) {
                perror("send failed");
                close(file_fd);
                return;
            }
            total_sent += sent;
        }
    }

    if (bytes == -1) {
        perror("read failed");
    }

    close(file_fd);
}

// Function to handle POST /ping
void handle_post_ping(int client_socket, const char *body) {
    (void)body; // Unused in this handler

    // Create JSON response
    char response_body[SMALL_BUFFER];
    int response_length = snprintf(response_body, sizeof(response_body),
        "{ \"response\": \"Pong!\" }"
    );

    // Send JSON response
    send_response(client_socket, "200 OK", "application/json", response_body, response_length);
}

// Function to handle client requests
void *handle_client(void *arg) {
    client_info *cinfo = (client_info *)arg;
    int client_socket = cinfo->client_socket;
    char buffer[BUFFER_SIZE];
    ssize_t received;

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
            handle_post_ping(client_socket, body);
        }
        else {
            send_simple_response(client_socket, "400 Bad Request", "text/plain");
            printf("Bad request: No body found\n");
        }
    }
    else {
        // Method not supported
        send_simple_response(client_socket, "501 Not Implemented", "text/plain");
        printf("Unsupported method or path: %s %s\n", method, path);
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