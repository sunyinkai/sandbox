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
    reason: .string
}
*/

#include <assert.h>
#include "compiler.h"
#include "runner.h"
#include "checker.h"
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

//初始化资源配置函数
// usage: ./runner_FSM.out laguange sourceFile time memory disk inputFile outputFile
void InitResource(int argc, char *args[])
{

    init_log(UNIQ_LOG_ID, CLOG_INFO); //初始化日志信息

    //注册状态机
    FSMRegister(&fsm, transferTable); //注册转移表
    fsm.curState = OnProgramStart;    //在程序起始位置

//从args获取变量
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
    //初始化资源
    InitResource(argc, args);
    //启动程序
    ProgramStart();
    return 0;
}
