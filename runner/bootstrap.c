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

//初始化资源配置函数
void InitResource()
{

    //注册状态机
    FSMRegister(&fsm, transferTable); //注册转移表
    fsm.curState = OnProgramStart;    //在程序起始位置
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
    //初始化资源
    InitResource();
    ProgramStart();
    return 0;
}
