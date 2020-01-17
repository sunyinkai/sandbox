#ifndef SANDBOX_FSM
#define SANDBOX_FSM

#include<stdio.h>
//如果满足event事件,从curState状态切换到nextState,然后执行func函数
struct FSMEdge
{
    int curState;
    int event;
    int nextState;
    void (*fun)(const void *);
};

//状态
enum RunnerState
{
    OnProgramStart=0,
    OnCompilerCompile,
    OnRunnerStart,
    OnRunnerChildInit,
    OnRunnerParMonitor,
    OnRunnerChildRun,
    OnRunnerParAfterRun,
};


//事件
enum Event
{
    CondNeedCompile=0, //事件:需要编译
    CondNoNeedCompile, //事件:不需要编译
    CondCompileFinish, //事件:编译完成
    CondRunnerIsPar , //事件:是父进程
    CondRunnerIsChild,   //事件:是子进程
    CondRunnerAfterInit, //事件:子进程初始化完毕
    CondRunnerChildExit, //事件:子进程退出
};

//函数

void ProgramRun(const void*);
void Compile(const void*);

void Run(const void*);
void ChildInit(const void *);
void ParMonitor(const void *);
void ChildRun(const void *);
void ParAfterRun(const void *);
void Init(const void *);

//全部变量:转移表
extern struct FSMEdge transferTable[7];//这个地方要修改

//全局变量:状态机
struct FSM
{
    int curState;
    struct FSMEdge *pFSMTable;
    int size;
};

extern struct FSM fsm;

//注册状态转移表
extern void FSMRegister(struct FSM *pFSM, struct FSMEdge *pTable);

//状态迁移
extern void FSMTransfer(struct FSM *pFSM, int state);

//事件处理
extern void FSMEventHandler(struct FSM *pFSM, int event, void *params);

#endif