#!/bin/bash

export BDIR=${1:-~} && if [ ! -d $BDIR ]; then echo [fail] $BDIR does not exist; exit 1; fi


# build the markdown apache handler

cd $BDIR/repo/apache2-module-demo/asdbTest/

apxs -c -i -a mod_asdbTest.c



# build the javascript apache handler

cd $BDIR/repo/apache2-module-demo/asdbjs/

apxs -c -i -a mod_asdbjs.c



#build the v8-int-demo

cd $BDIR/repo/apache2-module-demo/v8-int-demo

HOME=  make



#build the mod-jsv8

cd $BDIR/repo/apache2-module-demo/mod-jsv8/

HOME=  make -f cpp.Makefile
HOME=  sudo make -f cpp.Makefile deploy



