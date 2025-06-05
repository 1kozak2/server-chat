# Windows Multithreaded Chat Server

This project is a multithreaded TCP chat server written in C++ for Windows. It uses the **WinSock2** networking API and manual synchronization via **CRITICAL_SECTION** to manage concurrent clients. Each connected user is prompted to enter a name and is then placed into a shared chatroom, where all sent messages are **timestamped** and **broadcast** to all participants.

---

##  Overview

The server accepts multiple clients simultaneously and handles each in a separate thread. Incoming messages from one client are:

- Formatted with a timestamp and their username.
- Broadcast to all connected clients.

This implementation uses low-level socket programming and manual concurrency control to demonstrate a foundational networking and threading approach in C++ on Windows.

---

##  Features

- Prompts each client for a display name (no IP/port IDs shown).
- Clients receive a welcome banner and usage instructions.
- All messages include a timestamp.
- Messages are echoed back to the sender and broadcast to all clients.
- Supports backspace and character echoing for terminal-style input.
- Clients can disconnect gracefully by typing `exit`.
- Server logs connection and disconnection events.
- Uses `std::thread` for concurrent client handling.
- Shared resources are protected using `CRITICAL_SECTION`.

---

##  Prerequisites

- Windows OS (Windows 10 or newer recommended)
- C++17 or later
- C++ compiler that supports WinSock2 (e.g., Visual Studio)
- Link against `ws2_32.lib`

---

##  Compilation & Setup

### Visual Studio

1. Open a new Console Application project.
2. Add `chat_server.cpp` to the project.
3. Link against `ws2_32.lib`.
4. Enable multithreaded runtime support (`/MT` or `/MD` depending on your setup).

### Command Line (MSVC)

```bash
cl /EHsc chat_server.cpp /link ws2_32.lib
```

---

##  How the Server Works

1. **WinSock Initialization**  
   The server initializes the Windows Sockets API (WinSock):
   ```cpp
   WSAStartup(MAKEWORD(2, 2), &wsa);
   ```
   This is required before using any socket-related functions.

2. **Server Setup**  
   - Creates a TCP socket with IPv4 (`AF_INET`) and `SOCK_STREAM`.
   - Binds to port `12345`.
   - Starts listening for connections using `listen()`.

3. **Accepting Clients**  
   ```cpp
   SOCKET client_socket = accept(...);
   ```
   - Each new client is accepted and a new thread is created for communication.
   - The socket is added to a global list of active connections.

4. **Reading Client Name**  
   - The `get_client_name()` function reads input one character at a time.
   - Supports backspace and echoes input to mimic a terminal.
   - Used for client identification in the chat.

5. **Welcome Banner**  
   - After entering a name, the client receives a welcome message and usage instructions using a predefined `WELCOME_MESSAGE`.

6. **Message Handling**  
   - Each client's thread reads input character-by-character.
   - When Enter is pressed (`\n` or `\r`):
     - If the message is `"exit"`, the client is disconnected.
     - Otherwise:
       - The message is formatted using `format_message()` with a timestamp and username.
       - Then broadcast using `broadcast_to_all()`.

7. **Client Disconnection**  
   - Triggered by `"exit"` or network disconnect.
   - Socket is closed and removed from the client list.
   - A disconnection message is sent to remaining clients.

---

##  Code Breakdown

###  WinSock Initialization
```cpp
WSAStartup(MAKEWORD(2, 2), &wsa);
```
Initializes WinSock. All networking requires this call to succeed first.

###  Server Setup
- Uses:
  ```cpp
  AF_INET, SOCK_STREAM, IPPROTO_TCP
  ```
- Binds to port `12345`.
- Uses `listen()` to wait for incoming connections.

###  Accepting Clients
```cpp
SOCKET client_socket = accept(...);
```
When a client connects:
- A new thread is created.
- The socket is stored in a `std::vector<ClientInfo>`.

###  Reading Client Name
- Done via `get_client_name()`.
- Reads input one character at a time.
- Echoes characters and supports backspace.

###  Welcome Banner
- A message string (constant `WELCOME_MESSAGE`) is sent after login.

###  Message Handling
- Executed inside `handle_client()`.
- Reads and accumulates characters.
- On newline:
  - Checks for `"exit"`.
  - Else: adds timestamp, formats the message, and broadcasts it.

---

##  Shared State & Synchronization

All active clients are stored in:
```cpp
std::vector<ClientInfo> clients;
```

To prevent race conditions, a critical section is used:
```cpp
CRITICAL_SECTION clients_lock;
```

### Usage:
```cpp
InitializeCriticalSection(&clients_lock);

EnterCriticalSection(&clients_lock);
// Critical section
LeaveCriticalSection(&clients_lock);

DeleteCriticalSection(&clients_lock);
```

This ensures only one thread can modify the shared data at a time.

---

##  Theoretical Background

## Multithreading 

Multithreading allows the server to handle multiple clients concurrently without blocking or slowing down other connections. In this chat server, each client connection is assigned to a separate thread using C++’s `std::thread` class from the Standard Library.

### Why Multithreading?

Without multithreading, the server would have to handle one client at a time. A blocking operation such as `recv()` (which waits for incoming data) would prevent the server from responding to other clients. Multithreading solves this by running each client's communication in an independent execution flow.

### How It’s Used in the Server

When a new client connects, the main server loop accepts the connection using `accept()` and immediately spawns a new thread to handle that specific client:

```cpp
std::thread client_thread(handle_client, client_socket);
client_thread.detach();
```

Each thread:
- Prompts for and stores the client’s display name.
- Continuously listens for input using `recv()`.
- Processes and broadcasts messages.
- Handles client disconnection and cleanup.

Using `.detach()` makes the thread self-managing, meaning it will clean up after itself once execution finishes—without needing to be joined by the main thread.

### Critical Considerations

#### 1. Concurrency Control

Multiple threads may access shared resources (like the `clients` list) simultaneously. Without protection, this would result in race conditions and undefined behavior. To prevent this, the server uses a Windows-specific synchronization primitive:

```cpp
CRITICAL_SECTION clients_lock;
```

All accesses to shared resources are wrapped with:

```cpp
EnterCriticalSection(&clients_lock);
// modify shared resource
LeaveCriticalSection(&clients_lock);
```

#### 2. Scalability

This one-thread-per-client model works well for a small number of users, but threads are expensive in terms of memory and CPU context-switching. For large-scale systems, thread pools or asynchronous I/O models (like IOCP or `epoll` in Linux) are preferred.

#### 3. Thread Lifecycle

By detaching threads, they become "fire-and-forget". While convenient, this makes it harder to manage or track their state later. In more complex applications, you might consider storing `std::thread` objects and joining them during shutdown.

###  TCP/IP & Sockets
- TCP provides:
  - Ordered delivery
  - Reliable transmission
  - Error checking
- A socket is a bidirectional communication endpoint.

###  Character-by-Character Input
- The server processes input per character to:
  - Support real-time backspace.
  - Echo characters to the terminal.
  - Filter unsafe input.
- This greatly improves client UX, especially with Telnet.

---

##  Limitations

-  No encryption — all communication is plaintext.
-  No authentication or login system.
-  No private messaging or command parsing.
-  No username conflict resolution.
-  No message history or logging to file.

---

