//
// Created by jack ye on 6/17/26.
//

#ifndef CJQ_TIME_FIELD_H
#define CJQ_TIME_FIELD_H

#endif //CJQ_TIME_FIELD_H

#pragma once

typedef struct {

    char **fields;

    int count;

    int offset_hours;

} TimeFieldSet;

int time_field_init(
    TimeFieldSet *set,
    const char *field_list,
    int offset_hours);

void time_field_destroy(
    TimeFieldSet *set);

int is_time_field(
    TimeFieldSet *set,
    const char *name);

char *shift_rfc3339_time(
    const char *src,
    int offset_hours);