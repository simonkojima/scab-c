ifeq ($(OS),Windows_NT)
    GOOS := windows
    GOMAKEFILE := make-windows.mk
else
    UNAME_OS := $(shell uname -s)
    ifeq ($(UNAME_OS),Linux)
        GOOS := linux
        GOMAKEFILE := make-linux.mk
    else ifeq ($(UNAME_OS),Darwin)
        GOOS := darwin
        GOMAKEFILE := make-macos.mk
    else
        GOOS := unknown
        GOMAKEFILE := make-unknown.mk
        echo "Error: OS unknown"
        exit 1
    endif
endif

# set architecture
UNAME_MACHINE:=$(shell uname -m)
ifeq ($(UNAME_MACHINE),x86_64)
    export GOARCH := amd64
else ifeq (${UNAME_MACHINE},i686)
    export GOARCH := 386
else ifeq (${UNAME_MACHINE},aarch64)
    export GOARCH := arm64
else
    export GOARCH := unknown
endif

.PHONY: env
env:
    ${MAKE} -f ${GOMAKEFILE}

CC  = g++
CFLAGS    =
TARGET  = main	
SRCS    = main.cpp
OBJS    = $(SRCS:.cpp=.o)
 

INCDIR  = -I../inc 
LIBDIR  = -L../libs
LIBS = -lwsock32 -lws2_32
LIB_PORTAUDIO = -lportaudio_x64

MACROS = -DPORT=49152 -DHEADER_LENGTH=64 -DBUFFER_LENGTH=12880

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBDIR) $(LIBS)
	

$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) $(INCDIR) -c $(SRCS)


all: clean $(OBJS) $(TARGET)

notrigger: 
	$(CC) $(SRCS) $(CFLAGS) $(INCDIR) $(LIBDIR) $(LIB_PORTAUDIO) $(LIBS) -o ../build/scab-notrigger.exe $(MACROS) -DENTRIG=0
lsl:
	$(CC) $(SRCS) $(CFLAGS) $(INCDIR) $(LIBDIR) $(LIB_PORTAUDIO) $(LIBS) -llsl -o ../build/scab-lsl.exe $(MACROS) -DENTRIG=1 -DTRIG_DEV=2
nidaq:
	$(CC) $(SRCS) $(CFLAGS) $(INCDIR) $(LIBDIR) $(LIB_PORTAUDIO) $(LIBS) -lNIDAQmx -o ../build/scab-nidaq.exe $(MACROS) -DENTRIG=1 -DTRIG_DEV=0