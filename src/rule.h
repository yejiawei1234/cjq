#pragma once

typedef enum {

    RULE_KEY,

    RULE_PATH

} RuleType;

typedef struct {

    RuleType type;

    /*
     * host
     *
     * context.CallbackData.AppToken
     */
    char pattern[256];

    /*
     * alias
     *
     * host:h
     * AppToken:app_token
     */
    char output_key[128];

    int found;

} Rule;

typedef struct {

    Rule *rules;

    int count;

    int has_path_rule;

} RuleSet;

int rule_parse(
    const char *keys,
    RuleSet *set);

void rule_destroy(
    RuleSet *set);