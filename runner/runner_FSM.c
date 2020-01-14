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


//ϵͳ��Դ��������
struct ResourceConfig
{
    int time;       //��λms,������CPUʱ��
    int memory;     //��λM
    int disk;       //��λM
    char *language; //����
} resouceConfig;

//�ӽ��������Ϣ
struct ChildProgresInfo
{
    int system_status; //ϵͳ״̬
    int judge_status;  //����������״̬
    int child_pid;     //�ӽ���pid
    int exit_code;     //�ӽ����˳�״̬��
} childProgress;


//���óɹ�����0,���򷵻�-1
int setProgressLimit(int resource, int val)
{
    struct rlimit limit;
    limit.rlim_cur = limit.rlim_max = val;
    int retCode = setrlimit(resource, &limit);
    return retCode;
}

//��ʱɱ������
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

//�Ƿ���Ҫ����
int needCompile(const char *s)
{
    return 1;
}

//�������
int compile(struct ResourceConfig *config)
{
    //�������Ҫ����
    if (!needCompile(config->language))
        return 0;
    /*
        ����Ĵ����ڶ���Ӧ��д�ɱ�����������õ���ʽ
        language,�Լ���Ӧ��ִ�д���
    */
    int compilePid = fork();
    if (compilePid < 0)
    {
        return -1;
    }
    else if (compilePid == 0) //�ӽ���
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

//״̬
enum RunnerState{
    OnStart=0,
    OnChildInit,
    OnParMonitor,
    OnChildRun,
    OnParAfterRun,
};

//�¼�
enum Event{
    CondIsPar=0,//�¼�:�Ǹ�����
    CondIsChild,//�¼�:���ӽ���
    CondAfterInit,//�¼�:�ӽ��̳�ʼ�����
    CondChildExit,//�¼�:�ӽ����˳�
};

void ChildInit();
void ParMonitor();
void ChildRun();
void ParAfterRun();
void Init();

//ȫ������:ת�Ʊ�
FSMEdge transferTable[]={
    {OnStart,CondIsChild,OnChildInit,ChildInit,},
    {OnStart,CondIsPar,OnParMonitor,ParMonitor,},
    {OnChildInit,CondAfterInit,OnChildRun,ChildRun,},
    {OnParMonitor,CondChildExit,OnParAfterRun,ParAfterRun,},
};

//״̬��
typedef struct FSM{
    int curState;
    FSMEdge*pFSMTable;
    int size;
}FSM;

//ע��״̬ת�Ʊ�
void FSMRegister(FSM*pFSM,FSMEdge*pTable){
    pFSM->pFSMTable=pTable;
    pFSM->curState=OnStart;
    pFSM->size=sizeof(transferTable)/sizeof(FSMEdge);
    printf("FSM size:%d\n",pFSM->size);
}

//״̬Ǩ��
void FSMTransfer(FSM*pFSM,int state){
    pFSM->curState=state;
}

//�¼�����
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
        if(func){//ע�����л�״̬��˳��
            func();
        }
    }else{
        printf("Not find such event\n");
    }
}


//ȫ�ֱ���:״̬��
FSM fsm;

//�ӽ��̳�ʼ������
void ChildInit(){
    //������Դ����
    // setProgressLimit(RLIMIT_CPU, (resouceConfig.time + 1000) / 1000);
    // setProgressLimit(RLIMIT_AS, resouceConfig.memory + 10240);
    // setProgressLimit(RLIMIT_FSIZE, resouceConfig.disk + 10240);
    // setProgressLimit(RLIMIT_CORE,0);

    //�ض���IO
    const char *path = "/home/naoh/Program/go/src/sandbox/output";
    char infileName[100];
    char outfileName[100];
    sprintf(infileName, "%s/in.txt", path);
    sprintf(outfileName, "%s/output.txt", path);
    int read_fd = open(infileName, O_RDONLY);
    int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; //�������ļ�Ȩ��
    int openFlags = O_WRONLY | O_CREAT | O_TRUNC;
    int write_fd = open(outfileName, openFlags, filePerms);
    dup2(read_fd, STDIN_FILENO);
    dup2(write_fd, STDOUT_FILENO);
    // //��װsystem_call filter
    char cmd[100];
    sprintf(cmd, "%s/a.out", path);
    struct InformationToFilter info;
    info.exeFileName = cmd;
    printf("before:cmd:%s\n",cmd);
    printf("info.exeFile:%s\n",info.exeFileName);
    install_seccomp_filter(&info);

    // �޸�uid��gid
    //setgid(65534);
    //setuid(65534);

    FSMEventHandler(&fsm,CondAfterInit);
}

//�ӽ������к���
void ChildRun(){
    const char *path = "/home/naoh/Program/go/src/sandbox/output";
    char cmd[100];
    sprintf(cmd, "%s/a.out", path);
    //ִ������
    char *argv[] = {cmd, NULL};
    printf("now:cmd:%s\n",cmd);
    int ret = execvp(cmd, argv);
    if (ret == -1)
        printf("execvp error");
}


//�����̼�������
void ParMonitor(){
    int fpid=childProgress.child_pid;
    signal(SIGALRM, time_out_kill); //ע�ᳬʱɱ�������¼�
    int runKillTime = (resouceConfig.time + 1000) / 1000;
    alarm(runKillTime);
    int childStatus = -1;
    struct rusage rusage;
    //��ȡ�ӽ�����Դ�������
    while (1)
    {
        int retCode = wait4(fpid, &childStatus, WNOHANG, &rusage);
        if (retCode != 0) //�ӽ���״̬�����˱仯
        {
            clog_info(CLOG(UNIQ_LOG_ID), "child pid is %d,retCode is %d,status is %d\n", fpid, retCode, childStatus);
            break;
        }
    }

    FSMEventHandler(&fsm,CondChildExit);
}


//���н�������OnParAfterRun
void ParAfterRun(){
    printf("now par after run\n");
}

//��ʼ����Դ���ú���
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


//��ʼ����
void Run(){
    //ע��״̬��
    FSMRegister(&fsm,transferTable);//ע��ת�Ʊ�
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
    //��ʼ����Դ
    InitResource();
    //�������г���
    clog_info(CLOG(UNIQ_LOG_ID), "before compile");
    compile(&resouceConfig);
    //���г���
    clog_info(CLOG(UNIQ_LOG_ID), "begin to run exe file");
    Run();
}