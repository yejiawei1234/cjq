#include <stdlib.h>
#include <string.h>

#include "rule.h"

int rule_parse(
    const char *keys,
    RuleSet *set) {
    char *copy = strdup(keys);

    if (!copy) {
        return -1;
    }

    int count = 1;

    for (char *p = copy; *p;p++) {
        if (*p == ',') {
            count++;
        }
    }

    set->rules =
            calloc(
                count,
                sizeof(Rule));

    if (!set->rules) {
        free(copy);

        return -1;
    }

    set->count = count;
    set->has_path_rule = 0;

    int idx = 0;

    char *token =
            strtok(
                copy,
                ",");

    while (token) {
        Rule *rule = &set->rules[idx++];

        char *alias =
                strchr(
                    token,
                    ':');

        /*
         * host:h
         *
         * context.CallbackData.AppToken:app_token
         */
        if (alias) {
            *alias = '\0';

            alias++;

            strncpy(
                rule->pattern,
                token,
                sizeof(rule->pattern) - 1);

            strncpy(
                rule->output_key,
                alias,
                sizeof(rule->output_key) - 1);
        } else {
            strncpy(
                rule->pattern,
                token,
                sizeof(rule->pattern) - 1);

            strncpy(
                rule->output_key,
                token,
                sizeof(rule->output_key) - 1);
        }

        if (strchr(
            rule->pattern,
            '.')) {
            rule->type =
                    RULE_PATH;

            set->has_path_rule = 1;
        } else {
            rule->type =
                    RULE_KEY;
        }

        token =
                strtok(
                    NULL,
                    ",");
    }

    free(copy);

    return 0;
}

void rule_destroy(
    RuleSet *set) {
    free(set->rules);

    set->rules = NULL;

    set->count = 0;
}
