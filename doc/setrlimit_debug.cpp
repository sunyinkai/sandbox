#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include<assert.h>
int main()
{
    int *a = NULL;
    // 150M
    int v = 150000000;
    struct rlimit memory_limit;
    // 90M
    memory_limit.rlim_cur = memory_limit.rlim_max = 90000;
    //setrlimit(RLIMIT_AS, &memory_limit);
    int ret=setrlimit(RLIMIT_NOFILE, &memory_limit);
    assert(ret!=-1);
    a = (int *)malloc(v);
    if(a == NULL){
        printf("error\n");
    }
    else {
        memset(a, 0, v);
        printf("success\n");
    }
    return 0;
}
