# -------------------------------------
# Windows Console
# - Console Host
# -------------------------------------

# The console host is the application that services all requests from a Win32
# console mode application. Since a console application has no visual representation
# on its own (it simply uses STDIN and STDOUT to process a text stream)
# the console host window provides a visual representation (output) and a means
# of capturing user-interaction (input) on behalf of the hosted application.

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
C_DEFINES               = $(C_DEFINES) -DUNICODE -D_UNICODE -DFE_IME

# -------------------------------------
# Compiler Settings
# -------------------------------------

# Warning 4201: nonstandard extension used: nameless struct/union
MSC_WARNING_LEVEL       = /W4 /WX /wd4201

# -------------------------------------
# Program Information
# -------------------------------------

TARGET_DESTINATION      = retail

# -------------------------------------
# Build System Settings
# -------------------------------------

# Code in the OneCore depot automatically excludes default Win32 libraries.

# Defines IME and Codepage support
W32_SB                  = 1


# -------------------------------------
# Sources, Headers, and Libraries
# -------------------------------------

SOURCES = \
    $(SOURCES) \
    ..\selection.cpp \
    ..\selectionInput.cpp \
    ..\selectionState.cpp \
    ..\scrolling.cpp \
    ..\cmdline.cpp   \
    ..\alias.cpp   \
    ..\history.cpp   \
    ..\VtIo.cpp   \
    ..\VtInputThread.cpp   \
    ..\PtySignalInputThread.cpp \
    ..\consoleInformation.cpp \
    ..\cursor.cpp    \
    ..\search.cpp    \
    ..\directio.cpp  \
    ..\getset.cpp    \
    ..\globals.cpp   \
    ..\handle.cpp    \
    ..\init.cpp      \
    ..\input.cpp     \
    ..\inputBuffer.cpp \
    ..\inputKeyInfo.cpp \
    ..\inputReadHandleData.cpp \
    ..\misc.cpp      \
    ..\output.cpp    \
    ..\srvinit.cpp   \
    ..\outputStream.cpp \
    ..\stream.cpp    \
    ..\dbcs.cpp      \
    ..\convarea.cpp  \
    ..\textBuffer.cpp \
    ..\screenInfo.cpp \
    ..\newdelete.cpp \
    ..\_output.cpp   \
    ..\_stream.cpp   \
    ..\utils.cpp     \
    ..\telemetry.cpp \
    ..\tracing.cpp   \
    ..\registry.cpp  \
    ..\settings.cpp  \
    ..\ntprivapi.cpp \
    ..\readData.cpp \
    ..\readDataCooked.cpp \
    ..\readDataDirect.cpp \
    ..\readDataRaw.cpp \
    ..\writeData.cpp \
    ..\renderData.cpp \
    ..\renderFontDefaults.cpp \
    ..\utf8ToWideCharParser.cpp \
    ..\conimeinfo.cpp \
    ..\conattrs.cpp \
    ..\KeyEventHelpers.cpp \
    ..\ConsoleArguments.cpp \
    ..\AttrRow.cpp \
    ..\CharRow.cpp \
    ..\Row.cpp \
    ..\TextAttribute.cpp \
    ..\TextAttributeRun.cpp \


# -------------------------------------
# Sources, Headers, and Libraries
# -------------------------------------

PRECOMPILED_CXX         = 1
PRECOMPILED_INCLUDE     = ..\precomp.h
PRECOMPILED_PCH         = precomp.pch
PRECOMPILED_OBJ         = precomp.obj

INCLUDES = \
    ..; \
    ..\..\inc; \
    ..\..\propsheet; \
    ..\..\propslib; \
    ..\..\terminal\parser; \
    ..\..\terminal\adapter; \
    ..\..\types; \
    ..\..\renderer\inc; \
    ..\..\renderer\gdi; \
    ..\..\renderer\vt; \
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
CRTLIBS = \
    $(WINCORE_OBJ_PATH)\console\open\src\host\lib\$(O)\conhostv2.lib \
    $(CRTLIBS) \

# The LIB linking order is $(CRTLIBS) <Build System added refs to msvcrt.lib and friends> $(TARGETLIBS)

