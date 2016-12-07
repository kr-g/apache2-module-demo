#!/bin/bash

export BDIR=${1:-~} && if [ ! -d $BDIR ]; then echo [fail] $BDIR does not exist; exit 1; fi

mkdir -p $BDIR/v8

cd $BDIR/v8

tar -xvf ../archive-v8.tar

#ldconfig `pwd`

#ldconfig -p|grep v8

