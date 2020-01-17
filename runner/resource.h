#ifndef SANDBOX_RESOURCE
#define SANDBOX_RESOURCE
//系统资源限制配置
struct ResourceConfig
{
    int time;       //单位ms,这里是CPU时间
    long memory;    //单位Bytes
    long disk;      //单位Bytes
    char *language; //语言
} ;

//子进程相关信息
struct ChildProgresInfo
{
    int system_status; //系统状态
    int judge_status;  //测评机评测状态
    int child_pid;     //子进程pid
    int exit_code;     //子进程退出状态码
} ;
extern struct ResourceConfig resouceConfig;
extern struct ChildProgresInfo childProgress;

#endif