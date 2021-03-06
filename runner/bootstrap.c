/*
program input:laguange sourceFile time memory disk 
              exeFile sysInputFile sysOutputFile usrOutputFile
program output: a json file,format:
result.json
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

void *ProgramStart(const void *param)
{
    assert(param==NULL);
    if (needCompile(resouceConfig.language))
    {
        FSMEventHandler(&fsm, CondNeedCompile, NULL);
    }
    else
    {
        FSMEventHandler(&fsm, CondNoNeedCompile, NULL);
    }
    return NULL;
}

//初始化资源配置函数
// usage: ./runner_FSM.out laguange sourceFile time memory disk
//                   exeFile sysInputFile sysOutputFile usrOutputFile resultJsonFile
void InitResource(int argc, char *args[])
{

    init_log(UNIQ_LOG_ID, CLOG_INFO); //初始化日志信息

    FSMRegister(&fsm, transferTable); //注册转移表
    fsm.curState = OnBootstrap;    //初始化当前状态

//从args获取变量
#ifndef DEBUG
    if (argc != 11 && argc != 12)
    {
        printf("usage: ./runner.out language sourceFile time memory disk exeFile \
 sysInputFile sysOutputFile usroutputFile resultJsonFile [specialJudgeExe]\n");
        printf("time: ms  , memory: KB , disk: KB\n");
        exit(1);
    }
    char *language = args[1];
    char *sourceFile = args[2];
    int runTime = atoi(args[3]);
    long runMemory = atol(args[4]);
    long runDisk = atol(args[5]);
    char *exeFile = args[6];
    char *sysInputFile = args[7];
    char *sysOutputFile = args[8];
    char *usrOutputFile = args[9];
    char *resultJsonFile = args[10];
    char *specialJudgeExe = NULL;
    if (argc == 12)
    {
        specialJudgeExe = args[11];
    }

    clog_info(CLOG(UNIQ_LOG_ID), "the args is %s,%s,%s,%ld,%ld,%ld,%s,%s,%s,%s,%s,[%s]",
              args[0], language,
              sourceFile, runTime, runMemory, runDisk, exeFile,
              sysInputFile, sysOutputFile, usrOutputFile, resultJsonFile, specialJudgeExe);
    resouceConfig.time = runTime;
    resouceConfig.memory = runMemory * KB_TO_BYTES;
    resouceConfig.disk = runDisk * KB_TO_BYTES;
    resouceConfig.language = language;

    fileInfo.path = "/";
    fileInfo.sysInputFileName = sysInputFile;
    fileInfo.sysOutputFileName = sysOutputFile;
    fileInfo.usrOutputFileName = usrOutputFile;
    fileInfo.exeFileName = exeFile;
    fileInfo.sourceFileName = sourceFile;
    fileInfo.resultJsonFileName = resultJsonFile;
    fileInfo.specialJudgeExe = specialJudgeExe;

    LoadConfig(&configNode, resouceConfig.language);
    clog_info(CLOG(UNIQ_LOG_ID), "the language:%s,needCompile:%d,compileArgs:%s,runArgs:%s",
              configNode.language, configNode.needCompile, configNode.compileArgs, configNode.runArgs);
#endif

#ifdef DEBUG
    resouceConfig.time = 2000;
    resouceConfig.memory = 65536 * KB_TO_BYTES;
    resouceConfig.disk = 65536 * KB_TO_BYTES;
    resouceConfig.language = "g++";
    fileInfo.path = "/";
    /*
    fileInfo.sysInputFileName = "/home/naoh/Program/go/src/sandbox/output/GO/go_in.txt";
    fileInfo.usrOutputFileName = "/home/naoh/Program/go/src/sandbox/output/GO/usr_output.txt";
    fileInfo.exeFileName = "/home/naoh/Program/go/src/sandbox/output/GO/a_go";
    fileInfo.sourceFileName = "/home/naoh/Program/go/src/sandbox/output/GO/a.go";
    fileInfo.sysOutputFileName = "/home/naoh/Program/go/src/sandbox/output/GO/go_output.txt";
    fileInfo.resultJsonFileName = "/home/naoh/Program/go/src/sandbox/output/GO/result_go_1.json";
    */

    fileInfo.sysInputFileName = "/home/naoh/Program/go/src/sandbox/output/CPP/cpp_in.txt";
    fileInfo.usrOutputFileName = "/home/naoh/Program/go/src/sandbox/output/CPP/usr_output.txt";
    fileInfo.exeFileName = "/home/naoh/Program/go/src/sandbox/output/CPP/a_cpp";
    fileInfo.sourceFileName = "/home/naoh/Program/go/src/sandbox/output/CPP/main.cpp";
    fileInfo.sysOutputFileName = "/home/naoh/Program/go/src/sandbox/output/CPP/cpp_output.txt";
    fileInfo.resultJsonFileName = "/home/naoh/Program/go/src/sandbox/output/CPP/result_cpp_1.json";
    fileInfo.specialJudgeExe = "/home/naoh/Program/go/src/sandbox/output/CPP/spj";

    LoadConfig(&configNode, resouceConfig.language);
    //printf("%s %d %s %s\n", configNode.language, configNode.needCompile,
     //      configNode.compileArgs, configNode.runArgs);
#endif
}

void BootStrapLogic(const void *params)
{
    struct FuncPointerNode *objFuncList=NULL;
    FuncPointer funcList[] = {ProgramStart};
    for (int i = 0; i < sizeof(funcList) / sizeof(FuncPointer); i++)
        AddFuncToList(&objFuncList, funcList[i]);
    FuncListRun(objFuncList,params);
    DestroyFuncList(objFuncList);
}

int main(int argc, char *args[])
{
    InitResource(argc, args);
    //启动程序
    BootStrapLogic(NULL);
    return 0;
}
