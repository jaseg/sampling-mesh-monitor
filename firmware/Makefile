
########################################################################################################################
# Dependency directories
########################################################################################################################

MUSL_DIR			?= upstream/musl

########################################################################################################################
# Algorithm parameters
########################################################################################################################

# - none -

########################################################################################################################
# High-level build parameters
########################################################################################################################

DEBUG			?= 1
OPT				?= 0

BUILDDIR 		?= build
BINARY 			:= main.elf
LDSCRIPT 		:= generic_stm32.ld
DEVICE 			:= STM32G474RB


########################################################################################################################
# Sources
########################################################################################################################

ASM_SOURCES 		:= startup.s

C_SOURCES		 	:= src/main.c

CPP_SOURCES			:= # - none -

MUSL_SOURCES		:= # - none -
MUSL_SOURCES		:= $(addprefix $(MUSL_DIR)/src/,$(MUSL_SOURCES))

C_SOURCES += $(MUSL_SOURCES)


########################################################################################################################
# Low-level build parameters
########################################################################################################################

PREFIX		?= arm-none-eabi-

HOSTCC		:= gcc
CC			:= $(PREFIX)gcc
CPP			:= $(PREFIX)cpp
CXX			:= $(PREFIX)g++
LD			:= $(PREFIX)gcc
AR			:= $(PREFIX)ar
AS			:= $(PREFIX)as
SIZE		:= $(PREFIX)size
NM			:= $(PREFIX)nm
OBJCOPY		:= $(PREFIX)objcopy
OBJDUMP		:= $(PREFIX)objdump
GDB			:= $(PREFIX)gdb

HOST_CC				?= $(HOST_PREFIX)gcc
HOST_CXX			?= $(HOST_PREFIX)g++
HOST_LD				?= $(HOST_PREFIX)gcc
HOST_AR				?= $(HOST_PREFIX)ar
HOST_AS				?= $(HOST_PREFIX)as
HOST_OBJCOPY		?= $(HOST_PREFIX)objcopy
HOST_OBJDUMP		?= $(HOST_PREFIX)objdump

PYTHON3 ?= python3
DOT		?= dot

MUSL_DIR_ABS			:= $(abspath $(MUSL_DIR))

DEVICE_FAMILY	:= $(shell echo $(DEVICE) | grep -Eio 'STM32[a-z]{1,2}[0-9]'|cut -c 6-)
DEVICE_DEFINES 	:= -DSTM32$(DEVICE_FAMILY) $(addprefix -D,$(shell cat stm32_buildinfo.defines))

ARCH_FLAGS		?= -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
SYSTEM_FLAGS	?= -nostdlib -ffreestanding -nostartfiles

COMMON_CFLAGS	+= -I$(abspath include)
COMMON_CFLAGS	+= -I$(BUILDDIR)

CFLAGS			+= -I$(abspath tools/musl_include_shims)

CFLAGS			+= $(ARCH_FLAGS) $(SYSTEM_FLAGS)
CFLAGS			+= -fno-common -ffunction-sections -fdata-sections

COMMON_CFLAGS	+= -O$(OPT) -std=gnu2x -g
COMMON_CFLAGS 	+= $(DEVICE_DEFINES)
COMMON_CFLAGS 	+= -DDEBUG=$(DEBUG)

HOST_CFLAGS		+= $(COMMON_CFLAGS)

# for musl
CFLAGS			+= -Dhidden=

SIM_CFLAGS	+= -lm -DSIMULATION
SIM_CFLAGS	+= -Wall -Wextra -Wpedantic -Wshadow -Wimplicit-function-declaration -Wundef -Wno-unused-parameter

INT_CFLAGS	+= -Wall -Wextra -Wpedantic -Wshadow -Wimplicit-function-declaration -Wundef -Wno-unused-parameter
INT_CFLAGS	+= -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes

