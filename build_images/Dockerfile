# install g++,gcc,python
# usage: docker build -t myimages .
FROM ubuntu:latest
MAINTAINER yinkaisun@gmail.com
RUN apt-get update && \
    apt-get -y install g++ && \
    mkdir -p /home/sandbox && \
    rm -rf /var/lib/vapt/lists/* 

#    apt-get -y install libcap2-bin && \ 
#    apt-get -y install python3.7 && \
#    apt-get -y install golang && \

COPY  ./runner_FSM_docker.out  /home/sandbox
COPY ./config /home/sandbox/config
ENV WELCOME "wlecome to my docker" 
WORKDIR /home/sandbox
