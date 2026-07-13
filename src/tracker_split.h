//
// Created by jack ye on 6/20/26.
//

#pragma once

#include <stddef.h>

typedef struct {

    char network[256];

    char campaign[256];

    char adgroup[256];

    char creative[256];

} TrackerParts;

typedef struct {

    char **fields;

    int count;

} TrackerSplitSet;

int tracker_split_init(
    TrackerSplitSet *set,
    const char *field_list);

void tracker_split_destroy(
    TrackerSplitSet *set);

int should_split_tracker(
    TrackerSplitSet *set,
    const char *path,
    const char *key);

int split_tracker_name(
    const char *src,
    TrackerParts *parts);

int split_tracker_name_len(
    const char *src,
    size_t len,
    TrackerParts *parts);

const char *tracker_output_prefix(
    const char *key);
