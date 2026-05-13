#include "cliente_http.h"
#include "net_compat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void split_host_port(const char *host_port, char *host, int host_size, char *port, int port_size) {
    const char *sep = strrchr(host_port, ':');
    if (sep && sep != host_port) {
        int len = (int)(sep - host_port);
        if (len >= host_size) len = host_size - 1;
        memcpy(host, host_port, (size_t)len);
        host[len] = '\0';
        snprintf(port, (size_t)port_size, "%s", sep + 1);
    } else {
        snprintf(host, (size_t)host_size, "%s", host_port);
        snprintf(port, (size_t)port_size, "%d", HTTP_DEFAULT_PORT);
    }
}

int http_status_code(const char *response) {
    int code = 0;
    if (response) sscanf(response, "HTTP/%*s %d", &code);
    return code;
}

int http_body_offset(const char *response, int response_size) {
    int i;
    if (!response || response_size <= 4) return -1;
    for (i = 0; i <= response_size - 4; i++) {
        if (response[i] == '\r' && response[i + 1] == '\n' && response[i + 2] == '\r' && response[i + 3] == '\n') {
            return i + 4;
        }
    }
    return -1;
}

int http_get(const char *host_port, const char *path, char *response, int max_size) {
    char host[256];
    char port[16];
    char request[1024];
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    struct addrinfo *rp = NULL;
    socket_t fd = SOCK_ERR;
    int total = 0;
    int received;

    if (!host_port || !path || !response || max_size <= 1) return -1;

    split_host_port(host_port, host, sizeof(host), port, sizeof(port));

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &result) != 0) {
        fprintf(stderr, "Erro ao resolver host: %s\n", host);
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == SOCK_ERR) continue;
        if (connect(fd, rp->ai_addr, (int)rp->ai_addrlen) == 0) break;
        CLOSESOCKET(fd);
        fd = SOCK_ERR;
    }

    freeaddrinfo(result);

    if (fd == SOCK_ERR) {
        fprintf(stderr, "Erro ao conectar em %s:%s\n", host, port);
        return -1;
    }

    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: DASH-C-Lab/1.0\r\n"
             "Accept: */*\r\n"
             "Connection: close\r\n"
             "\r\n",
             path, host_port);

    if (send(fd, request, (int)strlen(request), SOCK_SEND_FLAGS) < 0) {
        perror("send");
        CLOSESOCKET(fd);
        return -1;
    }

    while (total < max_size - 1) {
        received = recv(fd, response + total, max_size - total - 1, 0);
        if (received <= 0) break;
        total += received;
    }

    response[total] = '\0';
    CLOSESOCKET(fd);
    return total;
}
