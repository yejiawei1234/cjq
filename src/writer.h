//
// Created by jack ye on 6/16/26.
//

#ifndef CJQ_WRITER_H
#define CJQ_WRITER_H

#endif //CJQ_WRITER_H


#pragma once

#include <stdio.h>

#include "rule.h"

/*
 * CSV Header
 */
void write_csv_header(
    FILE *out,
    RuleSet *rules);

/*
 * CSV Field
 */
void csv_write_field(
    FILE *out,
    const char *value);

/*
 * CSV Row
 */
void csv_write_row(
    FILE *out,
    char **values,
    int count);
