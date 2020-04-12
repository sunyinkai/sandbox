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
#include <errno.h>
#include <ctype.h>

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
static int child_pid;

//设置成功返回0,否则返回-1
static int setProgressLimit(int resource, int val)
{
    struct rlimit limit;
    limit.rlim_cur = limit.rlim_max = val;
    int retCode = setrlimit(resource, &limit);
    return retCode;
}

//超时杀死程序
static void time_limit_kill(int sigId)
{
    if (child_pid > 0)
    {
        int retCode = kill(child_pid, SIGKILL);
        if (retCode != 0)
        {
            struct ArgsDumpAndExit argsDumpAndExit;
            ArgsDumpAndExitInit(&argsDumpAndExit);
            argsDumpAndExit.systemStatus = EXIT_SYSTEM_ERROR;
            clog_error(CLOG(UNIQ_LOG_ID), "kill system call error,reason is %s\n", strerror(errno));
            FSMEventHandler(&fsm, CondProgramNeedToExit, &argsDumpAndExit);
            return;
        }
    }
    alarm(0);
}

//Runner起始函数
void *Run(const void *params)
{
    assert(params == NULL);
    int fpid = fork();
    if (fpid < 0)
    {
        struct ArgsDumpAndExit args;
        BuildSysErrorExitArgs(&args, " in runProgress fork error");
        clog_error(CLOG(UNIQ_LOG_ID), "in runProgress fork error");
        FSMEventHandler(&fsm, CondProgramNeedToExit, &args);
    }
    else if (fpid == 0)
    {
        FSMEventHandler(&fsm, CondRunnerIsChild, NULL);
    }
    else
    {
        child_pid = fpid;
    }
    return NULL;
}

//子进程初始化函数
void *ChildInit(const void *params)
{
    assert(params == NULL);
    //设置资源限制
    setProgressLimit(RLIMIT_CPU, (resouceConfig.time + 1000) / 1000);
    setProgressLimit(RLIMIT_AS, resouceConfig.memory + 10 * MB_TO_BYTES);
    //   setProgressLimit(RLIMIT_NPROC, 10);
    setProgressLimit(RLIMIT_FSIZE, resouceConfig.disk + 10240);
    //  setProgressLimit(RLIMIT_CORE,0);
    //   setProgressLimit(RLIMIT_NOFILE,20);

    //重定向IO
    int sizeInFileSize = strlen(fileInfo.path) + strlen(fileInfo.sysInputFileName) + 10;
    char *infileName = (char *)malloc(sizeInFileSize);
    int sizeOutFileSize = strlen(fileInfo.path) + strlen(fileInfo.usrOutputFileName) + 10;
    char *outfileName = (char *)malloc(sizeOutFileSize);
    snprintf(infileName, sizeInFileSize, "%s/%s", fileInfo.path, fileInfo.sysInputFileName);
    snprintf(outfileName, sizeOutFileSize, "%s/%s", fileInfo.path, fileInfo.usrOutputFileName);
    int read_fd = open(infileName, O_RDONLY);
    int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; //创建的文件权限
    int openFlags = O_WRONLY | O_CREAT | O_TRUNC;
    int write_fd = open(outfileName, openFlags, filePerms);
    //int erro_fd = open("/dev/null",O_WRONLY);
    dup2(read_fd, STDIN_FILENO);
    dup2(write_fd, STDOUT_FILENO);
    //dup2(erro_fd, STDERR_FILENO);
    free(infileName);
    free(outfileName);

    // 修改uid和gid
    setgid(65529);
    setuid(65529);
    clog_info(CLOG(UNIQ_LOG_ID), "uid:%d,gid:%d\n", getuid(), getgid());

    //安装system_call filter
    install_seccomp_filter();
    return NULL;
}

//子进程运行函数
void *ChildRun(const void *params)
{
    assert(params == NULL);
    //获取命令执行参数
    char *tmp = ReplaceFlag(configNode.runArgs, "$SRC", fileInfo.sourceFileName);
    char *argvStr = ReplaceFlag(tmp, "$EXE", fileInfo.exeFileName);
    //获取程序路径
    int len = strlen(argvStr);
    char *path = (char *)malloc(len);
    clog_info(CLOG(UNIQ_LOG_ID), "the argvLen:%d,configRunArgs:%s,str:%s", len, configNode.runArgs, argvStr);
    for (int i = 0; i <= len; ++i)
    {
        if (isspace(argvStr[i]))
        {
            path[i] = '\0';
            break;
        }
        path[i] = argvStr[i];
    }
    char *argv[] = {argvStr, NULL};
    clog_info(CLOG(UNIQ_LOG_ID), "the child run path:%s,argv:%s", path, argvStr);

    int ret = execve(path, argv, NULL);
    if (ret == -1)
    {
        extern int errno;
        clog_error(CLOG(UNIQ_LOG_ID), "execve error,errno:%d,errnoinfo:%s", errno, strerror(errno));
        exit(EXEC_ERROR_EXIT_CODE);
    }
    return NULL;
}

