!include ..\..\project.inc

# -------------------------------------
# Windows Console
# - Console Types Library
# -------------------------------------

# This module encapsulates types and helpers that are common
# across the entire console project

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
    ..\IInputEvent.cpp \
    ..\FocusEvent.cpp \
    ..\KeyEvent.cpp \
    ..\MenuEvent.cpp \
    ..\ModifierKeyState.cpp \
    ..\MouseEvent.cpp \
    ..\Viewport.cpp \
    ..\WindowBufferSizeEvent.cpp \
    ..\convert.cpp \

INCLUDES= \
    $(INCLUDES); \
    ..; \
    