#ifndef SANDBOX_RESOURCE
#define SANDBOX_RESOURCE
//ϵͳ��Դ��������
struct ResourceConfig
{
    int time;       //��λms,������CPUʱ��
    long memory;    //��λBytes
    long disk;      //��λBytes
    char *language; //����
};

//�ļ������Ϣ
struct FileInfo
{
    char *sourceFileName; //Դ�ļ�����
    char *path;           //�ļ�����Ŀ¼
    char *inputFileName;  //�����ļ����� input.txt
    char *outputFileName; //����ļ����ơ�output.txt
    char *exeFileName;    //��ִ���ļ����� a.out
    char *answerFileName; //������ļ�
};

extern struct ResourceConfig resouceConfig;
extern struct FileInfo fileInfo;

#endif