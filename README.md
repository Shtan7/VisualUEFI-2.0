## The description

This is the VisualUEFI like project, but configurated to use the Clang compiler and emit DWARF debug information. 
It gives you a possibility to debug your UEFI applications in the source level on Windows using Vmware Workstation and
GDB.

## Installation

Install nasm https://www.nasm.us/ and add ```NASM\``` folder to the PATH variable, install Clang 
https://learn.microsoft.com/en-us/cpp/build/clang-support-msbuild?view=msvc-170#install-1 and build ```edk2 libs```
project. At this point you are ready to build the main project in the ```samples``` folder.

## Environment setting

I will cover setting up and using Clion as a GDB frontend. This is the easiest and hassle-free option.
First, open the virtual machine's .vmx configuration file and put in these lines.

```
debugStub.listen.guest64 = "TRUE"
debugStub.port.guest64 = "55555"
debugStub.hideBreakpoints = "TRUE"
```

Next, you need to open the run \ debug configurations menu.

![plot](/pictures/slide1.jpg)

Add a new configuration.

![plot](/pictures/slide2.jpg)

Select remote debug and fill the connection information like below.

![plot](/pictures/slide3.jpg)

Then launch a virtual machine and load your UEFI application. You should use a dead loop in your app
where an application waits for a debugger in an endless loop. After this we start remote debugging in Clion
and pause code execution. After pausing enter ```add-symbol-file path_to_your_executable``` to GDB console.

![plot](/pictures/slide4.jpg)

Put a breakpoint at an endless loop, change a loop condition variable and after this you can freely do the source level
debugging of your code.

![plot](/pictures/slide5.jpg)
