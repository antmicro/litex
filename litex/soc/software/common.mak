ifeq ($(TRIPLE),--native--)
TARGET_PREFIX=
else
TARGET_PREFIX=$(TRIPLE)-
endif

RM ?= rm -f
PYTHON ?= python3

ifeq ($(CLANG),1)
CC_normal      := clang -target $(TRIPLE) -integrated-as
CX_normal      := clang++ -target $(TRIPLE) -integrated-as
else
CC_normal      := $(TARGET_PREFIX)gcc -std=gnu99
CX_normal      := $(TARGET_PREFIX)g++
endif
AR_normal      := $(TARGET_PREFIX)gcc-ar
LD_normal      := $(TARGET_PREFIX)ld
OBJCOPY_normal := $(TARGET_PREFIX)objcopy

CC_quiet      = @echo " CC      " $@ && $(CC_normal)
CX_quiet      = @echo " CX      " $@ && $(CX_normal)
AR_quiet      = @echo " AR      " $@ && $(AR_normal)
LD_quiet      = @echo " LD      " $@ && $(LD_normal)
OBJCOPY_quiet = @echo " OBJCOPY " $@ && $(OBJCOPY_normal)

ifeq ($(V),1)
	CC = $(CC_normal)
	CX = $(CX_normal)
	AR = $(AR_normal)
	LD = $(LD_normal)
	OBJCOPY = $(OBJCOPY_normal)
else
	CC = $(CC_quiet)
	CX = $(CX_quiet)
	AR = $(AR_quiet)
	LD = $(LD_quiet)
	OBJCOPY = $(OBJCOPY_quiet)
endif

# http://scottmcpeak.com/autodepend/autodepend.html
# Generate *.d Makefile dependencies fragments, include using;
# -include $(OBJECTS:.o=.d)
DEPFLAGS += -MD -MP

# Toolchain options
#
INCLUDES = -I$(SOC_DIRECTORY)/software/include/base -I$(SOC_DIRECTORY)/software/include -I$(SOC_DIRECTORY)/common -I$(BUILDINC_DIRECTORY)
COMMONFLAGS = $(DEPFLAGS) $(CPUFLAGS) -g3 -fomit-frame-pointer -Wall -fno-builtin -nostdinc $(INCLUDES) -flto -fuse-linker-plugin -O2 -ffunction-sections -fdata-sections -nostartfiles -nostdlib -nodefaultlibs -fno-align-functions -fno-align-jumps -fno-align-loops -fno-align-labels -fno-prefetch-loop-arrays -findirect-inlining -fira-region=one -fira-hoist-pressure -fno-tree-ch -freorder-blocks-algorithm=simple -mtune=size
CFLAGS = $(COMMONFLAGS) -fexceptions -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes
CXXFLAGS = $(COMMONFLAGS) -std=c++11 -I$(SOC_DIRECTORY)/software/include/basec++ -fexceptions -fno-rtti -ffreestanding
LDFLAGS = -nostdlib -nodefaultlibs -Os $(CPUFLAGS) -L$(BUILDINC_DIRECTORY)

define compilexx
$(CX) -c $(CXXFLAGS) $(1) $< -o $@
endef

define compile
$(CC) -c $(CFLAGS) $(1) $< -o $@
endef

define assemble
$(CC) -c $(CFLAGS) -o $@ $<
endef
