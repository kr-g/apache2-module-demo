#!/bin/bash

. /etc/apache2/envvars

export PATH=$PATH:~/repo/apache2-module-demo/gecko-dev/js/src/build_OPT.OBJ/dist/bin/

#  start in debug mod 
apache2 -X


