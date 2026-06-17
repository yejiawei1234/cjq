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

    int found_count;
} ExtractContext;

/*
 * ============================================================
 * Rule Match
 * ============================================================
 */

static int rule_match(
    Rule *rule,
    const VisitContext *ctx)
{
    if (rule->found)
    {
        return 0;
    }

    if (rule->type == RULE_KEY)
    {
        return
            ctx->key &&
            strcmp(
                ctx->key,
                rule->pattern) == 0;
    }

    if (rule->type == RULE_PATH)
    {
        if (!ctx->path)
        {
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
            (ExtractContext *) user_data;

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

        if (ec->time_fields
    &&
    ec->time_fields->offset_hours
    &&
    yyjson_is_str(
        ctx->value)
    &&
    is_time_field(
        ec->time_fields,
        output_key))
        {
            char *shifted =
                shift_rfc3339_time(
                    yyjson_get_str(
                        ctx->value),

                    ec->time_fields
                        ->offset_hours);
            if (shifted)
            {
                char key[256];

                snprintf(
                    key,
                    sizeof(key),
                    "%s_offset",
                    output_key);

                yyjson_mut_obj_add(
                    ec->out_root,

                    yyjson_mut_strcpy(
                        ec->out_doc,
                        key),

                    yyjson_mut_strcpy(
                        ec->out_doc,
                        shifted));

                free(
                    shifted);
            }
        }

        rule->found = 1;

        ec->found_count++;

        if (ec->found_count ==
            ec->rules->count)
        {
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

        .found_count =
            0
    };

    bfs_walk(
        yyjson_doc_get_root(doc),

        rules->has_path_rule,

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

typedef struct {

    RuleSet *rules;

    char **values;


    TimeFieldSet *time_fields;
    int found_count;

} CsvContext;

static VisitResult csv_visitor(
    const VisitContext *ctx,
    void *user_data)
{
    CsvContext *cc =
        (CsvContext *)user_data;

    for (int i = 0;
         i < cc->rules->count;
         i++)
    {
        Rule *rule =
            &cc->rules->rules[i];

        if (!rule_match(
                rule,
                ctx))
        {
            continue;
        }

        /*
         * string
         */
        if (yyjson_is_str(
                ctx->value))
        {
            cc->values[i] =
                strdup(
                    yyjson_get_str(
                        ctx->value));
        }

        /*
         * int
         */
        else if (
            yyjson_is_int(
                ctx->value))
        {
            char buf[64];

            snprintf(
                buf,
                sizeof(buf),
                "%lld",
                (long long)
                yyjson_get_sint(
                    ctx->value));

            cc->values[i] =
                strdup(buf);
        }

        /*
         * uint
         */
        else if (
            yyjson_is_uint(
                ctx->value))
        {
            char buf[64];

            snprintf(
                buf,
                sizeof(buf),
                "%llu",
                (unsigned long long)
                yyjson_get_uint(
                    ctx->value));

            cc->values[i] =
                strdup(buf);
        }

        /*
         * real
         */
        else if (
            yyjson_is_real(
                ctx->value))
        {
            char buf[64];

            snprintf(
                buf,
                sizeof(buf),
                "%.15g",
                yyjson_get_real(
                    ctx->value));

            cc->values[i] =
                strdup(buf);
        }

        /*
         * bool
         */
        else if (
            yyjson_is_bool(
                ctx->value))
        {
            cc->values[i] =
                strdup(
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
                ctx->value))
        {
            cc->values[i] =
                strdup("");
        }

        /*
         * object / array
         *
         * serialize to json string
         */
        else if (
            yyjson_is_obj(
                ctx->value)

            ||

            yyjson_is_arr(
                ctx->value))
        {
            char *json =
                value_to_json_string(
                    ctx->value);

            if (json)
            {
                cc->values[i] =
                    strdup(json);

                free(json);
            }
            else
            {
                cc->values[i] =
                    strdup("");
            }
        }

        /*
         * fallback
         */
        else
        {
            cc->values[i] =
                strdup("");
        }

        rule->found = 1;

        cc->found_count++;

        /*
         * all rules found
         */
        if (cc->found_count ==
            cc->rules->count)
        {
            return VISIT_STOP;
        }
    }

    return VISIT_CONTINUE;
}

static char *
    value_to_json_string(
    yyjson_val *value)
{
    yyjson_mut_doc *doc =
        yyjson_mut_doc_new(NULL);

    if (!doc)
    {
        return NULL;
    }

    yyjson_mut_val *root =
        clone_value(
            doc,
            value);

    yyjson_mut_doc_set_root(
        doc,
        root);

    char *json =
        yyjson_mut_write(
            doc,
            0,
            NULL);

    yyjson_mut_doc_free(
        doc);

    return json;
}

void extract_csv(
    yyjson_doc *doc,
    RuleSet *rules,
    TimeFieldSet *time_fields,
    FILE *out)
{
    for (int i = 0;
         i < rules->count;
         i++)
    {
        rules->rules[i].found =
            0;
    }

    char **values =
        calloc(
            rules->count,
            sizeof(char *));

    if (!values)
    {
        return;
    }

    CsvContext cc = {

        .rules =
            rules,

        .values =
            values,

        .time_fields = time_fields,

        .found_count =
            0
    };

    bfs_walk(
        yyjson_doc_get_root(doc),

        rules->has_path_rule,

        csv_visitor,

        &cc);

    /*
     * nothing matched
     */
    if (cc.found_count == 0)
    {
        free(values);

        return;
    }

    csv_write_row(
        out,
        values,
        rules->count);

    for (int i = 0;
         i < rules->count;
         i++)
    {
        free(
            values[i]);
    }

    free(values);
}
