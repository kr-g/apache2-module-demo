#!/bin/bash

export BDIR=${1:-~} && if [ ! -d $BDIR ]; then echo [fail] $BDIR does not exist; exit 1; fi


cd $BDIR/repo/hiredis

make

make install





