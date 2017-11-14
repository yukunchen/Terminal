@echo off

rem Run the console unit tests.

call %TAEF% ^
    %OPENCON%\bin\%ARCH%\%_LAST_BUILD_CONF%\Conhost.Unit.Tests.dll ^
    %OPENCON%\bin\%ARCH%\%_LAST_BUILD_CONF%\Conhost.Interactivity.Win32.Unit.Tests.dll ^
    %OPENCON%\bin\%ARCH%\%_LAST_BUILD_CONF%\ConParser.Unit.Tests.dll ^
    %OPENCON%\bin\%ARCH%\%_LAST_BUILD_CONF%\ConAdapter.Unit.Tests.dll ^
    %*
