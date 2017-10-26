# -------------------------------------
# Windows Console 
# - Console Renderer for GDI
# -------------------------------------

# This module provides a rendering engine implementation that 
# utilizes the GDI framework for drawing the console to a window.

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

SOURCES = \
    ..\invalidate.cpp \
    ..\math.cpp \
    ..\paint.cpp \
    ..\state.cpp \
    ..\GdiCursor.cpp \

INCLUDES = \
    ..; \
    $(ONECORE_SDK_INC_PATH); \
    $(MINWIN_PRIV_SDK_INC_PATH); \
