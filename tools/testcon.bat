@rem Run the console tests.

@set TAEF=%~dp0..\dep\DDK\TAEF\x86\TE.exe
@if not (%1) == () (
    @call %TAEF% %1
) else (
    @call %TAEF% %~dp0..\bin\x64\Debug\ConHost.Api.Tests.dll
    @call %TAEF% %~dp0..\bin\x64\Debug\Conhost.Unit.Tests.dll
    @rem add more binaries once they're added to project
)
dir
