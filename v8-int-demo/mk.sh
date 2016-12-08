#!/bin/bash

export V8DIR=~/repo/v8
export V8INC=$V8DIR
export V8BDIR=$V8DIR/out/x64.release
export V8LDIR=$V8BDIR/lib.target

echo V8DIR=$V8DIR
echo V8INC=$V8INC
echo V8BDIR=$V8BDIR
echo V8LDIR=$V8LDIR


g++ -I. -I$V8INC -I$V8INC/include hello-world.cc -o hello-world -lrt -ldl -lm -Wl,--start-group $V8LDIR/{libv8,libv8_{libbase,libplatform},libicu{uc,i18n}}.so -Wl,--end-group -pthread -std=c++0x 

#cp $V8BDIR/*.bin .

echo use ldconfig or call with

echo LD_LIBRARY_PATH=$V8LDIR ./hello-world


