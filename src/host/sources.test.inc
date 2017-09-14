!include ..\sources.inc

# -------------------------------------
# Program Information
# -------------------------------------

TARGET_DESTINATION      = UnitTests

UNIVERSAL_TEST          = 1
TEST_CODE               = 1

# -------------------------------------
# Preprocessor Settings
# -------------------------------------

C_DEFINES               = $(C_DEFINES) -DINLINE_TEST_METHOD_MARKUP -DUNIT_TESTING

# -------------------------------------
# Sources, Headers, and Libraries
# -------------------------------------

INCLUDES = \
    $(INCLUDES); \
    ..\..\inc\test; \
    $(SDKTOOLS_INC_PATH)\WexTest\Cue; \

TARGETLIBS = \
    $(TARGETLIBS) \
    $(SDKTOOLS_LIB_PATH)\WexTest\Cue\Wex.Common.lib \
    $(SDKTOOLS_LIB_PATH)\WexTest\Cue\Wex.Logger.lib \
    $(SDKTOOLS_LIB_PATH)\WexTest\Cue\Te.Common.lib \
