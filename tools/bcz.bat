@echo off

rem bcz - Clean and build the project
rem This is another script to help Microsoft developers feel at home working on the openconsole project.

set _LAST_BUILD_CONF=%DEFAULT_CONFIGURATION%

:ARGS_LOOP
if (%1) == () goto :POST_ARGS_LOOP
if (%1) == (dgb) (
    echo Manually building debug
    set _LAST_BUILD_CONF=Debug
)
if (%1) == (rel) (
    echo Manually building release
    set _LAST_BUILD_CONF=Release
)
shift
goto :ARGS_LOOP
:POST_ARGS_LOOP
echo starting build

call msbuild.exe %OPENCON%\OpenConsole.sln /t:Clean;Build /m /nr:true /p:Configuration=%_LAST_BUILD_CONF%