# install g++,gcc,python
# usage: docker build -t myimages .
FROM ubuntu:xenial
MAINTAINER yinkaisun@gmail.com
RUN apt-get update && \
    apt-get -y install g++ && \
    mkdir -p /home/sandbox && \
    apt-get -y install python3 && \
    apt-get -y install golang && \
    rm -rf /var/lib/vapt/lists/* 

#    apt-get -y install libcap2-bin && \ 
RUN useradd --create-home --no-log-init --shell /bin/bash --uid 65529 test

COPY  ./runner_FSM_docker.out  /home/sandbox
COPY ./config /home/sandbox/config
ENV WELCOME "wlecome to my docker" 
WORKDIR /home/sandbox
