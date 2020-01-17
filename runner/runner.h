#ifndef SANDBOX_RUNNER
#define SANDBOX_RUNNER
#include<sys/resource.h>

extern int setProgressLimit(int resource, int val);
extern void time_limit_kill(int sigId);
extern void memory_limit_kill();
struct ArgsParAfterRun
{
    int childExitStatus;  //子进程退出状态
    long maxMemUsage;     //最大内存使用量
    struct rusage rusage; //子进程资源使用情况
};

extern void ChildInit(const void *params);
extern void ChildRun(const void *params);
extern void ParMonitor(const void *params);
extern void ParAfterRun(const void *params);




#endif