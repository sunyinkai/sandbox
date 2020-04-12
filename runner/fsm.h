#ifndef SANDBOX_FSM
#define SANDBOX_FSM

#include <stdio.h>
//函数调用链代码
typedef void* (*FuncPointer)(const void*);
struct FuncPointerNode{
    FuncPointer func;
    struct FuncPointerNode* next;
};
int AddFuncToList(struct FuncPointerNode **fpn,FuncPointer func);
void FuncListRun(struct FuncPointerNode *fpn,const void*param);


struct FSMEdge
{
    int curState;
    int event;
    int nextState;
    void (*fun)(const void*);
};

//状态
enum RunnerState
{
    OnBootstrap,
    OnCompiler,
    OnRunner,
    OnExecutor,
    OnChecker,
    OnExit,
};

//事件
enum Event
{
    CondNeedCompile = 0,   //事件:需要编译
    CondNoNeedCompile,     //事件:不需要编译
    CondCompileFinish,     //事件:编译完成
    CondRunnerIsChild,     //事件:是子进程
    CondResultNeedCompare, //事件:计算结果需要比较
    CondProgramNeedToExit, //事件:进程需要退出
};

//函数
extern void BootStrapLogic(const void*);
extern void CompilerLogic(const void*);
extern void RunnerLogic(const void*);
extern void ExecutorLogic(const void*);
extern void CheckerLoigc(const void*);
extern void ExitLogic(const void*);


//全部变量:转移argsDumpAndExit
extern struct FSMEdge transferTable[];

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
extern void FSMTransfer(struct FSM *pFSM, enum RunnerState state);

//事件处理
extern void FSMEventHandler(struct FSM *pFSM, enum Event event, void *params);

#endif