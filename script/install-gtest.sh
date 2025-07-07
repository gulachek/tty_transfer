#!/bin/bash

set -x
set -e

. script/util.sh

cmake_build_install() {
	cmake --build build
	cmake --install build --prefix "$VENDOR"
	rm -rf build
}

SRC="$PWD"

# gtest
GTEST_DOWNLOAD="$VENDORSRC/download-gtest.tgz"

download \
	-u "https://github.com/google/googletest/releases/download/v1.16.0/googletest-1.16.0.tar.gz" \
	-c "78c676fc63881529bf97bf9d45948d905a66833fbfa5318ea2cd7478cb98f399" \
	-o "$GTEST_DOWNLOAD"
	
GTEST="$VENDORSRC/googletest"

md "$GTEST"

untar -f "$GTEST_DOWNLOAD" -d "$GTEST"

cd "$GTEST"
cmake -DBUILD_GMOCK=OFF "-DCMAKE_INSTALL_PREFIX=$VENDOR" -S . -B build
cmake_build_install

rm "$GTEST_DOWNLOAD"
