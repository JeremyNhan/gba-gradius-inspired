#---------------------------------------------------------------------------------------------------------------------
# Space Shooter - Game Boy Advance (C, libtonc + Maxmod, devkitARM)
#
#   make            release ROM -> spaceshooter.gba
#   make DEBUG=1    debug ROM   -> spaceshooter_debug.gba (asserts, debug overlay, stage select)
#   make TESTS=1    test ROM    -> spaceshooter_test.gba (release code + automated test scenarios)
#   make clean      (add DEBUG=1 / TESTS=1 to clean those builds)
#
# The asset generator (tools/assetgen, host C) runs first and writes assets/generated/ (graphics as
# C data, music/effects for mmutil). The project path must not contain spaces (devkitPro make
# rules); on Windows use build.ps1, which maps the project to a temporary drive letter.
#---------------------------------------------------------------------------------------------------------------------
.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/gba_rules

#---------------------------------------------------------------------------------------------------------------------
# Build variants
#---------------------------------------------------------------------------------------------------------------------
ifeq ($(DEBUG),1)
	TARGET      :=  spaceshooter_debug
	BUILD       :=  build_debug
	VARIANT     :=  -DSS_DEBUG=1
	TESTDIR     :=
else ifeq ($(TESTS),1)
	TARGET      :=  spaceshooter_test
	BUILD       :=  build_test
	VARIANT     :=  -DSS_TESTS=1
	TESTDIR     :=  src/test
else
	TARGET      :=  spaceshooter
	BUILD       :=  build
	VARIANT     :=
	TESTDIR     :=
endif

GENERATED   :=  assets/generated
SOURCES     :=  src src/core src/data src/game src/screens $(TESTDIR) $(GENERATED)
INCLUDES    :=  src src/core src/data src/game src/screens src/test $(GENERATED)
MUSIC       :=  $(GENERATED)/audio

GAME_TITLE  :=  SPACESHOOTER
GAME_CODE   :=  SSHT
MAKER_CODE  :=  00

#---------------------------------------------------------------------------------------------------------------------
# Code generation
#---------------------------------------------------------------------------------------------------------------------
ARCH    :=  -mthumb -mthumb-interwork

CFLAGS  :=  -g -Wall -Wextra -Wno-unused-parameter -O2 -std=gnu11 \
            -mcpu=arm7tdmi -mtune=arm7tdmi -fno-strict-aliasing \
            $(ARCH) $(VARIANT)

CFLAGS  +=  $(INCLUDE)

ASFLAGS :=  -g $(ARCH)
LDFLAGS  =  -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS    :=  -lmm -ltonc
LIBDIRS :=  $(DEVKITPRO)/libtonc $(LIBGBA)

#---------------------------------------------------------------------------------------------------------------------
# Host asset generator
#---------------------------------------------------------------------------------------------------------------------
HOSTCC      ?=  gcc
ifeq ($(OS),Windows_NT)
	EXE     :=  .exe
endif
ASSETGEN    :=  tools/assetgen/bin/assetgen$(EXE)
ASSETGEN_SRC := $(wildcard tools/assetgen/*.c) $(wildcard tools/assetgen/*.h)

ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------------------------------------------
# Top level: generate assets, then compile in a second pass (so the wildcards below see them).
#---------------------------------------------------------------------------------------------------------------------
export OUTPUT   :=  $(CURDIR)/$(TARGET)
export VPATH    :=  $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR  :=  $(CURDIR)/$(BUILD)

CFILES      :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
SFILES      :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

export AUDIOFILES   :=  $(foreach f,$(notdir $(wildcard $(MUSIC)/*.*)),$(CURDIR)/$(MUSIC)/$(f))
BINFILES    :=  soundbank.bin

export LD           :=  $(CC)
export OFILES_BIN   :=  $(addsuffix .o,$(BINFILES))
export OFILES_SOURCES := $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES       :=  $(OFILES_BIN) $(OFILES_SOURCES)
export HFILES       :=  $(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE  :=  $(foreach dir,$(INCLUDES),-iquote $(CURDIR)/$(dir)) \
                    $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                    -I$(CURDIR)/$(BUILD)

export LIBPATHS :=  $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: all assets compile clean

all: assets
	@$(MAKE) --no-print-directory -f $(CURDIR)/Makefile compile

$(ASSETGEN): $(ASSETGEN_SRC)
	@mkdir -p $(dir $@)
	@echo building assetgen
	@$(HOSTCC) -std=c99 -O2 -Wall -Wextra -o $@ $(filter %.c,$^) -lm

assets: $(ASSETGEN)
	@$(ASSETGEN) $(GENERATED)

compile:
	@[ -d $(BUILD) ] || mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).gba $(TARGET).elf.map

#---------------------------------------------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------------------------------------------
# Inside $(BUILD)
#---------------------------------------------------------------------------------------------------------------------
$(OUTPUT).gba   :   $(OUTPUT).elf

$(OUTPUT).elf   :   $(OFILES)

$(OFILES_SOURCES) : $(HFILES)

soundbank.bin soundbank.h : $(AUDIOFILES)
	@mmutil $^ -osoundbank.bin -hsoundbank.h

%.bin.o %_bin.h :   %.bin
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPSDIR)/*.d

#---------------------------------------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------------------------------------
