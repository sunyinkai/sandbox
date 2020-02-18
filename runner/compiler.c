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
void Compile(const void *params)
{
    //�������Ҫ����
    extern struct ResourceConfig resouceConfig;
    /*
        ����Ĵ����ڶ���Ӧ��д�ɱ�����������õ���ʽ
        language,�Լ���Ӧ��ִ�д���
    */
    int compilePid = fork();
    if (compilePid < 0)
    {
        clog_error(CLOG(UNIQ_LOG_ID), "fork error\n");
    }
    else if (compilePid == 0) //�ӽ���
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
        if (status != 0) //����������������,����ط���Ҫ��һ���޸�
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