#pragma once

typedef struct {

    char *input;

    char *output;

    char *keys;

    int csv_mode;

    int keep_all_key;
    
    char *time_fields;

    int offset_hours;
    int split_tracker;

    char *split_tracker_fields;

    int scan_lines;
} CliConfig;

int cli_parse(
    int argc,
    char **argv,
    CliConfig *cfg);