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
    char *sourceFileName;     //Դ�ļ�����
    char *path;               //�ļ�����Ŀ¼
    char *sysInputFileName;   //�����ļ����� input.txt
    char *sysOutputFileName;  //������ļ�
    char *usrOutputFileName;  //����ļ����ơ�output.txt
    char *exeFileName;        //��ִ���ļ����� a.out
    char *resultJsonFileName; //������ɵ�json����
    char *specialJudgeExe;    //���г���
};
extern struct FileInfo fileInfo;

struct DataItem
{
    void *data;
    struct DataItem *next;
};

struct ConfigNode
{
    char *language;
    int needCompile;
    char *compileArgs;
    char *runArgs;
    char *syscallMode;             //ģʽ
    struct DataItem *syscallItems; //ϵͳ����list
};
extern struct ConfigNode configNode;
int LoadConfig(struct ConfigNode *, char *);
char *ReplaceFlag(const char *originStr, char *flag, char *dst);
#endif
