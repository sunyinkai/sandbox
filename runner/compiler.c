#include <assert.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include "fsm.h"
#include "resource.h"
#include "compiler.h"
#include "checker.h"
#include "include/clog.h"
#include "contants.h"

extern int UNIQ_LOG_ID;
//�Ƿ���Ҫ����
int needCompile(const char *s)
{
    return configNode.needCompile;
}

//�������
static int compilePid;
static void timeout_kill(int sigId)
{
    if (compilePid > 0)
    {
        kill(compilePid, SIGKILL);
    }
    alarm(0);
}

void Compile(const void *params)
{
    //�������Ҫ����
    extern struct ResourceConfig resouceConfig;
    compilePid = fork();
    if (compilePid < 0)
    {
        clog_error(CLOG(UNIQ_LOG_ID), "fork error");
    }
    else if (compilePid == 0) //�ӽ���
    {
        //setrlimit,���ƴ��̴�С
        struct rlimit limit;
        limit.rlim_cur = limit.rlim_max = COMPILE_RLIMIT_FSIZE;
        setrlimit(RLIMIT_FSIZE, &limit);

        char *tmp = ReplaceFlag(configNode.compileArgs, "$SRC", fileInfo.sourceFileName);
        char *cmd = ReplaceFlag(tmp, "$EXE", fileInfo.exeFileName);
        clog_info(CLOG(UNIQ_LOG_ID), "the compile cmd is %s", cmd);
        char *argv[] = {"/bin/bash", "-c", cmd, NULL};
        int ret = execvp("/bin/bash", argv);
        printf("compile ret:%d\n", ret);
        if (ret == -1)
        {
            extern int errno;
            clog_error(CLOG(UNIQ_LOG_ID), "execvp error,errno:%d,errnoinfo:%s", errno, strerror(errno));
            exit(EXEC_ERROR_EXIT_CODE);
        }
    }
    else //������
    {
        signal(SIGALRM, timeout_kill); //���ñ��볬ʱʱ��
        alarm(COMPILE_TIME_OUT);
        int status;
        int retCode = wait(&status);
        clog_info(CLOG(UNIQ_LOG_ID), "the compile subprogress pid is %d,retcode is %d", retCode, status);
        if (WIFEXITED(status) != 0)
        {
            if (WEXITSTATUS(status) == EXEC_ERROR_EXIT_CODE)
            {
                struct ArgsDumpAndExit args;
                BuildSysErrorExitArgs(&args, "compile progress execve error");
                FSMEventHandler(&fsm, CondProgramNeedToExit, &args);
            }
            else if (WEXITSTATUS(status) != 0)
            {
                struct ArgsDumpAndExit args;
                ArgsDumpAndExitInit(&args);
                args.judgeStatus = EXIT_JUDGE_CE;
                args.resultString = "CE";
                FSMEventHandler(&fsm, CondProgramNeedToExit, &args);
            }
            else
            {
                FSMEventHandler(&fsm, CondCompileFinish, NULL);
            }
        }
        else
        {
            struct ArgsDumpAndExit args;
            BuildSysErrorExitArgs(&args, "compile progress exit abnormal");
            FSMEventHandler(&fsm, CondProgramNeedToExit, &args);
        }
    }
}
