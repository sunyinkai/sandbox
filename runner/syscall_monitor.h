#ifndef _MONITOR_H
#define _MONITOR_H

struct InformationToFilter
{
    char *inFileName;  //�����ļ�����
    char *exeFileName; //��ִ���ļ�����
};

extern void install_seccomp_filter(struct InformationToFilter *infoFile);

#endif