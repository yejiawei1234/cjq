#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "extractor.h"
#include "traversal.h"
#include "writer.h"

static char *
value_to_json_string(
    yyjson_val *value);

/*
 * ============================================================
 * Extract Context
 * ============================================================
 */

typedef struct {
    RuleSet *rules;

    yyjson_mut_doc *out_doc;

    yyjson_mut_val *out_root;

    TimeFieldSet *time_fields;

    TrackerSplitSet *tracker_set;

    int found_count;
} ExtractContext;

/*
 * ============================================================
 * Rule Match
 * ============================================================
 */

static int rule_match(
    Rule *rule,
    const VisitContext *ctx) {
    if (rule->found) {
        return 0;
    }

    if (rule->type == RULE_KEY) {
        return
                ctx->key &&
                strcmp(
                    ctx->key,
                    rule->pattern) == 0;
    }

    if (rule->type == RULE_PATH) {
        if (!ctx->path) {
            return 0;
        }

        return
                strcmp(
                    ctx->path,
                    rule->pattern) == 0;
    }

    return 0;
}


/*
 * ============================================================
 * Clone JSON Value
 * ============================================================
 */

static yyjson_mut_val *
clone_value(
    yyjson_mut_doc *doc,
    yyjson_val *src) {
    /*
     * string
     */
    if (yyjson_is_str(src)) {
        return yyjson_mut_strcpy(
            doc,
            yyjson_get_str(src));
    }

    /*
     * int
     */
    if (yyjson_is_int(src)) {
        return yyjson_mut_sint(
            doc,
            yyjson_get_sint(src));
    }

    /*
     * uint
     */
    if (yyjson_is_uint(src)) {
        return yyjson_mut_uint(
            doc,
            yyjson_get_uint(src));
    }

    /*
     * real
     */
    if (yyjson_is_real(src)) {
        return yyjson_mut_real(
            doc,
            yyjson_get_real(src));
    }

    /*
     * bool
     */
    if (yyjson_is_bool(src)) {
        return yyjson_get_bool(src)
                   ? yyjson_mut_true(doc)
                   : yyjson_mut_false(doc);
    }

    /*
     * null
     */
    if (yyjson_is_null(src)) {
        return yyjson_mut_null(doc);
    }

    /*
     * object
     */
    if (yyjson_is_obj(src)) {
        yyjson_mut_val *obj =
                yyjson_mut_obj(doc);

        size_t idx;
        size_t max;

        yyjson_val *key;
        yyjson_val *val;

        yyjson_obj_foreach(
            src,
            idx,
            max,
            key,
            val) {
            yyjson_mut_obj_add(
                obj,

                yyjson_mut_strcpy(
                    doc,
                    yyjson_get_str(key)),

                clone_value(
                    doc,
                    val));
        }

        return obj;
    }

    /*
     * array
     */
    if (yyjson_is_arr(src)) {
        yyjson_mut_val *arr =
                yyjson_mut_arr(doc);

        size_t idx;
        size_t max;

        yyjson_val *item;

        yyjson_arr_foreach(
            src,
            idx,
            max,
            item) {
            yyjson_mut_arr_add_val(
                arr,

                clone_value(
                    doc,
                    item));
        }

        return arr;
    }

    return yyjson_mut_null(doc);
}

/*
 * ============================================================
 * Copy Value To Output
 * ============================================================
 */

static void copy_value(
    yyjson_mut_doc *doc,
    yyjson_mut_val *root,
    const char *key,
    yyjson_val *src) {
    yyjson_mut_obj_add(
        root,

        yyjson_mut_strcpy(
            doc,
            key),

        clone_value(
            doc,
            src));
}

/*
 * ============================================================
 * Visitor
 * ============================================================
 */

