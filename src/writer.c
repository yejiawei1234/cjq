#include <stdio.h>
#include <string.h>

#include "writer.h"

/*
 * ============================================================
 * CSV Header
 * ============================================================
 */

void write_csv_header(
    FILE *out,
    RuleSet *rules)
{
    for (int i = 0;
         i < rules->count;
         i++)
    {
        fprintf(
            out,
            "%s",
            rules->rules[i].output_key);

        if (i + 1
            < rules->count)
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