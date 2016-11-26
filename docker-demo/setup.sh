

cd ~/repo

git clone https://github.com/kr-g/apache2-module-demo.git

cd apache2-module-demo/asdbTest/

sudo apxs -c -i -a mod_asdbTest.c

sudo make all reload


