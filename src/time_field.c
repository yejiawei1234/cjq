//
// Created by jack ye on 6/17/26.
//

#include "time_field.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "time_field.h"

int time_field_init(
    TimeFieldSet *set,
    const char *field_list,
    int offset_hours)
{
    memset(
        set,
        0,
        sizeof(*set));

    set->offset_hours =
        offset_hours;

    if (!field_list)
    {
        return 0;
    }

    char *copy =
        strdup(field_list);

    int count = 1;

    for (char *p = copy;
         *p;
         p++)
    {
        if (*p == ',')
        {
            count++;
        }
    }

    set->fields =
        calloc(
            count,
            sizeof(char *));

    set->count =
        count;

    int idx = 0;

    char *token =
        strtok(
            copy,
            ",");

    while (token)
    {
        set->fields[idx++] =
            strdup(token);

        token =
            strtok(
                NULL,
                ",");
    }

    free(copy);

    return 0;
}

void time_field_destroy(
    TimeFieldSet *set)
{
    for (int i = 0;
         i < set->count;
         i++)
    {
        free(
            set->fields[i]);
    }

    free(
        set->fields);

    set->fields = NULL;

    set->count = 0;
}

int is_time_field(
    TimeFieldSet *set,
    const char *name)
{
    for (int i = 0;
         i < set->count;
         i++)
    {
        if (strcmp(
                set->fields[i],
                name) == 0)
        {
            return 1;
        }
    }

    return 0;
}

char *shift_rfc3339_time(
    const char *src,
    int offset_hours)
{
    int year;
    int mon;
    int day;

    int hour;
    int min;
    int sec;

    if (sscanf(
            src,
            "%d-%d-%dT%d:%d:%d",
            &year,
            &mon,
            &day,
            &hour,
            &min,
            &sec)
        != 6)
    {
        return NULL;
    }

    /*
     * fractional seconds
     *
     * ".490270496"
     */
    char fraction[64];

    fraction[0] = '\0';

    const char *dot =
        strchr(
            src,
            '.');

    const char *z =
        strrchr(
            src,
            'Z');

    if (dot && z && z > dot)
    {
        size_t len =
            (size_t)(z - dot);

        if (len >= sizeof(fraction))
        {
            len =
                sizeof(fraction) - 1;
        }

        memcpy(
            fraction,
            dot,
            len);

        fraction[len] =
            '\0';
    }

    struct tm tm_val;

    memset(
        &tm_val,
        0,
        sizeof(tm_val));

    tm_val.tm_year =
        year - 1900;

    tm_val.tm_mon =
        mon - 1;

    tm_val.tm_mday =
        day;

    tm_val.tm_hour =
        hour;

    tm_val.tm_min =
        min;

    tm_val.tm_sec =
        sec;

    time_t ts =
        timegm(
            &tm_val);

    ts +=
        offset_hours * 3600;

    struct tm out_tm;

    gmtime_r(
        &ts,
        &out_tm);

    char date_part[32];

    strftime(
        date_part,
        sizeof(date_part),
        "%Y-%m-%dT%H:%M:%S",
        &out_tm);

    size_t total_len =
        strlen(date_part)
        + strlen(fraction)
        + 1;

    char *result =
        malloc(
            total_len);

    if (!result)
    {
        return NULL;
    }

    snprintf(
        result,
        total_len,
        "%s%s",
        date_part,
        fraction);

    return result;
}

long long rfc3339_to_epoch(
    const char *src)
{
    int year;
    int mon;
    int day;

    int hour;
    int min;
    int sec;

    if (sscanf(
            src,
            "%d-%d-%dT%d:%d:%d",
            &year,
            &mon,
            &day,
            &hour,
            &min,
            &sec)
        != 6)
    {
        return -1;
    }

    struct tm tm_val;

    memset(
        &tm_val,
        0,
        sizeof(tm_val));

    tm_val.tm_year =
        year - 1900;

    tm_val.tm_mon =
        mon - 1;

    tm_val.tm_mday =
        day;

    tm_val.tm_hour =
        hour;

    tm_val.tm_min =
        min;

    tm_val.tm_sec =
        sec;

    return
        (long long)
        timegm(
            &tm_val);
}