#!/bin/bash

export BDIR=${1:-~} && if [ ! -d $BDIR ]; then echo [fail] $BDIR does not exist; exit 1; fi


# build the markdown apache handler

cd $BDIR/repo/apache2-module-demo/asdbTest/

sudo apxs -c -i -a mod_asdbTest.c



# build the javascript apache handler

cd $BDIR/repo/apache2-module-demo/asdbjs/

sudo apxs -c -i -a mod_asdbjs.c








