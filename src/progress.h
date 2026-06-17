#pragma once

#include <stdio.h>

typedef struct {

    long total_bytes;

    long current_bytes;

    long last_refresh;

    int bar_width;

} Progress;

long get_file_size(
    FILE *fp);

void progress_init(
    Progress *p,
    long total);

void progress_update(
    Progress *p,
    long bytes);

void progress_finish(
    Progress *p);