## Sandbox
This project is used for judge programs submitted by users in ACM-ICPC or OI.</br>
Your hacks or issues are strongly welcomed,please feel free to contact me!</br>
</br>
## Constructure
-----  runner: inner judgement core,it's s organized by a state machine programming model </br>
  ------- include: 3rd library,such as cJSON,clog </br>
----   docker: docker management,such as docker pool,the interactive between docker and runner </br>
----   build\_image:build images,dockerfile </br>
</br>
## Debug:
strace: monitor the used system calls.</br>
