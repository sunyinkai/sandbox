# install g++,gcc,python
# usage: docker build -t myimages .
FROM ubuntu:latest
MAINTAINER yinkaisun@gmail.com
RUN apt-get update && \
    apt-get -y install g++ && \
    apt-get -y install python3.7 && \
    apt-get -y install libcap2-bin && \ 
    mkdir -p /home/sandbox  && \
    rm -rf /var/lib/vapt/lists/* 
#  groupadd -g  2047 boxtaker && \
#    useradd  -r -u 2047 -g boxtaker boxtaker 
COPY  ./runner_FSM_docker.out  /home/sandbox
ENV WELCOME "wlecome to my docker" 
