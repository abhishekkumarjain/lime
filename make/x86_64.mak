#DEFS += -DVERSION=$(VERSION)

ifneq ($(findstring SYSTEMC,$(DEFS)),)
  SRC += $(SHARED)/sysc
  SRC += $(SC_IP)
else
  DEFS += -DTIMEOFDAY
  SRC += $(SHARED)/linux
endif

ifneq ($(findstring USE_MARGS,$(DEFS)),)
  DEFS += -DMARGS='"$(RUN_ARGS)"'
endif

ifneq ($(findstring M5,$(DEFS)),)
  SRC += $(SHARED)/m5
  MODULES += m5op_x86
endif

ifneq ($(findstring CLOCKS,$(DEFS)),)
  MODULES += clocks
endif

ifneq ($(filter %STATS %TRACE,$(DEFS)),)
  MODULES += monitor
endif

ifneq ($(filter $(NEED_DEVTREE),$(DEFS)),)
  ifeq ($(filter %SYSTEMC,$(DEFS)),)
    MODULES += devtree
  endif
endif

ifneq ($(filter $(NEED_STREAM),$(DEFS)),)
  # DEFS += -DUSE_SP -DUSE_OCM
  ifeq ($(filter %SYSTEMC,$(DEFS)),)
    SRC += $(SHARED)/linux/xil_com
    SRC += $(SHARED)/linux/xllfifo
    SRC += $(DRIVERS)
    MODULES += xil_cache xllfifo accmem
  endif
  MODULES += aport stream
endif

ifneq ($(findstring SYSTEMC,$(DEFS)),)
  ifneq ($(findstring HMCSIM,$(DEFS)),)
    # special case for proprietary hmcsim
    SRC += $(HOME)/work/hmcsim-2.3
    LDFLAGS += -L$(HOME)/work/hmcsim-2.3
    LDLIBS += -lhmcsim
    SCDIR ?= $(HOME)/src/systemc-2.3.0a
    LDFLAGS += -L$(SCDIR)/objdir/src/sysc/.libs
  else
    SCDIR ?= $(HOME)/src/systemc-2.3.2
    LDFLAGS += -L$(SCDIR)/objdir/src/.libs
    ifeq ($(findstring c++11,$(CXXFLAGS)),)
      CXXFLAGS += -std=c++11
    endif
  endif
  SRC += $(SCDIR)/src
  # squelch warning from SystemC sc_bit_proxies.h
  CFLAGS += -Wno-strict-overflow
  # application build -std=option must match SystemC library build
  LDLIBS += -lsystemc -lpthread
  LDFLAGS += -static
endif

OBJECTS = $(addsuffix .o,$(MODULES))
VPATH = $(subst ' ',:,$(SRC))

OPT ?= -O3
#OPT += -ftree-vectorize -ffast-math
ifdef OMP
  OPT += -fopenmp
endif
#MACH = -march=core2 -mfpmath=sse
CPPFLAGS += -MMD $(DEFS)
CPPFLAGS += $(patsubst %,-I%,$(SRC))
CFLAGS += $(MACH) $(OPT) -Wall
CXXFLAGS += $(CFLAGS)
#LDFLAGS += -static
#LDLIBS += -lrt

.PHONY: all
all: $(TARGET)

.PHONY: run
run: $(TARGET)
ifdef OMP
	OMP_NUM_THREADS=$(OMP) ./$(TARGET) $(RUN_ARGS)
else
	./$(TARGET) $(RUN_ARGS)
endif

.PHONY: clean
clean:
	$(RM) $(wildcard *.o) $(wildcard *.d) $(TARGET) makeflags

.PHONY: vars
vars:
	@echo TARGET: $(TARGET)
	@echo VERSION: $(VERSION)
	@echo DEFS: $(DEFS)
	@echo SRC: $(SRC)
	@echo OBJECTS: $(OBJECTS)
	@echo MAKEFILE_LIST: $(MAKEFILE_LIST)

$(TARGET): $(OBJECTS)
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(OBJECTS): $(MAKEFILE_LIST) # rebuild if MAKEFILEs change

$(OBJECTS): makeflags # rebuild if MAKEFLAGS change
# Select only command line variables
cvars = _$(strip $(foreach flag,$(MAKEFLAGS),$(if $(findstring =,$(flag)),$(flag),)))_
makeflags: FORCE
	@[ "$(if $(wildcard $@),$(shell cat $@),)" = "$(cvars)" ] || echo $(cvars)> $@
FORCE: ;

# Establish module specific dependencies
-include $(OBJECTS:.o=.d)
