#include <stdio.h>

#include "progress.h"

/*
 * ============================================================
 * File Size
 * ============================================================
 */

long get_file_size(
    FILE *fp)
{
    long current =
        ftell(fp);

    fseek(
        fp,
        0,
        SEEK_END);

    long size =
        ftell(fp);

    fseek(
        fp,
        current,
        SEEK_SET);

    return size;
}

/*
 * ============================================================
 * Draw Progress
 * ============================================================
 */

static void draw_progress(
    Progress *p)
{
    double percent =
        (double)p->current_bytes
        / (double)p->total_bytes;

    if (percent > 1.0)
    {
        percent = 1.0;
    }

    int filled =
        (int)(
            percent
            * p->bar_width);

    printf("\r[");

    for (int i = 0;
         i < p->bar_width;
         i++)
    {
        putchar(
            i < filled
            ? '#'
            : '-');
    }

    printf(
        "] %6.2f%%",
        percent * 100.0);

    fflush(stdout);
}

/*
 * ============================================================
 * Init
 * ============================================================
 */

void progress_init(
    Progress *p,
    long total)
{
    p->total_bytes =
        total;

    p->current_bytes =
        0;

    p->last_refresh =
        0;

    p->bar_width =
        40;

    draw_progress(p);
}

/*
 * ============================================================
 * Update
 * ============================================================
 */

void progress_update(
    Progress *p,
    long bytes)
{
    p->current_bytes +=
        bytes;

    /*
     * 每 1MB 刷新一次
     */
    if (p->current_bytes
        - p->last_refresh
        < 1024 * 1024)
    {
        return;
    }

    p->last_refresh =
        p->current_bytes;

    draw_progress(p);
}

/*
 * ============================================================
 * Finish
 * ============================================================
 */

void progress_finish(
    Progress *p)
{
    p->current_bytes =
        p->total_bytes;

    draw_progress(p);

    printf("\n");
}