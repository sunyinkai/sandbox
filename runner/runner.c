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
static struct ChildProgressInfo childProgressInfo; //?

//���óɹ�����0,���򷵻�-1
int setProgressLimit(int resource, int val)
{
    struct rlimit limit;
    limit.rlim_cur = limit.rlim_max = val;
    int retCode = setrlimit(resource, &limit);
    return retCode;
}

//��ʱɱ������
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

//errno չʾԭ��
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

//Runner��ʼ����
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

//�ӽ��̳�ʼ������
void ChildInit(const void *params)
{
    //������Դ����
    // setProgressLimit(RLIMIT_CPU, (resouceConfig.time + 2000) / 1000);
    // setProgressLimit(RLIMIT_AS, resouceConfig.memory + 10240);
    // setProgressLimit(RLIMIT_NPROC, 5);
    // setProgressLimit(RLIMIT_FSIZE, resouceConfig.disk + 10240);
    // setProgressLimit(RLIMIT_CORE,0);
    //setProgressLimit(RLIMIT_NOFILE,20);
    //�ض���IO
    char infileName[100];
    char outfileName[100];
    sprintf(infileName, "%s/%s", fileInfo.path, fileInfo.sysInputFileName);
    sprintf(outfileName, "%s/%s", fileInfo.path, fileInfo.usrOutputFileName);
    int read_fd = open(infileName, O_RDONLY);
    int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; //�������ļ�Ȩ��
    int openFlags = O_WRONLY | O_CREAT | O_TRUNC;
    int write_fd = open(outfileName, openFlags, filePerms);
    dup2(read_fd, STDIN_FILENO);
    dup2(write_fd, STDOUT_FILENO);
    // �޸�uid��gid
    setuid(65529);
    setgid(65529);

    //��װsystem_call filter
    install_seccomp_filter();
    FSMEventHandler(&fsm, CondRunnerAfterInit, NULL);
}

//�ӽ������к���
void ChildRun(const void *params)
{
    //��ȡ����ִ�в���
    char *tmp = ReplaceFlag(configNode.runArgs, "$SRC", fileInfo.sourceFileName);
    char *argvStr = ReplaceFlag(tmp, "$EXE", fileInfo.exeFileName);
    int len=strlen(argvStr);
    char path[100];
    for(int i=0;i<len;++i){
        if(isspace(argvStr[i])){
            path[i]='\0';
            break;
        }
        path[i]=argvStr[i];
    }
    char *argv[] = {argvStr, NULL};
    clog_info(CLOG(UNIQ_LOG_ID), "the child run path:%s,argv:%s", path,argvStr);

    int ret = execve(path, argv,NULL);
    if (ret == -1)
    {
        extern int errno;
        clog_error(CLOG(UNIQ_LOG_ID), "execvp error,errno:%d,errnoinfo:%s", errno, strerror(errno));
        exit(1); //����ᵼ�½��RE������system error
    }
}

//�����̼�������
void ParMonitor(const void *params)
{
    int fpid = childProgressInfo.child_pid;
    signal(SIGALRM, time_limit_kill); //ע�ᳬʱɱ�������¼�
    int runKillTime = (resouceConfig.time + 1000) / 1000;
    alarm(runKillTime);
    int childStatus = -1;
    struct rusage rusage;
    //��ȡ�ӽ�����Դ�������
    long maxMemUse = 0;
    while (1)
    {
        int retCode = wait4(fpid, &childStatus, WUNTRACED, &rusage);
        if (rusage.ru_minflt != 0)
        {
            printf("minflt:%ld\n", rusage.ru_minflt);
        }
        long nowMemUse = (long)rusage.ru_minflt * (sysconf(_SC_PAGE_SIZE) / KB_TO_BYTES); //ȱҳ�ж�����
        if (nowMemUse > maxMemUse)
        {
            maxMemUse = nowMemUse;
            if (maxMemUse > resouceConfig.memory)
            {
                memory_limit_kill();
            }
        }

        if (retCode != 0) //�ӽ���״̬�����˱仯
        {
            clog_info(CLOG(UNIQ_LOG_ID), "child pid is %d,retCode is %d,normal_exited %d,status is %d", fpid, retCode,WIFEXITED(childStatus),childStatus);
            break;
        }
    }
    struct ArgsParAfterRun args;
    args.childExitStatus = childStatus;
    args.rusage = rusage;
    args.maxMemUsage = maxMemUse;
    FSMEventHandler(&fsm, CondRunnerChildExit, &args);
}

//���н�������OnParAfterRun
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
        FSMEventHandler(&fsm, CondProgramNeedToExit, &argsDumpAndExit);
        return;
    }
    else if (usedTime > resouceConfig.time) //TLE
    {
        argsDumpAndExit.judgeStatus = EXIT_JUDGE_TLE;
        argsDumpAndExit.resultString = "TLE";
        FSMEventHandler(&fsm, CondProgramNeedToExit, &argsDumpAndExit);
        return;
    }
    else if (args->childExitStatus == 0) //��Ҫ�ȽϽ��
    {
        argsDumpAndExit.judgeStatus = EXIT_JUDGE_WA; //�ȳ�ʼ��ΪWA
        argsDumpAndExit.resultString = "WA";
        FSMEventHandler(&fsm, CondResultNeedCompare, &argsDumpAndExit);
        return;
    }
    else //RE
    {
        argsDumpAndExit.judgeStatus = EXIT_JUDGE_RE;
        argsDumpAndExit.resultString = "RE";
        FSMEventHandler(&fsm, CondProgramNeedToExit, &argsDumpAndExit);
        return;
    }
}
