# Modbus TCP Protocol Implementation

A lightweight Modbus TCP protocol implementation in C, including both client and server components.

## Features

- **Modbus TCP Client**: Interactive command-line client for testing Modbus operations
- **Modbus TCP Server**: Event-driven server using libevent
- **Supported Function Codes**:
  - FC01: Read Coils
  - FC03: Read Holding Registers
  - FC05: Write Single Coil
  - FC06: Write Single Register
  - FC0F: Write Multiple Coils
  - FC10: Write Multiple Registers

## Project Structure

```
Modbus/
├── include/
│   ├── Modbus_Basic.h              # Basic definitions and connection functions
│   ├── Modbus_Protocol_Construction.h  # Protocol frame construction
│   └── Modbus_Function.h           # High-level Modbus function APIs
├── src/
│   ├── Modbus_Client.c             # Client application
│   ├── Modbus_Server.c             # Server application
│   ├── Modbus_Basic.c              # Connection and receive implementations
│   ├── Modbus_Protocol_Construction.c  # MBAP header and PDU builders
│   └── Modbus_Function.c           # Modbus function implementations
├── build/                          # Compiled binaries output directory
├── makefile                        # Build configuration
└── README.md
```

## Prerequisites

- GCC compiler
- libevent library (for server)

Install libevent on Ubuntu/Debian:
```bash
sudo apt-get install libevent-dev
```

## Building

```bash
make
```

This will compile both `Modbus_Client` and `Modbus_Server` into the `build/` directory.

To clean build artifacts:
```bash
make clean
```

## Usage

### Start the Server

```bash
sudo ./build/Modbus_Server
```

**Note**: Root privileges are required because Modbus TCP uses port 502 (privileged port).

### Run the Client

```bash
./build/Modbus_Client
```

The client provides an interactive menu with the following commands:
- `01` - Read Coils
- `03` - Read Holding Registers
- `05` - Write Single Coil
- `06` - Write Single Register
- `0F` - Write Multiple Coils
- `10` - Write Multiple Registers
- `q`  - Quit

## Configuration

Default connection settings in `Modbus_Client.c`:
- Server IP: `127.0.0.1`
- Port: `502`
- Slave ID: `1`

Modify these values in the source code if needed.

## License

This project is open source.
