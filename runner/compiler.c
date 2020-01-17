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
extern int  UNIQ_LOG_ID;

//�Ƿ���Ҫ����
int needCompile(const char *s)
{
    return 1;
}

//�������
void Compile(const void*params)
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
        //
    }
    else if (compilePid == 0) //�ӽ���
    {
        char cmd[500];
        const char *path = "/home/naoh/Program/go/src/sandbox/output";
        sprintf(cmd, "g++ %s/a.cpp -lpthread -o %s/a.out", path, path);
        char *argv[] = {"/bin/bash", "-c", cmd, NULL};
        execvp("/bin/bash", argv);
    }
    else
    {
        int status;
        int retCode = wait(&status);
 //       clog_info(CLOG(UNIQ_LOG_ID), "the compile subprogress pid is %d,retcode is %d", retCode, status);
        printf("the compile subprogress pid is %d,retcode is %d\n", retCode, status);
        FSMEventHandler(&fsm,CondCompileFinish,NULL);
    }

    //
}