#include <stdio.h>

#include <argtable3.h>

#include "cli.h"

#include <stdlib.h>
#include <string.h>


static const char *
last_key(
    const char *path)
{
    const char *p =
        strrchr(
            path,
            '.');

    return p
        ? p + 1
        : path;
}

int cli_parse(
    int argc,
    char **argv,
    CliConfig *cfg) {
    struct arg_file *input;
    struct arg_file *output;
    struct arg_str *keys;
    struct arg_lit *help;
    struct arg_lit *csv;
    struct arg_str *time_fields;
    struct arg_int *offset;
    struct arg_lit *split_tracker;

    struct arg_lit *keep_all_key;

    struct arg_int *scan_lines;

    struct arg_str *split_tracker_fields;
    struct arg_end *end;

    void *argtable[] = {

        help =
            arg_lit0(
                "h",
                "help",
                "show help"),

        time_fields =
            arg_str0(
                "t",
                "time_fields",
                "<datetime field>",
                "timestamp fields"),

        split_tracker =
            arg_lit0(
                "s",
                "split_tracker_name",
                "split tracker"),

        split_tracker_fields =
            arg_str0(
                NULL,
                "split_tracker_field",
                "<fields>",
                "tracker fields"),

        offset =
            arg_int0(
                "O",
                "offset",
                "<hours>",
                "timezone offset"),

        csv =
            arg_lit0(
                NULL,
                "csv",
                "csv output"),

        keep_all_key =
            arg_lit0(
                "a",
                "all",
                "keep all keys"),

        scan_lines =
            arg_int0(
                "n",
                "scan-lines",
                "<N>",
                "scan first N lines for keys (default 200, only with -a --csv)"),

        input =
            arg_file0(
                "i",
                "input",
                "<file>",
                "input jsonl"),


        output =
            arg_file0(
                "o",
                "output",
                "<file>",
                "output file path"),

        keys =
            arg_str0(
                "k",
                "keys",
                "<keys>",
                "extract keys"),


        end =
        arg_end(20)
    };

    int nerrors =
            arg_parse(
                argc,
                argv,
                argtable);

    if (help->count) {
        arg_print_syntax(
            stdout,
            argtable,
            "\n");

        arg_freetable(
            argtable,
            sizeof(argtable)
            / sizeof(argtable[0]));

        return -1;
    }

    if (nerrors) {
        arg_print_errors(
            stderr,
            end,
            argv[0]);

        arg_freetable(
            argtable,
            sizeof(argtable)
            / sizeof(argtable[0]));

        return -1;
    }

    // cfg->input = (char *) input->sval[0];
    cfg->input = input->count ? (char *)input->filename[0] : NULL;

    cfg->output = output->count ? (char *) output->filename[0] : NULL;

    cfg->keys =
            keys->count
            ? strdup(
                keys->sval[0])
            : NULL;

    cfg->csv_mode =
            csv->count > 0;
    cfg->keep_all_key = keep_all_key->count > 0;

    cfg->scan_lines = scan_lines->count ? scan_lines->ival[0] : 200;

    cfg->time_fields =
        time_fields->count
            ? (char *)time_fields->sval[0]
            : NULL;

    cfg->offset_hours = offset->count ? offset->ival[0]: 0;

    cfg->split_tracker = split_tracker->count > 0;

    cfg->split_tracker_fields =
                                split_tracker_fields->count
                                    ? (char *)
                                      split_tracker_fields->sval[0]
                                    : NULL;

    if (cfg->split_tracker)
    {
        const char *tracker_key =
            "context.tracker_name:tracker_name";
        const char *tracker_name =
            "tracker_name";

        if (!cfg->keys)
        {
            cfg->keys =
                strdup(
                    tracker_key);
        }
        else if (!strstr(
                cfg->keys,
                tracker_name))
        {
            char *new_keys =
                malloc(
                    strlen(cfg->keys)
                    + strlen(tracker_key)
                    + 2);

            sprintf(
                new_keys,
                "%s,%s",
                cfg->keys,
                tracker_key);

            cfg->keys =
                new_keys;
        }
    }
    if (cfg->split_tracker_fields)
    {
        const char *tracker_key = last_key(
            cfg->split_tracker_fields);
        if (!cfg->keys)
        {
            cfg->keys =
                strdup(
                    tracker_key);
        }
        else if (!strstr(
                cfg->keys,
                tracker_key))
        {
            char *new_keys =
                malloc(
                    strlen(cfg->keys)
                    + strlen(tracker_key)
                    + 2);

            sprintf(
                new_keys,
                "%s,%s",
                cfg->keys,
                tracker_key);

            cfg->keys =
                new_keys;
        }
    }

    arg_freetable(
        argtable,
        sizeof(argtable)
        / sizeof(argtable[0]));

    return 0;
}
