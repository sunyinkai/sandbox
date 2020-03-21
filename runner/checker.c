#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
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
//忽略行末空格
extern int UNIQ_LOG_ID;
extern int errno;
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
    return isSame ? SAME : DIFF;
}

void CheckerCompare(const void *params)
{
    assert(params != NULL);
    int isSpecial = 0;
    struct ArgsDumpAndExit *args = (struct ArgsDumpAndExit *)params;

    int retCode = DIFF;
    if (isSpecial) //special judge
    {
        ;
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

void DumpAndExit(const void *params)
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
