#include "contants.h"
#include <stdio.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include "syscall_monitor.h"
#define CLOG_MAIN
#include "logger.h"
const int UNIQ_LOG_ID = 0;
//ϵͳ��Դ��������
typedef struct
{
    int time;       //��λms,������CPUʱ��
    int memory;     //��λM
    int disk;       //��λM
    char *language; //����
} ResourceConfig;

//�ӽ��������Ϣ
struct ChildProgresInfo
{
    int system_status; //ϵͳ״̬
    int judge_status;  //����������״̬
    int child_pid;     //�ӽ���pid
    int exit_code;     //�ӽ����˳�״̬��
} childProgress;

//���óɹ�����0,���򷵻�-1
int setProgressLimit(int resource, int val)
{
    struct rlimit limit;
    limit.rlim_cur = limit.rlim_max = val;
    int retCode = setrlimit(resource, &limit);
    return retCode;
}

//��ʱɱ������
void time_out_kill(int sigId)
{
    if (childProgress.child_pid > 0)
    {
        int retCode = kill(childProgress.child_pid, SIGKILL);
        if (retCode == 0)
        {
            childProgress.judge_status = EXIT_JUDGE_TLE;
        }
    }
    alarm(0);
}

//�Ƿ���Ҫ����
int needCompile(const char *s)
{
    return 1;
}

//�������
int compile(ResourceConfig *config)
{
    //�������Ҫ����
    if (!needCompile(config->language))
        return 0;
    /*
        ����Ĵ����ڶ���Ӧ��д�ɱ�����������õ���ʽ
        language,�Լ���Ӧ��ִ�д���
    */
    int compilePid = fork();
    if (compilePid < 0)
    {
        return -1;
    }
    else if (compilePid == 0) //�ӽ���
    {
        char cmd[500];
        const char *path = "/home/naoh/Program/go/src/sandbox/output";
        sprintf(cmd, "g++ %s/a.cpp -lpthread -o %s/a.out", path, path);
        char *argv[] = {"/bin/bash", "-c", cmd, NULL};
        execvp("/bin/bash", argv);
    }
    else
    {
        int status;
        int retCode = wait(&status);
        clog_info(CLOG(UNIQ_LOG_ID), "the compile subprogress pid is %d,retcode is %d\n", retCode, status);
    }
    return 0;
}

//���г���
int runProgress(ResourceConfig *config)
{
    if (config == NULL)
    {
        childProgress.system_status = EXIT_SYSTEM_ERROR;
        clog_error(CLOG(UNIQ_LOG_ID), "the resource config is NULL");
        return EXIT_SYSTEM_ERROR;
    }

    int fpid = fork();
    if (fpid < 0)
    {
        childProgress.system_status = EXIT_SYSTEM_ERROR;
        clog_error(CLOG(UNIQ_LOG_ID), "in runProgress fork error");
        return EXIT_SYSTEM_ERROR;
    }
    else if (fpid == 0) //�ӽ���
    {
        //������Դ����
        //setProgressLimit(RLIMIT_CPU, (config->time + 1000) / 1000);
        //setProgressLimit(RLIMIT_AS, config->memory + 10240);
        //setProgressLimit(RLIMIT_FSIZE, config->disk + 10240);

        //�ض���IO
        //������Ҫ��������ļ���Ȩ��
        const char *path = "/home/naoh/Program/go/src/sandbox/output";
        char infileName[100];
        char outfileName[100];
        sprintf(infileName, "%s/in.txt", path);
        sprintf(outfileName, "%s/output.txt", path);
        int read_fd = open(infileName, O_RDONLY);
        int write_fd = open(outfileName, O_WRONLY | O_CREAT | O_TRUNC);
        dup2(read_fd, STDIN_FILENO);
        dup2(write_fd, STDOUT_FILENO);

        //��װsystem_call filter
        char cmd[100];
        sprintf(cmd, "%s/a.out", path);
        struct InformationToFilter info;
        info.exeFileName = cmd;
        install_seccomp_filter(&info);

        //ִ������
        char *argv[] = {cmd, NULL};
        int ret = execvp(cmd, argv);
        if (ret == -1)
            printf("execvp error");
    }
    else //������
    {
        childProgress.child_pid = fpid;
        signal(SIGALRM, time_out_kill); //ע�ᳬʱɱ�������¼�
        alarm((config->time + 1000) / 1000);
        int status = -1;
        int childPid = wait(&status);
        clog_info(CLOG(UNIQ_LOG_ID), "parent:the childPid is %d,status is %d", childPid, status);
    }
    return EXIT_SYSTEM_SUCCESS;
}

int main()
{
    init_log(UNIQ_LOG_ID, CLOG_INFO);

    ResourceConfig config;
    config.disk = 10 * MB_TO_BYTES;
    config.memory = 1 * MB_TO_BYTES;
    config.time = 1000;
    config.language = "g++";

    childProgress.child_pid = -1;
    childProgress.judge_status = EXIT_JUDGE_AC;
    childProgress.system_status = EXIT_SYSTEM_SUCCESS;
    childProgress.exit_code = 0;

    //�������г���
    clog_info(CLOG(UNIQ_LOG_ID), "before compile");
    compile(&config);

    clog_info(CLOG(UNIQ_LOG_ID), "begin to run exe file");
    runProgress(&config);
    clog_info(CLOG(UNIQ_LOG_ID), "finish run");
    return 0;
}