static VisitResult extract_visitor(
    const VisitContext *ctx,
    void *user_data) {
    ExtractContext *ec =
            user_data;

    for (int i = 0;
         i < ec->rules->count;
         i++) {
        Rule *rule =
                &ec->rules->rules[i];

        if (!rule_match(
            rule,
            ctx)) {
            continue;
        }

        const char *output_key =
                rule->output_key;

        copy_value(
            ec->out_doc,
            ec->out_root,
            output_key,
            ctx->value);

        if (ec->tracker_set
            &&
            yyjson_is_str(
                ctx->value)
            &&
            should_split_tracker(
                ec->tracker_set,
                ctx->path,
                output_key)) {
            TrackerParts parts;

            if (split_tracker_name(
                    yyjson_get_str(
                        ctx->value),
                    &parts) > 0) {
                const char *prefix =
                        tracker_output_prefix(
                            output_key);
                if (prefix[0] == '\0') {
                    yyjson_mut_obj_add_str(
                        ec->out_doc,
                        ec->out_root,
                        "network",
                        parts.network);

                    if (parts.campaign[0]) {
                        yyjson_mut_obj_add_str(
                            ec->out_doc,
                            ec->out_root,
                            "campaign",
                            parts.campaign);
                    }

                    if (parts.adgroup[0]) {
                        yyjson_mut_obj_add_str(
                            ec->out_doc,
                            ec->out_root,
                            "adgroup",
                            parts.adgroup);
                    }


                    if (parts.creative[0]) {
                        yyjson_mut_obj_add_str(
                            ec->out_doc,
                            ec->out_root,
                            "creative",
                            parts.creative);
                    }
                } else {
                    char key[256];
                    if (parts.network[0]) {
                        snprintf(
                            key,
                            sizeof(key),
                            "%s_network",
                            prefix);

                        yyjson_mut_obj_add(
                            ec->out_root,

                            yyjson_mut_strcpy(
                                ec->out_doc,
                                key),

                            yyjson_mut_strcpy(
                                ec->out_doc,
                                parts.network));
                    }
                    if (parts.campaign[0]) {
                        snprintf(
                            key,
                            sizeof(key),
                            "%s_campaign",
                            prefix);

                        yyjson_mut_obj_add(
                            ec->out_root,

                            yyjson_mut_strcpy(
                                ec->out_doc,
                                key),

                            yyjson_mut_strcpy(
                                ec->out_doc,
                                parts.campaign));
                    }
                    if (parts.adgroup[0]) {
                        snprintf(
                            key,
                            sizeof(key),
                            "%s_adgroup",
                            prefix);

                        yyjson_mut_obj_add(
                            ec->out_root,

                            yyjson_mut_strcpy(
                                ec->out_doc,
                                key),

                            yyjson_mut_strcpy(
                                ec->out_doc,
                                parts.adgroup));
                    }
                    if (parts.creative[0]) {
                        snprintf(
                            key,
                            sizeof(key),
                            "%s_creative",
                            prefix);

                        yyjson_mut_obj_add(
                            ec->out_root,

                            yyjson_mut_strcpy(
                                ec->out_doc,
                                key),

                            yyjson_mut_strcpy(
                                ec->out_doc,
                                parts.creative));
                    }
                }
            }
        }

        if (ec->time_fields
            &&
            yyjson_is_str(
                ctx->value)
            &&
            is_time_field(
                ec->time_fields,
                output_key)) {
            /*
             * epoch
             */
            long long epoch =
                    rfc3339_to_epoch(
                        yyjson_get_str(
                            ctx->value));

            if (epoch >= 0) {
                char epoch_key[256];

                snprintf(
                    epoch_key,
                    sizeof(epoch_key),
                    "%s_epoch",
                    output_key);

                yyjson_mut_obj_add_int(
                    ec->out_doc,

                    ec->out_root,

                    epoch_key,

                    epoch);
            }

            /*
             * offset
             */
            if (ec->time_fields
                ->offset_hours != 0) {
                char *shifted =
                        shift_rfc3339_time(
                            yyjson_get_str(
                                ctx->value),

                            ec->time_fields
                            ->offset_hours);

                if (shifted) {
                    char offset_key[256];

                    snprintf(
                        offset_key,
                        sizeof(offset_key),
                        "%s_offset",
                        output_key);

                    yyjson_mut_obj_add(
                        ec->out_root,

                        yyjson_mut_strcpy(
                            ec->out_doc,
                            offset_key),

                        yyjson_mut_strcpy(
                            ec->out_doc,
                            shifted));

                    free(
                        shifted);
                }
            }
        }

        rule->found = 1;

        ec->found_count++;

        if (ec->found_count ==
            ec->rules->count) {
            return VISIT_STOP;
        }
    }

    return VISIT_CONTINUE;
}

