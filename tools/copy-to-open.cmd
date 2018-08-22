@echo off
@setlocal

set target_path=
set reverse=0

:ARGS_LOOP
if (%1) == () goto :POST_ARGS_LOOP
if "%1" == "--reverse" (
    set reverse=1
) else (
    set target_path=%1
)
shift
goto :ARGS_LOOP

:POST_ARGS_LOOP

if (%target_path%)==() (
    echo Usage: copy-to-open [--reverse] ^<path to opensource project^>
    echo Copies all the files from open-source-paths.txt to the given open source path
    echo If the --reverse flag is specified, performs an RI ^(taking code from the open project into OpenConsole^)
    goto :eof
)

if (%reverse%)==(0) (
    set _ProjectName=Closed Source OpenConsole
    set _SRC=%OPENCON%
    set _TGT=%target_path%
    echo performing an FI from %_SRC% to %_TGT%
) else (
    set _ProjectName=Open Source Console
    set _SRC=%target_path%
    set _TGT=%OPENCON%
    echo performing an RI from %_SRC% to %_TGT%
)

rem echo Copying to %target_path%

for /F "usebackq tokens=*" %%p in (%OPENCON%\tools\open-source-paths.txt) do (
    echo %%p

    if exist %_SRC%\%%p (
        echo %_SRC%\%%p -^> %_TGT%\%%p
        echo f | xcopy /y %_SRC%\%%p %_TGT%\%%p
    ) else (
        echo [33mWARNING[m %%p does not exist in the %_ProjectName% project, but is listed in open-source-paths.txt
    )
)

