## Description

This is VisualUEFI like project, but configurated to use Clang compiler and emit DWARF debug information. 
It gives you a possibility to debug your UEFI applications in source level on Windows using Vmware Workstation and
GDB.

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

Add new configuration.

![plot](/pictures/slide2.jpg)

Select remote debug and fill the connection information like below.

![plot](/pictures/slide3.jpg)

Then launch virtual machine and load your UEFI application. You should use dead loop in your app
where application waits for debugger in endless loop. After this we start remote debugging in Clion
and pause code execution. After pausing enter ```add-symbol-file path_to_your_executable``` to GDB console.

![plot](/pictures/slide4.jpg)

Put breakpoint at endless loop, change loop condition variable and after this you can freely do source level
debugging of your code.

![plot](/pictures/slide5.jpg)
