#include "fsm.h"
#include <stdlib.h>

struct FSMEdge transferTable[] = {
    {
        OnBootstrap,
        CondNeedCompile,
        OnCompiler,
        CompilerLogic,
    },
    {
        OnBootstrap,
        CondNoNeedCompile,
        OnRunner,
        RunnerLogic,
    },
    {
        OnCompiler,
        CondCompileFinish,
        OnRunner,
        RunnerLogic,
    },
    {
        OnRunner,
        CondRunnerIsChild,
        OnExecutor,
        ExecutorLogic,
    },
    {
        OnRunner,
        CondResultNeedCompare,
        OnChecker,
        CheckerLoigc,
    },
    //所有状态都可以直接转移到退出状态,即 *----CondProgramNeedToExit---->OnProgramEnd
    {
        OnBootstrap,
        CondProgramNeedToExit,
        OnExit,
        ExitLogic,
    },
    {
        OnCompiler,
        CondProgramNeedToExit,
        OnExit,
        ExitLogic,
    },
    {
        OnRunner,
        CondProgramNeedToExit,
        OnExit,
        ExitLogic,
    },
    {
        OnExecutor,
        CondProgramNeedToExit,
        OnExit,
        ExitLogic,
    },
    {
        OnChecker,
        CondProgramNeedToExit,
        OnExit,
        ExitLogic,
    },
};

//注册状态转移表
void FSMRegister(struct FSM *pFSM, struct FSMEdge *pTable)
{
    pFSM->pFSMTable = pTable;
    pFSM->curState = OnBootstrap;
    pFSM->size = sizeof(transferTable) / sizeof(struct FSMEdge);
}

//状态迁移
void FSMTransfer(struct FSM *pFSM, enum RunnerState state)
{
    pFSM->curState = state;
}

//事件处理
void FSMEventHandler(struct FSM *pFSM, enum Event event, void *params)
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
        //注意与切换状态的顺序
        FSMTransfer(pFSM, nextState);
        if (func)
        {
            func(params);
        }
    }
    else
    {
        printf("Not find such event\n");
    }
}

int AddFuncToList(struct FuncPointerNode **fpn, FuncPointer func)
{
    struct FuncPointerNode *newNode = (struct FuncPointerNode *)malloc(sizeof(struct FuncPointerNode));
    newNode->func = func;
    newNode->next = NULL;
    if (*fpn == NULL)
    {
        *fpn = newNode;
        return 0;
    }
    struct FuncPointerNode *tmp = *fpn;
    while (tmp->next != NULL)
        tmp = tmp->next;
    tmp->next = newNode;
    return 0;
}

void FuncListRun(struct FuncPointerNode *fpn, const void *param)
{
    const void *inArgs = param;
    while (fpn != NULL)
    {
        FuncPointer func = fpn->func;
        inArgs = func(inArgs);
        fpn = fpn->next;
    }
}

struct FSM fsm;