//父进程监听函数
void *ParMonitor(const void *params)
{
    assert(params == NULL);

    int fpid = child_pid;
    signal(SIGALRM, time_limit_kill); //注册超时杀死进程事件
    int runKillTime = (resouceConfig.time + 2000) / 1000;
    alarm(runKillTime);

    //获取子进程资源消耗情况
    int childStatus = -1;
    struct rusage rusage;
    int retCode = wait4(fpid, &childStatus, WUNTRACED, &rusage);
    long childMemUse = (long)rusage.ru_minflt * (sysconf(_SC_PAGE_SIZE) / KB_TO_BYTES); //缺页中断数量
    clog_info(CLOG(UNIQ_LOG_ID), "child pid is %d,retCode is %d,WIFEXITED %d,WEXITSTATUS %d,WIFSINALED:%d,WTERMSIG:%d",
              fpid, retCode, WIFEXITED(childStatus), WEXITSTATUS(childStatus),
              WIFSIGNALED(childStatus), WTERMSIG(childStatus));

    struct ArgsParAfterRun *args = (struct ArgsParAfterRun *)malloc(sizeof(struct ArgsParAfterRun));
    args->childExitStatus = childStatus;
    args->rusage = rusage;
    args->childMemUsage = childMemUse;
    return args;
}

//运行结束函数OnParAfterRun
void *ParAfterRun(const void *params)
{
    assert(params != NULL);
    struct ArgsParAfterRun *args = (struct ArgsParAfterRun *)params;
    clog_info(CLOG(UNIQ_LOG_ID), "user memory usage:%ld KB", args->childMemUsage);

    struct timeval user, system;
    user = args->rusage.ru_utime;
    system = args->rusage.ru_stime;
    long usedTime = (user.tv_sec + system.tv_sec) * 1000 + (user.tv_usec + system.tv_usec) / 1000;
    clog_info(CLOG(UNIQ_LOG_ID), "user time usage:%ld ms", usedTime);

    struct ArgsDumpAndExit argsDumpAndExit;
    ArgsDumpAndExitInit(&argsDumpAndExit);
    argsDumpAndExit.timeUsage = usedTime;
    argsDumpAndExit.memoryUsage = args->childMemUsage;
    argsDumpAndExit.systemStatus = EXIT_SYSTEM_SUCCESS;

    if (args->childMemUsage > resouceConfig.memory) //MLE
    {
        argsDumpAndExit.judgeStatus = EXIT_JUDGE_MLE;
        argsDumpAndExit.resultString = "MLE";
        FSMEventHandler(&fsm, CondProgramNeedToExit, &argsDumpAndExit);
    }
    else if (usedTime > resouceConfig.time) //TLE
    {
        argsDumpAndExit.judgeStatus = EXIT_JUDGE_TLE;
        argsDumpAndExit.resultString = "TLE";
        FSMEventHandler(&fsm, CondProgramNeedToExit, &argsDumpAndExit);
    }

    if (WIFEXITED(args->childExitStatus) != 0) //normal exit
    {
        int childExitStatus = WEXITSTATUS(args->childExitStatus);
        if (childExitStatus == EXEC_ERROR_EXIT_CODE) //exec fail
        {
            BuildSysErrorExitArgs(&argsDumpAndExit, "runner child exec failed");
            FSMEventHandler(&fsm, CondProgramNeedToExit, &args);
        }
        else if (childExitStatus == 0) // to be compare
        {
            argsDumpAndExit.judgeStatus = EXIT_JUDGE_WA;
            argsDumpAndExit.resultString = "WA";
            FSMEventHandler(&fsm, CondResultNeedCompare, &argsDumpAndExit);
        }
        else
        {
            argsDumpAndExit.judgeStatus = EXIT_JUDGE_RE;
            argsDumpAndExit.resultString = "RE";
            argsDumpAndExit.reason = "unexpected exit code";
            FSMEventHandler(&fsm, CondResultNeedCompare, &argsDumpAndExit);
        }
    }
    else //stop by sinal
    {
        argsDumpAndExit.judgeStatus = EXIT_JUDGE_RE;
        argsDumpAndExit.resultString = "RE";
        FSMEventHandler(&fsm, CondProgramNeedToExit, &argsDumpAndExit);
    }
}

void RunnerLogic(const void *params)
{
    struct FuncPointerNode *objFuncList=NULL;
    FuncPointer funcList[] = {Run, ParMonitor, ParAfterRun};
    for (int i = 0; i < sizeof(funcList) / sizeof(FuncPointer); i++)
        AddFuncToList(&objFuncList, funcList[i]);
    FuncListRun(objFuncList, params);
}

void ExecutorLogic(const void *params)
{
    struct FuncPointerNode *objFuncList=NULL;
    FuncPointer funcList[] = {ChildInit, ChildRun};
    for (int i = 0; i < sizeof(funcList) / sizeof(FuncPointer); i++)
        AddFuncToList(&objFuncList, funcList[i]);
    FuncListRun(objFuncList, params);
}