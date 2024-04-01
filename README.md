# Modbus RTU client shm

Modbus RTU client that stores its data (registers) in shared memory objects.

## Dependencies
- cxxopts by jarro2783 (https://github.com/jarro2783/cxxopts) (only required for building the application)
- libmodbus by Stéphane Raimbault (https://github.com/stephane/libmodbus)
- cxxshm (https://github.com/NikolasK-source/cxxshm)
- cxxsemaphore (https://github.com/NikolasK-source/cxxsemaphore)

On Arch linux they are available via the official repositories and the AUR:
- https://archlinux.org/packages/extra/any/cxxopts/
- https://aur.archlinux.org/packages/libmodbus
- https://aur.archlinux.org/packages/cxxshm
- https://aur.archlinux.org/packages/cxxsemaphore

## Build
```
cmake -B build -DCMAKE_CXX_COMPILER=$(which clang++) -DCMAKE_BUILD_TYPE=Release -DCLANG_FORMAT=OFF -DCLANG_TIDY=OFF -DCOMPILER_WARNINGS=OFF -DBUILD_DOC=OFF
cmake --build .
```

## Use
```
modbus-rtu-client-shm [OPTION...]

  -d, --device arg            mandatory: serial device
  -i, --id arg                mandatory: modbus RTU client id
  -p, --parity arg            serial parity bit (N(one), E(ven), O(dd)) (default: N)
      --data-bits arg         serial data bits (5-8) (default: 8)
      --stop-bits arg         serial stop bits (1-2) (default: 1)
  -b, --baud arg              serial baud (default: 9600)
      --rs485                 force to use rs485 mode
      --rs232                 force to use rs232 mode
  -n, --name-prefix arg       shared memory name prefix (default: modbus_)
      --do-registers arg      number of digital output registers (default: 65536)
      --di-registers arg      number of digital input registers (default: 65536)
      --ao-registers arg      number of analog output registers (default: 65536)
      --ai-registers arg      number of analog input registers (default: 65536)
  -m, --monitor               output all incoming and outgoing packets to stdout
      --byte-timeout arg      timeout interval in seconds between two consecutive bytes of the same message. In most 
                              cases it is sufficient to set the response timeout. Fractional values are possible.
      --response-timeout arg  set the timeout interval in seconds used to wait for a response. When a byte timeout is 
                              set, if the elapsed time for the first byte of response is longer than the given timeout, 
                              a timeout is detected. When byte timeout is disabled, the full confirmation response must 
                              be received before expiration of the response timeout. Fractional values are possible.
      --force                 Force the use of the shared memory even if it already exists. Do not use this option per 
                              default! It should only be used if the shared memory of an improperly terminated instance 
                              continues to exist as an orphan and is no longer used.
  -h, --help                  print usage
      --version               print version information
      --license               show licences


The modbus registers are mapped to shared memory objects:
    type | name                      | mb-server-access | shm name
    -----|---------------------------|------------------|----------------
    DO   | Discrete Output Coils     | read-write       | <name-prefix>DO
    DI   | Discrete Input Coils      | read-only        | <name-prefix>DI
    AO   | Discrete Output Registers | read-write       | <name-prefix>AO
    AI   | Discrete Input Registers  | read-only        | <name-prefix>AI
```
