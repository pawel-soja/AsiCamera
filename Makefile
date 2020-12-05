OUT = asitest

CXXSRCS = \
	./main.cpp

INCPATH = \
	./libasi/include \
	./

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

#debug
WRAP += memcpy

BOOST=y

ifeq ($(BOOST), y)

CXXSRCS += ./wrap.cpp
CXXSRCS += ./cameraboost.cpp
CXXSRCS += ./libusb-cpp/libusbbulktransfer.cpp
CXXSRCS += ./libusb-cpp/libusbchunkedbulktransfer.cpp

# boost
WRAP += _ZN6CirBuf8ReadBuffEPhii
WRAP += _ZN6CirBuf10InsertBuffEPhititiii
WRAP += _ZN10CCameraFX313initAsyncXferEiiihPh
WRAP += _ZN10CCameraFX314startAsyncXferEjjPiPbi
WRAP += _ZN10CCameraFX316releaseAsyncXferEv
WRAP += _ZN10CCameraFX311ResetDeviceEv
# relationship
WRAP += ASIOpenCamera
WRAP += _ZN11CCameraBase12InitVariableEv
WRAP += _ZN6CirBufC1El
WRAP += libusb_open
endif

comma = ,
LIBS += $(addprefix -Wl$(comma)--wrap=,$(WRAP))
.PHONY: all
all: $(OUT)

$(OUT): $(CXXSRCS) $(CSRCS)
	$(CC) -o $@ $^ $(addprefix -I,$(INCPATH)) $(LIBS) -O2

.PHONY: test
test: purge all run

.PHONY: purge
purge:
	@rm -f $(OUT)
