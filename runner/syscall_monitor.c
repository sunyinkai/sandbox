#include "syscall_monitor.h"
#include <seccomp.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "resource.h"

void install_seccomp_filter()
{
    /*������������⣬����������seccomp������û����������õ�seccomp�������ô��?
      �������������⣬��Ϊseccomp��Ҫprctl������ã�������Ҫ�����ban���ˣ������Ͳ��������
    */
    scmp_filter_ctx ctx;
    unsigned int init_mode, rule_mode;
    if (strcmp(configNode.syscallMode, "white") == 0)
        init_mode = SCMP_ACT_KILL, rule_mode = SCMP_ACT_ALLOW;
    else if (strcmp(configNode.syscallMode, "black") == 0)
        init_mode = SCMP_ACT_ALLOW, rule_mode = SCMP_ACT_KILL;
    else
        exit(-1);
    ctx = seccomp_init(init_mode);
    struct DataItem *now = configNode.syscallItems;
    while (now != NULL)
    {
        seccomp_rule_add(ctx, rule_mode, (int)now->data, 0);
        now = now->next;
    }
    seccomp_load(ctx);
    return;
}