//
// Created by jack ye on 6/20/26.
//

#include "tracker_split.h"
#include <stdlib.h>
#include <string.h>

#include "tracker_split.h"

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
    if (!src || !parts)
    {
        return -1;
    }

    memset(
        parts,
        0,
        sizeof(*parts));

    char buf[2048];

    strncpy(
        buf,
        src,
        sizeof(buf) - 1);

    buf[
        sizeof(buf) - 1] = '\0';

    char *segments[4];

    int count = 0;

    char *start = buf;

    while (count < 4)
    {
        char *sep =
            strstr(
                start,
                "::");

        if (!sep)
        {
            segments[count++] =
                start;

            break;
        }

        *sep = '\0';

        segments[count++] =
            start;

        start =
            sep + 2;
    }

    if (count >= 1)
    {
        strncpy(
            parts->network,
            segments[0],
            sizeof(parts->network) - 1);
    }

    if (count >= 2)
    {
        strncpy(
            parts->campaign,
            segments[1],
            sizeof(parts->campaign) - 1);
    }

    if (count >= 3)
    {
        strncpy(
            parts->adgroup,
            segments[2],
            sizeof(parts->adgroup) - 1);
    }

    if (count >= 4)
    {
        strncpy(
            parts->creative,
            segments[3],
            sizeof(parts->creative) - 1);
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
            key,
            "tracker_name") == 0)
    {
        return "";
    }

    return key;
}