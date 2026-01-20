
#include <asm-generic/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT 8001
#define BUFFER_SIZE 4086

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server running on http://localhost:%d\n", PORT);

   while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        memset(buffer, 0, BUFFER_SIZE);
        int val_read = read(new_socket, buffer, BUFFER_SIZE - 1);
        if (val_read <= 0) {
            close(new_socket);
            continue;
        }

        char method[10], path[256];
        sscanf(buffer, "%s %s", method, path);
        
        // LOGGING: This is the most important part right now
        printf("--- New Request: %s %s ---\n", method, path);

        char* filename = NULL;
        char* content_type = NULL;

        if (strstr(path, "style2.css") != NULL) {
            filename = "style2.css";
            content_type = "text/css";
        } 
        else if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
            filename = "index.html";
            content_type = "text/html";
        } 
        else {
            printf("Result: 404 Not Found for %s\n", path);
            const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            write(new_socket, not_found, strlen(not_found));
            close(new_socket);
            continue;
        }

        FILE *file = fopen(filename, "rb");
        if (!file) {
            printf("Result: File Error opening %s\n", filename);
            close(new_socket);
            continue;
        }

        struct stat st;
        stat(filename, &st);
        
        char header[512];
        snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %lld\r\n"
            "Connection: close\r\n"
            "\r\n", content_type, (long long)st.st_size);

        write(new_socket, header, strlen(header));

        size_t bytes;
        while ((bytes = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
            write(new_socket, buffer, bytes);
        }

        printf("Result: Served %s (%lld bytes)\n", filename, (long long)st.st_size);
        fclose(file);
        close(new_socket);
    }


    return 0;
}

