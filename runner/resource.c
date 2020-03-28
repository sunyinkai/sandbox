#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "resource.h"
#include "include/cJSON.h"
struct ResourceConfig resouceConfig;
struct FileInfo fileInfo;
struct ConfigNode configNode;

static struct DataItem *GetSyscallItems(cJSON *syscallItems)
{
    struct DataItem *head = NULL;
    struct DataItem *now = head;
    int arraySize = cJSON_GetArraySize(syscallItems);
    cJSON *item;
    for (int i = 0; i < arraySize; ++i)
    {
        item = cJSON_GetArrayItem(syscallItems, i);
        assert(item != NULL);
        struct DataItem *newDataItem = (struct DataItem *)malloc(sizeof(struct DataItem *));
        newDataItem->data = item->valueint;
        newDataItem->next = NULL;
        if (now == NULL)
        {
            head = now = newDataItem;
        }
        else
        {
            now->next = newDataItem;
            now = now->next;
        }
    }
    return head;
}

int LoadConfig(struct ConfigNode *configNode, char *targetLanguage)
{
    FILE *fp = fopen("config/config.json", "r");
    if (fp == NULL)
    {
        return 0;
    }
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    char *data = (char *)malloc(sizeof(char) * (length + 1));
    fseek(fp, 0, SEEK_SET);
    long readSize = fread(data, 1, length, fp);
    assert(readSize == length);
    data[length] = '\0';
    cJSON *json = cJSON_Parse(data);
    assert(json != NULL);
    cJSON *jsList = cJSON_GetObjectItem(json, "language");
    assert(jsList != NULL);
    cJSON *item, *language, *needCompile, *compileArgs, *runArgs, *syscallMode, *syscallItems;
    int arraySize = cJSON_GetArraySize(jsList);

    for (int i = 0; i < arraySize; ++i)
    {
        item = cJSON_GetArrayItem(jsList, i); //获取数组里每个item
        assert(item != NULL);
        char *itemJson = cJSON_PrintUnformatted(item); //获得item字符串

        cJSON *it = cJSON_Parse(itemJson);
        assert(it != NULL);
        language = cJSON_GetObjectItem(it, "language");
        assert(language != NULL);
        needCompile = cJSON_GetObjectItem(it, "needCompile");
        assert(needCompile != NULL);
        compileArgs = cJSON_GetObjectItem(it, "compileArgs");
        assert(compileArgs != NULL);
        runArgs = cJSON_GetObjectItem(it, "runArgs");
        assert(runArgs != NULL);
        syscallMode = cJSON_GetObjectItem(it, "syscallMode");
        assert(syscallMode != NULL);
        syscallItems = cJSON_GetObjectItem(it, "syscallItems");
        if (strcmp(language->valuestring, targetLanguage) == 0)
        {
            configNode->language = language->valuestring;
            configNode->needCompile = needCompile->valueint;
            configNode->compileArgs = compileArgs->valuestring;
            configNode->runArgs = runArgs->valuestring;
            configNode->syscallMode = syscallMode->valuestring;
            configNode->syscallItems = GetSyscallItems(syscallItems);
            break;
        }
    }

    fclose(fp);
    free(data);
    return 1;
}

/*
将originStr中flag字符串替换为dst字符串
成功返回1,失败返回0
*/
char *ReplaceFlag(const char *originStr, char *flag, char *dst)
{
    int lenStr = strlen(originStr);
    int lenFlag = strlen(flag);
    int lenDst = strlen(dst);
    int mallocSize = 300;
    char *resultStr = (char *)malloc(sizeof(char) * mallocSize);
    int now = 0;
    for (int i = 0; i < lenStr;)
    {
        if (now + lenDst > mallocSize)
        { //重新分配内存
            mallocSize *= 2;
            void *newStr = realloc(resultStr, mallocSize);
            if (newStr == NULL)
            {
                free(resultStr);
                resultStr = NULL;
                return NULL;
            }
            resultStr = (char *)newStr;
        }

        int j = 0;
        for (; j < lenFlag && i + j < lenStr;)
        {
            if (flag[j] == originStr[j + i])
                ++j;
            else
                break;
        }

        if (j == lenFlag) //如果匹配成功
        {
            for (int k = 0; k < lenDst; ++k)
                resultStr[now++] = dst[k];
            i += lenFlag;
        }
        else //否则
        {
            resultStr[now++] = originStr[i];
            i++;
        }
    }
    resultStr[now] = '\0';
    return resultStr;
}
