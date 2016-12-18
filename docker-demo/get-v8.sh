#!/bin/bash

export BDIR=${1:-~} && if [ ! -d $BDIR ]; then echo [fail] $BDIR does not exist; exit 1; fi

cd $BDIR/repo

git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git


export PATH=`pwd`/depot_tools:"$PATH"


fetch v8


gclient sync


cd v8

git checkout 083a5dcdfee3a6b7c3e41e4a2dc4420604e86641


#make x64.release -j 4 library=shared

