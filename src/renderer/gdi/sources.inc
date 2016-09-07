MSC_WARNING_LEVEL=/W4 /WX

USE_STL=1
USE_MSVCRT=1

USE_DEFAULT_WIN32_LIBS=0
BUILD_FOR_CORESYSTEM=1

C_DEFINES=-DUNICODE -D_UNICODE

INCLUDES= \
    $(INCLUDES); \
    ..; \
    ..\..\inc; \
    ..\..\..\host; \
    $(MINCORE_PRIV_SDK_INC_PATH); \
    $(MINWIN_PRIV_SDK_INC_PATH); \

SOURCES= \
    $(SOURCES) \
    ..\invalidate.cpp \
    ..\math.cpp \
    ..\paint.cpp \
    ..\state.cpp \

TARGETLIBS= \
    $(TARGETLIBS) \
    $(WINCORE_OBJ_PATH)\console\open\src\renderer\base\lib\$(O)\ConRenderBase.lib \
    $(ONECORE_SDK_LIB_VPATH)\onecore.lib \


PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h


