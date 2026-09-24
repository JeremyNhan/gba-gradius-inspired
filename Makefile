#---------------------------------------------------------------------------------------------------------------------
# Space Shooter — Game Boy Advance
#
#   make            release ROM  -> spaceshooter.gba
#   make DEBUG=1    debug ROM    -> spaceshooter_debug.gba (asserts, logging, debug overlay, test hooks)
#   make PROFILE=1  profile ROM  -> spaceshooter_profile.gba (release code + test hooks, for measurements)
#   make clean      (add DEBUG=1 to clean the debug build)
#
# The project path must not contain spaces (Butano/devkitARM limitation, see docs/research.md).
# On Windows use build.ps1, which maps the project to a temporary drive letter automatically.
#
# Variable meanings are documented in external/butano/template/Makefile.
#---------------------------------------------------------------------------------------------------------------------
# Butano re-invokes this makefile from inside $(BUILD); remember the project root once.
ifndef SS_ROOT
	export SS_ROOT := $(CURDIR)
endif

ifeq ($(DEBUG),1)
	TARGET      :=  spaceshooter_debug
	BUILD       :=  build_debug
	USERFLAGS   :=  -DSS_DEBUG=1
else ifeq ($(PROFILE),1)
	TARGET      :=  spaceshooter_profile
	BUILD       :=  build_profile
	USERFLAGS   :=  -DSS_DEBUG=0 -DSS_TEST_HOOKS=1 -DBN_CFG_ASSERT_ENABLED=false -DBN_CFG_LOG_ENABLED=false
else
	TARGET      :=  spaceshooter
	BUILD       :=  build
	USERFLAGS   :=  -DSS_DEBUG=0 -DBN_CFG_ASSERT_ENABLED=false -DBN_CFG_LOG_ENABLED=false
endif

LIBBUTANO       :=  external/butano/butano
PYTHON          ?=  python
SOURCES         :=  src src/core src/game src/data src/screens
INCLUDES        :=  src src/core src/game src/data src/screens
DATA            :=
GRAPHICS        :=  assets/generated/graphics
AUDIO           :=  assets/generated/audio
AUDIOBACKEND    :=  maxmod
AUDIOTOOL       :=
DMGAUDIO        :=
DMGAUDIOBACKEND :=  null
ROMTITLE        :=  SPACESHOOTER
ROMCODE         :=  SSHT
USERCXXFLAGS    :=
USERASFLAGS     :=
USERLDFLAGS     :=
USERLIBDIRS     :=
USERLIBS        :=
DEFAULTLIBS     :=
STACKTRACE      :=
USERBUILD       :=
EXTTOOL         :=  $(PYTHON) -B $(SS_ROOT)/tools/gen_assets.py --quiet

#---------------------------------------------------------------------------------------------------------------------
# Export absolute butano path:
#---------------------------------------------------------------------------------------------------------------------
ifndef LIBBUTANOABS
	export LIBBUTANOABS	:=	$(realpath $(LIBBUTANO))
endif

#---------------------------------------------------------------------------------------------------------------------
# Include main makefile:
#---------------------------------------------------------------------------------------------------------------------
include $(LIBBUTANOABS)/butano.mak