/*
 * ============================================================
 * Extract JSON
 * ============================================================
 */

void extract_json(
    yyjson_doc *doc,
    RuleSet *rules,
    TimeFieldSet *time_fields,
    TrackerSplitSet *tracker_set,
    FILE *out) {
    for (int i = 0;
         i < rules->count;
         i++) {
        rules->rules[i].found =
                0;
    }

    yyjson_mut_doc *out_doc =
            yyjson_mut_doc_new(NULL);

    yyjson_mut_val *root =
            yyjson_mut_obj(out_doc);

    yyjson_mut_doc_set_root(
        out_doc,
        root);

    ExtractContext ec = {

        .rules =
        rules,

        .out_doc =
        out_doc,

        .out_root =
        root,

        .time_fields =
        time_fields,

        .tracker_set =
        tracker_set,

        .found_count =
        0
    };

    TraversalOptions opt = {

        .build_path =
            rules->has_path_rule,

        .parse_embedded_json =
            1,

        .max_embedded_depth =
            5
    };

    bfs_walk(
    yyjson_doc_get_root(doc),
    &opt,
    extract_visitor,
    &ec);

    if (ec.found_count == 0) {
        yyjson_mut_doc_free(
            out_doc);

        return;
    }

    char *json =
            yyjson_mut_write(
                out_doc,
                0,
                NULL);

    if (json) {
        fprintf(
            out,
            "%s\n",
            json);

        free(json);
    }

    yyjson_mut_doc_free(
        out_doc);
}

static void csv_add_column(
    CsvContext *ctx,
    const char *name,
    const char *value) {
    for (int i = 0;
         i < ctx->column_count;
         i++) {
        if (strcmp(
                ctx->columns[i].name,
                name) == 0) {
            free(
                ctx->columns[i].value);

            ctx->columns[i].value =
                    strdup(
                        value
                            ? value
                            : "");

            return;
        }
    }

    if (ctx->column_count
        == ctx->column_cap) {
        int new_cap =
                ctx->column_cap == 0
                    ? 16
                    : ctx->column_cap * 2;

        ctx->columns =
                realloc(
                    ctx->columns,
                    sizeof(CsvColumn)
                    * new_cap);

        ctx->column_cap =
                new_cap;
    }

    CsvColumn *col =
            &ctx->columns[
                ctx->column_count++];

    col->name =
            strdup(name);

    col->value =
            strdup(
                value
                    ? value
                    : "");
}

static const char *
csv_get_value(
    CsvContext *ctx,
    const char *name) {
    for (int i = 0;
         i < ctx->column_count;
         i++) {
        if (strcmp(
                ctx->columns[i].name,
                name) == 0) {
            return
                    ctx->columns[i].value;
        }
    }

    return "";
}

