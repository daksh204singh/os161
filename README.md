# Operating System OS/161 Kernel Development

## About OS/161
OS/161 is a teaching operating system, that is, a simplified system used for teaching undergraduate operating systems classes. It is BSD-like in feel and has more "reality" than most other teaching OSes; while it runs on a simulator it has the structure and design of a larger system.

## About System/161
System/161 is a machine simulator that provides a simplified but still realistic environment for OS hacking. It is a 32-bit MIPS system supporting up to 32 processors, with up to 31 hardware slots each holding a single simple device (disk, console, network, etc.) It was designed to support OS/161, with a balance of simplicity and realism chosen to make it maximally useful for teaching. However, it also has proven useful as a platform for rapid development of research kernel projects.

System/161 supports fully transparent debugging, via remote gdb into the simulator. It also provides transparent kernel profiling, statistical monitoring, event tracing (down to the level of individual machine instructions) and one can connect multiple running System/161 instances together into a network using a "hub" program.

## Source directory
The source directory contains the following files:- 

* CHANGES: describes the evolution of OS/161 and changes in previous versions.
* configure: the top-level configuration script that you ran previously.
* Makefile: the top-level Makefile used to build the user space binaries.

The source directory contains the following subdirectories:
* common/: code used both by the kernel and user programs, mostly standard C library functions.
* design/: contains design documents describing several OS/161 components. Assignments design docs are present in this directory.
* kern/: the kernel source code, and the subdirectory contains the code used by the kernel. This directory is 
* man/: the OS/161 man pages appear here. The man pages document (or specify) every program, every function in the C library, and every system call. 
mk/: fragments of Makefiles used to build the system.
* userland/: user space libraries and program code.
	
### userland
The userland/ sub directory contains user space libraries and program code.
* bin/: all the utilities that are typically found in /bin/—`cat`, cp, ls, etc. Programs in /bin/ are considered fundamental utilities that the system needs to run.
* include/: these are the include files that you would typically find in /usr/include (in our case, a subset of them). These are user include files, not kernel include files.
* lib/: library code lives here. We have only two libraries: libc, the C standard library, and hostcompat, which is for recompiling OS/161 programs for the host UNIX system. There is also a crt0 directory, which contains the startup code for user programs.
* sbin/: this is the source code for the utilities typically found in /sbin on a typical UNIX installation. In our case, there are some utilities that let you halt the machine, power it off, and reboot it, among other things.
* testbin/: these are pieces of test code that we will use to test and grade your assignments.

### kernel sources
The kernel sources are inside the kern/ subdirectory. The kern directory contains:- 
	
* Makefile: The Makefile installs header files. 

* kern/arch: This directory contains the architecture-specific code. Inside it their are two subdirectories: mips, which contains the code specific to the mips processor and sys161, which contains code for the System/161 machine simulator.
	
* kern/arch/mips/conf: The directory contains the mips-specific configuration files.
  * kern/arch/mips/conf/conf.arch: file is responsible for specfying machine dependent sources required by the kernel.
  * kern//arch/mips/conf/ldscript: Tells the linker how to link the kernel.
  * kern/arch/mips/include: this folder and its subdirectories include filess for machine-specific constants and functions. 
  * kern/arch/mips/: The directories left like locore, vm, etc, contain source files for the machine-dependent code that the kernel needs to run. It mostly contains low-level machine specific code. 
  * kern/arch/sys161/conf/conf.arch: responsible for specifying platform-dependent sources for System/161. 
  * kern/arch/sys161/include: These files are include file for the System/161 specific hardware, constants, and functions.
  * kern/arch/sys161/main/start.S: This is the entry point for the kernel. It sets up the stack frame, exception handling, etc for the kernel. 
  * kern/arch/sys161/dev: contains System/161 device code.

* kern/compile/: This is the directory where the kernel builds. Here, you will find one subdirectory for each, kernel configuration target.

* kern/test/: This directory contains kernel tests.

* kern/dev/: This directory contains all the low-level device management code.

* kern/include: This directory contains all the include files required by the kernel.	
  * kern/include/kern: This directory contains include files that are visible to both the kernel and the user programs.

* kern/lib/: These contain library code used throughout the kernel: arrays, kerenl printf(kprintf), etc.

* kern/main/: This directory contains the kernel main and menu functions of the kernel.
  * kern/main/main.c: initializes the kernel. Executes the menu.
  * kern/main/menu.c: contains the command menu functions.

* kern/thread/: The directory contains the code implementing the thread abstraction and synchronization primitives.
  * kern/thread/clock.c: contains time handling functions. 
  * kern/thread/hangman.c: contains implementation of simple deadlock detector. 
  * kern/thread/spinlock.c: contains implementation of spinlock.
  * kern/thread/spl.c: contains machine-independent interrupt handling functions. spl's full form is "set priority level".
  * kern/thread/synch.c: contains implementation of synchronization primitives available to the kernel. TO BE IMPLEMENTED IN ASST1
  * kern/thread/thread.c: contains core kernel-level thread system.
  * kern/thread/threadlist.c: contais threadlist functions.

* kern/synchprobs: contains code for synchronization problems, whalemating and stoplight.

* kern/syscall/: contains code to create and manage user level processes. TO BE IMPLEMNETED IN ASST2
  * kern/syscall/loadelf.c: contains code that loads ELF(Executable and Linkable Format) executable.
  * kern/syscall/runprogram.c: contains sample/test code for running a user program.
  * kern/syscall/time\_syscalls.c: sample syscall that returns the time of the day. 

* kern/vm/: contains implementation of virtual memory. TO BE DONE IN ASST3

* kern/vfs/: 'vfs' stands for virtual file system. It provides a framework to add new file systems.

* kern/fs/: Contains the actual file system implementation.
  * kern/fs/sfs: contains implementation of simple file system.

## OS/161, System/161 and toolchain 
OS/161, System/161 and its toolchain can be downloaded from http://os161.eecs.harvard.edu/download/
The OS/161 toolchain includes:- 
	
* binutils package: it includes a collection of programs for creating and managing binary programs, object file, profile data, and assembly source code. Some programs are linker, assembler. It is GNU binutils for OS/161.

* gcc: GCC is GNU's Compiler collection for OS/161.

* gdb: GDB is GNU's debugger for OS/161

* bmake: It is BSD is make. It is different from GNU's make.

* mk: BSD make's make library.


## Running the kernel
You can run the kernel using the System/161 simulator.
```console
$ sys161 kernel
```
To run the kernel in debugger mode
```console
$ sys161 -w kernel
```

## Debugging the kernel using os161 gdb
```console
$ os161-gdb kernel
```
Inside the gdb connect to the sys161 socket(you have to be in the directory where the executable kernel is)
```console
(gdb) target remote unix:.sockets/gdb
```

## References
* https://www.ops-class.org/asst/0/
* http://os161.eecs.harvard.edu
* http://www.eecs.harvard.edu/~cs161/
