资源:cpu,磁盘,内存,网络


### 安全防御:
1:程序级别
setrlimit限制资源使用
seccomp限制系统调用,fork,clone


2:用户级别
设置一个只读用户(nobody),不加入root组,用管道传递数据

3:系统级别
chroot
低权限,运行在docker上面,docker再次对资源限制

selinux:用法


4:对于高频访问，提供分布式方案


feature:
支持多种语言
使用json可配置化
靠限制系统调用，更彻底
使用docker，更安全
inner部分纯c编写,速度更快



### inner判题部分
采用状态机:
compiler----> runner---->comparer


优点:
    代码优雅，易于维护


资源分为3部分:
分别有3个全局变量
资源相关部分,内存，磁盘,cpu,语言,初始化后只读
判题相关部分,退出状态，系统状态，花费的时间和cpu
文件相关部分,输入输出文件,初始化后只读



### outter Docker部分
 一,限制资源,防止docker把宿主机搞崩
1,限制内存


二,以非root启动



黑科技:
限制代码长度
