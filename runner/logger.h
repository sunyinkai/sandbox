#ifndef RUNNER_LOGGER
#define RUNNER_LOGGER
#define CLOG_MAIN
#include "clog.h"
void init_log(int uinqId, enum clog_level level)
{
    int r;
    r = clog_init_path(uinqId, "log.txt");
    if (r != 0)
    {
        exit(1);
    }
    clog_set_level(uinqId, level);
}

#endif