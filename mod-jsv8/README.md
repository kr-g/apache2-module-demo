
##query apxs settings
apxs -q

##make the demo
1. make -f cpp.Makefile clean 
2. make -f cpp.Makefile 

##deploy the demo
one of...
- sudo make -f cpp.Makefile deploy
- sudo apxs -i -n apxscpp mod_apxscpp.so && sudo service apache2 restart && curl localhost:83

##get the respond
one of...
- make -f cpp.Makefile curl
- make -f cpp.Makefile curln
- make -f cpp.Makefile curldata
- make -f cpp.Makefile curlform
- curl localhost:83
- for i in \`seq 1 3\` ; do echo $i ; curl localhost:83  ; done

##start the multi-threaded hello-world demo
./hello-world



