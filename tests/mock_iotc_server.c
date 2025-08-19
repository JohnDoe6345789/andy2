
/*
 * Mock IOTC Server for Testing
 * 
 * This server simulates the ThroughTek IOTC infrastructure for testing purposes.
 * It implements basic IOTC protocol responses and can be used to test client
 * implementations without requiring real IOTC servers.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define DEFAULT_PORT 8080

typedef struct {
    int socket;
    struct sockaddr_in address;
    int active;
    char device_uid[21];
    time_t last_activity;
} client_info_t;

typedef struct {
    client_info_t clients[MAX_CLIENTS];
    int server_socket;
    int running;
    pthread_mutex_t clients_mutex;
    int port;
    int verbose;
} mock_server_t;

static mock_server_t server = {0};

void cleanup_and_exit(int signum) {
    printf("\nShutting down mock IOTC server...\n");
    server.running = 0;
    if (server.server_socket > 0) {
        close(server.server_socket);
    }
    exit(signum);
}

void setup_signal_handlers(void) {
    signal(SIGINT, cleanup_and_exit);
    signal(SIGTERM, cleanup_and_exit);
}

int find_free_client_slot(void) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!server.clients[i].active) {
            return i;
        }
    }
    return -1;
}

void handle_device_login(int client_socket, const char* uid) {
    pthread_mutex_lock(&server.clients_mutex);
    
    int slot = find_free_client_slot();
    if (slot >= 0) {
        server.clients[slot].socket = client_socket;
        server.clients[slot].active = 1;
        strncpy(server.clients[slot].device_uid, uid, sizeof(server.clients[slot].device_uid) - 1);
        server.clients[slot].last_activity = time(NULL);
        
        if (server.verbose) {
            printf("Device %s connected in slot %d\n", uid, slot);
        }
        
        // Send positive response
        const char* response = "IOTC_LOGIN_OK";
        send(client_socket, response, strlen(response), 0);
    } else {
        // Send error response
        const char* response = "IOTC_ER_EXCEED_MAX_SESSION";
        send(client_socket, response, strlen(response), 0);
    }
    
    pthread_mutex_unlock(&server.clients_mutex);
}

void handle_session_request(int client_socket, const char* uid) {
    // Find device by UID
    pthread_mutex_lock(&server.clients_mutex);
    
    int device_found = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i].active && strcmp(server.clients[i].device_uid, uid) == 0) {
            device_found = 1;
            break;
        }
    }
    
    if (device_found) {
        if (server.verbose) {
            printf("Session request for device %s - granted\n", uid);
        }
        
        // Send session info (simplified)
        char response[256];
        snprintf(response, sizeof(response), "IOTC_SESSION_OK:127.0.0.1:%d", server.port + 1);
        send(client_socket, response, strlen(response), 0);
    } else {
        if (server.verbose) {
            printf("Session request for device %s - device not found\n", uid);
        }
        
        const char* response = "IOTC_ER_DEVICE_NOT_LISTENING";
        send(client_socket, response, strlen(response), 0);
    }
    
    pthread_mutex_unlock(&server.clients_mutex);
}

void handle_heartbeat(int client_socket) {
    const char* response = "IOTC_HEARTBEAT_OK";
    send(client_socket, response, strlen(response), 0);
    
    // Update last activity for this client
    pthread_mutex_lock(&server.clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i].active && server.clients[i].socket == client_socket) {
            server.clients[i].last_activity = time(NULL);
            break;
        }
    }
    pthread_mutex_unlock(&server.clients_mutex);
}

void* client_handler(void* arg) {
    int client_socket = *(int*)arg;
    free(arg);
    
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    
    while (server.running) {
        bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            break;
        }
        
        buffer[bytes_received] = '\0';
        
        if (server.verbose) {
            printf("Received: %s\n", buffer);
        }
        
        // Parse and handle different message types
        if (strncmp(buffer, "DEVICE_LOGIN:", 13) == 0) {
            char* uid = buffer + 13;
            handle_device_login(client_socket, uid);
        } else if (strncmp(buffer, "SESSION_REQUEST:", 16) == 0) {
            char* uid = buffer + 16;
            handle_session_request(client_socket, uid);
        } else if (strcmp(buffer, "HEARTBEAT") == 0) {
            handle_heartbeat(client_socket);
        } else if (strcmp(buffer, "QUIT") == 0) {
            break;
        } else {
            // Unknown command
            const char* response = "IOTC_ER_INVALID_COMMAND";
            send(client_socket, response, strlen(response), 0);
        }
    }
    
    // Clean up client slot
    pthread_mutex_lock(&server.clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i].active && server.clients[i].socket == client_socket) {
            server.clients[i].active = 0;
            memset(&server.clients[i], 0, sizeof(client_info_t));
            if (server.verbose) {
                printf("Client disconnected from slot %d\n", i);
            }
            break;
        }
    }
    pthread_mutex_unlock(&server.clients_mutex);
    
    close(client_socket);
    return NULL;
}

void* cleanup_thread(void* arg) {
    (void)arg;
    
    while (server.running) {
        sleep(30); // Check every 30 seconds
        
        time_t now = time(NULL);
        pthread_mutex_lock(&server.clients_mutex);
        
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (server.clients[i].active && (now - server.clients[i].last_activity) > 300) {
                // Client inactive for 5 minutes
                if (server.verbose) {
                    printf("Cleaning up inactive client in slot %d\n", i);
                }
                close(server.clients[i].socket);
                server.clients[i].active = 0;
                memset(&server.clients[i], 0, sizeof(client_info_t));
            }
        }
        
        pthread_mutex_unlock(&server.clients_mutex);
    }
    
    return NULL;
}

int start_server(int port) {
    struct sockaddr_in server_addr;
    
    server.server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server.server_socket < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    int opt = 1;
    if (setsockopt(server.server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(server.server_socket);
        return -1;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server.server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server.server_socket);
        return -1;
    }
    
    if (listen(server.server_socket, 10) < 0) {
        perror("Listen failed");
        close(server.server_socket);
        return -1;
    }
    
    server.port = port;
    server.running = 1;
    pthread_mutex_init(&server.clients_mutex, NULL);
    
    printf("Mock IOTC server started on port %d\n", port);
    return 0;
}

void run_server(void) {
    pthread_t cleanup_tid;
    pthread_create(&cleanup_tid, NULL, cleanup_thread, NULL);
    
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    while (server.running) {
        int client_socket = accept(server.server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (server.running) {
                perror("Accept failed");
            }
            continue;
        }
        
        if (server.verbose) {
            printf("New client connected from %s:%d\n", 
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        }
        
        int* client_sock_ptr = malloc(sizeof(int));
        *client_sock_ptr = client_socket;
        
        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, client_handler, client_sock_ptr) != 0) {
            perror("Thread creation failed");
            close(client_socket);
            free(client_sock_ptr);
        } else {
            pthread_detach(client_thread);
        }
    }
    
    pthread_cancel(cleanup_tid);
    pthread_join(cleanup_tid, NULL);
}

void print_usage(const char* program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -p <port>    Port to listen on (default: %d)\n", DEFAULT_PORT);
    printf("  -v           Verbose output\n");
    printf("  -h           Show this help message\n");
    printf("\nCommands (send via telnet or nc):\n");
    printf("  DEVICE_LOGIN:<uid>     Register a device\n");
    printf("  SESSION_REQUEST:<uid>  Request session to device\n");
    printf("  HEARTBEAT              Send heartbeat\n");
    printf("  QUIT                   Disconnect\n");
}

int main(int argc, char* argv[]) {
    int port = DEFAULT_PORT;
    int verbose = 0;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
            if (port <= 0 || port > 65535) {
                fprintf(stderr, "Invalid port number: %d\n", port);
                return 1;
            }
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    server.verbose = verbose;
    setup_signal_handlers();
    
    if (start_server(port) != 0) {
        return 1;
    }
    
    printf("Mock IOTC server ready for connections\n");
    printf("Use Ctrl+C to stop the server\n");
    
    run_server();
    
    printf("Server shutting down...\n");
    pthread_mutex_destroy(&server.clients_mutex);
    return 0;
}
