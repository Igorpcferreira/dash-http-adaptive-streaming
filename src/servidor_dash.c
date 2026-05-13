#include "net_compat.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_PORT 8080
#define BACKLOG 16
#define REQUEST_BUFFER 4096
#define CHUNK_SIZE 4096

static const char MPD_CONTENT[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\"\n"
    "     mediaPresentationDuration=\"48\"\n"
    "     minBufferTime=\"2\"\n"
    "     type=\"static\">\n"
    "  <Period duration=\"48\">\n"
    "    <AdaptationSet mimeType=\"video/mp4\" codecs=\"avc1.42E01E\">\n"
    "      <SegmentTemplate timescale=\"1000\" duration=\"4000\"\n"
    "        initialization=\"seg_$RepresentationID$_init.mp4\"\n"
    "        media=\"seg_$RepresentationID$_$Number$.m4s\"\n"
    "        startNumber=\"1\"/>\n"
    "      <Representation id=\"1\" bandwidth=\"500000\"  width=\"640\"  height=\"360\"/>\n"
    "      <Representation id=\"2\" bandwidth=\"1500000\" width=\"1280\" height=\"720\"/>\n"
    "      <Representation id=\"3\" bandwidth=\"4000000\" width=\"1920\" height=\"1080\"/>\n"
    "    </AdaptationSet>\n"
    "  </Period>\n"
    "</MPD>\n";

static int contains(const char *text, const char *pattern) {
    return strstr(text, pattern) != NULL;
}

static int parse_request_path(const char *request, char *path, int path_size) {
    const char *start;
    const char *end;
    int len;
    if (strncmp(request, "GET ", 4) != 0) return -1;
    start = request + 4;
    end = strchr(start, ' ');
    if (!end) return -1;
    len = (int)(end - start);
    if (len >= path_size) len = path_size - 1;
    memcpy(path, start, (size_t)len);
    path[len] = '\0';
    return 0;
}

static int parse_segment_info(const char *path, int *rep_id, int *seg_number, int *is_init) {
    char expected[128];
    *rep_id = 0;
    *seg_number = 0;
    *is_init = 0;

    if (sscanf(path, "/seg_%d_init.mp4", rep_id) == 1) {
        snprintf(expected, sizeof(expected), "/seg_%d_init.mp4", *rep_id);
        if (strcmp(path, expected) == 0) {
            *is_init = 1;
            return 0;
        }
    }

    if (sscanf(path, "/seg_%d_%d.m4s", rep_id, seg_number) == 2) {
        snprintf(expected, sizeof(expected), "/seg_%d_%d.m4s", *rep_id, *seg_number);
        if (strcmp(path, expected) == 0) {
            return 0;
        }
    }

    return -1;
}


static int bitrate_for_rep(int rep_id) {
    switch (rep_id) {
        case 1: return 500000;
        case 2: return 1500000;
        case 3: return 4000000;
        default: return 500000;
    }
}

static int delay_for_segment(int segment_number) {
    static const int delays[] = { 4, 12, 6, 28, 35, 14, 8, 42, 20, 7, 30, 10 };
    if (segment_number <= 0) return 2;
    return delays[(segment_number - 1) % (int)(sizeof(delays) / sizeof(delays[0]))];
}

static void send_all(socket_t client, const char *data, int length) {
    int sent_total = 0;
    while (sent_total < length) {
        int sent = send(client, data + sent_total, length - sent_total, SOCK_SEND_FLAGS);
        if (sent <= 0) return;
        sent_total += sent;
    }
}

static void send_header(socket_t client, int status, const char *status_text, const char *content_type, int content_length) {
    char header[512];
    snprintf(header, sizeof(header),
             "HTTP/1.1 %d %s\r\n"
             "Server: DASH-C-Lab/1.0\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Connection: close\r\n"
             "\r\n",
             status, status_text, content_type, content_length);
    send_all(client, header, (int)strlen(header));
}

static void send_text_response(socket_t client, int status, const char *status_text, const char *content_type, const char *body) {
    int length = (int)strlen(body);
    send_header(client, status, status_text, content_type, length);
    send_all(client, body, length);
}

static void send_synthetic_segment(socket_t client, int rep_id, int segment_number, int is_init) {
    int bitrate = bitrate_for_rep(rep_id);
    int length = is_init ? 2048 : (bitrate * 4 / 8 / 10); /* scaled demo segment */
    int delay_ms = is_init ? 1 : delay_for_segment(segment_number);
    int remaining = length;
    int offset = 0;
    char chunk[CHUNK_SIZE];
    int i;

    memset(chunk, 'A' + (rep_id % 26), sizeof(chunk));

    send_header(client, 200, "OK", "video/mp4", length);

    while (remaining > 0) {
        int n = remaining > CHUNK_SIZE ? CHUNK_SIZE : remaining;
        for (i = 0; i < n; i++) {
            chunk[i] = (char)('A' + ((rep_id + segment_number + offset + i) % 26));
        }
        send_all(client, chunk, n);
        remaining -= n;
        offset += n;
        if (!is_init) sleep_ms(delay_ms);
    }

    printf("[200] segmento rep=%d seg=%d bytes=%d atraso_chunk=%dms\n",
           rep_id, segment_number, length, delay_ms);
}

static socket_t start_server(int port) {
    socket_t server_fd;
    struct sockaddr_in addr;
    int opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == SOCK_ERR) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

#ifdef _WIN32
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
#else
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((unsigned short)port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        CLOSESOCKET(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        CLOSESOCKET(server_fd);
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

static void handle_client(socket_t client) {
    char request[REQUEST_BUFFER];
    char path[512];
    int bytes;
    int rep_id, seg_number, is_init;

    bytes = recv(client, request, sizeof(request) - 1, 0);
    if (bytes <= 0) return;
    request[bytes] = '\0';

    if (parse_request_path(request, path, sizeof(path)) < 0) {
        send_text_response(client, 400, "Bad Request", "text/plain", "Requisicao invalida\n");
        return;
    }

    printf("[GET] %s\n", path);

    if (strcmp(path, "/") == 0 || strcmp(path, "/stream.mpd") == 0) {
        send_text_response(client, 200, "OK", "application/dash+xml", MPD_CONTENT);
        printf("[200] manifesto MPD enviado\n");
        return;
    }

    if (parse_segment_info(path, &rep_id, &seg_number, &is_init) == 0) {
        send_synthetic_segment(client, rep_id, seg_number, is_init);
        return;
    }

    if (contains(path, "favicon")) {
        send_text_response(client, 404, "Not Found", "text/plain", "Not found\n");
        return;
    }

    send_text_response(client, 404, "Not Found", "text/plain", "Recurso nao encontrado\n");
    printf("[404] %s\n", path);
}

int main(int argc, char **argv) {
    socket_t server_fd;
    int port = argc > 1 ? atoi(argv[1]) : SERVER_PORT;
    printf("=== Servidor HTTP/DASH ===\n");

    if (net_init() != 0) {
        fprintf(stderr, "Falha ao inicializar sockets.\n");
        return EXIT_FAILURE;
    }

    server_fd = start_server(port);
    printf("Ouvindo em http://localhost:%d/stream.mpd\n", port);
    printf("Pressione Ctrl+C para encerrar.\n\n");

    while (1) {
        struct sockaddr_in client_addr;
#ifdef _WIN32
        int client_len = sizeof(client_addr);
#else
        socklen_t client_len = sizeof(client_addr);
#endif
        socket_t client = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client == SOCK_ERR) {
            perror("accept");
            continue;
        }
        handle_client(client);
        CLOSESOCKET(client);
    }

    CLOSESOCKET(server_fd);
    net_cleanup();
    return EXIT_SUCCESS;
}