CXXFLAGS	+= -Os -g
CXXFLAGS	+= $(ARCH_FLAGS) $(SYSTEM_FLAGS)
CXXFLAGS	+= -fno-common -ffunction-sections -fdata-sections
CXXFLAGS	+= -Wall -Wextra -Wshadow -Wundef -Wredundant-decls
CXXFLAGS	+= -I.

LDFLAGS			+= $(ARCH_FLAGS) $(SYSTEM_FLAGS)

LIBS		+= -lgcc
#LDFLAGS		+= -Wl,--gc-sections

LINKMEM_FLAGS ?= --trim-stubs=startup.o --trace-sections .isr_vector --highlight-subdirs $(BUILDDIR)

OBJS := $(addprefix $(BUILDDIR)/,$(C_SOURCES:.c=.o) $(CXX_SOURCES:.cpp=.o) $(ASM_SOURCES:.s=.o))
ALL_OBJS := $(OBJS)
ALL_OBJS += $(BUILDDIR)/system.o
# Add generated source here.

########################################################################################################################
# Rules
########################################################################################################################

all: binsize

.PHONY: binsize
binsize: $(BUILDDIR)/$(BINARY) $(BUILDDIR)/$(BINARY:.elf=-symbol-sizes.pdf)
	$(LD) -T$(LDSCRIPT) $(LDFLAGS) -Wl,--print-memory-usage -o /dev/null $(ALL_OBJS) $(LIBS)
	@echo
	@echo "▐▬▬▬▌ SyMbOL sIzE HiGhScORe LiSt ▐▬▬▬▌"
	$(NM) --print-size --size-sort --radix=d $< | tail -n 20

.PRECIOUS: $(BUILDDIR)/$(BINARY) 
$(BUILDDIR)/$(BINARY) $(BUILDDIR)/$(BINARY:.elf=.map) &: $(ALL_OBJS)
	$(LD) -T$(LDSCRIPT) $(LDFLAGS) -o $@ -Wl,-Map=$(BUILDDIR)/$(BINARY:.elf=.map) $(ALL_OBJS) $(LIBS)

build/$(BINARY:.elf=-symbol-sizes.dot): $(ALL_OBJS)
	$(PYTHON3) tools/linkmem.py $(LINKMEM_FLAGS) $(LD) -T$(LDSCRIPT) $(LDFLAGS) $^ $(LIBS) > $@

%.pdf: %.dot
	$(DOT) -T pdf $< -o $@

%.dot: %.elf
	r2 -a arm -qc 'aa;agRd' $< 2>/dev/null >$@

$(BUILDDIR)/src/%.o: src/%.s
	mkdir -p $(@D)
	$(CC) $(COMMON_CFLAGS) $(CFLAGS) $(INT_CFLAGS) -o $@ -c $<

$(BUILDDIR)/src/%.o: src/%.c
	mkdir -p $(@D)
	$(CC) $(COMMON_CFLAGS) $(CFLAGS) $(INT_CFLAGS) -o $@ -c $<

$(BUILDDIR)/src/%.o: src/%.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(BUILDDIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(COMMON_CFLAGS) $(CFLAGS) $(EXT_CFLAGS) -o $@ -c $<

$(BUILDDIR)/%.o: %.s
	mkdir -p $(@D)
	$(CC) $(COMMON_CFLAGS) $(CFLAGS) $(EXT_CFLAGS) -o $@ -c $<

venv:
	test -d venv || python3 -m venv --system-site-packages venv
	source venv/bin/activate && pip install cxxfilt pyelftools libarchive matplotlib

clean:
	rm -rf $(BUILDDIR)/src
	rm -f $(BUILDDIR)/**.o
	rm -f $(BUILDDIR)/$(BINARY)
	rm -f $(BUILDDIR)/$(BINARY:.elf=.map)
	rm -f $(BUILDDIR)/$(BINARY:.elf=-symbol-sizes.dot)
	rm -f $(BUILDDIR)/$(BINARY:.elf=-symbol-sizes.pdf)

mrproper: clean
	rm -rf build

.PHONY: clean mrproper

-include $(OBJS:.o=.d)
