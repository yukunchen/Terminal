# -------------------------------------
# Windows Console
# - Console Renderer Base
# -------------------------------------

# This module provides the base layer for all rendering activities.
# It will fetch data from the main console host server and prepare it
# in a rendering-engine-agnostic fashion so the console code
# can interface with displays on varying platforms.

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

# Check out https://osgwiki.com/wiki/Dev_14_Migration for more details about -Zc:implicitNoexcept-
USER_C_FLAGS=$(USER_C_FLAGS) -Zc:implicitNoexcept-

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
    ..\FontInfo.cpp \
    ..\FontInfoBase.cpp \
    ..\FontInfoDesired.cpp \
    ..\math.cpp \
    ..\renderer.cpp \
    ..\thread.cpp \

INCLUDES = \
    ..; \
    $(ONECORE_SDK_INC_PATH); \
    $(MINWIN_PRIV_SDK_INC_PATH); \
