@echo off

rem Run the console UI Automation tests.

rem Get AppDriver going first... You'll have to close it yourself.

start %OPENCON%\dep\WinAppDriver\WinAppDriver.exe

rem Then run the tests.

call %TAEF% ^
    %OPENCON%\bin\%ARCH%\Debug\Host.Tests.UIA.dll ^
    /p:VtApp=%OPENCON%\bin\%ARCH%\Debug\VtApp.exe ^
    %*
