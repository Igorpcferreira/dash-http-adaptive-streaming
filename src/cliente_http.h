#ifndef CLIENTE_HTTP_H
#define CLIENTE_HTTP_H

#define HTTP_DEFAULT_PORT 80
#define HTTP_DASH_PORT 8080
#define HTTP_BUFFER_SIZE (4 * 1024 * 1024)

int http_get(const char *host, const char *path, char *response, int max_size);
int http_body_offset(const char *response, int response_size);
int http_status_code(const char *response);

#endif
