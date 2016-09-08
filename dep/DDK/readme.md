# Windows Driver Kit #
The Windows Driver Kit (WDK) also historically known as the Driver Development Kit (DDK) contains headers, libraries, and testing frameworks used for low-level Windows OS development.

The official documentation is available [here](https://msdn.microsoft.com/en-us/library/windows/hardware/ff557573\(v=vs.85\).aspx) with [additional download details](https://developer.microsoft.com/en-us/windows/hardware/windows-driver-kit) and a [direct link](https://go.microsoft.com/fwlink/p/?LinkId=526733).

## Version Information ##
As of August 2016, we are using **10.0.14393**.

## Source Details ##
By default, the kit is installed to:
	
    C:\Program Files (x86)\Windows Kits\10

We have copied the contents of the following directories for ease of use in building and testing the console:

	C:\Program Files (x86)\Windows Kits\10\Testing\Development\inc
	C:\Program Files (x86)\Windows Kits\10\Testing\Development\lib
	C:\Program Files (x86)\Windows Kits\10\Testing\Runtimes\TAEF\x86
	C:\Program Files (x86)\Windows Kits\10\Testing\Runtimes\TAEF\x64

## Usage ##
### TAEF ###
TAEF, the Test Authoring and Execution Framework, is used extensively within the Windows organization to test the operating system code in a unified manner for system, driver, and application code. As the console is a Windows OS Component, we strive to continue using the same system such that tests can be ran in a unified manner both externally to Microsoft as well as inside the official OS Build/Test system.

The [official documentation](https://msdn.microsoft.com/en-us/library/windows/hardware/hh439725\(v=vs.85\).aspx) for TAEF describes the basic architecture, usage, and functionality of the test system. It is similar to Visual Studio test, but a bit more comprehensive and flexible.

For the purposes of the console project, you can run the tests using the *TE.exe* that matches the architecture for which the test was build (x86/x64) in the pattern

	te.exe Console.Unit.Tests.dll

Replacing the binary name with any other test binary name that might need running. Wildcard patterns or multiple binaries can be specified and all found tests will be executed.

Limiting the tests to be run is also useful with:

	te.exe Console.Unit.Tests.dll /name:*BufferTests*

Any pattern of class/method names can be specified after the */name:* flag with wildcard patterns.

For any further details on the functionality of the TAEF test runner, *TE.exe*, please see the documentation above or run the embedded help with

	te.exe /!

## History ##
The Windows Console code has historically made use of pieces of both the traditional Software Development Kit (SDK) as well as the Driver kit. Typically this was done because the driver kit was required for certain activities (drivers, subsystems, OS kernel communication) and the software kit had certain functionality available that made development easier or more user-accessible.

Over time, the console has slowly moved further from kernel/driver mode into a user-mode process, most specifically in the Windows 7 era when it was broken out of the critical system process *csrss.exe*. 

Within the Windows Build System, it's very easy to mix and match these kits and other private OS information very easily. Different developers with different backgrounds use whatever they were comfortable with intermixed, the code evolves over time, and it all just keeps working. In the outside world with strictly public SDKs, Visual Studio, and public Microsoft compilers, it's a bit more complicated to mix these in a reasonable manner. To ease ongoing development and reduce complexity, a majority of console code has been moved to use only the SDK.

It is expected that certain Driver kit dependencies will remain as necessary and that they will be embedded in this directory within the repository so the code can build with only the SDK installed. The long-term goal is to remove as much from the DDK as possible so this directory and its dependencies will no longer be necessary. 

