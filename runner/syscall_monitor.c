#include "syscall_monitor.h"
#include <seccomp.h>
#include <stdio.h>

void install_seccomp_filter(struct InformationToFilter *infoFile)
{
    int blackList[] = {
        //线程进程相关的系统调用
        SCMP_SYS(fork),
        SCMP_SYS(clone),
        SCMP_SYS(vfork),
        SCMP_SYS(chown),
    };
    scmp_filter_ctx ctx;
    ctx = seccomp_init(SCMP_ACT_ALLOW);
    for (int i = 0; i < sizeof(blackList) / sizeof(int); ++i)
    {
        seccomp_rule_add(ctx, SCMP_ACT_KILL, blackList[i], 0);
    }

   // seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(execve), 1,
    //                 SCMP_A0(SCMP_CMP_NE,infoFile->exeFileName));
    seccomp_load(ctx);
    return;
}