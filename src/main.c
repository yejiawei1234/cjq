#include <stdio.h>
#include <stdlib.h>

#include "yyjson.h"

#include "cli.h"
#include "rule.h"
#include "progress.h"
#include "extractor.h"
#include "writer.h"
#include "time_field.h"
#include "tracker_split.h"
#include "usage_log.h"

int main(
    int argc,
    char **argv)
{
    /*
     * --------------------------------------------------------
     * Parse CLI
     * --------------------------------------------------------
     */

    CliConfig cfg;
    int show_progress;

    if (cli_parse(
            argc,
            argv,
            &cfg) != 0)
    {
        return 1;
    }
    write_usage_log();

    /*
     * --------------------------------------------------------
     * Parse Rules
     * --------------------------------------------------------
     */

    RuleSet rules;

    if (rule_parse(
            cfg.keys,
            &rules) != 0)
    {
        fprintf(
            stderr,
            "rule_parse failed\n");

        return 1;
    }

    /*
     * --------------------------------------------------------
     * Open Files
     * --------------------------------------------------------
     */

    FILE *input;

    if (cfg.input)
    {
        input =
            fopen(
                cfg.input,
                "r");

        if (!input)
        {
            perror(
                "input");

            rule_destroy(
                &rules);

            return 1;
        }
    }
    else
    {
        input = stdin;
    }


    FILE *output;

    if (cfg.output)
    {
        output =
            fopen(
                cfg.output,
                "w");

        if (!output)
        {
            perror(
                "output");

            rule_destroy(
                &rules);

            return 1;
        }

        setvbuf(
            output,
            NULL,
            _IOFBF,
            1024 * 1024 * 5);
    }
    else
    {
        output = stdout;
    }



    if (!output)
    {
        perror(
            "output");

        fclose(input);

        rule_destroy(
            &rules);

            return 1;
    }

    TimeFieldSet time_fields;

    time_field_init(
        &time_fields,

        cfg.time_fields,

        cfg.offset_hours);

    /*
     * --------------------------------------------------------
     * Progress
     * --------------------------------------------------------
     */



    TrackerSplitSet tracker_set;

    if (cfg.split_tracker || cfg.split_tracker_fields)
    {
        tracker_split_init(
            &tracker_set,
            cfg.split_tracker_fields);
    }
    else
    {
        memset(
            &tracker_set,
            0,
            sizeof(tracker_set));
    }



    Progress progress;

    if (input != stdin && output != stdout) {
        progress_init(
        &progress,
        get_file_size(
            input));
        show_progress = 1;
    } else {
        show_progress = 0;
    }


    if (cfg.csv_mode)
    {
        write_csv_header(
            output,
            &rules,
            &time_fields,
            &tracker_set);
    }

    /*
     * --------------------------------------------------------
     * Read JSONL
     * --------------------------------------------------------
     */

    char *line = NULL;

    size_t cap = 0;

    ssize_t line_len;

    while ((line_len =
            getline(
                &line,
                &cap,
                input))
           != -1)
    {
        yyjson_doc *doc =
            yyjson_read(
                line,
                (size_t)line_len,
                0);

        if (!doc)
        {
            fprintf(
                stderr,
                "skip invalid json line\n");

            continue;
        }

        if (cfg.csv_mode)
        {
            extract_csv(
                doc,
                &rules,
                &time_fields,
                &tracker_set,
                output);
        }
        else
        {
            extract_json(
                doc,
                &rules,
                &time_fields,
                &tracker_set,
                output);
        }

        yyjson_doc_free(
            doc);

        if (show_progress)
        {
            progress_update(
            &progress,
            line_len);
        }

    }

    /*
     * --------------------------------------------------------
     * Cleanup
     * --------------------------------------------------------
     */

    if (show_progress)
    {
        progress_finish(
        &progress);
    }


    free(line);

    fclose(input);

    if (output != stdout)
    {
        fclose(output);
    }

    rule_destroy(&rules);

    time_field_destroy(&time_fields);

    if (cfg.split_tracker)
    {
        tracker_split_destroy(
            &tracker_set);
    }

    return 0;
}
