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

void bfs_walk(
    yyjson_val *root,
    int build_path,
    Visitor visitor,
    void *user_data);