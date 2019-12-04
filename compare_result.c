#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAME 0
#define DIFF 1
#define OPEN_FILE_ERR 2
#define MAX_ONE_LINE_SIZE 10000000
#define MAX_FILE_NAME_SIZE 107
#define FALSE 0
#define TRUE 1
//忽略行末空格
int compare(const char *source, const char *target)
{
    char file_name0[MAX_FILE_NAME_SIZE], file_name2[MAX_FILE_NAME_SIZE];
    sprintf(file_name0, "output/%s", source);
    sprintf(file_name2, "output/%s", target);

    //打开待比对文件file_0,以及标准输出file_2
    FILE *fp0 = fopen(file_name0, "r");
    FILE *fp2 = fopen(file_name2, "r");
    if (fp0 == NULL || fp2 == NULL)
    {
        return OPEN_FILE_ERR;
    }
    char *line0 = (char *)malloc(sizeof(char) * MAX_ONE_LINE_SIZE);
    char *line2 = (char *)malloc(sizeof(char) * MAX_ONE_LINE_SIZE);
    char isSame = TRUE;
    while (!feof(fp2))
    {
        fgets(line0, MAX_ONE_LINE_SIZE, fp2); //fgets会读入换行
        if (feof(fp0))
        {
            isSame = FALSE;
            break;
        }
        fgets(line2, MAX_ONE_LINE_SIZE, fp0);
        if (strcmp(line0, line2) != 0)
        {
            isSame = FALSE;
            break;
        }
    }
    if (!feof(fp2) || !feof(fp0))
        isSame = FALSE;
    free(line0);
    free(line2);
    return isSame ? SAME : DIFF;
}

int main()
{
    printf("%d\n", compare("a.out", "b.out"));
    return 0;
}