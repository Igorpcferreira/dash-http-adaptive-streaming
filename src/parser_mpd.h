#ifndef PARSER_MPD_H
#define PARSER_MPD_H

#define MAX_REPRESENTATIONS 16
#define MAX_TEMPLATE_LENGTH 256

typedef struct {
    int id;
    int bandwidth;
    int width;
    int height;
    char media_template[MAX_TEMPLATE_LENGTH];
    char initialization_template[MAX_TEMPLATE_LENGTH];
} Representation;

typedef struct {
    int duration_seconds;
    int segment_duration_ms;
    int start_number;
    int num_representations;
    Representation representations[MAX_REPRESENTATIONS];
} MPD;

int mpd_parse(const char *xml, MPD *mpd);
void mpd_print(const MPD *mpd);
void mpd_build_segment_path(const Representation *rep, int segment_number, char *out, int out_size);
void mpd_build_init_path(const Representation *rep, char *out, int out_size);

#endif
