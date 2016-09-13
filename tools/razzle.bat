@rem Open Console build environment setup
@rem Adds msbuild to your path, and adds the open\tools directory as well 
@rem This recreates what it's like to be an actual windows developer!

@rem skip the setup if we're already ready.
@if not "%OpenConBuild%" == "" goto :END


@rem Add path to MSBuild Binaries
@if exist "%ProgramFiles%\MSBuild\14.0\bin" set PATH=%ProgramFiles%\MSBuild\14.0\bin;%PATH%
@if exist "%ProgramFiles(x86)%\MSBuild\14.0\bin" set PATH=%ProgramFiles(x86)%\MSBuild\14.0\bin;%PATH%

@rem Add Opencon build scripts to path
@set PATH=%PATH%;%~dp0;

@rem call .razzlerc - for your generic razzle environment stuff
@if exist "%~dp0\.razzlerc.bat" @call %~dp0\.razzlerc.bat

@rem if there are args, run them. This can be used for additional env. customization,
@rem    especially on a per shortcut basis.
:ARGS_LOOP
@if (%1) == () goto :POST_ARGS_LOOP
@echo %1
@if exist %1 (
    @call %1
) else (
    @echo Could not locate "%1"
)
@shift
@goto :ARGS_LOOP
:POST_ARGS_LOOP

@rem Set this envvar so setup won't repeat itself
@set OpenConBuild=true


:END
@echo The dev environment is ready to go!
