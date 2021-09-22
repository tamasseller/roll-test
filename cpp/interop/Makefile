OUTPUT = rpc-cpp-interop-test

SOURCES += main.cpp
SOURCES += InteropTest.cpp
SOURCES += InteropListener.cpp
SOURCES += Tcp.cpp

INCLUDE_DIRS += ..

CXXFLAGS += -O0 -g3
CXXFLAGS += --std=c++17
CXXFLAGS += -fno-inline
CXXFLAGS += -fmax-errors=5 -ftemplate-backtrace-limit=100
CXXFLAGS += -Wall -Wpedantic -Werror -Wno-unused-value -Wreturn-local-addr
CXXFLAGS += 

LD=$(CXX)

LIBS += pthread

COVROOT = ..

include ../ultimate-makefile/Makefile.ultimate

RPCTOOL = ../../tool/rpc-tool

$(abspath InteropTest.h): $(abspath Contract.gen.h)

$(abspath Contract.gen.h): contract.rcd
	$(RPCTOOL) codegen -s -c -l c++ -i $< -o $@
