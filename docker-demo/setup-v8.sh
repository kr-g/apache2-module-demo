#!/bin/bash

export BDIR=${1:-~} && if [ ! -d $BDIR ]; then echo [fail] $BDIR does not exist; exit 1; fi

export IDIR=$BDIR/repo/v8/out/x64.release/lib.target/

echo using $IDIR
echo $IDIR > /etc/ld.so.conf.d/v8x64.conf

ldconfig

ldconfig -p |grep v8

