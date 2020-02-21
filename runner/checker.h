#ifndef SANDBOX_CHECKER
#define SANDBOX_CHECKER
int compare(const char *source, const char *target);
extern void DumpAndExit(const void *);
extern void CheckerCompare(const void *);

struct ArgsDumpAndExit
{
    long timeUsage;
    long memoryUsage;
    int systemStatus;
    int judgeStatus;
    char *resultString;
    char *reason;
};
void ArgsDumpAndExitInit(struct ArgsDumpAndExit *);
#endif