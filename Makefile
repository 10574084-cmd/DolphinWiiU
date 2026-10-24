#-------------------------------------------------------------------------------
# DolphinWiiU - Makefile para Wii U (devkitPPC + wut)
#
#   make          genera DolphinWiiU.rpx
#   make clean
#
# Las pruebas de la logica portable (en Linux) estan en tests/: make -C tests run
#-------------------------------------------------------------------------------
.SUFFIXES:

ifeq ($(strip $(DEVKITPRO)),)
$(error "Define DEVKITPRO (por ejemplo: export DEVKITPRO=/opt/devkitpro)")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/wut/share/wut_rules

TARGET   := DolphinWiiU
BUILD    := build
SOURCES  := src/core src/ui src/wiiu
DATA     :=
INCLUDES := src

#-------------------------------------------------------------------------------
# opciones de compilacion
#-------------------------------------------------------------------------------
CFLAGS   := -g -Wall -O2 -ffunction-sections $(MACHDEP)
CFLAGS   += $(INCLUDE) -D__WIIU__ -D__WUT__
CXXFLAGS := $(CFLAGS) -std=c++17
ASFLAGS  := -g $(ARCH)
LDFLAGS   = -g $(ARCH) $(RPXSPECS) -Wl,-Map,$(notdir $*.map)

LIBS     := -lwut
LIBDIRS  := $(PORTLIBS) $(WUT_ROOT)

#-------------------------------------------------------------------------------
# no hace falta tocar nada de aqui para abajo
#-------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)

export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

export LD := $(CXX)
export OFILES_BIN := $(addsuffix .o,$(BINFILES))
export OFILES_SRC := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES := $(OFILES_BIN) $(OFILES_SRC)
export HFILES := $(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(CURDIR)/$(BUILD)

export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean all

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).rpx $(TARGET).elf

else

DEPENDS := $(OFILES:.o=.d)

all: $(OUTPUT).rpx
$(OUTPUT).rpx: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

$(OFILES_SRC): $(HFILES_BIN)

-include $(DEPENDS)

endif
