#include <stdio.h>

#include <argtable3.h>

#include "cli.h"

int cli_parse(
    int argc,
    char **argv,
    CliConfig *cfg) {
    struct arg_str *input;
    struct arg_str *output;
    struct arg_str *keys;
    struct arg_lit *help;
    struct arg_lit *csv;
    struct arg_end *end;
    struct arg_str *time_fields;

    struct arg_int *offset;

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
            "<fields>",
            "timestamp fields"),

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

        input =
        arg_str0(
            "i",
            "input",
            "<file>",
            "input jsonl"),


        output =
        arg_str0(
            "o",
            "output",
            "<file>",
            "output file path"),

        keys =
        arg_str1(
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
    cfg->input = input->count ? (char *) input->sval[0] : NULL;

    cfg->output = output->count ? (char *) output->sval[0] : NULL;

    cfg->keys =
            (char *) keys->sval[0];

    cfg->csv_mode =
            csv->count > 0;

    cfg->time_fields =
        time_fields->count
            ? (char *)time_fields->sval[0]
            : NULL;

    cfg->offset_hours =
            offset->count
                ? offset->ival[0]
                : 0;

    arg_freetable(
        argtable,
        sizeof(argtable)
        / sizeof(argtable[0]));

    return 0;
}
