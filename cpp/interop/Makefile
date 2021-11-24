OUTPUT = rpc-cpp-interop-test

SOURCES += main.cpp
SOURCES += InteropTest.cpp
SOURCES += InteropListener.cpp
SOURCES += Tcp.cpp

GENDIR = .g
CLEAN_EXTRA += $(GENDIR) 

INCLUDE_DIRS += ..
INCLUDE_DIRS += $(GENDIR)

CXXFLAGS += -O0 -g3
CXXFLAGS += --std=c++17
CXXFLAGS += -fno-inline
CXXFLAGS += -fmax-errors=5 -ftemplate-backtrace-limit=100
CXXFLAGS += -Wall -Wpedantic -Werror -Wno-unused-value -Wreturn-local-addr
CXXFLAGS += 

LD=$(CXX)

LIBS += pthread
CXXFLAGS += -rdynamic
CXXFLAGS += --coverage
LDFLAGS += -rdynamic
LIBS += gcov 

COVROOT = ../../..

%.cpp: $(abspath $(GENDIR)/Contract.gen.h) ;

RPCTOOL = $(shell command -v roll-contract-tool 2>/dev/null)
ifeq ($(RPCTOOL),)
$(error "you need to have roll-contract-tool installed for building this")
endif

$(abspath $(GENDIR)/Contract.gen.h): contract.rcd $(MAKEFILE_LIST)
	@$(call print_extra_step,codegen $<)
	$Smkdir -p $(dir $@)
	$S$(RPCTOOL) codegen -s -c -l c++ -i $< -o $@

include ../ultimate-makefile/Makefile.ultimate
include ../../../cpp/mod.mk