MSC_WARNING_LEVEL=/W4 /WX

USE_STL=1
USE_MSVCRT=1

USE_DEFAULT_WIN32_LIBS=0
BUILD_FOR_CORESYSTEM=1

C_DEFINES=-DUNICODE -D_UNICODE

INCLUDES= \
    $(INCLUDES); \
    $(MINCORE_PRIV_SDK_INC_PATH); \
    $(MINWIN_PRIV_SDK_INC_PATH); \
    ..\..\parser; \
    ..; \

SOURCES= \
    $(SOURCES) \
    ..\adaptDispatch.cpp \
    ..\adaptDispatchGraphics.cpp \
    ..\terminalInput.cpp \
    ..\terminalOutput.cpp \
    ..\telemetry.cpp \
    ..\tracing.cpp \

TARGETLIBS= \
    $(TARGETLIBS) \
    $(ONECORE_SDK_LIB_VPATH)\onecore.lib \
    $(SDK_LIB_PATH)\user32.lib \


PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h


