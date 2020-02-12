/*
program input:laguange sourceFile time memory disk inputFile outputFile
program output: a json file,format:
sourceName.json
{
    timeUsage: .long
    memoryUsage: .long
    systemStatus: .int
    judgeStatus:  .int
    resultString: .string 
}
*/


#include <assert.h>
#include "runner.h"
#include "compiler.h"
#include "fsm.h"
#include "resource.h"
#include "contants.h"
#define CLOG_MAIN
#include "include/clog.h"

const int UNIQ_LOG_ID = 1;
void init_log(int uinqId, enum clog_level level)
{
    int r;
    r = clog_init_path(uinqId, "log.txt");
    if (r != 0)
    {
        exit(1);
    }
    clog_set_level(uinqId, level);
}

void ProgramStart()
{
    if (needCompile(resouceConfig.language))
    {
        FSMEventHandler(&fsm, CondNeedCompile, NULL);
    }
    else
    {
        FSMEventHandler(&fsm, CondNoNeedCompile, NULL);
    }
}

//��ʼ����Դ���ú���
// usage: ./runner_FSM.out laguange sourceFile time memory disk inputFile outputFile
void InitResource(int argc, char *args[])
{

    init_log(UNIQ_LOG_ID, CLOG_INFO); //��ʼ����־��Ϣ

    //ע��״̬��
    FSMRegister(&fsm, transferTable); //ע��ת�Ʊ�
    fsm.curState = OnProgramStart;    //�ڳ�����ʼλ��

//��args��ȡ����
#ifndef DEBUG
    assert(argc == 8);
    char *language = args[1];
    char *sourceFile = args[2];
    int runTime = atoi(args[3]);
    long runMemory = atol(args[4]);
    long runDisk = atol(args[5]);
    char *inputFile = args[6];
    char *outputFile = args[7];
    clog_info(CLOG(UNIQ_LOG_ID), "the args is %s,%s,%s,%ld,%ld,%ld,%s,%s", args[0], language,
              sourceFile, runTime, runMemory, runDisk, inputFile, outputFile);

    resouceConfig.time = runTime;
    resouceConfig.memory = runMemory;
    resouceConfig.disk = runDisk;
    resouceConfig.language = language;

    childProgress.child_pid = -1;
    childProgress.judge_status = EXIT_JUDGE_AC;
    childProgress.system_status = EXIT_SYSTEM_SUCCESS;
    childProgress.child_exit_code = 0;

    //fileInfo.path = "/home/naoh/Program/go/src/sandbox/output";
    fileInfo.path = "/";
    fileInfo.inputFileName = inputFile;
    fileInfo.outputFileName = outputFile;
    fileInfo.exeFileName = "abcxxxxx.out";
    fileInfo.sourceFileName = sourceFile;
#endif

#ifdef DEBUG
    resouceConfig.time = 1000;
    resouceConfig.memory = 65536;
    resouceConfig.disk = 65536;
    resouceConfig.language = "g++";

    childProgress.child_pid = -1;
    childProgress.judge_status = EXIT_JUDGE_AC;
    childProgress.system_status = EXIT_SYSTEM_SUCCESS;
    childProgress.child_exit_code = 0;

    fileInfo.path = "/home/naoh/Program/go/src/sandbox/output";
    //fileInfo.path = "/tmp/";
    fileInfo.inputFileName = "in.txt";
    fileInfo.outputFileName = "output.txt";
    fileInfo.exeFileName = "a.out";
    fileInfo.sourceFileName = "a.cpp";
#endif
}

// usage: ./runner_FSM.out sourceFile time memory disk inputFile outputFile language
int main(int argc, char *args[])
{
    //��ʼ����Դ
    InitResource(argc, args);
    //��������
    ProgramStart();
    return 0;
}
