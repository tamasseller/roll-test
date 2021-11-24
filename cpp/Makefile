OUTPUT = rpc-cpp-test

SOURCES += main.cpp

SOURCES += TestCTStr.cpp
SOURCES += TestSignatureGeneration.cpp
SOURCES += TestSerDes.cpp
SOURCES += TestEndpoint.cpp

SOURCES += pet/ubiquitous/PrintfWriter.cpp

INCLUDE_DIRS += .
INCLUDE_DIRS += ..

CXXFLAGS += -O0 -g3
CXXFLAGS += --std=c++17
CXXFLAGS += -fno-inline
CXXFLAGS += -fmax-errors=5
CXXFLAGS += -Wall -Wpedantic -Werror -Wno-unused-value -Wreturn-local-addr

ifeq (_,)
CXXFLAGS += -fsanitize=address
LIBS += asan
CXXFLAGS += -fsanitize=undefined
LIBS += ubsan
endif

CXXFLAGS += -rdynamic
CXXFLAGS += --coverage
LDFLAGS += -rdynamic
LIBS += gcov 

LD=$(CXX)

COVROOT = ../..
COVFLAGS := -e $(dir $(abspath $(lastword $(MAKEFILE_LIST)))).*

include ../../cpp/mod.mk
include pet/mod.mk
include ultimate-makefile/Makefile.ultimate