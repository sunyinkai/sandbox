#include "runner.h"
#include "compiler.h"
#include "fsm.h"
#include "resource.h"
#include "contants.h"

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

    //ע��״̬��
    FSMRegister(&fsm, transferTable); //ע��ת�Ʊ�
    fsm.curState = OnProgramStart;    //�ڳ�����ʼλ��
                                      // init_log(UNIQ_LOG_ID, CLOG_INFO);

    resouceConfig.disk = 65536;
    resouceConfig.memory = 65536;
    resouceConfig.time = 3000;
    resouceConfig.language = "g++";

    childProgress.child_pid = -1;
    childProgress.judge_status = EXIT_JUDGE_AC;
    childProgress.system_status = EXIT_SYSTEM_SUCCESS;
    childProgress.exit_code = 0;
}

int main()
{
    //��ʼ����Դ
    InitResource();
    ProgramStart();
    return 0;
}
