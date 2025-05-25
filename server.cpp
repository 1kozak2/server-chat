#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <winsock2.h>
#include <windows.h>
#include <algorithm>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345
#define BUFFER_SIZE 1024

struct ClientInfo {
    SOCKET socket;
    std::string name;
};

std::vector<ClientInfo> clients;
CRITICAL_SECTION clients_lock; // Manual synchronization object

// Little menu that welcomes the person and asks for their name instead of writing the port
const std::string NAME_PROMPT =
        "+-------------------------------------------+\r\n"
        "|             WELCOME TO CHAT SERVER        |\r\n"
        "+-------------------------------------------+\r\n\r\n"
        "Please enter your name: ";

// Welcome message with ASCII art + instructions how to use the chat
const std::string WELCOME_MESSAGE =
        "\r\n"
        "+-------------------------------------------+\r\n"
        "|            WELCOME TO LITTLE CHAT         |\r\n"
        "+-------------------------------------------+\r\n\r\n"
        "INSTRUCTIONS:\r\n\r\n"
        "  > Type your message and press Enter to send\r\n"
        "  > All messages will be broadcast to all connected users\r\n"
        "  > Type 'exit' to disconnect\r\n\r\n"
        "+-------------------------------------------+\r\n"
        "| Enjoy chatting!                           |\r\n"
        "+-------------------------------------------+\r\n\r\n";

// Function to get current timestamp, will be next to the message itself
std::string get_timestamp() {
    time_t now = time(0);
    struct tm* timeinfo = localtime(&now);
    char buffer[9];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
    return std::string(buffer);
}

// Format of the message so its readable and visible
std::string format_message(const std::string& name, const std::string& message) {
    std::string timestamp = get_timestamp();
    return "\r\n[" + timestamp + "] " + name + ": " + message + "\r\n";
}

void broadcast(const std::string& message, SOCKET sender_socket) {
    EnterCriticalSection(&clients_lock); // BEGIN CRITICAL SECTION
    for (SOCKET client : clients) {
        if (client != sender_socket) {
            send(client, message.c_str(), message.length(), 0);
        }
    }
    LeaveCriticalSection(&clients_lock); // END CRITICAL SECTION
}


// Function to get clients name instead of port number
std::string get_client_name(SOCKET client_socket) {
    // Ask for client's name with simple prompt
    send(client_socket, NAME_PROMPT.c_str(), NAME_PROMPT.length(), 0);

    char buffer[BUFFER_SIZE];
    std::string name;

    // Read characters until we get a newline
    while (true) {
        int bytes_received = recv(client_socket, buffer, 1, 0); // Read one character at a time
        if (bytes_received <= 0) {
            return "Anonymous";
        }

        char c = buffer[0];

        // If it's a newline or carriage return, we're done
        if (c == '\n' || c == '\r') {
            break;
        }

        // If it's a backspace, remove last character
        if (c == '\b' || c == 127) { // 127 is DEL character
            if (!name.empty()) {
                name.pop_back();
                // Send backspace sequence to client for visual feedback
                const char backspace_seq[] = "\b \b";
                send(client_socket, backspace_seq, 3, 0);
            }
        }
            // If it's a printable character, add it to name
        else if (c >= 32 && c <= 126) {
            name += c;
            // Echo the character back to client
            send(client_socket, &c, 1, 0);
        }
    }

    if (name.empty()) {
        name = "Anonymous";
    }

    return name;
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
