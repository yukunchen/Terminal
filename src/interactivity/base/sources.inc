# -------------------------------------
# Windows Console 
# - Console Interactivity Base Layer
# -------------------------------------

# This module defines the interfaces by which the console will interact
# with a user. This includes unifying various input methods into a single
# abstract method of talking to the console.

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

# Check out https://osgwiki.com/wiki/Dev_14_Migration for more details about -Zc:implicitNoexcept-
USER_C_FLAGS=$(USER_C_FLAGS) -Zc:implicitNoexcept-

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
    ..\ApiDetector.cpp \
    ..\InteractivityFactory.cpp \
    ..\ServiceLocator.cpp \

INCLUDES = \
    ..; \
    $(ONECORE_SDK_INC_PATH); \
    $(MINWIN_PRIV_SDK_INC_PATH); \
