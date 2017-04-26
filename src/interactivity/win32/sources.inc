# -------------------------------------
# Windows Console 
# - Console Interactivity for Win32
# -------------------------------------

# This module provides user interaction with the standard
# windowing and input system used by classic Win32 platforms.

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
C_DEFINES               = $(C_DEFINES) -DUNICODE -D_UNICODE -DFE_SB

# -------------------------------------
# Compiler Settings
# -------------------------------------

# Warning 4201: nonstandard extension used: nameless struct/union
MSC_WARNING_LEVEL       = /W4 /WX /wd4201

# -------------------------------------
# Build System Settings
# -------------------------------------

# Code in the OneCore depot automatically excludes default Win32 libraries.

# Defines IME and Codepage support
W32_SB                  = 1

# -------------------------------------
# Sources, Headers, and Libraries
# -------------------------------------

PRECOMPILED_CXX         = 1
PRECOMPILED_INCLUDE     = ..\precomp.h

SOURCES = \
    ..\AccessibilityNotifier.cpp \
    ..\clipboard.cpp \
    ..\ConsoleControl.cpp \
    ..\ConsoleInputThread.cpp \
    ..\consoleKeyInfo.cpp \
    ..\find.cpp \
    ..\icon.cpp \
    ..\InputServices.cpp \
    ..\menu.cpp \
    ..\screenInfoUiaProvider.cpp \
    ..\SystemConfigurationProvider.cpp \
    ..\window.cpp \
    ..\windowdpiapi.cpp \
    ..\windowime.cpp \
    ..\windowio.cpp \
    ..\WindowMetrics.cpp \
    ..\windowproc.cpp \
    ..\windowUiaProvider.cpp \

INCLUDES = \
    ..; \
    ..\..\..\inc; \
    $(ONECORE_SDK_INC_PATH); \
    $(MINWIN_PRIV_SDK_INC_PATH); \

TARGETLIBS = \
    $(ONECORE_SDK_LIB_VPATH)\onecore.lib \
