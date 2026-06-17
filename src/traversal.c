#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "traversal.h"

#define INITIAL_QUEUE_CAP 1024

/*
 * ============================================================
 * Queue Node
 * ============================================================
 */

typedef struct {

    yyjson_val *node;

    const char *key;

    int depth;

    char *path;

} QueueNode;

/*
 * ============================================================
 * Queue
 * ============================================================
 */

typedef struct {

    QueueNode *items;

    size_t head;

    size_t tail;

    size_t cap;

} Queue;

/*
 * ============================================================
 * Queue Helpers
 * ============================================================
 */

static void queue_init(
    Queue *q)
{
    q->cap =
        INITIAL_QUEUE_CAP;

    q->items =
        malloc(
            sizeof(QueueNode)
            * q->cap);

    if (!q->items)
    {
        perror("malloc");

        exit(EXIT_FAILURE);
    }

    q->head = 0;
    q->tail = 0;
}

static void free_node(
    QueueNode *node)
{
    if (node->path)
    {
        free(node->path);

        node->path = NULL;
    }
}

static void queue_destroy(
    Queue *q)
{
    for (size_t i = q->head;
         i < q->tail;
         i++)
    {
        free_node(
            &q->items[i]);
    }

    free(q->items);

    q->items = NULL;

    q->head = 0;
    q->tail = 0;
    q->cap = 0;
}

static int queue_empty(
    Queue *q)
{
    return q->head == q->tail;
}

static size_t queue_size(
    Queue *q)
{
    return q->tail - q->head;
}

static void queue_expand(
    Queue *q)
{
    size_t size =
        queue_size(q);

    size_t new_cap =
        q->cap * 2;

    QueueNode *new_items =
        malloc(
            sizeof(QueueNode)
            * new_cap);

    if (!new_items)
    {
        perror("malloc");

        exit(EXIT_FAILURE);
    }

    for (size_t i = 0;
         i < size;
         i++)
    {
        new_items[i] =
            q->items[q->head + i];
    }

    free(q->items);

    q->items =
        new_items;

    q->head = 0;
    q->tail = size;
    q->cap = new_cap;
}

static void enqueue(
    Queue *q,
    const QueueNode *node)
{
    if (q->tail >= q->cap)
    {
        queue_expand(q);
    }

    q->items[q->tail++] =
        *node;
}

static QueueNode dequeue(
    Queue *q)
{
    return q->items[q->head++];
}

/*
 * ============================================================
 * Path Helpers
 * ============================================================
 */

static char *path_join_key(
    const char *parent,
    const char *key)
{
    if (!parent || parent[0] == '\0')
    {
        return strdup(key);
    }

    size_t len =
        strlen(parent)
        + strlen(key)
        + 2;

    char *path =
        malloc(len);

    if (!path)
    {
        return NULL;
    }

    snprintf(
        path,
        len,
        "%s.%s",
        parent,
        key);

    return path;
}

static char *path_join_index(
    const char *parent,
    size_t idx)
{
    char buf[64];

    snprintf(
        buf,
        sizeof(buf),
        "[%zu]",
        idx);

    if (!parent || parent[0] == '\0')
    {
        return strdup(buf);
    }

    size_t len =
        strlen(parent)
        + strlen(buf)
        + 1;

    char *path =
        malloc(len);

    if (!path)
    {
        return NULL;
    }

    snprintf(
        path,
        len,
        "%s%s",
        parent,
        buf);

    return path;
}

/*
 * ============================================================
 * BFS
 * ============================================================
 */

void bfs_walk(
    yyjson_val *root,
    int build_path,
    Visitor visitor,
    void *user_data)
{
    if (!root || !visitor)
    {
        return;
    }

    Queue q;

    queue_init(&q);

    QueueNode root_node;

    memset(
        &root_node,
        0,
        sizeof(root_node));

    root_node.node =
        root;

    root_node.depth =
        0;

    root_node.path =
        NULL;

    enqueue(
        &q,
        &root_node);

    while (!queue_empty(&q))
    {
        QueueNode current =
            dequeue(&q);

        VisitContext ctx = {

            .path =
                current.path,

            .key =
                current.key,

            .value =
                current.node,

            .depth =
                current.depth
        };

        if (visitor(
                &ctx,
                user_data)
            == VISIT_STOP)
        {
            free_node(
                &current);

            break;
        }

        yyjson_val *node =
            current.node;

        /*
         * ----------------------------------------------------
         * OBJECT
         * ----------------------------------------------------
         */

        if (yyjson_is_obj(node))
        {
            size_t idx;
            size_t max;

            yyjson_val *key;
            yyjson_val *val;

            yyjson_obj_foreach(
                node,
                idx,
                max,
                key,
                val)
            {
                QueueNode child;

                memset(
                    &child,
                    0,
                    sizeof(child));

                child.node =
                    val;

                child.key =
                    yyjson_get_str(key);

                child.depth =
                    current.depth + 1;

                if (build_path)
                {
                    child.path =
                        path_join_key(
                            current.path,
                            child.key);
                }

                enqueue(
                    &q,
                    &child);
            }
        }

        /*
         * ----------------------------------------------------
         * ARRAY
         * ----------------------------------------------------
         */

        else if (
            yyjson_is_arr(node))
        {
            size_t idx;
            size_t max;

            yyjson_val *item;

            yyjson_arr_foreach(
                node,
                idx,
                max,
                item)
            {
                QueueNode child;

                memset(
                    &child,
                    0,
                    sizeof(child));

                child.node =
                    item;

                child.depth =
                    current.depth + 1;

                if (build_path)
                {
                    child.path =
                        path_join_index(
                            current.path,
                            idx);
                }

                enqueue(
                    &q,
                    &child);
            }
        }

        free_node(
            &current);
    }

    queue_destroy(&q);
}