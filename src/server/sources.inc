# -------------------------------------
# Windows Console 
# - Console Server Communications Layer
# -------------------------------------

# This module encapsulates the communication layer between the console driver
# and the console server application.
# All IOCTL driver communications are handled in this layer as well as management
# activities like process and handle tracking.

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
# Warning 4702: unreachable code
MSC_WARNING_LEVEL       = /W4 /WX /wd4201 /wd4702

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
    ..\ApiDispatchers.cpp \
    ..\ApiDispatchersInternal.cpp \
    ..\ApiMessage.cpp \
    ..\ApiMessageState.cpp \
    ..\ApiRoutinesBase.cpp \
    ..\ApiSorter.cpp \
    ..\ConsoleArguments.cpp \
    ..\DeviceComm.cpp \
    ..\DeviceHandle.cpp \
    ..\Entrypoints.cpp \
    ..\IoDispatchers.cpp \
    ..\IoSorter.cpp \
    ..\ObjectHandle.cpp \
    ..\ObjectHeader.cpp \
    ..\ProcessHandle.cpp \
    ..\ProcessList.cpp \
    ..\WaitBlock.cpp \
    ..\WaitQueue.cpp \
    ..\WinNTControl.cpp \

INCLUDES= \
    ..; \
    $(ONECORE_SDK_INC_PATH); \
    $(MINWIN_PRIV_SDK_INC_PATH); \
