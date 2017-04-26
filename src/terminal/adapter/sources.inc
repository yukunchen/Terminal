# -------------------------------------
# Windows Console 
# - Console Virtual Terminal Adapter
# -------------------------------------

# This module converts Virtual Terminal style actions into 
# class Win32 API calls back into the console host application.
# In conjunction with the parser module, this allows APIs to be called
# simply using the STDOUT stream.

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

SOURCES= \
    ..\adaptDispatch.cpp \
    ..\adaptDispatchGraphics.cpp \
    ..\MouseInput.cpp \
    ..\terminalInput.cpp \
    ..\terminalOutput.cpp \
    ..\telemetry.cpp \
    ..\tracing.cpp \

INCLUDES = \
    ..; \
    ..\..\parser; \
    $(ONECORE_SDK_INC_PATH); \
    $(MINWIN_PRIV_SDK_INC_PATH); \

TARGETLIBS= \
    $(ONECORE_SDK_LIB_VPATH)\onecore.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-ntuser-keyboard-l1.lib \

DLOAD_ERROR_HANDLER     = kernelbase

DELAYLOAD = \
    ext-ms-win-ntuser-keyboard-l1.dll; \
