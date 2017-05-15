# DAEbot Internal Robot Controller Subsystem

**Authors**
Hector Gerardo Munoz Hernandez  
Vladimir Poliakov

## Overview

*DAEbot Internal Robot Controller Subsytem* (**DIRCS**) aims to connect OS-independent layer of application with currently used RTOS (ChibiOS.16.1.7). 
DIRCS includes:
1. *main.c* - main() file that initializes RTOS and launches the application
2. *chconf.h, halconf.h, mcuconf.h* - configs for ChibiOS
3. *ctrl* - application directory, includes application (*ctrl.c, ctrl.h*) and OS-dependent abstraction layer (*ictrl.c, ictrl.h*)
4. *Makefile* - project makefile used for both builds from Eclipse and from Shell

## TODO

- [x] Install ChibiOS
- [ ] Create ictrl abstraction layer
- [ ] Add ctrl to Makefile

## Features

- Basic ChibiOS example with blinking LEDs

## Installation, flash and run on STM32F3-DISCO

![alt text](http://www.rlocman.ru/i/Image/2012/09/28/stm32f3discovery.jpg "STM32F3DISCOVERY!")

## Software Required

+ Basic packages:

`sudo apt-get install git zlib1g-dev libtool flex bison libgmp3-dev \  libmpfr-dev libncurses5-dev libmpc-dev autoconf texinfo \ 
build-essential libftdi-dev libusb-1.0.0-dev default-jre default-jdk \ lib32ncurses5 lib32z1`

+ [Eclipse Java + C/C++](https://eclipse.org/)  
+ [GNU ARM Embedded Toolchain](https://launchpad.net/gcc-arm-embedded)  
+ [OpenOCD](https://sourceforge.net/projects/openocd/files/openocd/0.10.0-rc1/)  
+ [ChibiOS/RT](https://sourceforge.net/projects/chibios/files/ChibiOS_RT%20stable/) (v16.1.7 recommended)


## Setting up the enviroment

### GNU Tools

After downloading GCC for should be added to PATH:

`echo 'PATH=\$PATH:/path/to/gcc-arm-none-eabi/bin' >> ~/.profile`

### OpenOCD compiling

`cd /path/to/openocd-${VERSION}`  
`./configure --enable-maintainer-mode --enable-stlink --prefix=/path/to/desired/directory/`  
`make`  
`sudo make install`  

**NOTE**: If specified prefix while configuring, you should add openocd directory to PATH:

`echo 'PATH=\$PATH:/path/to/opeocd/bin' >> ~/.profile`

### Rules for OpenoOCD

This step is for running openocd commands without sudo.

First add openocd rules to udev rules directory:

`sudo cp /path/to/opencd/contrib/99-openocd.rules /etc/udev/rules.d/`

Then reload rules

`sudo udevadm control --reload-rules`

### Update OpenOCD config file

For STM32F3DISCOVERY, stlinkv-2 should be replaced with stlinkv-2-1, so run
	
`sed -i 's/stlink-v2.cfg/stlink-v2-1.cfg/g'`

## Test

Connect STM32F3DISCOVERY with USB A to B cable, B in the ST-LINK port on the board
Now run:

`openocd -f /path/to/openocd/scripts/board/stm32f3discovery.cfg`

If everything went good the output should be something like this:

	Open On-Chip Debugger 0.7.0 (2013-05-11-11:14)
	Licensed under GNU GPL v2
	For bug reports, read
		http://openocd.sourceforge.net/doc/doxygen/bugs.html
	srst_only separate srst_nogate srst_open_drain connect_deassert_srst
	Info : This adapter doesn't support configurable speed
	Info : STLINK v2 JTAG v16 API v2 SWIM v0 VID 0x0483 PID 0x3748
	Info : Target voltage: 2.888758
	Info : stm32f3x.cpu: hardware has 6 breakpoints, 4 watchpoints
	

## Eclipse Configuration

Go to Help > Install New Software  
Add:

_name:_ `Eclipse Neon Releases`  
_site:_	`http://download.eclipse.org/releases/neon`

Search for **"C/C++ GDB Hardware Debugging"** and install it.

Go to Project > Build

+ Uncheck Build Automatically.
	
Go to Window > Preferences > C/C++ > Code Analysis 

+ Disable everything
 

Go to File > Import > General > Existing Projects into Workspace

And browse to the directory containing this project.

> This project runs three threads, Two of them are toggling two leds, and the third thread is waiting for the button to be pressed to toogle yet another led.


+ Press finish.

+ Browse to the Makefile of the project and search for variable called **CHIBIOS**.

+ Change it to the path where you decompressed CHIBIOS files:

`CHIBIOS = /path/to/chibios`

+ Now right-click the project at Project Explorer and build.


+ Go to External Configuration and in Program right-click and add new.

**Main:** 

_Location:_	`/path/to/openocd/bin/openocd`

_Working Directory:_ `${workspace_loc:/project_name}`

_Arguments:_ `-f /usr/local/share/openocd/scripts/board/stm32f3discovery.cfg -c init -c "reset init" -c "poll"`


**NOTE:** if you do not wish to debugg as well, you can just modify Arguments to:

_Arguments:_ `-f /path/to/openocd/scripts/board/stm32f3discovery.cfg -c "program ${workspace_loc:/project_name//build/ch.elf} verify reset"`

**Build:** 


+ Make sure that build before launch is unchecked
 
Go to Debug Configurations

+ **GDB Hardware Debugung (right-click) add new**

**Main:**
_Project:_ `Name of your project`

_C/C++ Applications:_`./build/ch.elf`
 Check `Disable auto build`

**Debugger:**

_GDB Command:_	`arm-none-eabi-gdb`
		
_Use remote target:_ `check`

_JTAG Device:_	`Generic TCP/IP`

_Host name or IP address:_ `localhost`

_Port number:_ `3333`

**Startup:**

_Reset and Delay(seconds):_ `1`

_Set breakpoint at:_ `main`


+ Connect board (via ST-Link).

+ Run External Configuration.

And _if you choose to debug as well_

+ run debug configuration.

_Congratulations!_ now the code is in the board.