static VisitResult csv_visitor(
    const VisitContext *ctx,
    void *user_data) {
    CsvContext *cc =
            (CsvContext *) user_data;

    for (int i = 0;
         i < cc->rules->count;
         i++) {
        Rule *rule =
                &cc->rules->rules[i];

        if (!rule_match(
            rule,
            ctx)) {
            continue;
        }

        const char *output_key =
                rule->output_key;

        /*
         * string
         */
        if (yyjson_is_str(
            ctx->value)) {
            const char *str =
                    yyjson_get_str(
                        ctx->value);

            csv_add_column(
                cc,
                output_key,
                str);

            /*
             * tracker split
             */
            if (cc->tracker_set
                &&
                should_split_tracker(
                    cc->tracker_set,
                    ctx->path,
                    output_key)) {
                TrackerParts parts;

                if (split_tracker_name(
                        str,
                        &parts) > 0) {
                    const char *prefix =
                            tracker_output_prefix(
                                output_key);

                    char key[256];

                    if (parts.network[0]) {
                        snprintf(
                            key,
                            sizeof(key),
                            prefix[0]
                            ? "%s_network"
                            : "network",
                            prefix);

                        csv_add_column(
                            cc,
                            key,
                            parts.network);
                    }

                    if (parts.campaign[0]) {
                        snprintf(
                            key,
                            sizeof(key),
                            prefix[0]
                            ? "%s_campaign"
                            : "campaign",
                            prefix);

                        csv_add_column(
                            cc,
                            key,
                            parts.campaign);
                    }

                    if (parts.adgroup[0]) {
                        snprintf(
                            key,
                            sizeof(key),
                            prefix[0]
                            ? "%s_adgroup"
                            : "adgroup",
                            prefix);

                        csv_add_column(
                            cc,
                            key,
                            parts.adgroup);
                    }

                    if (parts.creative[0]) {
                        snprintf(
                            key,
                            sizeof(key),
                            prefix[0]
                            ? "%s_creative"
                            : "creative",
                            prefix);

                        csv_add_column(
                            cc,
                            key,
                            parts.creative);
                    }
                }
            }

            /*
             * time fields
             */
            if (cc->time_fields
                &&
                is_time_field(
                    cc->time_fields,
                    output_key)) {
                long long epoch =
                        rfc3339_to_epoch(
                            str);

                if (epoch >= 0) {
                    char epoch_key[256];
                    char epoch_value[64];

                    snprintf(
                        epoch_key,
                        sizeof(epoch_key),
                        "%s_epoch",
                        output_key);

                    snprintf(
                        epoch_value,
                        sizeof(epoch_value),
                        "%lld",
                        epoch);

                    csv_add_column(
                        cc,
                        epoch_key,
                        epoch_value);
                }

                if (cc->time_fields
                    ->offset_hours != 0) {
                    char *shifted =
                            shift_rfc3339_time(
                                str,
                                cc->time_fields
                                ->offset_hours);

                    if (shifted) {
                        char offset_key[256];

                        snprintf(
                            offset_key,
                            sizeof(offset_key),
                            "%s_offset",
                            output_key);

                        csv_add_column(
                            cc,
                            offset_key,
                            shifted);

                        free(
                            shifted);
                    }
                }
            }
        }

        /*
         * int
         */
        else if (
            yyjson_is_int(
                ctx->value)) {
            char buf[64];

            snprintf(
                buf,
                sizeof(buf),
                "%lld",
                (long long)
                yyjson_get_sint(
                    ctx->value));

            csv_add_column(
                cc,
                output_key,
                buf);
        }

        /*
         * uint
         */
        else if (
            yyjson_is_uint(
                ctx->value)) {
            char buf[64];

            snprintf(
                buf,
                sizeof(buf),
                "%llu",
                (unsigned long long)
                yyjson_get_uint(
                    ctx->value));

            csv_add_column(
                cc,
                output_key,
                buf);
        }

        /*
         * real
         */
        else if (
            yyjson_is_real(
                ctx->value)) {
            char buf[64];

            snprintf(
                buf,
                sizeof(buf),
                "%.15g",
                yyjson_get_real(
                    ctx->value));

            csv_add_column(
                cc,
                output_key,
                buf);
        }

        /*
         * bool
         */
        else if (
            yyjson_is_bool(
                ctx->value)) {
            csv_add_column(
                cc,
                output_key,
                yyjson_get_bool(
                    ctx->value)
                    ? "true"
                    : "false");
        }

        /*
         * null
         */
        else if (
            yyjson_is_null(
                ctx->value)) {
            csv_add_column(
                cc,
                output_key,
                "");
        }

        /*
         * object / array
         */
        else if (
            yyjson_is_obj(
                ctx->value)
            ||
            yyjson_is_arr(
                ctx->value)) {
            char *json =
                    value_to_json_string(
                        ctx->value);

            if (json) {
                csv_add_column(
                    cc,
                    output_key,
                    json);

                free(
                    json);
            } else {
                csv_add_column(
                    cc,
                    output_key,
                    "");
            }
        }

        /*
         * fallback
         */
        else {
            csv_add_column(
                cc,
                output_key,
                "");
        }

        rule->found = 1;

        cc->found_count++;

        if (cc->found_count ==
            cc->rules->count) {
            return VISIT_STOP;
        }
    }

    return VISIT_CONTINUE;
}


