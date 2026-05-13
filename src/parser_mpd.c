#include "parser_mpd.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int attr_int(const char *xml, const char *name, int fallback) {
    char pattern[80];
    const char *p = xml;
    snprintf(pattern, sizeof(pattern), "%s=\"", name);
    while ((p = strstr(p, pattern)) != NULL) {
        if (p == xml || !isalnum((unsigned char)*(p - 1))) {
            return atoi(p + strlen(pattern));
        }
        p++;
    }
    return fallback;
}

static void attr_string(const char *xml, const char *name, char *out, int out_size) {
    char pattern[80];
    const char *p, *start, *end;
    int len;
    if (out_size <= 0) return;
    out[0] = '\0';
    snprintf(pattern, sizeof(pattern), "%s=\"", name);
    p = strstr(xml, pattern);
    if (!p) return;
    start = p + strlen(pattern);
    end = strchr(start, '"');
    if (!end) return;
    len = (int)(end - start);
    if (len >= out_size) len = out_size - 1;
    memcpy(out, start, (size_t)len);
    out[len] = '\0';
}

static int parse_duration_seconds(const char *xml) {
    const char *p = strstr(xml, "mediaPresentationDuration=\"");
    if (!p) return 0;
    p += strlen("mediaPresentationDuration=\"");
    if (strncmp(p, "PT", 2) == 0) {
        p += 2;
        return atoi(p);
    }
    return atoi(p);
}

static void apply_template(const char *template_value, int rep_id, int segment_number, char *out, int out_size) {
    int written = 0;
    const char *p = template_value;
    char token[32];
    out[0] = '\0';

    while (*p && written < out_size - 1) {
        if (strncmp(p, "$RepresentationID$", 18) == 0) {
            snprintf(token, sizeof(token), "%d", rep_id);
            written += snprintf(out + written, (size_t)(out_size - written), "%s", token);
            p += 18;
        } else if (strncmp(p, "$Number$", 8) == 0) {
            snprintf(token, sizeof(token), "%d", segment_number);
            written += snprintf(out + written, (size_t)(out_size - written), "%s", token);
            p += 8;
        } else {
            out[written++] = *p++;
            out[written] = '\0';
        }
    }
    out[out_size - 1] = '\0';
}

int mpd_parse(const char *xml, MPD *mpd) {
    const char *cursor;
    char global_media[MAX_TEMPLATE_LENGTH];
    char global_init[MAX_TEMPLATE_LENGTH];

    if (!xml || !mpd) return 0;
    memset(mpd, 0, sizeof(*mpd));

    mpd->duration_seconds = parse_duration_seconds(xml);
    const char *segment_template = strstr(xml, "<SegmentTemplate");
    mpd->segment_duration_ms = segment_template ? attr_int(segment_template, "duration", 4000) : 4000;
    mpd->start_number = attr_int(xml, "startNumber", 1);
    attr_string(xml, "media", global_media, sizeof(global_media));
    attr_string(xml, "initialization", global_init, sizeof(global_init));

    cursor = xml;
    while ((cursor = strstr(cursor, "<Representation")) != NULL && mpd->num_representations < MAX_REPRESENTATIONS) {
        Representation *r = &mpd->representations[mpd->num_representations];
        r->id = attr_int(cursor, "id", mpd->num_representations + 1);
        r->bandwidth = attr_int(cursor, "bandwidth", 0);
        r->width = attr_int(cursor, "width", 0);
        r->height = attr_int(cursor, "height", 0);
        snprintf(r->media_template, sizeof(r->media_template), "%s", global_media);
        snprintf(r->initialization_template, sizeof(r->initialization_template), "%s", global_init);
        mpd->num_representations++;
        cursor += strlen("<Representation");
    }

    return mpd->num_representations;
}

void mpd_print(const MPD *mpd) {
    int i;
    if (!mpd) return;
    printf("=== Manifesto MPD ===\n");
    printf("Duracao total: %d s\n", mpd->duration_seconds);
    printf("Duracao segmento: %d ms\n", mpd->segment_duration_ms);
    printf("Start number: %d\n", mpd->start_number);
    printf("Representacoes: %d\n\n", mpd->num_representations);
    for (i = 0; i < mpd->num_representations; i++) {
        const Representation *r = &mpd->representations[i];
        printf("  id=%d | %d Kbps | %dx%d | template=%s\n",
               r->id, r->bandwidth / 1000, r->width, r->height, r->media_template);
    }
    printf("\n");
}

void mpd_build_segment_path(const Representation *rep, int segment_number, char *out, int out_size) {
    char relative[MAX_TEMPLATE_LENGTH];
    if (!rep || !out || out_size <= 0) return;
    if (rep->media_template[0]) {
        apply_template(rep->media_template, rep->id, segment_number, relative, sizeof(relative));
        snprintf(out, (size_t)out_size, "/%s", relative);
    } else {
        snprintf(out, (size_t)out_size, "/seg_%d_%d.m4s", rep->id, segment_number);
    }
}

void mpd_build_init_path(const Representation *rep, char *out, int out_size) {
    char relative[MAX_TEMPLATE_LENGTH];
    if (!rep || !out || out_size <= 0) return;
    if (rep->initialization_template[0]) {
        apply_template(rep->initialization_template, rep->id, 0, relative, sizeof(relative));
        snprintf(out, (size_t)out_size, "/%s", relative);
    } else {
        snprintf(out, (size_t)out_size, "/seg_%d_init.mp4", rep->id);
    }
}
