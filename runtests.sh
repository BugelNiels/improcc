#!/bin/sh
[ ! -d ./ccheckmate ] && git clone https://github.com/BugelNiels/ccheckmate.git
make -s -f Makefile.test clean
make -s -f Makefile.test
./improc_test
make -s -f Makefile.test clean
make -s -f Makefile.test RELEASE=1
./improc_test
