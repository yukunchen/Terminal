cd /d "C:\src\win\windows\Core\console\open\src\host\lib" &msbuild "hostlib.vcxproj" /t:sdvViewer /p:configuration="Debug" /p:platform=x64
exit %errorlevel% 