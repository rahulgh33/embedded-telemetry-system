#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <thread>
#include "../include/protocol.h"

using namespace telemetry;

class TelemetryClient {
private:
    int socket_fd_;
    struct sockaddr_in client_addr_, server_addr_;
    uint16_t expected_packet_id_;

public:
    TelemetryClient() : expected_packet_id_(0) {
        // Create UDP socket
        socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_fd_ < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        // Configure client address
        memset(&client_addr_, 0, sizeof(client_addr_));
        client_addr_.sin_family = AF_INET;
        client_addr_.sin_addr.s_addr = INADDR_ANY;
        client_addr_.sin_port = htons(CLIENT_PORT);

        // Bind client socket
        if (bind(socket_fd_, (struct sockaddr*)&client_addr_, sizeof(client_addr_)) < 0) {
            close(socket_fd_);
            throw std::runtime_error("Failed to bind client socket");
        }

        // Configure server address for sending ACK/NAK
        memset(&server_addr_, 0, sizeof(server_addr_));
        server_addr_.sin_family = AF_INET;
        server_addr_.sin_addr.s_addr = inet_addr(LOCALHOST);
        server_addr_.sin_port = htons(SERVER_PORT);

        std::cout << "Telemetry Client started on port " << CLIENT_PORT << std::endl;
    }

    ~TelemetryClient() {
        if (socket_fd_ >= 0) {
            close(socket_fd_);
        }
    }

    void run() {
        std::cout << "Client listening for telemetry packets..." << std::endl;
        
        while (true) {
            TelemetryPacket packet;
            socklen_t addr_len = sizeof(server_addr_);
            
            // Receive packet from server
            ssize_t bytes_received = recvfrom(socket_fd_, &packet, sizeof(packet), 0,
                                            (struct sockaddr*)&server_addr_, &addr_len);
            
            if (bytes_received < 0) {
                std::cerr << "Error receiving packet" << std::endl;
                continue;
            }
            
            if (bytes_received != sizeof(TelemetryPacket)) {
                std::cerr << "Received packet of wrong size: " << bytes_received 
                         << " (expected " << sizeof(TelemetryPacket) << ")" << std::endl;
                send_nak(packet.id);
                continue;
            }
            
            // Validate packet
            if (validate_packet(packet)) {
                process_telemetry(packet);
                send_ack(packet.id);
                expected_packet_id_ = packet.id + 1;
            } else {
                send_nak(packet.id);
            }
        }
    }

private:
    bool validate_packet(const TelemetryPacket& packet) {
        // Check packet type
        if (packet.type != static_cast<uint8_t>(PacketType::TELEMETRY)) {
            std::cerr << "Invalid packet type: " << (int)packet.type << std::endl;
            return false;
        }
        
        // Verify CRC32
        uint32_t calculated_crc = calculate_crc32(&packet, sizeof(packet) - sizeof(packet.crc32));
        if (calculated_crc != packet.crc32) {
            std::cerr << "CRC mismatch! Calculated: 0x" << std::hex << calculated_crc 
                     << ", Received: 0x" << packet.crc32 << std::dec << std::endl;
            return false;
        }
        
        // Check for duplicate or out-of-order packets
        if (packet.id < expected_packet_id_) {
            std::cout << "Duplicate packet ID " << packet.id << " (expected >= " 
                     << expected_packet_id_ << ")" << std::endl;
            return true; // Still send ACK for duplicates
        }
        
        return true;
    }
    
    void process_telemetry(const TelemetryPacket& packet) {
        std::cout << "Packet ID: " << packet.id 
                 << " | Timestamp: " << packet.timestamp
                 << " | Temp: " << packet.sensor1 << "°C"
                 << " | Pressure: " << packet.sensor2 << " atm"
                 << " | Voltage: " << packet.sensor3 << "V" << std::endl;
    }
    
    void send_ack(uint16_t packet_id) {
        AckPacket ack;
        ack.type = static_cast<uint8_t>(PacketType::ACK);
        ack.ack_id = packet_id;
        ack.crc32 = calculate_crc32(&ack, sizeof(ack) - sizeof(ack.crc32));
        
        ssize_t bytes_sent = sendto(socket_fd_, &ack, sizeof(ack), 0,
                                   (struct sockaddr*)&server_addr_, sizeof(server_addr_));
        
        if (bytes_sent < 0) {
            std::cerr << "Failed to send ACK for packet " << packet_id << std::endl;
        }
    }
    
    void send_nak(uint16_t packet_id) {
        AckPacket nak;
        nak.type = static_cast<uint8_t>(PacketType::NAK);
        nak.ack_id = packet_id;
        nak.crc32 = calculate_crc32(&nak, sizeof(nak) - sizeof(nak.crc32));
        
        ssize_t bytes_sent = sendto(socket_fd_, &nak, sizeof(nak), 0,
                                   (struct sockaddr*)&server_addr_, sizeof(server_addr_));
        
        if (bytes_sent < 0) {
            std::cerr << "Failed to send NAK for packet " << packet_id << std::endl;
        } else {
            std::cout << "Sent NAK for packet " << packet_id << std::endl;
        }
    }
};

int main() {
    try {
        TelemetryClient client;
        client.run();
    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}