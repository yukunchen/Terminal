!include $(NTMAKEENV)\system_defaultmk.inc
!include $(WINCORE_PATH)\core.inc
SOURCES_USED                    =   $(WINCORE_PATH)\core.inc

# C4201: nonstandard extension used : nameless struct/union
MSC_WARNING_LEVEL               =   /W4 /WX /wd4201

W32_SB                          =   1

USE_DEFAULT_WIN32_LIBS          =   0
BUILD_FOR_CORESYSTEM            =   1

_NT_TARGET_VERSION              =   $(_NT_TARGET_VERSION_LATEST)

C_DEFINES                       =   $(C_DEFINES) -DUNICODE -D_UNICODE -DFE_IME

CON_ROOT                        =   $(WINCORE_PATH)\console

INCLUDES                        =   $(INCLUDES) \
                                    ..; \
                                    ..\..\inc; \
                                    ..\..\propsheet; \
                                    ..\..\propslib; \
                                    ..\..\terminal\parser; \
                                    ..\..\terminal\adapter; \
                                    ..\..\renderer\inc; \
                                    ..\..\renderer\gdi; \
                                    ..\..\renderer\base; \
                                    $(ONECOREUAPWINDOWS_PRIVATE_INC_PATH)\core\w32inc; \
                                    $(SHELL_INC_PATH); \
                                    $(INTERNAL_SDK_INC_PATH); \
                                    $(MINCORE_PRIV_SDK_INC_PATH); \
                                    $(MINWIN_PRIV_SDK_INC_PATH)

# Because we redefine new/delete in our library, we need to use CRTLIBS
# to arrange the linking order such that our new/delete is used before the one
# that is automatically introduced by the CRT.
# The CRT version is a weak link and will be ignored if it's already found to be
# defined by someone else.
# Anything defined here will be defined BEFORE the CRT items the build system adds.
CRTLIBS                         =   $(OBJECT_ROOT)\windows\core\console\open\src\host\lib\$(O)\conhostv2.lib \
                                    $(CRTLIBS) \

# The LIB linking order is $(CRTLIBS) <Build System added refs to msvcrt.lib and friends> $(TARGETLIBS)

TARGETLIBS                      =   $(TARGETLIBS) \
                                    $(ONECORE_SDK_LIB_VPATH)\onecore.lib \
                                    $(ONECORE_PRIV_SDK_LIB_VPATH)\onecore_internal.lib \
                                    $(MINWIN_SDK_LIB_PATH)\ntdll.lib \
                                    $(MINCORE_PRIV_SDK_LIB_PATH)\isapisetpresent.lib \
                                    $(SDK_LIB_PATH)\winmm.lib \
                                    $(SDK_LIB_PATH)\propsys.lib \
                                    $(SDK_LIB_PATH)\shell32.lib \
                                    $(SDK_LIB_PATH)\uuid.lib \
                                    $(SDK_LIB_PATH)\gdi32.lib \
                                    $(SDK_LIB_PATH)\user32.lib \
                                    $(WINCORE_OBJ_PATH)\console\open\src\tsf\$(O)\contsf.lib \
                                    $(WINCORE_OBJ_PATH)\console\open\src\propslib\$(O)\conprops.lib \
                                    $(WINCORE_OBJ_PATH)\console\open\src\terminal\adapter\lib\$(O)\ConTermAdapter.lib \
                                    $(WINCORE_OBJ_PATH)\console\open\src\terminal\parser\lib\$(O)\ConTermParser.lib \
                                    $(WINCORE_OBJ_PATH)\console\open\src\renderer\base\lib\$(O)\ConRenderBase.lib \
                                    $(WINCORE_OBJ_PATH)\console\open\src\renderer\gdi\lib\$(O)\ConRenderGdi.lib \
                                    $(WINCORE_OBJ_PATH)\console\open\src\server\lib\$(O)\ConServer.lib \

DELAYLOAD                       =   \
                                    winmm.dll; \
                                    api-ms-win-shcore-scaling-l1.dll; \
                                    shell32.dll; \

DLOAD_ERROR_HANDLER             =   kernelbase

TARGET_DESTINATION              =   retail

PRECOMPILED_CXX                 =   1
PRECOMPILED_INCLUDE             =   ..\precomp.h
PRECOMPILED_PCH                 =   precomp.pch
PRECOMPILED_OBJ                 =   precomp.obj

USE_UNICRT                      =   1
USE_MSVCRT                      =   1

USE_STL                         =   1
STL_VER                         =   STL_VER_CURRENT
USE_NATIVE_EH                   =   1
