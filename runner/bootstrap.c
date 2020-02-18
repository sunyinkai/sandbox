/*
program input:laguange sourceFile time memory disk sysInputFile usrOutputFile
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
// usage: ./runner_FSM.out laguange sourceFile time memory disk
//                   exeFile sysInputFile usrOutputFile
void InitResource(int argc, char *args[])
{

    init_log(UNIQ_LOG_ID, CLOG_INFO); //初始化日志信息

    //注册状态机
    FSMRegister(&fsm, transferTable); //注册转移表
    fsm.curState = OnProgramStart;    //在程序起始位置

//从args获取变量
#ifndef DEBUG
    if (argc != 9)
    {
        printf("usage: ./runner.out language sourceFile time memory disk exeFile \
 sysInputFile usroutputFile\n");
        printf("time: ms  , memory: KB , disk: KB\n");
        assert(1);
    }
    char *language = args[1];
    char *sourceFile = args[2];
    int runTime = atoi(args[3]);
    long runMemory = atol(args[4]);
    long runDisk = atol(args[5]);
    char *exeFile = args[6];
    char *sysInputFile = args[7];
    char *usrOutputFile = args[8];
    clog_info(CLOG(UNIQ_LOG_ID), "the args is %s,%s,%s,%ld,%ld,%ld,%s,%s,%s", args[0], language,
              sourceFile, runTime, runMemory, runDisk, exeFile, sysInputFile, usrOutputFile);

    resouceConfig.time = runTime;
    resouceConfig.memory = runMemory;
    resouceConfig.disk = runDisk;
    resouceConfig.language = language;

    fileInfo.path = "/";
    fileInfo.sysInputFileName = sysInputFile;
    fileInfo.usrOutputFileName = usrOutputFile;
    fileInfo.exeFileName = exeFile;
    fileInfo.sourceFileName = sourceFile;

    LoadConfig(&configNode, resouceConfig.language);
#endif

#ifdef DEBUG
    resouceConfig.time = 1000;
    resouceConfig.memory = 65536;
    resouceConfig.disk = 65536;
    resouceConfig.language = "g++";

    fileInfo.path = "/";
    fileInfo.sysInputFileName = "/home/naoh/Program/go/src/sandbox/output/in.txt";
    fileInfo.usrOutputFileName = "/home/naoh/Program/go/src/sandbox/output/output.txt";
    fileInfo.exeFileName = "/home/naoh/Program/go/src/sandbox/output/a_runner_debug.out";
    fileInfo.sourceFileName = "/home/naoh/Program/go/src/sandbox/output/a.cpp";

    LoadConfig(&configNode, "g++");
    printf("%s %d %s %s\n", configNode.language, configNode.needCompile,
           configNode.compileArgs, configNode.runArgs);
#endif
}

int main(int argc, char *args[])
{
    InitResource(argc, args);
    //启动程序
    ProgramStart();
    return 0;
}
