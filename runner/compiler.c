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
//是否需要编译
int needCompile(const char *s)
{
    return configNode.needCompile;
}

//编译程序
static int compilePid;
static void timeout_kill(int sigId){
    if(compilePid>0){
        kill(compilePid,SIGKILL);
    }
    alarm(0);
}

void Compile(const void *params)
{
    //如果不需要编译
    extern struct ResourceConfig resouceConfig;
    compilePid = fork();
    if (compilePid < 0)
    {
        clog_error(CLOG(UNIQ_LOG_ID), "fork error");
    }
    else if (compilePid == 0) //子进程
    {
		//setrlimit,限制磁盘大小
        struct rlimit limit;
        rimit.rlim_cur=limit.rlim_max=64*MB_TO_BYTES;
        setrlimit(RLIMIT_FSIZE,&limit);	

        char *tmp = ReplaceFlag(configNode.compileArgs, "$SRC", fileInfo.sourceFileName);
        char *cmd = ReplaceFlag(tmp, "$EXE", fileInfo.exeFileName);
        clog_info(CLOG(UNIQ_LOG_ID), "the compile cmd is %s", cmd);
        char *argv[] = {"/bin/bash", "-c", cmd, NULL};
        int ret = execvp("/bin/bash", argv);
        if (ret == -1)
        {
            extern int errno;
            clog_error(CLOG(UNIQ_LOG_ID), "execvp error,errno:%d,errnoinfo:%s", errno, strerror(errno));
            exit(1); //这里会导致结果RE而不是system erro
        }
    }
    else
    {
        signal(SIGALRM,timeout_kill);//设置编译超时时间
        alarm(5);
        int status;
        int retCode = wait(&status);
        clog_info(CLOG(UNIQ_LOG_ID), "the compile subprogress pid is %d,retcode is %d", retCode, status);
        if (status != 0) //如果编译进程有问题,这个地方需要进一步修改
        {
            struct ArgsDumpAndExit argsDumpAndExit;
            ArgsDumpAndExitInit(&argsDumpAndExit);
            argsDumpAndExit.judgeStatus = EXIT_JUDGE_CE;
            argsDumpAndExit.resultString = "CE";
            FSMEventHandler(&fsm, CondProgramNeedToExit, &argsDumpAndExit);
            return;
        }
        FSMEventHandler(&fsm, CondCompileFinish, NULL);
        return;
    }
}
