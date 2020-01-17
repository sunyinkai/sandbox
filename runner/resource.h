#ifndef SANDBOX_RESOURCE
#define SANDBOX_RESOURCE
//系统资源限制配置
struct ResourceConfig
{
    int time;       //单位ms,这里是CPU时间
    long memory;    //单位Bytes
    long disk;      //单位Bytes
    char *language; //语言
};

//子进程相关信息
struct ChildProgresInfo
{
    int system_status; //系统状态
    int judge_status;  //测评机评测状态
    int child_pid;     //子进程pid
    int exit_code;     //子进程退出状态码
    long time_cost;    //时间使用量
    long memory_use;   //内存使用量
};

//文件相关信息
struct FileInfo
{
    char *inputFileName;  //输入文件名称 input.txt
    char *outputFileName; //输出文件名称　output.txt
    char *exeFileName;    //待执行文件名称 a.out
    int __can_modify;
};

extern struct ResourceConfig resouceConfig;
extern struct ChildProgresInfo childProgress;

#endif