TARGETLIBS = \
    $(TARGETLIBS) \
    $(INTERNAL_SDK_LIB_PATH)\onecoreuuid.lib \
    $(INTERNAL_SDK_LIB_PATH)\onecoreuapuuid.lib \
    $(ONECORE_PRIV_SDK_LIB_VPATH)\onecore_internal.lib \
    $(MINCORE_PRIV_SDK_LIB_PATH)\isapisetpresent.lib \
    $(SDK_LIB_PATH)\propsys.lib \
    $(MODERNCORE_PRIV_SDK_LIB_VPATH)\api-ms-win-mm-playsound-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-gdi-dc-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-gdi-dc-create-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-gdi-draw-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-gdi-font-l1.lib \
    $(ONECOREUAPWINDOWS_LIB_PATH)\ext-ms-win-gdi-internal-desktop-l1-1-0.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-ntuser-caret-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-ntuser-dialogbox-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-ntuser-draw-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-ntuser-keyboard-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-ntuser-gui-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-ntuser-menu-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-ntuser-message-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-ntuser-misc-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-ntuser-mouse-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-ntuser-rectangle-ext-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-ntuser-server-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-ntuser-sysparams-ext-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-ntuser-window-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-rtcore-gdi-object-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-rtcore-gdi-rgn-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-rtcore-ntuser-console-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-rtcore-ntuser-cursor-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-rtcore-ntuser-dc-access-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-rtcore-ntuser-rawinput-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-rtcore-ntuser-sysparams-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-rtcore-ntuser-window-ext-l1.lib \
    $(MINCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-shell-shell32-l1.lib \
    $(MODERNCORE_PRIV_SDK_LIB_VPATH)\ext-ms-win-uiacore-l1.lib \
    $(WINCORE_OBJ_PATH)\console\open\src\tsf\$(O)\contsf.lib \
    $(WINCORE_OBJ_PATH)\console\open\src\propslib\$(O)\conprops.lib \
    $(WINCORE_OBJ_PATH)\console\open\src\terminal\adapter\lib\$(O)\ConTermAdapter.lib \
    $(WINCORE_OBJ_PATH)\console\open\src\terminal\parser\lib\$(O)\ConTermParser.lib \
    $(WINCORE_OBJ_PATH)\console\open\src\renderer\base\lib\$(O)\ConRenderBase.lib \
    $(WINCORE_OBJ_PATH)\console\open\src\renderer\gdi\lib\$(O)\ConRenderGdi.lib \
    $(WINCORE_OBJ_PATH)\console\open\src\renderer\vt\lib\$(O)\ConRenderVt.lib \
    $(WINCORE_OBJ_PATH)\console\open\src\renderer\wddmcon\lib\$(O)\ConRenderWddmCon.lib \
    $(WINCORE_OBJ_PATH)\console\open\src\server\lib\$(O)\ConServer.lib \
    $(WINCORE_OBJ_PATH)\console\open\src\interactivity\base\lib\$(O)\ConInteractivityBaseLib.lib \
    $(WINCORE_OBJ_PATH)\console\open\src\interactivity\win32\lib\$(O)\ConInteractivityWin32Lib.lib \
    $(WINCORE_OBJ_PATH)\console\open\src\interactivity\onecore\lib\$(O)\ConInteractivityOneCoreLib.lib \
    $(WINCORE_OBJ_PATH)\console\open\src\types\lib\$(O)\ConTypes.lib \

DELAYLOAD = \
    PROPSYS.dll; \
    OLEAUT32.dll; \
    api-ms-win-mm-playsound-l1.dll; \
    api-ms-win-shcore-scaling-l1.dll; \
    api-ms-win-shell-namespace-l1.dll; \
    ext-ms-win-gdi-dc-l1.dll; \
    ext-ms-win-gdi-dc-create-l1.dll; \
    ext-ms-win-gdi-draw-l1.dll; \
    ext-ms-win-gdi-font-l1.dll; \
    ext-ms-win-gdi-internal-desktop-l1.dll; \
    ext-ms-win-ntuser-caret-l1.dll; \
    ext-ms-win-ntuser-dialogbox-l1.dll; \
    ext-ms-win-ntuser-draw-l1.dll; \
    ext-ms-win-ntuser-keyboard-l1.dll; \
    ext-ms-win-ntuser-gui-l1.dll; \
    ext-ms-win-ntuser-menu-l1.dll; \
    ext-ms-win-ntuser-message-l1.dll; \
    ext-ms-win-ntuser-misc-l1.dll; \
    ext-ms-win-ntuser-mouse-l1.dll; \
    ext-ms-win-ntuser-rectangle-ext-l1.dll; \
    ext-ms-win-ntuser-server-l1.dll; \
    ext-ms-win-ntuser-sysparams-ext-l1.dll; \
    ext-ms-win-ntuser-window-l1.dll; \
    ext-ms-win-rtcore-gdi-object-l1.dll; \
    ext-ms-win-rtcore-gdi-rgn-l1.dll; \
    ext-ms-win-rtcore-ntuser-console-l1.dll; \
    ext-ms-win-rtcore-ntuser-cursor-l1.dll; \
    ext-ms-win-rtcore-ntuser-dc-access-l1.dll; \
    ext-ms-win-rtcore-ntuser-rawinput-l1.dll; \
    ext-ms-win-rtcore-ntuser-sysparams-l1.dll; \
    ext-ms-win-rtcore-ntuser-window-ext-l1.dll; \
    ext-ms-win-shell-shell32-l1.dll; \
    ext-ms-win-uiacore-l1.dll; \

DLOAD_ERROR_HANDLER = kernelbase
