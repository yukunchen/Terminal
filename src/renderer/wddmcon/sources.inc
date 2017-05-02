# -------------------------------------
# Windows Console 
# - Console Renderer for WDDMCON
# -------------------------------------

# This module provides a rendering engine implementation that 
# utilizes the WDDMCON library for drawing the console to a fullscreen
# DirectX surface.

# -------------------------------------
# CRT Configuration
# -------------------------------------

USE_UNICRT              = 1
USE_MSVCRT              = 1

USE_STL                 = 1
STL_VER                 = STL_VER_CURRENT
USE_NATIVE_EH           = 1

BUILD_FOR_CORESYSTEM    = 1

# -------------------------------------
# Preprocessor Settings
# -------------------------------------

C_DEFINES               = -DUNICODE -D_UNICODE

# -------------------------------------
# Compiler Settings
# -------------------------------------

MSC_WARNING_LEVEL       = /W4 /WX

# -------------------------------------
# Sources, Headers, and Libraries
# -------------------------------------

PRECOMPILED_CXX         = 1
PRECOMPILED_INCLUDE     = ..\precomp.h

INCLUDES = \
    $(INCLUDES); \
    ..; \
    ..\..\inc; \
    ..\..\..\inc; \
    ..\..\..\host; \
    $(MINCORE_PRIV_SDK_INC_PATH); \
    $(MINWIN_PRIV_SDK_INC_PATH); \

SOURCES = \
    $(SOURCES) \
    ..\WddmConRenderer.cpp \
