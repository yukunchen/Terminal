# -------------------------------------
# Windows Console 
# - Console Interactivity for OneCore
# -------------------------------------

# This module defines interaction with the user on 
# system configurations with minimal input/output support

# -------------------------------------
# CRT Configuration
# -------------------------------------

USE_UNICRT              = 1
USE_MSVCRT              = 1

USE_STL                 = 1
STL_VER                 = STL_VER_CURRENT
USE_NATIVE_EH           = 1

# -------------------------------------
# Preprocessor Settings
# -------------------------------------

UNICODE                 = 1
C_DEFINES               = $(C_DEFINES) -DUNICODE -D_UNICODE

# -------------------------------------
# Compiler Settings
# -------------------------------------

# Warning 4201: nonstandard extension used: nameless struct/union
MSC_WARNING_LEVEL       = /W4 /WX /wd4201

# -------------------------------------
# Build System Settings
# -------------------------------------

# Code in the OneCore depot automatically excludes default Win32 libraries.

# -------------------------------------
# Sources, Headers, and Libraries
# -------------------------------------

PRECOMPILED_CXX         = 1
PRECOMPILED_INCLUDE     = ..\precomp.h

SOURCES = \
    ..\AccessibilityNotifier.cpp \
    ..\BgfxEngine.cpp \
    ..\ConsoleControl.cpp \
    ..\ConsoleInputThread.cpp \
    ..\ConsoleWindow.cpp \
    ..\InputServer.cpp \
    ..\SystemConfigurationProvider.cpp \
    ..\WindowMetrics.cpp \

INCLUDES = \
    ..; \
    ..\..\..\..\..\ConIoSrv; \
    $(ONECORE_SDK_INC_PATH); \
    $(MINWIN_PRIV_SDK_INC_PATH); \
