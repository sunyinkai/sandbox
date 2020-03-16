pid=fork();//fork一个新进程
if(pid<0){
    //error
}else if(pid==0){//子进程
    //重定向io
    //限制资源，内存
    execve()//运行子进程
}else{//父进程
    //监控子进程运行情况
}