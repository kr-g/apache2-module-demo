
V8DIR=${HOME}/repo/v8
V8INC=${V8DIR}
V8BDIR=${V8DIR}/out/x64.release
V8LDIR=${V8BDIR}/lib.target

BINDIR=${HOME}/repo/bin/v8

hello-world : hello-world.cc
	@echo V8DIR=${V8DIR}
	@echo V8INC=${V8INC}
	@echo V8BDIR=${V8BDIR}
	@echo V8LDIR=${V8LDIR}
	g++ -I. -I${V8INC} -I${V8INC}/include  hello-world.cc -o hello-world -lrt -ldl -lm -Wl,--start-group ${V8LDIR}/*.so -Wl,--end-group -pthread -std=c++0x 
	cp ${BINDIR}/*.bin .

.PHONY: clean
clean: 
	rm hello-world *.bin

