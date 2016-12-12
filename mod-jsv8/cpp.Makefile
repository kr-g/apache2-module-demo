#
#  makefile -- apxs build for cpp apache module
#

 
# your module name
MODNAM=apxscpp


V8DIR=${HOME}/repo/v8
V8INC=${V8DIR}
V8BDIR=${V8DIR}/out/x64.release
V8LDIR=${V8BDIR}/lib.target

# your inc and lib stuff


JS_CFLAGS=-Os -s -lz -lpthread -lm -ldl
JS_CPPFLAGS= -pthread -pipe -g -freorder-blocks -O3 -fno-omit-frame-pointer -lz -lm -ldl
#-fno-rtti -fno-exceptions -fno-math-errno

 
# apxs executable
APXS=apxs
 
# compiler
CXX=g++




#--------------------------------------------
#
# some apxs internal values
# use apxs -q to explore more instead of hard coding
#
APXS_CFLAGS=`$(APXS) -q CFLAGS`   
APXS_CFLAGS_SHLIB=`$(APXS) -q CFLAGS_SHLIB`   
APXS_INCLUDEDIR=`$(APXS) -q INCLUDEDIR`   
APXS_LDFLAGS_SHLIB=`$(APXS) -q LDFLAGS_SHLIB`
APXS_LD_SHLIB=`$(APXS) -q LD_SHLIB`
APXS_LIBEXECDIR=`$(APXS) -q LIBEXECDIR`
APXS_LIBS_SHLIB=`$(APXS) -q LIBS_SHLIB`
APXS_SBINDIR=`$(APXS) -q SBINDIR`   
APXS_SYSCONFDIR=`$(APXS) -q SYSCONFDIR`
APXS_TARGET=`$(APXS) -q TARGET`   
 
APR_INCLUDEDIR=`$(APXS) -q APR_INCLUDEDIR`


# default target

all: mod_$(MODNAM).so hello-world
#all:  main
	

# compile

 
mod_handler.o: handler.cpp
	$(CXX) -c -fPIC $(JS_CFLAGS) $(JS_CPPFLAGS) -I. -I$(APXS_INCLUDEDIR) -I$(APR_INCLUDEDIR) -I${V8INC} -I${V8INC}/include  $(APXS_CFLAGS) -Wall -o $@ $< -std=c++0x 

 
mod_$(MODNAM).o: mod_$(MODNAM).c
	gcc -fPIC $(JS_CFLAGS) -DSHARED_MODULE -I. -I$(APXS_INCLUDEDIR) -I$(APR_INCLUDEDIR) $(APXS_CFLAGS) -o $@ -c $<

 
# link with g++ instead of apxs / gcc

mod_$(MODNAM).so: mod_$(MODNAM).o mod_handler.o
	$(CXX) $(JS_CFLAGS) $(JS_CPPFLAGS) -fPIC -shared -o $@ mod_$(MODNAM).o mod_handler.o $(APXS_LIBS_SHLIB) -lrt -ldl -lm -Wl,--start-group ${V8LDIR}/*.so -Wl,--end-group -pthread -std=c++0x 
#	cp ${V8BDIR}/*.bin .


hello-world : hello-world.cc
	@echo V8DIR=${V8DIR}
	@echo V8INC=${V8INC}
	@echo V8BDIR=${V8BDIR}
	@echo V8LDIR=${V8LDIR}
	g++ -I. -I${V8INC} -I${V8INC}/include  hello-world.cc -o hello-world -fpermissive -lrt -ldl -lm -Wl,--start-group ${V8LDIR}/*.so -Wl,--end-group -pthread -std=c++0x 



# clean intermediate files target

clean:
	rm -f *.o *.so *.slo *.lo *.la 


#-----


