# Multithreaded Chat Server in C++ (Windows, WinSock, Mutex)

## 🧠 Project Overview

This project implements a multithreaded TCP chat server in C++ using the Windows Sockets API (`WinSock2`) and manual thread synchronization using **Windows mutexes and semaphores**.

It creates a separate thread for each client and broadcasts messages between them, while ensuring thread-safe access to shared resources like the list of connected clients.

This project is designed to:
- Explore manual synchronization in multithreaded applications
- Avoid high-level libraries or auto-managed synchronization
- Develop a real-time communication server from scratch

---

## 🧪 Theory & Motivation

In a multithreaded environment, multiple clients can interact with shared memory (e.g., a list of sockets). This requires protection against race conditions.

This implementation uses:
- **Mutex (via `CreateMutex`)** to protect access to shared state
- **Optional Semaphore (via `CreateSemaphore`)** to limit the number of concurrent clients (disabled by default)

This approach enforces **manual, low-level synchronization**, as required in the assignment — avoiding built-in features like `std::mutex` or `CRITICAL_SECTION`.

---

## ⚙️ Implementation Details

### 🔧 Technologies
- C++11 (threads)
- WinSock2 (network sockets)
- Windows API: `HANDLE`, `CreateMutex`, `CreateSemaphore`

### 🔁 Server Flow
1. Initialize WinSock
2. Create TCP socket, bind to port `12345`, and listen
3. Accept new clients in a loop
4. For each new client:
   - Add them to a shared `std::vector<SOCKET>` (protected by mutex)
   - Launch a thread to handle communication
5. Broadcast incoming messages to all connected clients

### 🔐 Synchronization

| Operation         | Mechanism         | API Call                        |
|------------------|-------------------|---------------------------------|
| Shared access     | Mutex             | `CreateMutex`, `WaitForSingleObject`, `ReleaseMutex` |
| (Optional) client limit | Semaphore         | `CreateSemaphore`, `ReleaseSemaphore`   |

---

## 🧵 Threads and Their Responsibilities

| Thread             | Role                                        |
|--------------------|---------------------------------------------|
| `main()`           | Accepts client connections, starts threads  |
| `handle_client()`  | Receives from a single client and broadcasts |

---

## 🔒 Critical Sections and Protection

| Critical Section                 | Purpose                          | Protection Used      |
|----------------------------------|----------------------------------|-----------------------|
| Add client to list               | Track active connections         | `clients_mutex`       |
| Remove client on disconnect      | Keep list clean                  | `clients_mutex`       |
| Broadcast message                | Send safely to all other clients | `clients_mutex`       |

> You may enable semaphore logic to limit the number of concurrent clients.

---

## ▶️ How to Build & Run

### 🧱 Requirements
- Windows 10 or newer
- MinGW / MSYS2 (for `g++`) or Visual Studio

### 🛠️ Build with MinGW (MSYS2 example)

```bash
g++ server.cpp -o server.exe -lws2_32 -std=c++11
