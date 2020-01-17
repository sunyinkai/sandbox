#ifndef SANDBOX_RUNNER
#define SANDBOX_RUNNER
#include <sys/resource.h>
struct ArgsParAfterRun
{
    int childExitStatus;  //�ӽ����˳�״̬
    long maxMemUsage;     //����ڴ�ʹ����
    struct rusage rusage; //�ӽ�����Դʹ�����
};

extern int setProgressLimit(int resource, int val);
extern void time_limit_kill(int sigId);
extern void memory_limit_kill();

//runner״̬��
extern void ChildInit(const void *params);
extern void ChildRun(const void *params);
extern void ParMonitor(const void *params);
extern void ParAfterRun(const void *params);

#endif