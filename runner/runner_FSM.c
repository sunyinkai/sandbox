#include "contants.h"
#include <stdio.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include "syscall_monitor.h"

#define CLOG_MAIN
#include "logger.h"
const int UNIQ_LOG_ID = 0;


//系统资源限制配置
struct ResourceConfig
{
    int time;       //单位ms,这里是CPU时间
    int memory;     //单位M
    int disk;       //单位M
    char *language; //语言
} resouceConfig;

//子进程相关信息
struct ChildProgresInfo
{
    int system_status; //系统状态
    int judge_status;  //测评机评测状态
    int child_pid;     //子进程pid
    int exit_code;     //子进程退出状态码
} childProgress;


//设置成功返回0,否则返回-1
int setProgressLimit(int resource, int val)
{
    struct rlimit limit;
    limit.rlim_cur = limit.rlim_max = val;
    int retCode = setrlimit(resource, &limit);
    return retCode;
}

//超时杀死程序
void time_out_kill(int sigId)
{
    if (childProgress.child_pid > 0)
    {
        int retCode = kill(childProgress.child_pid, SIGKILL);
        if (retCode == 0)
        {
            childProgress.judge_status = EXIT_JUDGE_TLE;
        }
    }
    alarm(0);
}

//是否需要编译
int needCompile(const char *s)
{
    return 1;
}

//编译程序
int compile(struct ResourceConfig *config)
{
    //如果不需要编译
    if (!needCompile(config->language))
        return 0;
    /*
        下面的代码在二期应该写成编译参数可配置的形式
        language,以及相应的执行代码
    */
    int compilePid = fork();
    if (compilePid < 0)
    {
        return -1;
    }
    else if (compilePid == 0) //子进程
    {
        char cmd[500];
        const char *path = "/home/naoh/Program/go/src/sandbox/output";
        sprintf(cmd, "g++ %s/a.cpp -lpthread -o %s/a.out", path, path);
        char *argv[] = {"/bin/bash", "-c", cmd, NULL};
        execvp("/bin/bash", argv);
    }
    else
    {
        int status;
        int retCode = wait(&status);
        clog_info(CLOG(UNIQ_LOG_ID), "the compile subprogress pid is %d,retcode is %d\n", retCode, status);
    }
    return 0;
}

//
typedef struct FSMEdge{
    int curState;
    int event;
    int nextState;
    void (*fun)();
}FSMEdge;

//状态
enum RunnerState{
    OnStart=0,
    OnChildInit,
    OnParMonitor,
    OnChildRun,
    OnParAfterRun,
};

//事件
enum Event{
    CondIsPar=0,//事件:是父进程
    CondIsChild,//事件:是子进程
    CondAfterInit,//事件:子进程初始化完毕
    CondChildExit,//事件:子进程退出
};

void ChildInit();
void ParMonitor();
void ChildRun();
void ParAfterRun();
void Init();

//全部变量:转移表
FSMEdge transferTable[]={
    {OnStart,CondIsChild,OnChildInit,ChildInit,},
    {OnStart,CondIsPar,OnParMonitor,ParMonitor,},
    {OnChildInit,CondAfterInit,OnChildRun,ChildRun,},
    {OnParMonitor,CondChildExit,OnParAfterRun,ParAfterRun,},
};

//状态机
typedef struct FSM{
    int curState;
    FSMEdge*pFSMTable;
    int size;
}FSM;

//注册状态转移表
void FSMRegister(FSM*pFSM,FSMEdge*pTable){
    pFSM->pFSMTable=pTable;
    pFSM->curState=OnStart;
    pFSM->size=sizeof(transferTable)/sizeof(FSMEdge);
    printf("FSM size:%d\n",pFSM->size);
}

//状态迁移
void FSMTransfer(FSM*pFSM,int state){
    pFSM->curState=state;
}

//事件处理
void FSMEventHandler(FSM*pFSM,int event){
    FSMEdge*pActTable=pFSM->pFSMTable;
    int isFind=0;
    void(*func)()=NULL;
    int nextState=-1;
    for(int i=0;i<pFSM->size;++i){
        if(event==pActTable[i].event&&pFSM->curState==pActTable[i].curState){
            isFind=1;
            func=pActTable[i].fun;
            nextState=pActTable[i].nextState;
            break;
        }
    }
    if(isFind){
        FSMTransfer(pFSM,nextState);
        if(func){//注意与切换状态的顺序
            func();
        }
    }else{
        printf("Not find such event\n");
    }
}


