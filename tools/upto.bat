@rem upto.bat

rem @echo off
setlocal
rem setlocal enableextensions ENABLEDELAYEDEXPANSION
SET dirpath=%cd%
set uppath=%1
echo %dirpath%
SET "VPath=%dirpath:bin=bin" & rem "%"
SET "VPath=%dirpath:%%1%%=%%1%%" & rem "%"
set path2=%dirpath:~%uppath%=%uppath%% & rem %
set path3=%%dirpath:~%uppath%=%uppath%%% & rem %
set path4=%dirpath:~%%uppath%%=%%uppath%%% & rem %
set path5=%dirpath:%%uppath%%=foo%
set path6=%%dirpath:~%%uppath%%=%% & rem %
set path7=%%dirpath:%uppath%=""%% & rem %
set path8=%dirpath:~%%uppath%%*=% & rem %
set path9=%dirpath:~%uppath%*=% & rem %
echo %VPath%