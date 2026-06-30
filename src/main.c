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

/*
 * Forward declarations.
 */
static void auto_csv_sort_keys(
    char **keys,
    int count);

static void auto_csv_write_header(
    FILE *out,
    char **keys,
    int count);

static void auto_csv_write_row(
    FILE *out,
    yyjson_val *root,
    char **keys,
    int count);

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

    if (!cfg.keep_all_key) {
        if (!cfg.keys) {
            fprintf(
                stderr,
                "-k <keys> is required\n");

            return 1;
        }

        if (rule_parse(
                cfg.keys,
                &rules) != 0)
        {
            fprintf(
                stderr,
                "rule_parse failed\n");

            return 1;
        }
    } else {
        memset(
            &rules,
            0,
            sizeof(rules));
    }

    /*
     * -a --csv state.
     */
    char **buffered_lines = NULL;
    int buf_line_count = 0;
    int buf_line_cap = 0;

    char **all_keys = NULL;
    int all_key_count = 0;
    int all_key_cap = 0;

    int auto_csv =
        cfg.keep_all_key && cfg.csv_mode;

    int auto_csv_scanned = 0;
    int auto_csv_header_done = 0;

    int scan_limit =
        cfg.scan_lines > 0
            ? cfg.scan_lines
            : 200;

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

    /*
     * --------------------------------------------------------
     * Progress
     * --------------------------------------------------------
     */

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


    if (cfg.csv_mode && !cfg.keep_all_key)
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

        if (auto_csv)
        {
            yyjson_val *root =
                yyjson_doc_get_root(doc);

            if (!yyjson_is_obj(root))
            {
                yyjson_doc_free(
                    doc);

                continue;
            }

            if (!auto_csv_scanned)
            {
                /*
                 * Scan phase: collect unique top-level keys.
                 */
                size_t idx;
                size_t max;

                yyjson_val *key;
                yyjson_val *val;

                yyjson_obj_foreach(
                    root,
                    idx,
                    max,
                    key,
                    val)
                {
                    const char *kname =
                        yyjson_get_str(key);

                    int found = 0;

                    for (int ki = 0;
                         ki < all_key_count;
                         ki++)
                    {
                        if (strcmp(
                                all_keys[ki],
                                kname)
                            == 0)
                        {
                            found = 1;
                            break;
                        }
                    }

                    if (!found)
                    {
                        if (all_key_count
                            == all_key_cap)
                        {
                            int new_cap =
                                all_key_cap == 0
                                    ? 64
                                    : all_key_cap * 2;

                            char **tmp =
                                realloc(
                                    all_keys,
                                    sizeof(char *)
                                    * new_cap);

                            if (!tmp)
                            {
                                perror("realloc");
                                exit(EXIT_FAILURE);
                            }

                            all_keys =
                                tmp;

                            all_key_cap =
                                new_cap;
                        }

                        all_keys[
                            all_key_count++] =
                            strdup(kname);
                    }
                }

                /*
                 * Buffer this line for replay.
                 */
                if (buf_line_count
                    == buf_line_cap)
                {
                    int new_cap =
                        buf_line_cap == 0
                            ? scan_limit
                            : buf_line_cap * 2;

                    char **tmp =
                        realloc(
                            buffered_lines,
                            sizeof(char *)
                            * new_cap);

                    if (!tmp)
                    {
                        perror("realloc");
                        exit(EXIT_FAILURE);
                    }

                    buffered_lines =
                        tmp;

                    buf_line_cap =
                        new_cap;
                }

                buffered_lines[
                    buf_line_count++] =
                    strdup(line);

                /*
                 * Hit scan limit: flush header + buffered rows,
                 * then switch to streaming mode.
                 */
                if (buf_line_count
                    >= scan_limit)
                {
                    auto_csv_scanned = 1;
                    auto_csv_header_done = 1;

                    auto_csv_sort_keys(
                        all_keys,
                        all_key_count);

                    auto_csv_write_header(
                        output,
                        all_keys,
                        all_key_count);

                    /*
                     * Replay buffered rows (includes current line).
                     */
                    for (int li = 0;
                         li < buf_line_count;
                         li++)
                    {
                        yyjson_doc *row_doc =
                            yyjson_read(
                                buffered_lines[li],
                                strlen(
                                    buffered_lines[li]),
                                0);

                        if (!row_doc)
                        {
                            continue;
                        }

                        yyjson_val *row_root =
                            yyjson_doc_get_root(
                                row_doc);

                        if (yyjson_is_obj(
                                row_root))
                        {
                            auto_csv_write_row(
                                output,
                                row_root,
                                all_keys,
                                all_key_count);
                        }

                        yyjson_doc_free(
                            row_doc);
                    }

                    /*
                     * Free buffered lines.
                     */
                    for (int i = 0;
                         i < buf_line_count;
                         i++)
                    {
                        free(
                            buffered_lines[i]);
                    }

                    buf_line_count = 0;

                    /*
                     * Current line was already output above.
                     * Skip the streaming phase for this iteration.
                     */
                    yyjson_doc_free(
                        doc);

                    if (show_progress)
                    {
                        progress_update(
                            &progress,
                            line_len);
                    }

                    continue;
                }
            }

            if (auto_csv_header_done)
            {
                /*
                 * Streaming phase: output current row directly.
                 */
                auto_csv_write_row(
                    output,
                    root,
                    all_keys,
                    all_key_count);
            }
        }
        else if (cfg.keep_all_key)
        {
            /*
             * -a mode (no csv):
             * passthrough the entire json line as-is
             */
            fprintf(
                output,
                "%s",
                line);

            /*
             * ensure trailing newline
             */
            if (line[line_len - 1]
                != '\n') {
                fprintf(
                    output,
                    "\n");
            }
        }
        else if (cfg.csv_mode)
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
     * Edge case: fewer lines than scan_limit.
     * Flush header + buffered rows now.
     * --------------------------------------------------------
     */
    if (auto_csv
        && !auto_csv_scanned
        && buf_line_count > 0
        && all_key_count > 0)
    {
        auto_csv_sort_keys(
            all_keys,
            all_key_count);

        auto_csv_write_header(
            output,
            all_keys,
            all_key_count);

        for (int li = 0;
             li < buf_line_count;
             li++)
        {
            yyjson_doc *row_doc =
                yyjson_read(
                    buffered_lines[li],
                    strlen(
                        buffered_lines[li]),
                    0);

            if (!row_doc)
            {
                continue;
            }

            yyjson_val *row_root =
                yyjson_doc_get_root(
                    row_doc);

            if (yyjson_is_obj(
                    row_root))
            {
                auto_csv_write_row(
                    output,
                    row_root,
                    all_keys,
                    all_key_count);
            }

            yyjson_doc_free(
                row_doc);
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

    /*
     * Cleanup auto-csv buffers.
     */
    for (int i = 0;
         i < buf_line_count;
         i++)
    {
        free(
            buffered_lines[i]);
    }

    free(
        buffered_lines);

    for (int i = 0;
         i < all_key_count;
         i++)
    {
        free(
            all_keys[i]);
    }

    free(
        all_keys);

    return 0;
}

/*
 * ============================================================
 * Auto-CSV Helpers
 * ============================================================
 */

static void auto_csv_sort_keys(
    char **keys,
    int count)
{
    for (int i = 0;
         i < count - 1;
         i++)
    {
        for (int j = 0;
             j < count - 1 - i;
             j++)
        {
            if (strcmp(
                    keys[j],
                    keys[j + 1])
                > 0)
            {
                char *tmp =
                    keys[j];

                keys[j] =
                    keys[j + 1];

                keys[j + 1] =
                    tmp;
            }
        }
    }
}

static void auto_csv_write_header(
    FILE *out,
    char **keys,
    int count)
{
    for (int i = 0;
         i < count;
         i++)
    {
        csv_write_field(
            out,
            keys[i]);

        if (i + 1 < count)
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

static void auto_csv_write_row(
    FILE *out,
    yyjson_val *root,
    char **keys,
    int count)
{
    for (int ki = 0;
         ki < count;
         ki++)
    {
        yyjson_val *val =
            yyjson_obj_get(
                root,
                keys[ki]);

        if (val)
        {
            csv_write_json_val(
                out,
                val);
        }

        if (ki + 1 < count)
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