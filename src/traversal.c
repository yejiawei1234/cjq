#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "traversal.h"

#define INITIAL_QUEUE_CAP 1024
#define DEFAULT_MAX_EMBEDDED_DEPTH 5

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

    /*
     * Embedded JSON 展开层级。
     *
     * 普通原始 JSON 节点:
     * embedded_depth = 0
     *
     * 从 JSON string 解析出来的节点:
     * embedded_depth = 1, 2, 3...
     */
    int embedded_depth;

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

    /*
     * 所有从 embedded JSON string 解析出来的 yyjson_doc
     * 统一由 Queue 持有生命周期。
     *
     * 不能把 yyjson_doc 挂在单个 QueueNode 上，
     * 因为父节点出队释放后，子节点可能还在队列里。
     */
    yyjson_doc **docs;

    size_t doc_count;

    size_t doc_cap;

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

    q->docs = NULL;
    q->doc_count = 0;
    q->doc_cap = 0;
}

static void queue_add_doc(
    Queue *q,
    yyjson_doc *doc)
{
    if (!doc)
    {
        return;
    }

    if (q->doc_count
        == q->doc_cap)
    {
        size_t new_cap =
            q->doc_cap == 0
                ? 16
                : q->doc_cap * 2;

        yyjson_doc **new_docs =
            realloc(
                q->docs,
                sizeof(yyjson_doc *)
                * new_cap);

        if (!new_docs)
        {
            perror("realloc");

            yyjson_doc_free(
                doc);

            exit(EXIT_FAILURE);
        }

        q->docs =
            new_docs;

        q->doc_cap =
            new_cap;
    }

    q->docs[
        q->doc_count++] =
        doc;
}

static void free_node(
    QueueNode *node)
{
    if (node->path)
    {
        free(
            node->path);

        node->path =
            NULL;
    }
}

static void queue_destroy(
    Queue *q)
{
    /*
     * 释放还没有 dequeue 的 QueueNode path。
     *
     * 已经 dequeue 出来的 current 会在主循环中单独 free_node()。
     */
    for (size_t i = q->head;
         i < q->tail;
         i++)
    {
        free_node(
            &q->items[i]);
    }

    free(
        q->items);

    q->items = NULL;
    q->head = 0;
    q->tail = 0;
    q->cap = 0;

    /*
     * 统一释放 embedded JSON docs。
     */
    for (size_t i = 0;
         i < q->doc_count;
         i++)
    {
        yyjson_doc_free(
            q->docs[i]);
    }

    free(
        q->docs);

    q->docs = NULL;
    q->doc_count = 0;
    q->doc_cap = 0;
}

static int queue_empty(
    Queue *q)
{
    return
        q->head == q->tail;
}

static size_t queue_size(
    Queue *q)
{
    return
        q->tail - q->head;
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

    /*
     * 只移动当前还在队列中的有效元素。
     *
     * 这里是浅拷贝 QueueNode。
     * path 指针的所有权从旧数组转移到新数组。
     */
    for (size_t i = 0;
         i < size;
         i++)
    {
        new_items[i] =
            q->items[
                q->head + i];
    }

    free(
        q->items);

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
        queue_expand(
            q);
    }

    q->items[
        q->tail++] =
        *node;
}

