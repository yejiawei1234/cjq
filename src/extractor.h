#pragma once

#include <stdio.h>

#include "yyjson.h"
#include "rule.h"
#include "time_field.h"

void extract_json(
    yyjson_doc *doc,
    RuleSet *rules,
    TimeFieldSet *time_fields,
    FILE *out);

void extract_csv(
    yyjson_doc *doc,
    RuleSet *rules,
    TimeFieldSet *time_fields,
    FILE *out);

