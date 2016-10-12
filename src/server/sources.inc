# C4201: nonstandard extension used : nameless struct/union
# C4702: unreachable code detected : intentionally unreachable exit code during startup
MSC_WARNING_LEVEL=/W4 /WX /wd4201 /wd4702

USE_UNICRT=1
USE_MSVCRT=1
USE_STL=1
STL_VER=STL_VER_CURRENT
USE_NATIVE_EH=1

USE_DEFAULT_WIN32_LIBS=0
BUILD_FOR_CORESYSTEM=1

_NT_TARGET_VERSION              =   $(_NT_TARGET_VERSION_LATEST)

C_DEFINES=-DUNICODE -D_UNICODE

INCLUDES= \
    $(INCLUDES); \
    $(MINCORE_PRIV_SDK_INC_PATH); \
    $(MINWIN_PRIV_SDK_INC_PATH); \
    ..; \

SOURCES= \
    $(SOURCES) \
    ..\DeviceHandle.cpp \
    ..\Entrypoints.cpp \
    ..\WinNTControl.cpp \

TARGETLIBS= \
    $(TARGETLIBS) \
    $(ONECORE_SDK_LIB_VPATH)\onecore.lib \

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
