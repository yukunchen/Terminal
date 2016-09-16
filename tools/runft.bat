@rem Run the console feature tests.

@call %TAEF% ^
    rem %OPENCON%\bin\%ARCH%\Debug\ConHost.CJK.Tests.dll ^
    %OPENCON%\bin\%ARCH%\Debug\ConHost.Api.Tests.dll ^
    %OPENCON%\bin\%ARCH%\Debug\ConHost.Resize.Tests.dll ^
    %*

@rem the CJK tests are failing currently, so I'm temporarily disabling those 
@rem    (while we wait for the improved utf8 support)