#!/bin/bash

. /etc/apache2/envvars

export PATH=$PATH:~/repo/spidermonkey

#  start in debug mod 
apache2 -X


