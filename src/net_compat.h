#ifndef NET_COMPAT_H
#define NET_COMPAT_H

#ifndef _WIN32
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#if defined(__GNUC__)
#define NET_UNUSED __attribute__((unused))
#else
#define NET_UNUSED
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
typedef SOCKET socket_t;
#define CLOSESOCKET closesocket
#define SOCK_ERR INVALID_SOCKET
#define SOCK_SEND_FLAGS 0
static NET_UNUSED int net_init(void) {
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa);
}
static NET_UNUSED void net_cleanup(void) {
    WSACleanup();
}
static NET_UNUSED void sleep_ms(int ms) {
    Sleep((DWORD)ms);
}
static NET_UNUSED double now_seconds(void) {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
}
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>
typedef int socket_t;
#define CLOSESOCKET close
#define SOCK_ERR (-1)
#define SOCK_SEND_FLAGS 0
static NET_UNUSED int net_init(void) { return 0; }
static NET_UNUSED void net_cleanup(void) { }
static NET_UNUSED void sleep_ms(int ms) {
    struct timespec req;
    req.tv_sec = ms / 1000;
    req.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&req, NULL);
}
static NET_UNUSED double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}
#endif

#endif
