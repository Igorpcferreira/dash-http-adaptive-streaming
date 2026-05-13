#include "cliente_http.h"
#include "net_compat.h"
#include "parser_mpd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HISTORY_SIZE 5
#define SAFETY_FACTOR 0.80
#define MAX_SEGMENTS_TO_PLAY 12

typedef struct {
    double values[HISTORY_SIZE];
    int start;
    int size;
} ThroughputHistory;

static void history_add(ThroughputHistory *h, double value) {
    int idx = (h->start + h->size) % HISTORY_SIZE;
    h->values[idx] = value;
    if (h->size < HISTORY_SIZE) {
        h->size++;
    } else {
        h->start = (h->start + 1) % HISTORY_SIZE;
    }
}

static double history_harmonic_mean(const ThroughputHistory *h) {
    double sum_inverse = 0.0;
    int i;
    if (h->size == 0) return 0.0;
    for (i = 0; i < h->size; i++) {
        int idx = (h->start + i) % HISTORY_SIZE;
        if (h->values[idx] > 0.0) sum_inverse += 1.0 / h->values[idx];
    }
    return sum_inverse > 0.0 ? (double)h->size / sum_inverse : 0.0;
}

static int select_representation(const MPD *mpd, double estimated_bps) {
    double target = estimated_bps * SAFETY_FACTOR;
    int best = 0;
    int i;
    for (i = 0; i < mpd->num_representations; i++) {
        if (mpd->representations[i].bandwidth <= target) best = i;
    }
    return best;
}

static double download_and_measure(const char *host, const char *path, int *body_bytes, int *status_code) {
    static char response[HTTP_BUFFER_SIZE];
    double start = now_seconds();
    double end;
    int total = http_get(host, path, response, sizeof(response));
    int offset;

    end = now_seconds();
    *body_bytes = 0;
    *status_code = 0;

    if (total <= 0) return 0.0;

    *status_code = http_status_code(response);
    offset = http_body_offset(response, total);
    if (offset >= 0 && offset <= total) *body_bytes = total - offset;

    if (end <= start || *body_bytes <= 0) return 0.0;
    return ((double)(*body_bytes) * 8.0) / (end - start);
}

static int download_mpd(const char *host, const char *path, char *xml_out, int xml_size) {
    static char response[HTTP_BUFFER_SIZE];
    int total;
    int offset;
    int body_len;
    int status;

    total = http_get(host, path, response, sizeof(response));
    if (total <= 0) return -1;

    status = http_status_code(response);
    offset = http_body_offset(response, total);
    if (status != 200 || offset < 0) {
        fprintf(stderr, "Falha ao baixar MPD. HTTP status=%d\n", status);
        return -1;
    }

    body_len = total - offset;
    if (body_len >= xml_size) body_len = xml_size - 1;
    memcpy(xml_out, response + offset, (size_t)body_len);
    xml_out[body_len] = '\0';
    return body_len;
}

static void play_dash(const MPD *mpd, const char *host) {
    ThroughputHistory history;
    int total_segments;
    int segment;
    int body_bytes;
    int status;
    int rep_index;
    char path[512];
    double estimated;
    double measured;

    memset(&history, 0, sizeof(history));

    total_segments = mpd->segment_duration_ms > 0
        ? mpd->duration_seconds / (mpd->segment_duration_ms / 1000)
        : MAX_SEGMENTS_TO_PLAY;
    if (total_segments <= 0 || total_segments > MAX_SEGMENTS_TO_PLAY) total_segments = MAX_SEGMENTS_TO_PLAY;

    printf("=== Reproducao DASH adaptativa ===\n");
    printf("Segmentos simulados: %d | margem ABR: %.0f%% | historico: %d medicoes\n\n",
           total_segments, SAFETY_FACTOR * 100.0, HISTORY_SIZE);

    for (segment = mpd->start_number; segment < mpd->start_number + total_segments; segment++) {
        estimated = history.size > 0
            ? history_harmonic_mean(&history)
            : (double)mpd->representations[0].bandwidth;

        rep_index = select_representation(mpd, estimated);
        mpd_build_segment_path(&mpd->representations[rep_index], segment, path, sizeof(path));

        printf("[Segmento %02d] escolhida rep=%d (%dx%d, %d Kbps) | estimado=%.2f Mbps | GET %s\n",
               segment,
               mpd->representations[rep_index].id,
               mpd->representations[rep_index].width,
               mpd->representations[rep_index].height,
               mpd->representations[rep_index].bandwidth / 1000,
               estimated / 1000000.0,
               path);

        measured = download_and_measure(host, path, &body_bytes, &status);
        if (status != 200 || measured <= 0.0) {
            printf("           erro: HTTP %d, bytes=%d\n", status, body_bytes);
            continue;
        }

        history_add(&history, measured);
        printf("           recebido=%d bytes | throughput medido=%.2f Mbps\n\n",
               body_bytes, measured / 1000000.0);
    }
}

int main(int argc, char **argv) {
    const char *host = argc > 1 ? argv[1] : "localhost:8080";
    const char *mpd_path = argc > 2 ? argv[2] : "/stream.mpd";
    char mpd_xml[128 * 1024];
    MPD mpd;

    printf("=== Cliente DASH HTTP ===\n");
    printf("Host: %s\n", host);
    printf("MPD : %s\n\n", mpd_path);

    if (net_init() != 0) {
        fprintf(stderr, "Falha ao inicializar sockets.\n");
        return EXIT_FAILURE;
    }

    if (download_mpd(host, mpd_path, mpd_xml, sizeof(mpd_xml)) < 0) {
        net_cleanup();
        return EXIT_FAILURE;
    }

    if (mpd_parse(mpd_xml, &mpd) <= 0) {
        fprintf(stderr, "MPD invalido ou sem representacoes.\n");
        net_cleanup();
        return EXIT_FAILURE;
    }

    mpd_print(&mpd);
    play_dash(&mpd, host);

    net_cleanup();
    return EXIT_SUCCESS;
}
