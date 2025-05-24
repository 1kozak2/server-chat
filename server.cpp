#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <winsock2.h>
#include <windows.h>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345
#define BUFFER_SIZE 1024

std::vector<SOCKET> clients;
CRITICAL_SECTION clients_lock; // Manual synchronization object

void broadcast(const std::string& message, SOCKET sender_socket) {
    EnterCriticalSection(&clients_lock); // BEGIN CRITICAL SECTION
    for (SOCKET client : clients) {
        if (client != sender_socket) {
            send(client, message.c_str(), message.length(), 0);
        }
    }
    LeaveCriticalSection(&clients_lock); // END CRITICAL SECTION
}

void handle_client(SOCKET client_socket) {
    char buffer[BUFFER_SIZE];

    while (true) {
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            break;
        }

        buffer[bytes_received] = '\0';
        std::string msg = "Client " + std::to_string((int)client_socket) + ": " + buffer;
        std::cout << msg;
        broadcast(msg, client_socket);
    }

    closesocket(client_socket);

    // Remove client
    EnterCriticalSection(&clients_lock);
    clients.erase(std::remove(clients.begin(), clients.end(), client_socket), clients.end());
    LeaveCriticalSection(&clients_lock);

    std::cout << "Client " << client_socket << " disconnected." << std::endl;
}

int main() {
    // Init WinSock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa)) {
        std::cerr << "Failed to initialize WinSock.\n";
        return 1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed.\n";
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed.\n";
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Chat server running on port " << PORT << "...\n";

    InitializeCriticalSection(&clients_lock); // Init lock

    while (true) {
        sockaddr_in client_addr{};
        int client_len = sizeof(client_addr);
        SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed.\n";
            continue;
        }

        // Add to client list
        EnterCriticalSection(&clients_lock);
        clients.push_back(client_socket);
        LeaveCriticalSection(&clients_lock);

        std::thread(handle_client, client_socket).detach(); // Spawn new thread
    }

    DeleteCriticalSection(&clients_lock);
    closesocket(server_socket);
    WSACleanup();

    return 0;
}
