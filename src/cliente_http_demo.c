#include "cliente_http.h"
#include "net_compat.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    const char *host = argc > 1 ? argv[1] : "localhost:8080";
    const char *path = argc > 2 ? argv[2] : "/stream.mpd";
    static char response[HTTP_BUFFER_SIZE];
    int total;
    int offset;

    if (net_init() != 0) {
        fprintf(stderr, "Falha ao inicializar sockets.\n");
        return EXIT_FAILURE;
    }

    total = http_get(host, path, response, sizeof(response));
    if (total <= 0) {
        net_cleanup();
        return EXIT_FAILURE;
    }

    offset = http_body_offset(response, total);
    printf("HTTP status: %d\n", http_status_code(response));
    printf("Bytes totais: %d\n", total);
    printf("Bytes corpo : %d\n\n", offset >= 0 ? total - offset : 0);
    printf("%.*s\n", total > 2000 ? 2000 : total, response);

    net_cleanup();
    return EXIT_SUCCESS;
}
