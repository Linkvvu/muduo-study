#!/bin/sh

# 开启调试模式，会输出每个执行的命令
set -x  

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-./build}
BUILD_TYPE=${BUILD_TYPE:-release}
INSTALL_DIR=${INSTALL_DIR:-"../${BUILD_TYPE}-install-cpp11"}
MUDUO_UNIT_TESTS=${MUDUO_UNIT_TESTS:-OFF}
MUDUO_USE_MEMPOOL=${MUDUO_USE_MEMPOOL:-ON}
CXX=${CXX:-g++}

ln -sf ${BUILD_DIR}/${BUILD_TYPE}-cpp11/compile_commands.json compile_commands.json

mkdir -p ${BUILD_DIR}/${BUILD_TYPE}-cpp11

cd ${BUILD_DIR}/${BUILD_TYPE}-cpp11             \
    && cmake 					                \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE}        \
        -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}   \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON      \
        -DMUDUO_UNIT_TESTS=${MUDUO_UNIT_TESTS}  \
        -DMUDUO_USE_MEMPOOL=${MUDUO_USE_MEMPOOL}\
        ${SOURCE_DIR}   			            \
    && make && make install
    
# If need to compile test files, 
# define environment variable MUDUO_UNIT_TESTS=ON(yes)