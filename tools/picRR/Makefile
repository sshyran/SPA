LEVEL = ../..
TOOLNAME = spa-picRR

include $(LEVEL)/Makefile.config

include $(LEVEL)/Makefile.config

USEDLIBS = spa.a kleeCore.a spaRuntime.a kleeBasic.a kleeModule.a  kleaverSolver.a kleaverExpr.a kleeSupport.a
LINK_COMPONENTS = jit bitreader bitwriter ipo linker engine

ifeq ($(shell echo "$(LLVM_VERSION_MAJOR).$(LLVM_VERSION_MINOR) >= 3.3" | bc), 1)
LINK_COMPONENTS += irreader
endif
include $(LEVEL)/Makefile.common

LIBS += -lstp

ifeq ($(ENABLE_METASMT),1)
  include $(METASMT_ROOT)/share/metaSMT/metaSMT.makefile
  LD.Flags += -L$(METASMT_ROOT)/../../deps/Z3-4.1/lib \
              -L$(METASMT_ROOT)/../../deps/boolector-1.5.118/lib \
              -L$(METASMT_ROOT)/../../deps/minisat-git/lib/ \
              -L$(METASMT_ROOT)/../../deps/boost-1_52_0/lib 
  CXX.Flags += -DBOOST_HAS_GCC_TR1
  CXX.Flags := $(filter-out -fno-exceptions,$(CXX.Flags)) 
  LIBS += -lrt -lgomp -lboost_iostreams -lboost_thread -lboost_system -lmetaSMT -lz3 -lboolector -lminisat_core
endif

ifeq ($(STP_NEEDS_BOOST),1)
	LIBS += $(UPSTREAM_STP_LINK_FLAGS)
endif
