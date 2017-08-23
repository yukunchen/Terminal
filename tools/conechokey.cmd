@echo off

if not exist %_last_build%\conechokey.exe (
    echo Could not locate the conechokey.exe in %_last_build%. Double check that it has been built and try again.
    goto :eof
)
%_last_build%\conechokey.exe %*