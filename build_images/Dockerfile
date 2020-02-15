# install g++,gcc,python
# usage: docker build -t myimages .
FROM ubuntu:latest
MAINTAINER yinkaisun@gmail.com
RUN apt-get update && \
    apt-get -y install g++ && \
    apt-get -y install python3.7 && \
    rm -rf /var/lib/vapt/lists/* && \
    mkdir -p /home/sandbox
COPY  ./runner_FSM.out  /home/sandbox
ENV WELCOME "wlecome to my docker" 
