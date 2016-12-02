#!/bin/bash

export BDIR=${1:-~} && if [ ! -d $BDIR ]; then echo [fail] $BDIR does not exist; exit 1; fi

# build spidermonkey 

cd $BDIR/repo/gecko-dev/js/src

autoconf2.13

mkdir -p build_OPT.OBJ
cd build_OPT.OBJ

../configure --disable-debug --enable-optimize 
#--prefix=$BDIR/repo/spidermonkey

make -j 1 
#install 

#chmod +x $BDIR/repo/gecko-dev/js/src/build_OPT.OBJ/js/src/shell/js

cp $BDIR/repo/gecko-dev/js/src/build_OPT.OBJ/js/src/shell/js $BDIR/repo/bin/spidermonkeyjs

sudo ln -s $BDIR/repo/bin/spidermonkeyjs /usr/local/sbin/spidermonkeyjs -f



