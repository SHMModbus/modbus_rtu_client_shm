# Shared Memory Modbus RTU Client

This project is a simple command line based Modbus TCP client for POSIX compatible operating systems that stores the 
content of its registers in shared memory.

## Basic operating principle

The client creates four shared memories.
One for each register type:
- Discrete Output Coils (DO)
- Discrete Input Coils (DI)
- Discrete Output Registers (AO)
- Discrete Input Registers (AI)

All registers are initialized with 0 at the beginning.
The Modbus server reads and writes directly the values from these shared memories.

The actual functionality of the client is realized by applications that read data from or write data to the shared memory.

The coils (DI, DO) use one byte per value, but only the least significant bit is used.
The registers (AI, AO) use 10 bit per value. The user is responsible for handling the endianness.

## Use the Application
The application requires a serial device (```--device```) and a client id (```--id```) as arguments. All other arguments are optional.

By using the command line argument ```--monitor``` all incoming and outgoing packets are printed on stdout.
This option should be used carefully, as it generates large amounts of output depending on the Modbus servers polling cycle and the number of used registers.

The client creates four shared memories and names them ```modbus_DO```, ```modbus_DI```, ```modbus_AO``` and ```modbus_AI``` by default.
The prefix modbus_ can be changed via the argument ```--name-prefix```. 
The suffixes for the register type (DO, DI, AO, AI) cannot be changed and will always be appended to the prefix.

By default, the client starts with the maximum possible number of modbus registers (65536 per register type).
The number of registers can be changed using the ```--xx-registers``` (replace xx with the register type) command line arguments.

### Examples

Connect as device with id ```1``` using serial device ```/dev/ttyS0```:
```
modbus-rtu-client-shm -d /dev/ttyS0 -i 1
```


Connect as device with id ```1``` using serial device ```/dev/ttyS0``` with a baud rate of ```115200``` and a odd parity bit:
```
modbus-rtu-client-shm -d /dev/ttyS0 -i 1 -b 115200 -p O
```

Connect as device with id ```1``` using serial device ```/dev/ttyS0``` and enforce rs232 mode:
```
modbus-rtu-client-shm -d /dev/ttyS0 -i 1 --rs232
```
## Install

### Using the Arch User Repository (recommended for Arch based Linux distributions)
The application is available as [modbus-rtu-client-shm](https://aur.archlinux.org/packages/modbus-rtu-client-shm) in the [Arch User Repository](https://aur.archlinux.org/).
See the [Arch Wiki](https://wiki.archlinux.org/title/Arch_User_Repository) for information about how to install AUR packages.


### Using the Modbus Collection Flatpak Package: Shared Memory Modbus (recommended)
[SHM-Modbus](https://nikolask-source.github.io/SHM_Modbus/) is a collection of the shared memory modbus tools.
It is available as flatpak and published on flathub as ```network.koesling.shm-modbs```.


### Using the Standalone Flatpak Package
The flatpak package can be installed via the .flatpak file.
This can be downloaded from the GitHub [projects release page](https://github.com/NikolasK-source/modbus_rtu_client_shm/releases):

```
flatpak install --user modbus-rtu-client-shm.flatpak
```

The application is then executed as follows:
```
flatpak run network.koesling.modbus-rtu-client-shm
```

To enable calling with ```modbus-rtu-client-shm``` [this script](https://gist.github.com/NikolasK-source/7586b3b2e9808e63dd3f111310eacc03) can be used.
In order to be executable everywhere, the path in which the script is placed must be in the ```PATH``` environment variable.


### Build from Source

The following packages are required for building the application:
- cmake
- clang or gcc

Additionally, the following packages are required to build the modbus library:
- autoconf
- automake
- libtool


Use the following commands to build the application:
```
git clone --recursive https://github.com/NikolasK-source/modbus_rtu_client_shm.git
cd modbus_rtu_client_shm
mkdir build
cmake -B build . -DCMAKE_BUILD_TYPE=Release -DCLANG_FORMAT=OFF -DCOMPILER_WARNINGS=OFF
cmake --build build
```

The binary is located in the build directory.


## Links to related projects

### General Shared Memory Tools
- [Shared Memory Dump](https://nikolask-source.github.io/dump_shm/)
- [Shared Memory Write](https://nikolask-source.github.io/write_shm/)
- [Shared Memory Random](https://nikolask-source.github.io/shared_mem_random/)

### Modbus Clients
- [RTU](https://nikolask-source.github.io/modbus_rtu_client_shm/)
- [TCP](https://nikolask-source.github.io/modbus_tcp_client_shm/)

### Modbus Shared Memory Tools
- [STDIN to Modbus](https://nikolask-source.github.io/stdin_to_modbus_shm/)
- [Float converter](https://nikolask-source.github.io/modbus_conv_float/)


## License

MIT License

Copyright (c) 2021-2022 Nikolas Koesling

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
