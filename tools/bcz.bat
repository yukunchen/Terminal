@rem bcz - Clean and build the project
@rem This is another script to help Microsoft developers feel at home working on the openconsole project.

@call msbuild.exe %~dp0\..\OpenConsole.sln /t:Clean;Build