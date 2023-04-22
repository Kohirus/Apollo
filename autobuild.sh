#!/bin/bash

BUILD_DIR=./build/
BIN_DIR=./bin/
LIB_DIR=./lib/

set -x

rm -rf $BUILD_DIR/*

if [ ! -d "$BUILD_DIR" ];then
  mkdir $BUILD_DIR
fi

if [ ! -d "$BIN_DIR" ];then
  mkdir $BIN_DIR
fi

if [ ! -d "$LIB_DIR" ];then
  mkdir $LIB_DIR
fi

cd $BUILD_DIR &&
  cmake -DCMAKE_PREFIX_PATH=/usr/local/protobuf -DCMAKE_INSTALL_PREFIX=/usr/local/apollo .. &&
  make install
