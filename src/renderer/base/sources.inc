MSC_WARNING_LEVEL=/W4 /WX

USE_UNICRT=1
USE_MSVCRT=1
USE_STL=1
STL_VER=STL_VER_CURRENT
USE_NATIVE_EH=1

USE_DEFAULT_WIN32_LIBS=0
BUILD_FOR_CORESYSTEM=1

C_DEFINES=-DUNICODE -D_UNICODE

INCLUDES= \
    $(INCLUDES); \
    ..; \
    ..\..\inc; \
    ..\..\..\inc; \
    ..\..\..\host; \
    $(MINCORE_PRIV_SDK_INC_PATH); \
    $(MINWIN_PRIV_SDK_INC_PATH); \

SOURCES= \
    $(SOURCES) \
    ..\FontInfo.cpp \
    ..\math.cpp \
    ..\renderer.cpp \
    ..\thread.cpp \

TARGETLIBS= \
    $(TARGETLIBS) \
    $(ONECORE_SDK_LIB_VPATH)\onecore.lib \

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h


