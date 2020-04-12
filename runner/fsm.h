#ifndef SANDBOX_FSM
#define SANDBOX_FSM

#include <stdio.h>
//��������������
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

//״̬
enum RunnerState
{
    OnBootstrap,
    OnCompiler,
    OnRunner,
    OnExecutor,
    OnChecker,
    OnExit,
};

//�¼�
enum Event
{
    CondNeedCompile = 0,   //�¼�:��Ҫ����
    CondNoNeedCompile,     //�¼�:����Ҫ����
    CondCompileFinish,     //�¼�:�������
    CondRunnerIsChild,     //�¼�:���ӽ���
    CondResultNeedCompare, //�¼�:��������Ҫ�Ƚ�
    CondProgramNeedToExit, //�¼�:������Ҫ�˳�
};

//����
extern void BootStrapLogic(const void*);
extern void CompilerLogic(const void*);
extern void RunnerLogic(const void*);
extern void ExecutorLogic(const void*);
extern void CheckerLoigc(const void*);
extern void ExitLogic(const void*);


//ȫ������:ת��argsDumpAndExit
extern struct FSMEdge transferTable[];

//ȫ�ֱ���:״̬��
struct FSM
{
    int curState;
    struct FSMEdge *pFSMTable;
    int size;
};

extern struct FSM fsm;

//ע��״̬ת�Ʊ�
extern void FSMRegister(struct FSM *pFSM, struct FSMEdge *pTable);

//״̬Ǩ��
extern void FSMTransfer(struct FSM *pFSM, enum RunnerState state);

//�¼�����
extern void FSMEventHandler(struct FSM *pFSM, enum Event event, void *params);

#endif