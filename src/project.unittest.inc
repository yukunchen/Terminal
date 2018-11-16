# -------------------------------------
# Windows Console
# - Common Test Project Configuration
# -------------------------------------

!include project.inc

# -------------------------------------
# Preprocessor Settings
# -------------------------------------

C_DEFINES               = $(C_DEFINES) -DINLINE_TEST_METHOD_MARKUP -DUNIT_TESTING

# -------------------------------------
# Program Information
# -------------------------------------

TARGET_DESTINATION      = UnitTests
UNIVERSAL_TEST          = 1
TEST_CODE               = 1

# -------------------------------------
# Common Console Includes and Libraries
# -------------------------------------

INCLUDES = \
    $(INCLUDES); \
    $(CONSOLE_SRC_PATH)\inc\test; \
    $(SDKTOOLS_INC_PATH)\WexTest\Cue; \

TARGETLIBS = \
    $(TARGETLIBS) \
    $(ONECORE_SDK_LIB_VPATH)\onecore.lib \
    $(SDKTOOLS_LIB_PATH)\WexTest\Cue\Wex.Common.lib \
    $(SDKTOOLS_LIB_PATH)\WexTest\Cue\Wex.Logger.lib \
    $(SDKTOOLS_LIB_PATH)\WexTest\Cue\Te.Common.lib \
