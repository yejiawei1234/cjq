#pragma once

#include <stdio.h>

#include "yyjson.h"
#include "rule.h"
#include "time_field.h"
#include "tracker_split.h"

void extract_json(
    yyjson_doc *doc,
    RuleSet *rules,
    TimeFieldSet *time_fields,
    TrackerSplitSet *tracker_set,
    FILE *out);

void extract_csv(
    yyjson_doc *doc,
    RuleSet *rules,
    TimeFieldSet *time_fields,
    TrackerSplitSet *tracker_set,
    FILE *out);

/*
 * Write a yyjson_val as a CSV cell value (with CSV escaping).
 * Used by keep-all mode to emit values for auto-discovered keys.
 */
void csv_write_json_val(
    FILE *out,
    yyjson_val *val);

typedef struct {
    char *name;
    char *value;
} CsvColumn;

typedef struct {
    CsvColumn *columns;
    int column_count;
    int column_cap;

    RuleSet *rules;
    TimeFieldSet *time_fields;
    TrackerSplitSet *tracker_set;

    int found_count;
} CsvContext;
