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

.PHONY: clean
clean:
    ${MAKE} -f ${GOMAKEFILE} go-clean

.PHONY: build
build:
    $(MAKE) -f  $(GOMAKEFILE) go-build