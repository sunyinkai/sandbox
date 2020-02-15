#include <stdio.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>

#include "syscall_monitor.h"
#include "fsm.h"
#include "resource.h"
#include "runner.h"
#include "checker.h"
#include "include/clog.h"
#include "include/cJSON.h"
#include "contants.h"

extern struct ResourceConfig resouceConfig;
extern struct FileInfo fileInfo;
extern int UNIQ_LOG_ID;
static struct ChildProgressInfo childProgressInfo; //?

//设置成功返回0,否则返回-1
int setProgressLimit(int resource, int val)
{
    struct rlimit limit;
    limit.rlim_cur = limit.rlim_max = val;
    int retCode = setrlimit(resource, &limit);
    return retCode;
}

//超时杀死程序
void time_limit_kill(int sigId)
{
    if (childProgressInfo.child_pid > 0)
    {
        int retCode = kill(childProgressInfo.child_pid, SIGKILL);
        if (retCode != 0)
        {
            struct ArgsDumpAndExit argsDumpAndExit;
            ArgsDumpAndExitInit(&argsDumpAndExit);
            argsDumpAndExit.systemStatus = EXIT_SYSTEM_ERROR;
            clog_error(CLOG(UNIQ_LOG_ID), "kill system call error\n");
            FSMEventHandler(&fsm, CondProgramNeedToExit, &argsDumpAndExit);
            return;
        }
    }
    alarm(0);
}

void memory_limit_kill()
{
    if (childProgressInfo.child_pid > 0)
    {
        int retCode = kill(childProgressInfo.child_pid, SIGKILL);
        if (retCode != 0)
        {
            struct ArgsDumpAndExit argsDumpAndExit;
            ArgsDumpAndExitInit(&argsDumpAndExit);
            argsDumpAndExit.systemStatus = EXIT_SYSTEM_ERROR;
            argsDumpAndExit.reason = "kill system cal error";
            clog_error(CLOG(UNIQ_LOG_ID), "kill system call error\n");
            FSMEventHandler(&fsm, CondProgramNeedToExit, &argsDumpAndExit);
            return;
        }
    }
    alarm(0);
}

//Runner起始函数
void Run(const void *params)
{
    printf("begin to run\n");
    int fpid = fork();
    if (fpid < 0)
    {
        struct ArgsDumpAndExit argsDumpAndExit;
        ArgsDumpAndExitInit(&argsDumpAndExit);
        argsDumpAndExit.systemStatus = EXIT_SYSTEM_ERROR;
        argsDumpAndExit.reason = "in runProgress fork error";
        clog_error(CLOG(UNIQ_LOG_ID), "in runProgress fork error");
        FSMEventHandler(&fsm, CondProgramNeedToExit, &argsDumpAndExit);
        return;
    }
    else if (fpid == 0)
    {
        FSMEventHandler(&fsm, CondRunnerIsChild, NULL);
    }
    else
    {
        childProgressInfo.child_pid = fpid;
        FSMEventHandler(&fsm, CondRunnerIsPar, NULL);
    }
}

//子进程初始化函数
void ChildInit(const void *params)
{
    //设置资源限制
    //setProgressLimit(RLIMIT_CPU, (resouceConfig.time + 2000) / 1000);
    //setProgressLimit(RLIMIT_AS, resouceConfig.memory + 10240);
    //setProgressLimit(RLIMIT_FSIZE, resouceConfig.disk + 10240);
    //setProgressLimit(RLIMIT_CORE,0);

    //重定向IO
    char infileName[100];
    char outfileName[100];
    sprintf(infileName, "%s/%s", fileInfo.path, fileInfo.inputFileName);
    sprintf(outfileName, "%s/%s", fileInfo.path, fileInfo.outputFileName);
    int read_fd = open(infileName, O_RDONLY);
    int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; //创建的文件权限
    int openFlags = O_WRONLY | O_CREAT | O_TRUNC;
    int write_fd = open(outfileName, openFlags, filePerms);
    dup2(read_fd, STDIN_FILENO);
    dup2(write_fd, STDOUT_FILENO);
    // //安装system_call filter
    char cmd[100];
    sprintf(cmd, "%s/%s", fileInfo.path, fileInfo.exeFileName);
    install_seccomp_filter();

    // 修改uid和gid
    //setgid(65534);
    //setuid(65534);

    FSMEventHandler(&fsm, CondRunnerAfterInit, NULL);
}

