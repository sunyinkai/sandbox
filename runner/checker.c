#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include "fsm.h"
#include "resource.h"
#include "checker.h"
#include "contants.h"
#include "include/cJSON.h"
#include "include/clog.h"
#include "contants.h"

#define SAME 0
#define DIFF 1
#define OPEN_FILE_ERR 2

#define MAX_ONE_LINE_SIZE 10000000
#define MAX_FILE_NAME_SIZE 107
#define FALSE 0
#define TRUE 1
extern int UNIQ_LOG_ID;
extern int errno;
//忽略行末空格
static void stripSpace(char *s, int len)
{
    for (int i = len - 1; i != 0; --i)
    {
        if (isspace(s[i]))
            s[i] = '\0';
        else
            break;
    }
}

int compare(const char *source, const char *target)
{
    //打开待比对文件file_0,以及标准输出file_2
    FILE *fp0 = fopen(source, "r");
    FILE *fp2 = fopen(target, "r");
    if (fp0 == NULL || fp2 == NULL)
    {
        return OPEN_FILE_ERR;
    }
    char *line0 = (char *)malloc(sizeof(char) * MAX_ONE_LINE_SIZE);
    char *line2 = (char *)malloc(sizeof(char) * MAX_ONE_LINE_SIZE);
    char isSame = TRUE;
    while (!feof(fp2))
    {
        fgets(line0, MAX_ONE_LINE_SIZE, fp2); //fgets会读入换行
        if (feof(fp0))
        {
            isSame = FALSE;
            break;
        }
        fgets(line2, MAX_ONE_LINE_SIZE, fp0);
        int len0 = strlen(line0), len2 = strlen(line2);
        if (len0 == MAX_ONE_LINE_SIZE || len2 == MAX_ONE_LINE_SIZE)
        {
            struct ArgsDumpAndExit args;
            BuildSysErrorExitArgs(&args, "one line size too large");
            FSMEventHandler(&fsm, CondProgramNeedToExit, &args);
        }
        stripSpace(line0, len0); //去掉空格以及换行
        stripSpace(line2, len2);
        if (strcmp(line0, line2) != 0)
        {
            isSame = FALSE;
            break;
        }
    }
    if (!feof(fp2) || !feof(fp0))
        isSame = FALSE;
    free(line0);
    free(line2);
    fclose(fp0);
    fclose(fp2);
    return isSame ? SAME : DIFF;
}

//special judge
static int spjPid;
static void timeout_kill(int sigId)
{
    if (spjPid > 0)
    {
        kill(spjPid, SIGKILL);
    }
    alarm(0);
}

int SpecialJudger(const char *sysInput, const char *usrOutput)
{
    spjPid = fork();
    if (spjPid < 0)
    {
        return DIFF;
    }
    else if (spjPid == 0)
    {
        struct rlimit limit;
        limit.rlim_cur = limit.rlim_max = 0;
        setrlimit(RLIMIT_FSIZE, &limit); //禁止生成磁盘文件
        char *const path = fileInfo.specialJudgeExe;
        char *const argv[] = {path, fileInfo.sysOutputFileName, fileInfo.usrOutputFileName, NULL};
        execve(path, argv, NULL);
        exit(EXEC_ERROR_EXIT_CODE);
    }
    else
    {
        alarm(SPJ_TIME_OUT);
        signal(SIGALRM, timeout_kill);
        int status;
        wait(&status);
        if (WIFEXITED(status) != 0) //正常退出
        {
            if (WEXITSTATUS(status) == EXEC_ERROR_EXIT_CODE)
            {
                struct ArgsDumpAndExit args;
                BuildSysErrorExitArgs(&args, "spj execve fail");
                FSMEventHandler(&fsm, CondProgramNeedToExit, &args);
            }
            else if (WEXITSTATUS(status) == 0)
                return SAME;
            else
                return DIFF;
        }
        else
        {
            struct ArgsDumpAndExit args;
            BuildSysErrorExitArgs(&args, "spj pid exit abnormal");
            FSMEventHandler(&fsm, CondProgramNeedToExit, &args);
        }
    }
}

void* CheckerCompare(const void *params)
{
    assert(params != NULL);
    struct ArgsDumpAndExit *args = (struct ArgsDumpAndExit *)params;

    int isSpecial = 0;
    if (fileInfo.specialJudgeExe != NULL)
        isSpecial = 1;

    int retCode = DIFF;
    if (isSpecial) //special judge
    {
        retCode = SpecialJudger(fileInfo.sysInputFileName, fileInfo.usrOutputFileName);
    }
    else
    {
        retCode = compare(fileInfo.usrOutputFileName, fileInfo.sysOutputFileName);
    }

    //根据comparer的结构，修改judgeStatus和resultString
    if (retCode != SAME)
    {
        args->judgeStatus = EXIT_JUDGE_WA;
        args->resultString = "WA";
    }
    else
    {
        args->judgeStatus = EXIT_JUDGE_AC;
        args->resultString = "AC";
    }
    FSMEventHandler(&fsm, CondProgramNeedToExit, args);
    return NULL;
}

void BuildSysErrorExitArgs(struct ArgsDumpAndExit *args, const char *reason)
{
    ArgsDumpAndExitInit(args);
    args->systemStatus = EXIT_SYSTEM_ERROR;
    args->reason = reason;
}

void ArgsDumpAndExitInit(struct ArgsDumpAndExit *args)
{
    args->timeUsage = 0;
    args->memoryUsage = 0;
    args->judgeStatus = EXIT_JUDGE_AC;
    args->systemStatus = EXIT_SYSTEM_SUCCESS;
    args->reason = "";
    args->resultString = "";
}

void* DumpAndExit(const void *params)
{
    assert(params != NULL);
    struct ArgsDumpAndExit *args = (struct ArgsDumpAndExit *)params;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "timeUsage", args->timeUsage);
    cJSON_AddNumberToObject(root, "memoryUsage", args->memoryUsage);
    cJSON_AddNumberToObject(root, "systemStatus", args->systemStatus);
    cJSON_AddNumberToObject(root, "judgeStatus", args->judgeStatus);
    cJSON_AddStringToObject(root, "resultString", args->resultString);
    cJSON_AddStringToObject(root, "reason", args->reason);
    char *formatedJson = cJSON_Print(root);
    printf("%s\n", formatedJson);

    //dump to result.json
    FILE *fp = fopen(fileInfo.resultJsonFileName, "w");
    if (fp == NULL)
    {
        clog_error(CLOG(UNIQ_LOG_ID), "fopen error,reason is %s", strerror(errno));
        exit(0);
    }
    fprintf(fp, formatedJson);
    fclose(fp);

    cJSON_Delete(root);
    if (formatedJson)
    {
        free(formatedJson);
    }
    exit(0);
}

void CheckerLoigc(const void*params)
{
    struct FuncPointerNode *objFuncList;
    FuncPointer funcList[] = {CheckerCompare};
    for (int i = 0; i < sizeof(funcList) / sizeof(FuncPointer); i++)
        AddFuncToList(&objFuncList, funcList[i]);
    FuncListRun(objFuncList,params);
}

void ExitLogic(const void*params)
{
    struct FuncPointerNode *objFuncList=NULL;
    FuncPointer funcList[] = {DumpAndExit};
    for (int i = 0; i < sizeof(funcList) / sizeof(FuncPointer); i++)
        AddFuncToList(&objFuncList, funcList[i]);
    FuncListRun(objFuncList,params);
}