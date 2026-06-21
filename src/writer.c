#include <stdio.h>
#include <string.h>

#include "writer.h"

#include "tracker_split.h"

/*
 * ============================================================
 * CSV Header
 * ============================================================
 */

void write_csv_header(
    FILE *out,
    RuleSet *rules,
    TimeFieldSet *time_fields,
    TrackerSplitSet *tracker_set)
{
    int first = 1;

    /*
     * normal rule fields
     */
    for (int i = 0;
         i < rules->count;
         i++)
    {
        if (!first)
        {
            fputc(
                ',',
                out);
        }

        first = 0;

        fprintf(
            out,
            "%s",
            rules->rules[i].output_key);
    }

    /*
     * tracker split fields
     */
    if (tracker_set)
    {
        for (int i = 0;
             i < tracker_set->count;
             i++)
        {
            const char *field =
                tracker_set->fields[i];

            const char *p =
                strrchr(
                    field,
                    '.');

            const char *key =
                p
                ? p + 1
                : field;

            if (strcmp(
                    key,
                    "tracker_name")
                == 0)
            {
                fprintf(
                    out,
                    ",network,campaign,adgroup,creative");
            }
            else
            {
                fprintf(
                    out,
                    ",%s_network",
                    key);

                fprintf(
                    out,
                    ",%s_campaign",
                    key);

                fprintf(
                    out,
                    ",%s_adgroup",
                    key);

                fprintf(
                    out,
                    ",%s_creative",
                    key);
            }
        }
    }

    /*
     * time fields
     */
    if (time_fields)
    {
        for (int i = 0;
             i < time_fields->count;
             i++)
        {
            fprintf(
                out,
                ",%s_epoch",
                time_fields->fields[i]);

            if (time_fields
                    ->offset_hours != 0)
            {
                fprintf(
                    out,
                    ",%s_offset",
                    time_fields->fields[i]);
            }
        }
    }

    fputc(
        '\n',
        out);
}

/*
 * ============================================================
 * CSV Field Escape
 * ============================================================
 */

void csv_write_field(
    FILE *out,
    const char *value)
{
    if (!value)
    {
        return;
    }

    int need_quote = 0;

    for (const char *p = value;
         *p;
         p++)
    {
        if (*p == ','
            || *p == '"'
            || *p == '\n'
            || *p == '\r')
        {
            need_quote = 1;
            break;
        }
    }

    /*
     * no escaping needed
     */
    if (!need_quote)
    {
        fputs(
            value,
            out);

        return;
    }

    /*
     * quoted csv field
     */
    fputc(
        '"',
        out);

    for (const char *p = value;
         *p;
         p++)
    {
        if (*p == '"')
        {
            /*
             * CSV:
             * " -> ""
             */
            fputc(
                '"',
                out);
        }

        fputc(
            *p,
            out);
    }

    fputc(
        '"',
        out);
}

/*
 * ============================================================
 * CSV Row
 * ============================================================
 */

void csv_write_row(
    FILE *out,
    char **values,
    int count)
{
    for (int i = 0;
         i < count;
         i++)
    {
        csv_write_field(
            out,
            values[i]);

        if (i + 1
            < count)
        {
            fputc(
                ',',
                out);
        }
    }

    fputc(
        '\n',
        out);
}