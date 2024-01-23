FROM ubuntu:latest
WORKDIR /muduo

RUN sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list

RUN apt-get update &&    \
    apt-get install -y build-essential cmake && \
    apt-get install -y libboost-all-dev

COPY . .

ENV INSTALL_DIR=/usr/local

RUN bash ./build.sh
