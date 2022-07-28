#pragma once

// headers
#if defined(__APPLE__) || defined(__linux__)
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#error Unsupported system
#endif

// helpers
#if defined(__APPLE__) || defined(__linux__)
typedef int m_sock_t;
#define ISINVALIDSOCKET(socket) ((socket) == -1)
#define ISSOCKETERROR(socket) ((socket) == -1)
#elif defined(_WIN32)
typedef SOCKET m_sock_t;
#define ISINVALIDSOCKET(socket) ((socket) == INVALID_SOCKET)
#define ISSOCKETERROR(socket) ((socket) == SOCKET_ERROR)
#else
#error Unsupported system
#endif

// errno
#if defined(__APPLE__) || defined(__linux__)
#define GETSOCKETERRNO() (errno)
#elif defined(_WIN32)
#define GETSOCKETERRNO() (WSAGetLastError())
#endif