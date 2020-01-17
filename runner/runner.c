#include "contants.h"
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
#include "clog.h"

extern struct ResourceConfig resouceConfig;
extern struct ChildProgresInfo childProgress;
extern int UNIQ_LOG_ID;

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
    if (childProgress.child_pid > 0)
    {
        int retCode = kill(childProgress.child_pid, SIGKILL);
        if (retCode != 0)
        {
            childProgress.system_status = EXIT_SYSTEM_ERROR;
            clog_error(CLOG(UNIQ_LOG_ID), "kill system call error\n");
        }
    }
    alarm(0);
}

void memory_limit_kill()
{
    if (childProgress.child_pid > 0)
    {
        int retCode = kill(childProgress.child_pid, SIGKILL);
        if (retCode != 0)
        {
            childProgress.system_status = EXIT_SYSTEM_ERROR;
            clog_error(CLOG(UNIQ_LOG_ID), "kill system call error\n");
        }
    }
    alarm(0);
}

//�ӽ��̳�ʼ������
void ChildInit(const void *params)
{
    //������Դ����
    //setProgressLimit(RLIMIT_CPU, (resouceConfig.time + 2000) / 1000);
    //setProgressLimit(RLIMIT_AS, resouceConfig.memory + 10240);
    //setProgressLimit(RLIMIT_FSIZE, resouceConfig.disk + 10240);
    //setProgressLimit(RLIMIT_CORE,0);

    //�ض���IO
    const char *path = "/home/naoh/Program/go/src/sandbox/output";
    char infileName[100];
    char outfileName[100];
    sprintf(infileName, "%s/in.txt", path);
    sprintf(outfileName, "%s/output.txt", path);
    int read_fd = open(infileName, O_RDONLY);
    int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; //�������ļ�Ȩ��
    int openFlags = O_WRONLY | O_CREAT | O_TRUNC;
    int write_fd = open(outfileName, openFlags, filePerms);
    dup2(read_fd, STDIN_FILENO);
    dup2(write_fd, STDOUT_FILENO);
    // //��װsystem_call filter
    char cmd[100];
    sprintf(cmd, "%s/a.out", path);
    struct InformationToFilter info;
    info.exeFileName = cmd;
    printf("before:cmd:%s\n", cmd);
    printf("info.exeFile:%s\n", info.exeFileName);
    install_seccomp_filter(&info);

    // �޸�uid��gid
    //setgid(65534);
    //setuid(65534);

    FSMEventHandler(&fsm, CondRunnerAfterInit, NULL);
}

//�ӽ������к���
void ChildRun(const void *params)
{
    const char *path = "/home/naoh/Program/go/src/sandbox/output";
    char cmd[100];
    sprintf(cmd, "%s/a.out", path);
    //ִ������
    char *argv[] = {cmd, NULL};
    printf("now:cmd:%s\n", cmd);
    int ret = execvp(cmd, argv);
    if (ret == -1)
        printf("execvp error");
}

//�����̼�������
void ParMonitor(const void *params)
{
    int fpid = childProgress.child_pid;
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

//���н�������OnParAfterRun
void ParAfterRun(const void *params)
{
    assert(params != NULL);
    struct ArgsParAfterRun *args = (struct ArgsParAfterRun *)params;
    printf("user memory usage:%ld KB\n", args->maxMemUsage);
    if (args->maxMemUsage > resouceConfig.memory)
    {
        printf("MLE\n");
        childProgress.judge_status = EXIT_JUDGE_MLE;
        return;
    }

    struct timeval user, system;
    user = args->rusage.ru_utime;
    system = args->rusage.ru_stime;
    long long usedTime = (user.tv_sec + system.tv_sec) * 1000 + (user.tv_usec + system.tv_usec) / 1000;
    printf("user time:%ld ms\n", user.tv_sec * 1000 + user.tv_usec / 1000);
    printf("system time:%ld ms \n", system.tv_sec * 1000 + system.tv_usec / 1000);
    printf("total UsedTime:%lld ms\n", usedTime);
    //TLE
    if (usedTime > resouceConfig.time)
    {
        printf("TLE\n ");
        childProgress.judge_status = EXIT_JUDGE_TLE;
    }
    else if (args->childExitStatus == 0) //AC
    {
        printf("AC\n");
        childProgress.judge_status = EXIT_JUDGE_AC;
    }
    else //RE
    {
        printf("RE\n");
        childProgress.judge_status = EXIT_JUDGE_RE;
    }
}

//Runner��ʼ����
void Run(const void *params)
{
    printf("begin to run\n");
    int fpid = fork();
    if (fpid < 0)
    {
        childProgress.system_status = EXIT_SYSTEM_ERROR;
        clog_error(CLOG(UNIQ_LOG_ID), "in runProgress fork error");
        printf("error\n");
    }
    else if (fpid == 0)
    {
        FSMEventHandler(&fsm, CondRunnerIsChild, NULL);
    }
    else
    {
        childProgress.child_pid = fpid;
        FSMEventHandler(&fsm, CondRunnerIsPar, NULL);
    }
}
