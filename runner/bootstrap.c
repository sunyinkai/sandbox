#include "runner.h"
#include "compiler.h"
#include "fsm.h"
#include "resource.h"
#include "contants.h"
#define CLOG_MAIN
#include "clog.h"

const int UNIQ_LOG_ID = 1;
void init_log(int uinqId, enum clog_level level)
{
    int r;
    r = clog_init_path(uinqId, "log.txt");
    if (r != 0)
    {
        exit(1);
    }
    clog_set_level(uinqId, level);
}

void ProgramStart()
{
    if (needCompile(resouceConfig.language))
    {
        FSMEventHandler(&fsm, CondNeedCompile, NULL);
    }
    else
    {
        FSMEventHandler(&fsm, CondNoNeedCompile, NULL);
    }
}

//��ʼ����Դ���ú���
void InitResource()
{

    init_log(UNIQ_LOG_ID, CLOG_INFO); //��ʼ����־��Ϣ

    //ע��״̬��
    FSMRegister(&fsm, transferTable); //ע��ת�Ʊ�
    fsm.curState = OnProgramStart;    //�ڳ�����ʼλ��

    resouceConfig.disk = 65536;
    resouceConfig.memory = 65536;
    resouceConfig.time = 1000;
    resouceConfig.language = "g++";

    childProgress.child_pid = -1;
    childProgress.judge_status = EXIT_JUDGE_AC;
    childProgress.system_status = EXIT_SYSTEM_SUCCESS;
    childProgress.exit_code = 0;

    //fileInfo.path = "/home/naoh/Program/go/src/sandbox/output";
    fileInfo.path="/tmp/";
    fileInfo.inputFileName = "in.txt";
    fileInfo.outputFileName = "output.txt";
    fileInfo.exeFileName = "a.out";
    fileInfo.sourceFileName = "a.cpp";
}

int main()
{
    //��ʼ����Դ
    InitResource();
    ProgramStart();
    return 0;
}
