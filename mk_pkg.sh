# !/bin/bash

mkdir -p build_pkg
cd build_pkg
cmake -D CMAKE_BUILD_TYPE=Release ..
make -j

mkdir -p pkg/v4l2cpp
mkdir -p pkg/lib
mkdir -p pkg/sample
cp ../inc/v4l2cpp/V4l2MultiplaneCapture.h ./pkg/v4l2cpp/
cp ./libv4l2cpp.so ./pkg/lib/
cp ../main.cpp ./pkg/sample/

# 打包
tar -czf v4l2.tar.gz pkg