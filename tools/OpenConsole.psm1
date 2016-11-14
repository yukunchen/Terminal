
# The project's root directory.
Set-Item -force -path "env:OpenConsoleRoot" -value "$PSScriptRoot\.."

#.SYNOPSIS
# Grabs all environment variable set after vcvarsall.bat is called and pulls
# them into the Powershell environment.
function Set-MsbuildDevEnvironment()
{
    $path = "$env:VS140COMNTOOLS\..\.."
    pushd $path
    cmd /c "vcvarsall.bat&set" | foreach {
        if ($_ -match "=")
        {
            $s = $_.Split("=");
            Set-Item -force -path "env:\$($s[0])" -value "$($s[1])"
        }
    }
    popd
    Write-Host "Dev environment variables set" -ForegroundColor Green
}

#.SYNOPSIS
# Runs OpenConsole's tests. Will only run unit tests by default. Each ft test is
# run in its own window.
#
#.PARAMETER AllTests
# When set, all tests will be run.
#
#.PARAMETER FTOnly
# When set, only ft tests will be run.
#
#.PARAMETER Test
# Can be used to specify that only a particular test should be run.
# Current values allowed are: unit, api, cjk, resize, message.
#
#.PARAMETER TaefArgs
# Used to pass any additional arguments to the test runner.
#
#.PARAMETER Platform
# The platform of the OpenConsole tests to run. Can be "x64" or "x86".
# Defaults to "x64".
#
#.PARAMETER Configuration
# The configuration of the OpenConsole tests to run. Can be "Debug" or
# "Release". Defaults to "Debug".
function Invoke-OpenConsoleTests()
{
    [CmdletBinding()]
    Param (
        [parameter(Mandatory=$false)]
        [switch]$AllTests,

        [parameter(Mandatory=$false)]
        [switch]$FTOnly,

        [parameter(Mandatory=$false)]
        [string]$Test,

        [parameter(Mandatory=$false)]
        [string]$TaefArgs,

        [parameter(Mandatory=$false)]
        [string]$Platform = "x64",

        [parameter(Mandatory=$false)]
        [string]$Configuration = "Debug"

    )
    if (($AllTests -and $FTOnly) -or ($AllTests -and $Test) -or ($FTOnly -and $Test))
    {
        Write-Host "Invalid combination of flags" -ForegroundColor Red
        return
    }
    $OpenConsolePath = "$env:OpenConsoleroot\bin\$Platform\$Configuration\OpenConsole.exe"
    $RunTePath = "$env:OpenConsoleRoot\tools\runte.cmd"
    $BinDir = "$env:OpenConsoleRoot\bin\$Platform\$Configuration"
    if ($AllTests)
    {
        te.exe "$BinDir\ConHost.Unit.Tests.dll" $TaefArgs
    }
    if ($FTOnly -or $AllTests)
    {
        & $OpenConsolePath $RunTePath "$BinDir\Conhost.Api.Tests.dll" $TaefArgs
        & $OpenConsolePath $RunTePath "$BinDir\Conhost.CJK.Tests.dll" $TaefArgs
        & $OpenConsolePath $RunTePath "$BinDir\Conhost.Resize.Tests.dll" $TaefArgs
        & $OpenConsolePath $RunTePath "$BinDir\Conhost.Message.Tests.dll" $TaefArgs
        return
    }
    elseif ($Test)
    {
        switch ($Test)
        {
            "unit"
            {
                te.exe "$BinDir\ConHost.Unit.Tests.dll" $TaefArgs
            }

            "api"
            {
                & $OpenConsolePath $RunTePath "$BinDir\Conhost.Api.Tests.dll" $TaefArgs
            }

            "cjk"
            {
                & $OpenConsolePath $RunTePath "$BinDir\Conhost.CJK.Tests.dll" $TaefArgs
            }

            "resize"
            {
                & $OpenConsolePath $RunTePath "$BinDir\Conhost.Resize.Tests.dll" $TaefArgs
            }

            "message"
            {
                & $OpenConsolePath $RunTePath "$BinDir\Conhost.Message.Tests.dll" $TaefArgs
            }
        }
    }
    # run just the unit tests by default
    else
    {
        te.exe "$env:OpenConsoleRoot\bin\$Platform\$Configuration\ConHost.Unit.Tests.dll" $TaefArgs
    }
}

#.SYNOPSIS
# Builds OpenConsole.sln using msbuild. Any arguments get passed on to msbuild.
function Invoke-OpenConsoleBuild()
{
    msbuild.exe "$env:OpenConsoleRoot\OpenConsole.sln" @args
}

#.SYNOPSIS
# Launches an OpenConsole process.
#
#.PARAMETER Platform
# The platform of the OpenConsole executable to launch. Can be "x64" or "x86".
# Defaults to "x64".
#
#.PARAMETER Configuration
# The configuration of the OpenConsole executable to launch. Can be "Debug" or
# "Release". Defaults to "Debug".
function Start-OpenConsole()
{
    [CmdletBinding()]
    Param (
        [parameter(Mandatory=$false)]
        [string]$Platform = "x64",

        [parameter(Mandatory=$false)]
        [string]$Configuration = "Debug"
    )
    if ($Platform -like "x86")
    {
        $Platform = "Win32"
    }
    & "$env:OpenConsoleRoot\bin\$Platform\$Configuration\OpenConsole.exe"
}

#.SYNOPSIS
# Launches an OpenConsole process and attaches the default debugger.
#
#.PARAMETER Platform
# The platform of the OpenConsole executable to launch. Can be "x64" or "x86".
# Defaults to "x64".
#
#.PARAMETER Configuration
# The configuration of the OpenConsole executable to launch. Can be "Debug" or
# "Release". Defaults to "Debug".
function Debug-OpenConsole()
{
    [CmdletBinding()]
    Param (
        [parameter(Mandatory=$false)]
        [string]$Platform = "x64",

        [parameter(Mandatory=$false)]
        [string]$Configuration = "Debug"
    )
    if ($Platform -like "x86")
    {
        $Platform = "Win32"
    }
    $process = [Diagnostics.Process]::Start("$env:OpenConsoleRoot\bin\$Platform\$Configuration\OpenConsole.exe")
    Debug-Process -Id $process.Id
}

Export-ModuleMember -Function Set-MsbuildDevEnvironment,Invoke-OpenConsoleTests,Invoke-OpenConsoleBuild,Start-OpenConsole,Debug-OpenConsole