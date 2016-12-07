#!/bin/bash

export BDIR=${1:-~} && if [ ! -d $BDIR ]; then echo [fail] $BDIR does not exist; exit 1; fi

cd $BDIR/repo

git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git


export PATH=`pwd`/depot_tools:"$PATH"


fetch v8


gclient sync


cd v8

make x64.release  library=shared 

