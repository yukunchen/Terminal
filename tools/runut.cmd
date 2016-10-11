@echo off

rem Run the console unit tests.

call %TAEF% ^
    %OPENCON%\bin\%ARCH%\Debug\Conhost.Unit.Tests.dll ^
    %OPENCON%\bin\%ARCH%\Debug\ConParser.Unit.Tests.dll ^
    %OPENCON%\bin\%ARCH%\Debug\ConAdapter.Unit.Tests.dll ^
    %*