static QueueNode dequeue(
    Queue *q)
{
    return
        q->items[
            q->head++];
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
    if (!key)
    {
        return NULL;
    }

    if (!parent
        || parent[0] == '\0')
    {
        return
            strdup(key);
    }

    size_t len =
        strlen(parent)
        + strlen(key)
        + 2;

    /*
     * +1 for '.'
     * +1 for '\0'
     */

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

    if (!parent
        || parent[0] == '\0')
    {
        return
            strdup(buf);
    }

    size_t len =
        strlen(parent)
        + strlen(buf)
        + 1;

    /*
     * +1 for '\0'
     */

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
 * Embedded JSON Helpers
 * ============================================================
 */

static int is_json_ws(
    char ch)
{
    return
        ch == ' '
        || ch == '\t'
        || ch == '\n'
        || ch == '\r';
}

static yyjson_doc *parse_embedded_json(
    yyjson_val *node)
{
    if (!yyjson_is_str(node))
    {
        return NULL;
    }

    const char *str =
        yyjson_get_str(node);

    size_t len =
        yyjson_get_len(node);

    if (!str
        || len == 0)
    {
        return NULL;
    }

    /*
     * 跳过前导空白。
     */
    while (len > 0
           && is_json_ws(*str))
    {
        str++;
        len--;
    }

    if (len == 0)
    {
        return NULL;
    }

    /*
     * 只尝试解析 object / array。
     *
     * 例如:
     * "{\"a\":1}" -> parse
     * "[1,2,3]"  -> parse
     * "hello"    -> skip
     * "123"      -> skip
     */
    if (*str != '{'
        && *str != '[')
    {
        return NULL;
    }

    return
        yyjson_read(
            str,
            len,
            0);
}

/*
 * ============================================================
 * Child Enqueue Helper
 * ============================================================
 */

static void enqueue_children(
    Queue *q,
    yyjson_val *node,
    const char *parent_path,
    int parent_depth,
    int embedded_depth,
    const TraversalOptions *opt)
{
    /*
     * --------------------------------------------------------
     * OBJECT
     * --------------------------------------------------------
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
                parent_depth + 1;

            child.embedded_depth =
                embedded_depth;

            if (opt->build_path)
            {
                child.path =
                    path_join_key(
                        parent_path,
                        child.key);
            }

            enqueue(
                q,
                &child);
        }

        return;
    }

    /*
     * --------------------------------------------------------
     * ARRAY
     * --------------------------------------------------------
     */

    if (yyjson_is_arr(node))
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
                parent_depth + 1;

            child.embedded_depth =
                embedded_depth;

            if (opt->build_path)
            {
                child.path =
                    path_join_index(
                        parent_path,
                        idx);
            }

            enqueue(
                q,
                &child);
        }
    }
}

/*
 * ============================================================
 * BFS
 * ============================================================
 */

void bfs_walk(
    yyjson_val *root,
    const TraversalOptions *opt,
    Visitor visitor,
    void *user_data)
{
    if (!root
        || !visitor)
    {
        return;
    }

    TraversalOptions default_opt = {

        .build_path =
            0,

        .parse_embedded_json =
            0,

        .max_embedded_depth =
            DEFAULT_MAX_EMBEDDED_DEPTH
    };

    if (!opt)
    {
        opt =
            &default_opt;
    }

    Queue q;

    queue_init(
        &q);

    QueueNode root_node;

    memset(
        &root_node,
        0,
        sizeof(root_node));

    root_node.node =
        root;

    root_node.depth =
        0;

    root_node.embedded_depth =
        0;

    enqueue(
        &q,
        &root_node);

    while (!queue_empty(&q))
    {
        QueueNode current =
            dequeue(
                &q);

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

        /*
         * 先访问当前节点。
         *
         * 例如:
         *
         * {
         *   "payload": "{\"user\":{\"id\":1}}"
         * }
         *
         * visitor 会先看到:
         *
         * path  = "payload"
         * value = "{\"user\":{\"id\":1}}"
         *
         * 然后 traversal 再把里面的:
         *
         * payload.user
         * payload.user.id
         *
         * 加入 BFS 队列。
         */
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
         * Embedded JSON Expansion
         * ----------------------------------------------------
         *
         * 如果当前节点是 string，并且内容可以解析成 JSON object/array，
         * 则把解析后的 root 的 children 加入当前 BFS 队列。
         *
         * 注意：
         *
         * 这里不把 embedded root 本身作为一个新的 visitor 节点。
         *
         * 也就是说:
         *
         * payload = "{\"user\":{\"id\":1}}"
         *
         * visitor 看到:
         *
         * payload
         * payload.user
         * payload.user.id
         *
         * 而不是:
         *
         * payload
         * payload
         * payload.user
         * payload.user.id
         *
         * 这样可以避免 path = payload 被重复访问两次。
         */

        if (opt->parse_embedded_json
            &&
            opt->max_embedded_depth > 0
            &&
            current.embedded_depth
                < opt->max_embedded_depth)
        {
            yyjson_doc *embedded_doc =
                parse_embedded_json(
                    node);

            if (embedded_doc)
            {
                yyjson_val *embedded_root =
                    yyjson_doc_get_root(
                        embedded_doc);

                if (embedded_root)
                {
                    queue_add_doc(
                        &q,
                        embedded_doc);

                    enqueue_children(
                        &q,
                        embedded_root,
                        current.path,
                        current.depth,
                        current.embedded_depth + 1,
                        opt);
                }
                else
                {
                    yyjson_doc_free(
                        embedded_doc);
                }
            }
        }

        /*
         * ----------------------------------------------------
         * Normal JSON Children
         * ----------------------------------------------------
         */

        enqueue_children(
            &q,
            node,
            current.path,
            current.depth,
            current.embedded_depth,
            opt);

        free_node(
            &current);
    }

    queue_destroy(
        &q);
}