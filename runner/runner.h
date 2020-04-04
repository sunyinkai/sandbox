#ifndef SANDBOX_RUNNER
#define SANDBOX_RUNNER
#include <sys/resource.h>
struct ArgsParAfterRun
{
    int childExitStatus;  //子进程退出状态
    long maxMemUsage;     //最大内存使用量
    struct rusage rusage; //子进程资源使用情况
};

static int setProgressLimit(int resource, int val);
static void time_limit_kill(int sigId);

//runner状态机
extern void ChildInit(const void *params);
extern void ChildRun(const void *params);
extern void ParMonitor(const void *params);
extern void ParAfterRun(const void *params);

#endif