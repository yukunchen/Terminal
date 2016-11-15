@echo off

rem bcz - Clean and build the project
rem This is another script to help Microsoft developers feel at home working on the openconsole project.

set _LAST_BUILD_CONF=%DEFAULT_CONFIGURATION%

set _MSBUILD_TARGET=Clean,Build

:ARGS_LOOP
if (%1) == () goto :POST_ARGS_LOOP
if (%1) == (dbg) (
    echo Manually building debug
    set _LAST_BUILD_CONF=Debug
)
if (%1) == (rel) (
    echo Manually building release
    set _LAST_BUILD_CONF=Release
)
if (%1) == (no_clean) (
    set _MSBUILD_TARGET=Build
)
shift
goto :ARGS_LOOP

:POST_ARGS_LOOP
echo starting build

nuget.exe restore %OPENCON%\OpenConsole.sln
msbuild.exe %OPENCON%\OpenConsole.sln /t:%_MSBUILD_TARGET% /m /nr:true /p:Configuration=%_LAST_BUILD_CONF%

rem Cleanup unused variables here. Note we cannot use setlocal because we need to pass modified
rem _LAST_BUILD_CONF out to OpenCon.cmd later.
rem
set _MSBUILD_TARGET=
