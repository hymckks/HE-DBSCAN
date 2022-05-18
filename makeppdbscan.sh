#! /bin/bash

cmake -Dhelib=/usr/local/share/cmake
make
sudo rm -r CMakeFiles/ CMakeCache.txt cmake_install.cmake Makefile