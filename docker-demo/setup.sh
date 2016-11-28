
# get the main repo 

cd ~/repo

git clone https://github.com/kr-g/apache2-module-demo.git


# build the markdown apache handler

cd apache2-module-demo/asdbTest/

sudo apxs -c -i -a mod_asdbTest.c

sudo make all 


# build the javascript apache handler

cd ~/repo/apache2-module-demo/asdbjs/

sudo apxs -c -i -a mod_asdbjs.c

sudo make all 


# get and build spidermonkey

cd ~/repo/

git clone https://github.com/mozilla/gecko-dev.git --branch GECKO4502_2016040719_RELBRANCH --single-branch

cd gecko-dev/js/src

autoconf2.13

mkdir build_OPT.OBJ

cd build_OPT.OBJ

../configure

make

cd dist/bin

# use link in case node.js is in the path
ln -s js spidermonkeyjs 





