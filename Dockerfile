FROM ubuntu:latest AS compile_stage
WORKDIR /muduo

RUN sed -i 's/https:\/\/mirrors.aliyun.com/http:\/\/mirrors.cloud.aliyuncs.com/g' /etc/apt/sources.list

RUN apt-get update &&    \
    apt-get install -y build-essential cmake && \
    apt-get install -y libboost-all-dev

COPY . .
ENV INSTALL_DIR=/muduo/build/install
RUN bash ./build.sh

# ===========================================================================

FROM ubuntu:latest
WORKDIR /muduo

COPY --from=compile_stage /muduo/build/install ./install
COPY --from=compile_stage /usr/include/boost /usr/local/include/boost
ENV MuduoNet_DIR=/muduo/install
