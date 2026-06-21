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

