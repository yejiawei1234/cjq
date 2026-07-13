//
// Created by jack ye on 6/20/26.
//

#include "tracker_split.h"
#include <stdlib.h>
#include <string.h>

static const char *
last_key(
    const char *path)
{
    const char *p =
        strrchr(
            path,
            '.');

    return p
        ? p + 1
        : path;
}

/*
 * ============================================================
 * Init
 * ============================================================
 */

int tracker_split_init(
    TrackerSplitSet *set,
    const char *field_list)
{
    memset(
        set,
        0,
        sizeof(*set));

    /*
     * default:
     * context.tracker_name
     */
    if (!field_list)
    {
        set->fields =
            calloc(
                1,
                sizeof(char *));

        set->fields[0] =
            strdup(
                "context.tracker_name");

        set->count = 1;

        return 0;
    }

    char *copy =
        strdup(field_list);

    int count = 1;

    for (char *p = copy;
         *p;
         p++)
    {
        if (*p == ',')
        {
            count++;
        }
    }

    set->fields =
        calloc(
            count,
            sizeof(char *));

    set->count =
        count;

    int idx = 0;

    char *token =
        strtok(
            copy,
            ",");

    while (token)
    {
        set->fields[idx++] =
            strdup(token);

        token =
            strtok(
                NULL,
                ",");
    }

    free(copy);

    return 0;
}

/*
 * ============================================================
 * Destroy
 * ============================================================
 */

void tracker_split_destroy(
    TrackerSplitSet *set)
{
    for (int i = 0;
         i < set->count;
         i++)
    {
        free(
            set->fields[i]);
    }

    free(
        set->fields);

    set->fields = NULL;

    set->count = 0;
}

/*
 * ============================================================
 * Match
 * ============================================================
 */

int should_split_tracker(
    TrackerSplitSet *set,
    const char *path,
    const char *key)
{
    for (int i = 0;
         i < set->count;
         i++)
    {
        if (path &&
            strcmp(
                path,
                set->fields[i]) == 0)
        {
            return 1;
        }

        if (strcmp(
                key,
                last_key(
                    set->fields[i]))
            == 0)
        {
            return 1;
        }
    }

    return 0;
}

/*
 * ============================================================
 * Split
 * ============================================================
 */

int split_tracker_name(
    const char *src,
    TrackerParts *parts)
{
    return split_tracker_name_len(
        src,
        src ? strlen(src) : 0,
        parts);
}

static void copy_clean_part(
    char *dst,
    size_t dst_size,
    const char *src,
    size_t len)
{
    if (!dst || dst_size == 0)
    {
        return;
    }

    size_t out = 0;

    for (size_t i = 0;
         i < len && out + 1 < dst_size;
         i++)
    {
        unsigned char ch =
            (unsigned char)src[i];

        if (ch == '\0')
        {
            if (i == 0
                && len >= 6
                && memcmp(
                    src + 1,
                    "ikTok",
                    5) == 0)
            {
                dst[out++] = 'T';
            }

            continue;
        }

        if (ch < 0x20 && ch != '\t')
        {
            continue;
        }

        dst[out++] =
            (char)ch;
    }

    dst[out] = '\0';
}

int split_tracker_name_len(
    const char *src,
    size_t len,
    TrackerParts *parts)
{
    if (!src || !parts)
    {
        return -1;
    }

    memset(
        parts,
        0,
        sizeof(*parts));

    char *targets[4] = {
        parts->network,
        parts->campaign,
        parts->adgroup,
        parts->creative
    };

    size_t sizes[4] = {
        sizeof(parts->network),
        sizeof(parts->campaign),
        sizeof(parts->adgroup),
        sizeof(parts->creative)
    };

    int count = 0;
    size_t start = 0;

    for (size_t i = 0;
         i <= len && count < 4;
         i++)
    {
        if (i == len
            || (i + 1 < len
                && src[i] == ':'
                && src[i + 1] == ':'))
        {
            copy_clean_part(
                targets[count],
                sizes[count],
                src + start,
                i - start);

            count++;

            if (i == len)
            {
                break;
            }

            i++;
            start = i + 1;
        }
    }

    return count;
}

/*
 * ============================================================
 * Prefix
 * ============================================================
 */

const char *tracker_output_prefix(
    const char *key)
{
    if (strcmp(
            last_key(key),
            "tracker_name") == 0)
    {
        return "";
    }

    return key;
}
