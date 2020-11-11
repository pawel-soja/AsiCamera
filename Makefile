OUT = asitest

CXXSRCS = \
	./main.cpp \
	./lib/asicamera.cpp \
	./lib/asicamerainfo.cpp \
	./lib/asicameracontrol.cpp

INCPATH = \
	./lib \
	./libasi/include

LIBS = \
	-lpthread \
	-lusb-1.0

CC = g++
ARCH=$(shell uname -m)

ifeq ($(ARCH), x86_64)
LIBS += ./libasi/lib/x64/libASICamera2.a

else ifeq ($(ARCH), armv7l)
LIBS += ./libasi/lib/armv7/libASICamera2.a

else
$(error Unknown architecture, please update the Makefile)

endif

.PHONY: all
all: $(OUT)

$(OUT): $(CXXSRCS)
	$(CC) -o $@ $^ $(addprefix -I,$(INCPATH)) $(LIBS) -lpthread -lusb-1.0

.PHONY: run
run:
	./$(OUT)

.PHONY: test
test: purge all run

.PHONY: purge
purge:
	@rm -f $(OUT)
