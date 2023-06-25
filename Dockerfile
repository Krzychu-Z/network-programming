FROM ubuntu:latest

WORKDIR /lab

RUN apt-get update &&\
    apt-get install -y software-properties-common &&\
    add-apt-repository ppa:adiscon/v8-devel &&\
    apt-get install -y net-tools iputils-ping iproute2 gcc make vim tcpdump kmod libsctp-dev rsyslog libpcap-dev

COPY . .

RUN mv configs/services /etc/services &&\
    mv configs/50-default.conf /etc/rsyslog.d/50-default.conf &&\
    mv configs/rsyslog.conf /etc/rsyslog.conf &&\
    touch /var/log/local7 &&\
    chown syslog:adm /var/log/local7 &&\
    chmod 640 /var/log/local7 
