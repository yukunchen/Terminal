!include $(NTMAKEENV)\system_defaultmk.inc
!include $(WINCORE_PATH)\core.inc
SOURCES_USED                    =   $(WINCORE_PATH)\core.inc

TARGET_DESTINATION = retail

# C4201: nonstandard extension used : nameless struct/union
MSC_WARNING_LEVEL=/W4 /WX /wd4201

W32_SB = 1

USE_UNICRT=1
USE_MSVCRT=1
USE_STL=1
STL_VER=STL_VER_CURRENT
USE_NATIVE_EH=1

USE_DEFAULT_WIN32_LIBS=0
BUILD_FOR_CORESYSTEM=1

C_DEFINES=-DUNICODE -D_UNICODE -DBUILD_ONECORE_INTERACTIVITY

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
    ..\ApiDetector.cpp \
    ..\InteractivityFactory.cpp \
    ..\ServiceLocator.cpp \

TARGETLIBS= \
    $(TARGETLIBS) \
    $(ONECORE_SDK_LIB_VPATH)\onecore.lib \

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH                 =   precomp.pch
PRECOMPILED_OBJ                 =   precomp.obj