//全局变量:状态机
FSM fsm;

//子进程初始化函数
void ChildInit(){
    //设置资源限制
    // setProgressLimit(RLIMIT_CPU, (resouceConfig.time + 1000) / 1000);
    // setProgressLimit(RLIMIT_AS, resouceConfig.memory + 10240);
    // setProgressLimit(RLIMIT_FSIZE, resouceConfig.disk + 10240);
    // setProgressLimit(RLIMIT_CORE,0);

    //重定向IO
    const char *path = "/home/naoh/Program/go/src/sandbox/output";
    char infileName[100];
    char outfileName[100];
    sprintf(infileName, "%s/in.txt", path);
    sprintf(outfileName, "%s/output.txt", path);
    int read_fd = open(infileName, O_RDONLY);
    int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; //创建的文件权限
    int openFlags = O_WRONLY | O_CREAT | O_TRUNC;
    int write_fd = open(outfileName, openFlags, filePerms);
    dup2(read_fd, STDIN_FILENO);
    dup2(write_fd, STDOUT_FILENO);
    // //安装system_call filter
    char cmd[100];
    sprintf(cmd, "%s/a.out", path);
    struct InformationToFilter info;
    info.exeFileName = cmd;
    printf("before:cmd:%s\n",cmd);
    printf("info.exeFile:%s\n",info.exeFileName);
    install_seccomp_filter(&info);

    // 修改uid和gid
    //setgid(65534);
    //setuid(65534);

    FSMEventHandler(&fsm,CondAfterInit);
}

//子进程运行函数
void ChildRun(){
    const char *path = "/home/naoh/Program/go/src/sandbox/output";
    char cmd[100];
    sprintf(cmd, "%s/a.out", path);
    //执行命令
    char *argv[] = {cmd, NULL};
    printf("now:cmd:%s\n",cmd);
    int ret = execvp(cmd, argv);
    if (ret == -1)
        printf("execvp error");
}


//父进程监听函数
void ParMonitor(){
    int fpid=childProgress.child_pid;
    signal(SIGALRM, time_out_kill); //注册超时杀死进程事件
    int runKillTime = (resouceConfig.time + 1000) / 1000;
    alarm(runKillTime);
    int childStatus = -1;
    struct rusage rusage;
    //获取子进程资源消耗情况
    while (1)
    {
        int retCode = wait4(fpid, &childStatus, WNOHANG, &rusage);
        if (retCode != 0) //子进程状态发生了变化
        {
            clog_info(CLOG(UNIQ_LOG_ID), "child pid is %d,retCode is %d,status is %d\n", fpid, retCode, childStatus);
            break;
        }
    }

    FSMEventHandler(&fsm,CondChildExit);
}


//运行结束函数OnParAfterRun
void ParAfterRun(){
    printf("now par after run\n");
}

//初始化资源配置函数
void InitResource(){
    init_log(UNIQ_LOG_ID, CLOG_INFO);

    resouceConfig.disk = 10 * MB_TO_BYTES;
    resouceConfig.memory = 1 * MB_TO_BYTES;
    resouceConfig.time = 1000;
    resouceConfig.language = "g++";

    childProgress.child_pid = -1;
    childProgress.judge_status = EXIT_JUDGE_AC;
    childProgress.system_status = EXIT_SYSTEM_SUCCESS;
    childProgress.exit_code = 0;
}


//起始函数
void Run(){
    //注册状态机
    FSMRegister(&fsm,transferTable);//注册转移表
    fsm.curState=OnStart;

    int fpid = fork();
    if (fpid < 0){
        childProgress.system_status = EXIT_SYSTEM_ERROR;
        clog_error(CLOG(UNIQ_LOG_ID), "in runProgress fork error");
        printf("error\n");

    }else if(fpid==0){
        FSMEventHandler(&fsm,CondIsChild);
    }else{
        childProgress.child_pid = fpid;
        FSMEventHandler(&fsm,CondIsPar);
    }
}

int main(){
    //初始化资源
    InitResource();
    //编译运行程序
    clog_info(CLOG(UNIQ_LOG_ID), "before compile");
    compile(&resouceConfig);
    //运行程序
    clog_info(CLOG(UNIQ_LOG_ID), "begin to run exe file");
    Run();
}