void extract_csv(
    yyjson_doc *doc,
    RuleSet *rules,
    TimeFieldSet *time_fields,
    TrackerSplitSet *tracker_set,
    FILE *out) {
    for (int i = 0;
         i < rules->count;
         i++) {
        rules->rules[i].found =
                0;
    }

    CsvContext cc = {

        .columns =
        NULL,

        .column_count =
        0,

        .column_cap =
        0,

        .rules =
        rules,

        .time_fields =
        time_fields,

        .tracker_set =
        tracker_set,

        .found_count =
        0
    };

    TraversalOptions opt = {

        .build_path =
            rules->has_path_rule,

        .parse_embedded_json =
            1,

        .max_embedded_depth =
            5
    };

    bfs_walk(
        yyjson_doc_get_root(doc),
        &opt,
        extract_visitor,
        &cc);

    if (cc.found_count == 0) {
        free(
            cc.columns);

        return;
    }

    /*
     * --------------------------------------------------------
     * Rule Columns
     * --------------------------------------------------------
     */

    for (int i = 0;
         i < rules->count;
         i++) {
        csv_write_field(
            out,
            csv_get_value(
                &cc,
                rules->rules[i]
                .output_key));

        if (i + 1
            < rules->count) {
            fputc(
                ',',
                out);
        }
    }

    /*
     * --------------------------------------------------------
     * Tracker Columns
     * --------------------------------------------------------
     */
    if (tracker_set
    && tracker_set->count > 0)
    {
        fputc(
            ',',
            out);
    }

    if (tracker_set) {
        for (int i = 0;
             i < tracker_set->count;
             i++) {
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

            char name[256];

            if (strcmp(
                    key,
                    "tracker_name")
                == 0) {
                csv_write_field(
                    out,
                    csv_get_value(
                        &cc,
                        "network"));

                fputc(
                    ',',
                    out);

                csv_write_field(
                    out,
                    csv_get_value(
                        &cc,
                        "campaign"));

                fputc(
                    ',',
                    out);

                csv_write_field(
                    out,
                    csv_get_value(
                        &cc,
                        "adgroup"));

                fputc(
                    ',',
                    out);

                csv_write_field(
                    out,
                    csv_get_value(
                        &cc,
                        "creative"));
            } else {
                snprintf(
                    name,
                    sizeof(name),
                    "%s_network",
                    key);

                csv_write_field(
                    out,
                    csv_get_value(
                        &cc,
                        name));

                fputc(
                    ',',
                    out);

                snprintf(
                    name,
                    sizeof(name),
                    "%s_campaign",
                    key);

                csv_write_field(
                    out,
                    csv_get_value(
                        &cc,
                        name));

                fputc(
                    ',',
                    out);

                snprintf(
                    name,
                    sizeof(name),
                    "%s_adgroup",
                    key);

                csv_write_field(
                    out,
                    csv_get_value(
                        &cc,
                        name));

                fputc(
                    ',',
                    out);

                snprintf(
                    name,
                    sizeof(name),
                    "%s_creative",
                    key);

                csv_write_field(
                    out,
                    csv_get_value(
                        &cc,
                        name));
            }

            if (i + 1
                < tracker_set->count) {
                fputc(
                    ',',
                    out);
            }
        }
    }

    /*
     * Time Columns
     */


    if (time_fields) {
        for (int i = 0;
             i < time_fields->count;
             i++) {
            char key[256];

            snprintf(
                key,
                sizeof(key),
                "%s_epoch",
                time_fields->fields[i]);

            fputc(
                ',',
                out);

            csv_write_field(
                out,
                csv_get_value(
                    &cc,
                    key));

            if (time_fields
                ->offset_hours != 0) {
                snprintf(
                    key,
                    sizeof(key),
                    "%s_offset",
                    time_fields->fields[i]);

                fputc(
                    ',',
                    out);

                csv_write_field(
                    out,
                    csv_get_value(
                        &cc,
                        key));
                }
             }
    }

    fputc(
        '\n',
        out);

    /*
     * --------------------------------------------------------
     * Cleanup
     * --------------------------------------------------------
     */

    for (int i = 0;
         i < cc.column_count;
         i++) {
        free(
            cc.columns[i]
            .name);

        free(
            cc.columns[i]
            .value);
    }

    free(
        cc.columns);
}
