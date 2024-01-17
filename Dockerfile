FROM ubuntu:latest
WORKDIR /workspace

RUN sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list

RUN apt-get update &&    \
    apt-get install -y build-essential cmake && \
    apt-get install -y libboost-all-dev

COPY . ./muduo

RUN cd muduo && \
    export INSTALL_DIR=/usr/local && \
    bash ./build.sh
