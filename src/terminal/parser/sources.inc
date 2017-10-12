# -------------------------------------
# Windows Console 
# - Console Virtual Terminal Parser
# -------------------------------------

# This module provides the ability to parse an incoming stream
# of text for Virtual Terminal Sequences. 
# These sequences are in-band signaling embedded within the stream
# that signals the console to do something special (beyond just displaying as text).

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
C_DEFINES               = $(C_DEFINES) -DUNICODE -D_UNICODE -DBUILD_ONECORE_INTERACTIVITY

# -------------------------------------
# Compiler Settings
# -------------------------------------

MSC_WARNING_LEVEL       = /W4 /WX 

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
    ..\stateMachine.cpp \
    ..\InputStateMachineEngine.cpp \
    ..\OutputStateMachineEngine.cpp \
    ..\telemetry.cpp \
    ..\tracing.cpp \

INCLUDES = \
    ..; \
    $(ONECORE_SDK_INC_PATH); \
    $(MINWIN_PRIV_SDK_INC_PATH); \

TARGETLIBS = \
    $(ONECORE_SDK_LIB_VPATH)\onecore.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-ntuser-keyboard-l1.lib \

DLOAD_ERROR_HANDLER     = kernelbase

DELAYLOAD = \
    ext-ms-win-ntuser-keyboard-l1.dll; \

