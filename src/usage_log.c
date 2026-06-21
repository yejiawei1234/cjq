//
// Created by jack ye on 6/21/26.
//

#include "usage_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "usage_log.h"

#define USAGE_LOG_FILE \
"/script/usage/py_jq_usage.log"

void write_usage_log(void)
{
    /*
     * file must already exist
     */
    FILE *check =
        fopen(
            USAGE_LOG_FILE,
            "r");

    if (!check)
    {
        return;
    }

    fclose(check);

    FILE *fp =
        fopen(
            USAGE_LOG_FILE,
            "a");

    if (!fp)
    {
        return;
    }

    time_t now =
        time(NULL);

    struct tm tm_now;

    localtime_r(
        &now,
        &tm_now);

    char time_buf[64];

    strftime(
        time_buf,
        sizeof(time_buf),
        "%Y/%m/%d %H:%M:%S",
        &tm_now);

    const char *user =
        getenv("USER");

    if (!user)
    {
        user = "unknown";
    }

    fprintf(
        fp,
        "%s %s start using cjq\n",
        time_buf,
        user);

    fclose(fp);
}