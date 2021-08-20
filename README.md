# Modbus TCP client shm

Modbus tcp client that stores its data (registers) in shared memory objects.

## Build
```
git submodule init
mkdir build
cd build
cmake .. -DCMAKE_CXX_COMPILER=$(which clang++) -DCMAKE_BUILD_TYPE=Release -DCLANG_FORMAT=OFF
cmake -build . 
```

## Use
```
Modbus_RTU_client_shm [OPTION...]

  -d, --device arg        mandatory: serial device
  -i, --id arg            mandatory: modbus RTU slave id
  -p, --parity arg        serial parity bit (N(one), E(ven), O(dd)) (default: N)
      --data-bits arg     serial data bits (5-8) (default: 8)
      --stop-bits arg     serial stop bits (1-2) (default: 1)
  -b, --baud arg          serial baud (default: 9600)
      --rs485             force to use rs485 mode
      --rs232             force to use rs232 mode
  -n, --name-prefix arg   shared memory name prefix (default: modbus_)
      --do-registers arg  number of digital output registers (default: 65536)
      --di-registers arg  number of digital input registers (default: 65536)
      --ao-registers arg  number of analog output registers (default: 65536)
      --ai-registers arg  number of analog input registers (default: 65536)
  -m, --monitor           output all incoming and outgoing packets to stdout
  -h, --help              print usage


The modbus registers are mapped to shared memory objects:
    type | name                      | master-access   | shm name
    -----|---------------------------|-----------------|----------------
    DO   | Discrete Output Coils     | read-write      | <name-prefix>DO
    DI   | Discrete Input Coils      | read-only       | <name-prefix>DI
    AO   | Discrete Output Registers | read-write      | <name-prefix>AO
    AI   | Discrete Input Registers  | read-only       | <name-prefix>AI
```

## Libraries
This application uses the following libraries:
- cxxopts by jarro2783 (https://github.com/jarro2783/cxxopts)
- libmodbus by Stéphane Raimbault (https://github.com/stephane/libmodbus)
