#ifndef SANDBOX_RESOURCE
#define SANDBOX_RESOURCE
//ϵͳ��Դ��������
struct ResourceConfig
{
    int time;       //��λms,������CPUʱ��
    long memory;    //��λBytes
    long disk;      //��λBytes
    char *language; //����
} ;

//�ӽ��������Ϣ
struct ChildProgresInfo
{
    int system_status; //ϵͳ״̬
    int judge_status;  //����������״̬
    int child_pid;     //�ӽ���pid
    int exit_code;     //�ӽ����˳�״̬��
} ;
extern struct ResourceConfig resouceConfig;
extern struct ChildProgresInfo childProgress;

#endif