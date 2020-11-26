OUT = asitest

CXXSRCS = \
	./main.cpp \
	./lib/asicamera.cpp \
	./lib/asicamerainfo.cpp \
	./lib/asicameracontrol.cpp \
	./fix.cpp

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

else ifeq ($(ARCH), aarch64)
LIBS += ./libasi/lib/armv8/libASICamera2.a

else
$(error Unknown architecture, please update the Makefile)

endif

LIBS += -Wl,--wrap=_ZN10CCameraFX314startAsyncXferEjjPiPbi

.PHONY: all
all: $(OUT)

$(OUT): $(CXXSRCS) $(CSRCS)
	$(CC) -o $@ $^ $(addprefix -I,$(INCPATH)) $(LIBS) -lpthread -lusb-1.0

.PHONY: run
run:
	./$(OUT)

.PHONY: test
test: purge all run

.PHONY: purge
purge:
	@rm -f $(OUT)
