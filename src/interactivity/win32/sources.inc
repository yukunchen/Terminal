# C4201: nonstandard extension used : nameless struct/union
# C4702: unreachable code detected : intentionally unreachable exit code during startup
MSC_WARNING_LEVEL=/W4 /WX /wd4201 /wd4702

USE_DEFAULT_WIN32_LIBS=0
BUILD_FOR_CORESYSTEM=1

_NT_TARGET_VERSION              =   $(_NT_TARGET_VERSION_LATEST)

C_DEFINES=-DUNICODE -D_UNICODE -DFE_SB

INCLUDES= \
    $(INCLUDES); \
    $(MINCORE_PRIV_SDK_INC_PATH); \
    $(MINWIN_PRIV_SDK_INC_PATH); \
    ..; \

# Because we redefine new/delete in our library, we need to use CRTLIBS
# to arrange the linking order such that our new/delete is used before the one
# that is automatically introduced by the CRT.
# The CRT version is a weak link and will be ignored if it's already found to be
# defined by someone else.
# Anything defined here will be defined BEFORE the CRT items the build system adds.
CRTLIBS                         =   $(OBJECT_ROOT)\windows\core\console\open\src\host\lib\$(O)\conhostv2.lib \
                                    $(OBJECT_ROOT)\windows\core\console\open\src\tsf\$(O)\contsf.lib \
                                    $(OBJECT_ROOT)\windows\core\console\open\src\propslib\$(O)\conprops.lib \
                                    $(OBJECT_ROOT)\windows\core\console\open\src\interactivity\win32\lib\$(O)\coninteractivitywin32lib.lib \
                                    $(OBJECT_ROOT)\windows\core\console\open\src\interactivity\base\dll\$(O)\coninteractivitybase.lib \
                                    $(OBJECT_ROOT)\windows\core\console\open\src\terminal\adapter\lib\$(O)\ConTermAdapter.lib \
                                    $(OBJECT_ROOT)\windows\core\console\open\src\terminal\parser\lib\$(O)\ConTermParser.lib \
                                    $(OBJECT_ROOT)\windows\core\console\open\src\renderer\base\lib\$(O)\ConRenderBase.lib \
                                    $(OBJECT_ROOT)\windows\core\console\open\src\renderer\gdi\lib\$(O)\ConRenderGdi.lib \
                                    $(OBJECT_ROOT)\windows\core\console\open\src\server\lib\$(O)\ConServer.lib \
                                    $(CRTLIBS) \

# The LIB linking order is $(CRTLIBS) <Build System added refs to msvcrt.lib and friends> $(TARGETLIBS)

TARGETLIBS= \
    $(TARGETLIBS) \
    $(ONECORE_SDK_LIB_VPATH)\onecore.lib \
    $(ONECORE_PRIV_SDK_LIB_VPATH)\onecore_internal.lib \
    $(MINWIN_SDK_LIB_PATH)\ntdll.lib \
    $(MINCORE_PRIV_SDK_LIB_PATH)\isapisetpresent.lib \
    $(SDK_LIB_PATH)\winmm.lib \
    $(SDK_LIB_PATH)\propsys.lib \
    $(SDK_LIB_PATH)\shell32.lib \
    $(SDK_LIB_PATH)\uuid.lib \
    $(SDK_LIB_PATH)\user32.lib \
    $(SDK_LIB_PATH)\imm32.lib \
    $(SDK_LIB_PATH)\gdi32.lib \
    $(SDK_LIB_PATH)\uiautomationcore.lib \



TARGET_DESTINATION = retail

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h

USE_UNICRT=1
USE_MSVCRT=1
USE_STL=1
STL_VER=STL_VER_CURRENT
USE_NATIVE_EH=1
