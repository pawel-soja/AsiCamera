CC=g++

OUT=asitest

CXXSRCS = \
	./main.cpp \
	./lib/asicamera.cpp \
	./lib/asicamerainfo.cpp \
	./lib/asicameracontrol.cpp

LIBS=./libasi/$(shell uname -m)/libASICamera2.bin
LD_LIBRARY_PATH=./libasi/$(shell uname -m)

.PHONY: all
all: $(OUT)

$(OUT): $(CXXSRCS)
	$(CC) -o $@ $^ -I./ -I./lib/ -I./libasi/ $(LIBS)

.PHONY: run
run:
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH) ./$(OUT)

.PHONY: test
test: purge all run

.PHONY: purge
purge:
	@rm -f $(OUT)

#!/bin/bash
#g++ -o main main.cpp asicameracontrol.cpp asicamerainfo.cpp asicamera.cpp -I ./ libASICamera2.bin  && LD_LIBRARY_PATH=./  ./main