//子进程运行函数
void ChildRun(const void *params)
{
    char cmd[100];
    sprintf(cmd, "%s/%s", fileInfo.path, fileInfo.exeFileName);
    //执行命令
    char *argv[] = {cmd, NULL};
    clog_info(CLOG(UNIQ_LOG_ID), "the child run cmd:%s", cmd);
    int ret = execvp(cmd, argv);
    if (ret == -1)
    {
        clog_error(CLOG(UNIQ_LOG_ID), "execvp error");
    }
}

//父进程监听函数
void ParMonitor(const void *params)
{
    int fpid = childProgressInfo.child_pid;
    signal(SIGALRM, time_limit_kill); //注册超时杀死进程事件
    int runKillTime = (resouceConfig.time + 1000) / 1000;
    alarm(runKillTime);
    int childStatus = -1;
    struct rusage rusage;
    //获取子进程资源消耗情况
    long maxMemUse = 0;
    while (1)
    {
        int retCode = wait4(fpid, &childStatus, WUNTRACED, &rusage);
        if (rusage.ru_minflt != 0)
        {
            printf("minflt:%ld\n", rusage.ru_minflt);
        }
        long nowMemUse = (long)rusage.ru_minflt * (sysconf(_SC_PAGE_SIZE) / KB_TO_BYTES); //缺页中断数量
        if (nowMemUse > maxMemUse)
        {
            maxMemUse = nowMemUse;
            if (maxMemUse > resouceConfig.memory)
            {
                memory_limit_kill();
            }
        }

        if (retCode != 0) //子进程状态发生了变化
        {
            clog_info(CLOG(UNIQ_LOG_ID), "child pid is %d,retCode is %d,status is %d", fpid, retCode, childStatus);
            break;
        }
    }
    struct ArgsParAfterRun args;
    args.childExitStatus = childStatus;
    args.rusage = rusage;
    args.maxMemUsage = maxMemUse;
    FSMEventHandler(&fsm, CondRunnerChildExit, &args);
}

//运行结束函数OnParAfterRun
void ParAfterRun(const void *params)
{
    assert(params != NULL);
    struct ArgsParAfterRun *args = (struct ArgsParAfterRun *)params;
    printf("user memory usage:%ld KB\n", args->maxMemUsage);
    clog_info(CLOG(UNIQ_LOG_ID), "user memory usage:%ld KB", args->maxMemUsage);

    struct timeval user, system;
    user = args->rusage.ru_utime;
    system = args->rusage.ru_stime;
    long usedTime = (user.tv_sec + system.tv_sec) * 1000 + (user.tv_usec + system.tv_usec) / 1000;
    printf("user time:%ld ms\n", user.tv_sec * 1000 + user.tv_usec / 1000);
    printf("system time:%ld ms \n", system.tv_sec * 1000 + system.tv_usec / 1000);
    printf("total UsedTime:%ld ms\n", usedTime);
    clog_info(CLOG(UNIQ_LOG_ID), "user time usage:%ld ms", usedTime);

    struct ArgsDumpAndExit argsDumpAndExit;
    ArgsDumpAndExitInit(&argsDumpAndExit);
    argsDumpAndExit.timeUsage = usedTime;
    argsDumpAndExit.memoryUsage = args->maxMemUsage;
    argsDumpAndExit.systemStatus = EXIT_SYSTEM_SUCCESS;
    if (args->maxMemUsage > resouceConfig.memory) //MLE
    {
        argsDumpAndExit.judgeStatus = EXIT_JUDGE_MLE;
        argsDumpAndExit.resultString = "MLE";
        FSMEventHandler(&fsm, OnProgramEnd, &argsDumpAndExit);
        return;
    }
    else if (usedTime > resouceConfig.time) //TLE
    {
        argsDumpAndExit.judgeStatus = EXIT_JUDGE_TLE;
        argsDumpAndExit.resultString = "TLE";
        FSMEventHandler(&fsm, OnProgramEnd, &argsDumpAndExit);
        return;
    }
    else if (args->childExitStatus == 0) //AC,go to taker
    {
        argsDumpAndExit.judgeStatus = EXIT_JUDGE_AC;
        argsDumpAndExit.resultString = "AC";
        FSMEventHandler(&fsm, OnProgramEnd, &argsDumpAndExit);
        return;
    }
    else //RE
    {
        argsDumpAndExit.judgeStatus = EXIT_JUDGE_RE;
        argsDumpAndExit.resultString = "RE";
        FSMEventHandler(&fsm, OnProgramEnd, &argsDumpAndExit);
        return;
    }
}
