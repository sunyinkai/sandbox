#ifndef SANDBOX_FSM
#define SANDBOX_FSM

#include <stdio.h>
//函数调用链代码
typedef void* (*FuncPointer)(const void*);

struct FuncPointerNode{
    FuncPointer func;
    struct FuncPointerNode* next;
};

//将函数加入调用链中
extern int AddFuncToList(struct FuncPointerNode **fpn,FuncPointer func);

//运行调用链
extern void FuncListRun(struct FuncPointerNode *fpn,const void*param);

//销毁调用链
extern void DestroyFuncList(struct FuncPointerNode*fpn);

//状态机部分的代码
struct FSMEdge
{
    int curState; //现态
    int event; //事件
    int nextState; //次态
    void (*fun)(const void*); //执行的动作
};

//状态
enum RunnerState
{
    OnBootstrap, //启动
    OnCompiler,  //编译
    OnRunner,    //父进程
    OnExecutor,  //子进程
    OnChecker,   //比对结果
    OnExit,     //退出
};

//事件
enum Event
{
    CondNeedCompile = 0,   //事件:需要编译
    CondNoNeedCompile,     //事件:不需要编译
    CondCompileFinish,     //事件:编译完成
    CondRunnerIsChild,     //事件:是子进程
    CondResultNeedCompare, //事件:需要比较输出
    CondProgramNeedToExit, //事件:进程需要退出
};

//动作
extern void BootStrapLogic(const void*);
extern void CompilerLogic(const void*);
extern void RunnerLogic(const void*);
extern void ExecutorLogic(const void*);
extern void CheckerLoigc(const void*);
extern void ExitLogic(const void*);

//状态转移表
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