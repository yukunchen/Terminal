# -------------------------------------
# Windows Console
# - Common Project Configuration
# -------------------------------------

# -------------------------------------
# Preprocessor Settings
# -------------------------------------

UNICODE                 = 1
C_DEFINES               = $(C_DEFINES) -DUNICODE -D_UNICODE

# -------------------------------------
# CRT Configuration
# -------------------------------------

USE_UNICRT              = 1
USE_MSVCRT              = 1

USE_STL                 = 1
STL_VER                 = STL_VER_CURRENT
USE_NATIVE_EH           = 1

# -------------------------------------
# Compiler Settings
# -------------------------------------

MSC_WARNING_LEVEL       = /W4 /WX
USER_C_FLAGS            = $(USER_C_FLAGS) /std:c++17

# -------------------------------------
# Common Console Includes and Libraries
# -------------------------------------
CONSOLE_SRC_PATH        = $(PROJECT_ROOT)\core\console\open\src

INCLUDES= \
    $(INCLUDES); \
    $(CONSOLE_SRC_PATH)\inc; \
    $(ONECORE_SDK_INC_PATH); \
    $(MINWIN_PRIV_SDK_INC_PATH); \
    $(MINCORE_PRIV_SDK_INC_PATH); \
    