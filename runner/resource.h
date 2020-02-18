#ifndef SANDBOX_RESOURCE
#define SANDBOX_RESOURCE
#include "include/list.h"
//ϵͳ��Դ��������
struct ResourceConfig
{
    int time;       //��λms,������CPUʱ��
    long memory;    //��λBytes
    long disk;      //��λBytes
    char *language; //����
};
extern struct ResourceConfig resouceConfig;

//�ļ������Ϣ
struct FileInfo
{
    char *sourceFileName;    //Դ�ļ�����
    char *path;              //�ļ�����Ŀ¼
    char *sysInputFileName;  //�����ļ����� input.txt
    char *usrOutputFileName; //����ļ����ơ�output.txt
    char *exeFileName;       //��ִ���ļ����� a.out
    char *sysOutputFileName; //������ļ�
};
extern struct FileInfo fileInfo;

//��ȡ����������ص����õ�
struct ConfigNode
{
    char *language;
    int needCompile;
    char *compileArgs;
    char *runArgs;
};
extern struct ConfigNode configNode;
int LoadConfig(struct ConfigNode *, char *);
char *ReplaceFlag(const char *originStr, char *flag, char *dst);
#endif