#ifndef SANDBOX_RESOURCE
#define SANDBOX_RESOURCE
#include "include/list.h"
//系统资源限制配置
struct ResourceConfig
{
    int time;       //单位ms,这里是CPU时间
    long memory;    //单位Bytes
    long disk;      //单位Bytes
    char *language; //语言
};
extern struct ResourceConfig resouceConfig;

//文件相关信息
struct FileInfo
{
    char *sourceFileName;     //源文件名称
    char *path;               //文件所在目录
    char *sysInputFileName;   //输入文件名称 input.txt
    char *sysOutputFileName;  //问题答案文件
    char *usrOutputFileName;  //输出文件名称　output.txt
    char *exeFileName;        //待执行文件名称 a.out
    char *resultJsonFileName; //结果生成的json名字
    char *specialJudgeExe;    //特判程序
};
extern struct FileInfo fileInfo;

struct DataItem
{
    void *data;
    struct DataItem *next;
};

struct ConfigNode
{
    char *language;
    int needCompile;
    char *compileArgs;
    char *runArgs;
    char *syscallMode;             //模式
    struct DataItem *syscallItems; //系统调用list
};
extern struct ConfigNode configNode;
int LoadConfig(struct ConfigNode *, char *);
char *ReplaceFlag(const char *originStr, char *flag, char *dst);
#endif
