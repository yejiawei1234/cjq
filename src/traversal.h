#pragma once

#include "yyjson.h"

typedef struct {

    const char *path;

    const char *key;

    yyjson_val *value;

    int depth;

} VisitContext;

typedef enum {

    VISIT_CONTINUE,

    VISIT_STOP

} VisitResult;

typedef VisitResult (*Visitor)(
    const VisitContext *ctx,
    void *user_data
);

typedef struct {

    int build_path;

    int parse_embedded_json;

    int max_embedded_depth;

} TraversalOptions;

void bfs_walk(
        yyjson_val *root,
        const TraversalOptions *opt,
        Visitor visitor,
        void *user_data);