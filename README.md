# Multithreaded Chat Server in C++ (Windows, WinSock)

## 🧠 Project Overview

This project is a multithreaded TCP chat server written in C++ for Windows, using the WinSock2 networking API. It supports multiple clients simultaneously by spawning a separate thread for each connection and safely synchronizes access to shared data using Windows `CRITICAL_SECTION`.

The main goals of the project are:
- Practice manual implementation of multithreading and synchronization
- Understand concurrent access issues in networked systems
- Implement core functionality of a real-time chat server

---

## 🧪 Theory & Motivation

Multithreaded programming allows programs to handle multiple operations concurrently — crucial for real-time applications like chat servers, where many clients can send/receive messages simultaneously.

**Challenges in such systems include:**
- Safely accessing shared data (like a list of client sockets)
- Avoiding race conditions and deadlocks
- Efficient communication with minimal blocking

This project **does not rely on ready-made synchronization classes**, aligning with educational goals: the developer must explicitly control thread safety.

---

## ⚙️ Implementation Details

### ➕ Technologies Used
- C++11 (for threads)
- Windows Sockets API (`winsock2.h`)
- Windows thread synchronization primitives (`CRITICAL_SECTION`)

### 🔁 Server Workflow
1. Initialize WinSock (WSAStartup)
2. Create a TCP server socket and bind to port 12345
3. Accept new client connections in a loop
4. For each client:
   - Store the socket in a shared vector
   - Launch a thread to handle receiving messages
   - Broadcast received messages to all other clients

### 🔒 Synchronization
Access to the shared `std::vector<SOCKET> clients` is protected using a **manually managed `CRITICAL_SECTION`** to ensure:
- Thread-safe additions/removals
- Consistent broadcasting to all clients

> This manual locking replaces high-level constructs like `std::mutex`, which are not permitted in this project.

---

## 🧵 Threads and Their Roles

| Thread               | Description |
|----------------------|-------------|
| `main()` thread      | Initializes the server, listens for incoming connections, spawns threads |
| `handle_client()`    | Created for every client — handles receiving data and forwarding messages |

---

## 🚨 Critical Sections

| Section | Purpose | Protected By |
|--------|---------|--------------|
| `clients.push_back()` | Add new client to list | `EnterCriticalSection(&clients_lock)` |
| `clients.erase()`     | Remove client on disconnect | Same |
| `broadcast()`         | Iterate and send to all sockets | Same |

---

## ▶️ How to Build & Run

### 🧱 Requirements
- Windows 10/11
- `g++` with MinGW/MSYS2 or Visual Studio
- No external libraries (pure WinSock + C++)

### 🏗️ Compilation

Using MSYS2 / MinGW:

```bash
g++ server.cpp -o server.exe -lws2_32 -std=c++11
