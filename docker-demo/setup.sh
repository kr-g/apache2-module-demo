
# build the markdown apache handler

cd ~/repo/apache2-module-demo/asdbTest/

sudo apxs -c -i -a mod_asdbTest.c



# build the javascript apache handler

cd ~/repo/apache2-module-demo/asdbjs/

sudo apxs -c -i -a mod_asdbjs.c



# build spidermonkey 

cd ~/repo/gecko-dev/js/src

autoconf2.13

mkdir build_OPT.OBJ
cd build_OPT.OBJ

../configure

make





