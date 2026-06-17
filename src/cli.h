#pragma once

typedef struct {

    char *input;

    char *output;

    char *keys;

    int csv_mode;
    
    char *time_fields;

    int offset_hours;

} CliConfig;

int cli_parse(
    int argc,
    char **argv,
    CliConfig *cfg);