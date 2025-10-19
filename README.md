# Reliable Telemetry over UDP

A small C++ practice project that implements basic reliability features over UDP using CRC checks, acknowledgments, and retry logic. The goal was to understand how low-level communication protocols handle data integrity and retransmission without relying on TCP.

## Overview

The project simulates a simple telemetry link between a server and a client.  
The server sends packets containing simulated sensor data, while the client validates them and replies with an acknowledgment (ACK) or negative acknowledgment (NAK).  
If the server does not receive an ACK within a timeout window, it retries the transmission.

This setup mirrors how embedded systems or networked devices often layer reliability on top of UDP.

## Features

- CRC32 checksum for packet integrity verification  
- ACK/NAK acknowledgment system  
- Retry and timeout mechanism for reliable delivery  
- Structured binary packet serialization  
- Clean C++ implementation using RAII and POSIX sockets  

## Project Structure

```
reliable-telemetry/
|-- include/
|   └── protocol.h              # Shared protocol definitions and constants
|
|-- src/
|   |-- telemetry_server.cpp    # Server-side implementation
|   |-- telemetry_client.cpp    # Client-side implementation
|   └── crc32.cpp               # CRC32 checksum utility
|
|-- Makefile                    # Build configuration
└── README.md                   # Project documentation
```



## How It Works

**Server**
- Generates random telemetry data (temperature, pressure, voltage)
- Calculates a CRC32 checksum for each packet
- Sends the packet to the client using UDP
- Waits for an ACK response; retries if timeout or NAK

**Client**
- Listens for incoming telemetry packets
- Verifies the CRC32 checksum and packet type
- Sends ACK for valid packets or NAK for corrupted ones
- Prints received sensor data for verification

## Packet Format

| Field     | Type     | Description                  |
|------------|----------|------------------------------|
| type       | uint8_t  | Packet type (telemetry/ack/nak) |
| id         | uint16_t | Sequential packet ID         |
| timestamp  | uint32_t | Time in milliseconds         |
| sensor1    | float    | Simulated temperature data   |
| sensor2    | float    | Simulated pressure data      |
| sensor3    | float    | Simulated voltage data       |
| crc32      | uint32_t | Checksum for data integrity  |

## Example Output

Server: Sending packet 4
Client: Received packet 4 | Temp=24.6°C | Pressure=1.12 atm | Voltage=3.3V
Server: ACK received for packet 4


If corruption or timeout occurs:

Client: CRC mismatch on packet 5
Client: Sent NAK for packet 5
Server: Timeout waiting for ACK (retry 1/3)


## Build and Run

make
./build/telemetry_client # Run in one terminal
./build/telemetry_server # Run in another terminal


Configuration parameters such as ports, timeout duration, and retry count are defined in `protocol.h`.

Author: Rahul Ghosh
