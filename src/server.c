
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <asm-generic/socket.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#ifndef BUILD_DIR
#error "Need to definie BUILD_DIR as your build directory"
#endif

#define PORT 8001

int32_t main(void) {
    int32_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        fprintf(stderr, "Failed to create server socket\n");
        exit(EXIT_FAILURE);
    }

    int32_t opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        fprintf(stderr, "Failed to set server socket option\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    int32_t addrlen = sizeof(address);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        fprintf(stderr, "Failed to bind server socket\n");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        fprintf(stderr, "Failed to listen on server socket\n");
        exit(EXIT_FAILURE);
    }

    printf("Server running on http://localhost:%d\n", PORT);

    while (1) {
        int32_t new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        #define BUFFER_SIZE 4086
        char input[BUFFER_SIZE];
        memset(input, 0, BUFFER_SIZE);
        int val_read = read(new_socket, input, BUFFER_SIZE - 1);
        if (val_read <= 0) {
            close(new_socket);
            continue;
        }

        char method[10], path[256];
        sscanf(input, "%s %s", method, path);

        printf("--- New Request: %s %s ---\n", method, path);

        char* filename = NULL;
        char* content_type = NULL;

        if (strcmp(method, "GET") == 0) {
            if (strcmp(path, "/style.css") == 0) {
                filename = "style.css";
                content_type = "text/css";
            } 
            else if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
                filename = "index.html";
                content_type = "text/html";
            } 
            else if (strcmp(path, "/index.js") == 0) {
                filename = "index.js";
                content_type = "application/javascript";
            }
            else if (strcmp(path, "/index.wasm") == 0) {
                filename = "index.wasm";
                content_type = "application/wasm";
            }
            else {
                printf("Result: 404 Not Found for %s\n", path);
                const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
                write(new_socket, not_found, strlen(not_found));
                close(new_socket);
                continue;
            }
        }

        FILE *file = fopen(filename, "rb");
        if (!file) {
            printf("Result: File Error opening %s\n", filename);
            close(new_socket);
            continue;
        }

        fseek(file, 0, SEEK_END);
        int64_t size = ftell(file);
        rewind(file);

        char buffer[size + 1];

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
        while ((bytes = fread(buffer, 1, size, file)) > 0) {
            write(new_socket, buffer, bytes);
        }

        printf("Result: Served %s (%lld bytes)\n", filename, (long long)st.st_size);
        fclose(file);
        close(new_socket);
    }


    return 0;
}

