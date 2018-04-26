!include ..\..\..\project.inc

# -------------------------------------
# Windows Console
# - Console Output Buffer
# -------------------------------------

# This module encapsulates the objects used to manage
# the output buffer of the console

# -------------------------------------
# Compiler Settings
# -------------------------------------

# Warning 4201: nonstandard extension used: nameless struct/union
MSC_WARNING_LEVEL       = $(MSC_WARNING_LEVEL) /wd4201

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
    ..\AttrRow.cpp \
    ..\OutputCell.cpp \
    ..\Row.cpp \
    ..\TextAttribute.cpp \
    ..\TextAttributeRun.cpp \
    ..\textBuffer.cpp \
    ..\CharRow.cpp \

INCLUDES= \
    $(INCLUDES); \
    ..; \
