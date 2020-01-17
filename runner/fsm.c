#include "fsm.h"

struct FSMEdge transferTable[7] = {
    {
        OnProgramStart,
        CondNeedCompile,
        OnCompilerCompile,
        Compile,
    },
    {
        OnProgramStart,
        CondNoNeedCompile,
        OnRunnerStart,
        Run,
    },
    {
        OnCompilerCompile,
        CondCompileFinish,
        OnRunnerStart,
        Run,
    },
    {
        OnRunnerStart,
        CondRunnerIsChild,
        OnRunnerChildInit,
        ChildInit,
    },
    {
        OnRunnerStart,
        CondRunnerIsPar,
        OnRunnerParMonitor,
        ParMonitor,
    },
    {
        OnRunnerChildInit,
        CondRunnerAfterInit,
        OnRunnerChildRun,
        ChildRun,
    },
    {
        OnRunnerParMonitor,
        CondRunnerChildExit,
        OnRunnerParAfterRun,
        ParAfterRun,
    },
};

//注册状态转移表
void FSMRegister(struct FSM *pFSM, struct FSMEdge *pTable)
{
    pFSM->pFSMTable = pTable;
    pFSM->curState = OnRunnerStart;
    pFSM->size = sizeof(transferTable) / sizeof(struct FSMEdge);
    printf("FSM size:%d\n", pFSM->size);
}

//状态迁移
void FSMTransfer(struct FSM *pFSM, int state)
{
    pFSM->curState = state;
}

//事件处理
void FSMEventHandler(struct FSM *pFSM, int event, void *params)
{
    struct FSMEdge *pActTable = pFSM->pFSMTable;
    int isFind = 0;
    void (*func)(const void *) = NULL;
    int nextState = -1;
    for (int i = 0; i < pFSM->size; ++i)
    {
        if (event == pActTable[i].event && pFSM->curState == pActTable[i].curState)
        {
            isFind = 1;
            func = pActTable[i].fun;
            nextState = pActTable[i].nextState;
            break;
        }
    }
    if (isFind)
    {
        FSMTransfer(pFSM, nextState);
        if (func)
        { //注意与切换状态的顺序
            func(params);
        }
    }
    else
    {
        printf("Not find such event\n");
    }
}

struct FSM fsm;