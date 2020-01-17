#ifndef SANDBOX_FSM
#define SANDBOX_FSM

#include<stdio.h>
//�������event�¼�,��curState״̬�л���nextState,Ȼ��ִ��func����
struct FSMEdge
{
    int curState;
    int event;
    int nextState;
    void (*fun)(const void *);
};

//״̬
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


//�¼�
enum Event
{
    CondNeedCompile=0, //�¼�:��Ҫ����
    CondNoNeedCompile, //�¼�:����Ҫ����
    CondCompileFinish, //�¼�:�������
    CondRunnerIsPar , //�¼�:�Ǹ�����
    CondRunnerIsChild,   //�¼�:���ӽ���
    CondRunnerAfterInit, //�¼�:�ӽ��̳�ʼ�����
    CondRunnerChildExit, //�¼�:�ӽ����˳�
};

//����

void ProgramRun(const void*);
void Compile(const void*);

void Run(const void*);
void ChildInit(const void *);
void ParMonitor(const void *);
void ChildRun(const void *);
void ParAfterRun(const void *);
void Init(const void *);

//ȫ������:ת�Ʊ�
extern struct FSMEdge transferTable[7];//����ط�Ҫ�޸�

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
extern void FSMTransfer(struct FSM *pFSM, int state);

//�¼�����
extern void FSMEventHandler(struct FSM *pFSM, int event, void *params);

#endif