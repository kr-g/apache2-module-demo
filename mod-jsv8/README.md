
##query apxs settings
apxs -q

##make the demo
make -f cpp.Makefile clean && make -f cpp.Makefile 

##deploy the demo
sudo apxs -i -n apxscpp mod_apxscpp.so && sudo service apache2 restart && curl localhost:83

##get the respond
one of...
- curl localhost:83
- for i in \`seq 1 3\` ; do echo $i ; curl localhost:83  ; done

##start the multi-threaded hello-world demo
./hello-world



