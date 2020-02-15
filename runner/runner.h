#ifndef SANDBOX_RUNNER
#define SANDBOX_RUNNER
#include <sys/resource.h>
struct ArgsParAfterRun
{
    int childExitStatus;  //子进程退出状态
    long maxMemUsage;     //最大内存使用量
    struct rusage rusage; //子进程资源使用情况
};

extern int setProgressLimit(int resource, int val);
extern void time_limit_kill(int sigId);
extern void memory_limit_kill();

//runner状态机
extern void ChildInit(const void *params);
extern void ChildRun(const void *params);
extern void ParMonitor(const void *params);
extern void ParAfterRun(const void *params);

//子进程相关信息
struct ChildProgressInfo
{
    int child_pid;       //子进程pid
    int child_exit_code; //子进程退出状态码
};
#endif