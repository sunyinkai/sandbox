#ifndef _MONITOR_H
#define _MONITOR_H

struct InformationToFilter
{
    char *inFileName;  //输入文件名称
    char *exeFileName; //待执行文件名称
};

extern void install_seccomp_filter(struct InformationToFilter *infoFile);

#endif