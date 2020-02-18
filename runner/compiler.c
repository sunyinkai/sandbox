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
void Compile(const void *params)
{
    //如果不需要编译
    extern struct ResourceConfig resouceConfig;
    /*
        下面的代码在二期应该写成编译参数可配置的形式
        language,以及相应的执行代码
    */
    int compilePid = fork();
    if (compilePid < 0)
    {
        clog_error(CLOG(UNIQ_LOG_ID), "fork error\n");
    }
    else if (compilePid == 0) //子进程
    {
        char *tmp = ReplaceFlag(configNode.compileArgs, "$SRC", fileInfo.sourceFileName);
        char *cmd = ReplaceFlag(tmp, "$EXE", fileInfo.exeFileName);
        clog_info(CLOG(UNIQ_LOG_ID), "the compile cmd is %s\n", cmd);
        // need free tmp and cmd?
        char *argv[] = {"/bin/bash", "-c", cmd, NULL};
        execvp("/bin/bash", argv);
    }
    else
    {
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