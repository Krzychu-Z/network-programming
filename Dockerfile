FROM ubuntu:latest

WORKDIR /lab

RUN apt-get update &&\
    apt-get install -y net-tools iputils-ping iproute2 gcc make vim tcpdump kmod libsctp-dev

COPY . .

RUN mv services /etc/services 
