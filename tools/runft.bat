@rem Run the console feature tests.

@call %TAEF% ^
    %OPENCON%\bin\%ARCH%\Debug\ConHost.Api.Tests.dll ^
    %OPENCON%\bin\%ARCH%\Debug\ConHost.Resize.Tests.dll ^
    %*

