#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <iostream>
#include <stdexcept>
#include <random>
#include <chrono>
#include <thread>
#include "../include/protocol.h"

using namespace telemetry;

class TelemetryServer {
private:
    int socket_fd_;
    struct sockaddr_in server_addr_, client_addr_;
    uint16_t packet_counter_;
    std::mt19937 rng_;
    std::uniform_real_distribution<float> temp_dist_;
    std::uniform_real_distribution<float> pressure_dist_;
    std::uniform_real_distribution<float> voltage_dist_;

public:
    TelemetryServer() : packet_counter_(0), rng_(std::random_device{}()),
                       temp_dist_(20.0f, 25.0f),    // Temperature: 20-25°C
                       pressure_dist_(1.0f, 1.2f),  // Pressure: 1.0-1.2 atm
                       voltage_dist_(3.2f, 3.4f) {  // Voltage: 3.2-3.4V
        
        // Create UDP socket
        socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_fd_ < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        // Configure server address
        memset(&server_addr_, 0, sizeof(server_addr_));
        server_addr_.sin_family = AF_INET;
        server_addr_.sin_addr.s_addr = INADDR_ANY;
        server_addr_.sin_port = htons(SERVER_PORT);

        // Bind socket
        if (bind(socket_fd_, (struct sockaddr*)&server_addr_, sizeof(server_addr_)) < 0) {
            close(socket_fd_);
            throw std::runtime_error("Failed to bind socket");
        }

        std::cout << "Telemetry Server started on port " << SERVER_PORT << std::endl;
    }

    ~TelemetryServer() {
        if (socket_fd_ >= 0) {
            close(socket_fd_);
        }
    }

    void run() {
        // Configure client address
        memset(&client_addr_, 0, sizeof(client_addr_));
        client_addr_.sin_family = AF_INET;
        client_addr_.sin_addr.s_addr = inet_addr(LOCALHOST);
        client_addr_.sin_port = htons(CLIENT_PORT);
        
        std::cout << "Server running... sending telemetry every 2 seconds" << std::endl;
        
        while (true) {
            TelemetryPacket packet = generate_telemetry_packet();
            
            if (send_packet_with_retry(packet)) {
                std::cout << "Successfully sent packet ID " << packet.id << std::endl;
            } else {
                std::cout << "Failed to send packet ID " << packet.id 
                         << " after " << MAX_RETRIES << " retries" << std::endl;
            }
            
            // Wait before sending next packet
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

private:
    bool send_packet_with_retry(const TelemetryPacket& packet) {
        for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
            // Send the packet
            ssize_t bytes_sent = sendto(socket_fd_, &packet, sizeof(packet), 0,
                                       (struct sockaddr*)&client_addr_, sizeof(client_addr_));
            
            if (bytes_sent < 0) {
                std::cerr << "Failed to send packet (attempt " << (attempt + 1) << ")" << std::endl;
                continue;
            }
            
            // Wait for ACK/NAK with timeout
            if (wait_for_ack(packet.id)) {
                return true; // Success!
            }
            
            std::cout << "Timeout waiting for ACK, retrying... (attempt " 
                     << (attempt + 1) << "/" << MAX_RETRIES << ")" << std::endl;
        }
        
        return false; // Failed after all retries
    }
    
    bool wait_for_ack(uint16_t packet_id) {
        // Set socket timeout
        struct timeval timeout;
        timeout.tv_sec = TIMEOUT_MS / 1000;
        timeout.tv_usec = (TIMEOUT_MS % 1000) * 1000;
        
        if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            std::cerr << "Failed to set socket timeout" << std::endl;
            return false;
        }
        
        AckPacket ack_packet;
        socklen_t addr_len = sizeof(client_addr_);
        
        ssize_t bytes_received = recvfrom(socket_fd_, &ack_packet, sizeof(ack_packet), 0,
                                         (struct sockaddr*)&client_addr_, &addr_len);
        
        if (bytes_received < 0) {
            return false; // Timeout or error
        }
        
        if (bytes_received != sizeof(AckPacket)) {
            std::cerr << "Received malformed ACK/NAK packet" << std::endl;
            return false;
        }
        
        // Verify CRC of ACK/NAK
        uint32_t calculated_crc = calculate_crc32(&ack_packet, sizeof(ack_packet) - sizeof(ack_packet.crc32));
        if (calculated_crc != ack_packet.crc32) {
            std::cerr << "ACK/NAK packet CRC mismatch" << std::endl;
            return false;
        }
        
        // Check if it's for the right packet
        if (ack_packet.ack_id != packet_id) {
            std::cout << "Received ACK/NAK for wrong packet ID: " << ack_packet.ack_id 
                     << " (expected " << packet_id << ")" << std::endl;
            return false;
        }
        
        if (ack_packet.type == static_cast<uint8_t>(PacketType::ACK)) {
            return true; // Success!
        } else if (ack_packet.type == static_cast<uint8_t>(PacketType::NAK)) {
            std::cout << "Received NAK for packet " << packet_id << std::endl;
            return false; // Need to retry
        }
        
        std::cerr << "Unknown ACK/NAK packet type: " << (int)ack_packet.type << std::endl;
        return false;
    }

    TelemetryPacket generate_telemetry_packet() {
        TelemetryPacket packet;
        packet.type = static_cast<uint8_t>(PacketType::TELEMETRY);
        packet.id = packet_counter_++;
        
        // Get current time in milliseconds
        auto now = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
        packet.timestamp = static_cast<uint32_t>(ms.count());
        
        // Generate random sensor data
        packet.sensor1 = temp_dist_(rng_);
        packet.sensor2 = pressure_dist_(rng_);
        packet.sensor3 = voltage_dist_(rng_);
        
        // Calculate CRC32 (excluding the CRC field itself)
        packet.crc32 = calculate_crc32(&packet, sizeof(packet) - sizeof(packet.crc32));
        
        return packet;
    }
};

int main() {
    try {
        TelemetryServer server;
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}