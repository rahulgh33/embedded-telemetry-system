#pragma once

#include <cstdint>
#include <cstddef>

namespace telemetry {

// Packet types
enum class PacketType : uint8_t {
    TELEMETRY = 0x01,
    ACK = 0x02,
    NAK = 0x03
};

// Main telemetry packet structure
struct TelemetryPacket {
    uint8_t type;           // Packet type (TELEMETRY)
    uint16_t id;            // Unique packet ID
    uint32_t timestamp;     // Timestamp (milliseconds since start)
    float sensor1;          // Temperature sensor (°C)
    float sensor2;          // Pressure sensor (atm)
    float sensor3;          // Voltage sensor (V)
    uint32_t crc32;         // CRC32 checksum (covers everything above)
} __attribute__((packed));

// ACK/NAK packet structure
struct AckPacket {
    uint8_t type;           // Packet type (ACK or NAK)
    uint16_t ack_id;        // ID of packet being acknowledged
    uint32_t crc32;         // CRC32 checksum
} __attribute__((packed));

// Network configuration
constexpr int SERVER_PORT = 8080;
constexpr int CLIENT_PORT = 8081;
constexpr int MAX_PACKET_SIZE = 256;
constexpr int TIMEOUT_MS = 1000;        // 1 second timeout
constexpr int MAX_RETRIES = 3;
constexpr const char* LOCALHOST = "127.0.0.1";

// Helper function to calculate CRC32
uint32_t calculate_crc32(const void* data, size_t length);

} // namespace telemetry