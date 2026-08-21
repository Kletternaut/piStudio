// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
//
// Copyright (C) 2025-2026 Kletternaut <https://github.com/Kletternaut/piStudio>
//
// udp_object_detection.c - Standalone UDP receiver for AI object detection data.
//
// Receives and parses binary detection packets sent by rpicam-apps
// object_detect_udp_stage (https://github.com/raspberrypi/rpicam-apps).
// Implements the same binary protocol (0xDDCCBBAA delimiter) independently
// in C with additional tracker ID and timestamp support.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <time.h>

#define MAX_BUFFER_SIZE 1024
#define DEFAULT_UDP_PORT 12347
#define START_DELIMITER_LE 0xDDCCBBAA
#define MIN_PACKET_SIZE 25  // Delimiter(4) + x(4) + y(4) + w(4) + h(4) + name_len(1) + conf(4)

// Structure to hold parsed detection data
typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    char name[256];
    float confidence;
    char timestamp[32];
    int32_t id; // optional tracker id, -1 if unset
} ParsedDetection;

// Function to get current timestamp
void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
}

// Function to parse detection from buffer
int parse_detection(const uint8_t *buffer, size_t len, ParsedDetection *det) {
    if (len < MIN_PACKET_SIZE) {
        fprintf(stderr, "Packet too small: %zu bytes\n", len);
        return -1;
    }

    // Check delimiter
    uint32_t delimiter;
    memcpy(&delimiter, buffer, sizeof(uint32_t));
    if (delimiter != START_DELIMITER_LE) {
        fprintf(stderr, "Invalid delimiter: 0x%08X\n", delimiter);
        return -1;
    }

    size_t offset = 4;

    // Parse coordinates and dimensions
    memcpy(&det->x, buffer + offset, sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy(&det->y, buffer + offset, sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy(&det->width, buffer + offset, sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy(&det->height, buffer + offset, sizeof(int32_t));
    offset += sizeof(int32_t);

    // Parse name length
    uint8_t name_length = buffer[offset];
    offset += 1;

    // Validate name length
    if (offset + name_length + 4 > len) {
        fprintf(stderr, "Invalid name length: %u\n", name_length);
        return -1;
    }

    // Parse name
    memcpy(det->name, buffer + offset, name_length);
    det->name[name_length] = '\0';
    offset += name_length;

    // Parse confidence
    memcpy(&det->confidence, buffer + offset, sizeof(float));
    offset += sizeof(float);

    // Optional: parse tracker id if present (int32)
    det->id = -1;
    if (offset + sizeof(int32_t) <= len) {
        memcpy(&det->id, buffer + offset, sizeof(int32_t));
        offset += sizeof(int32_t);
    }

    // Add timestamp
    get_timestamp(det->timestamp, sizeof(det->timestamp));

    return 0;
}

// Main receiver function
int main(int argc, char *argv[]) {
    int port = DEFAULT_UDP_PORT;
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    uint8_t buffer[MAX_BUFFER_SIZE];
    ssize_t recv_len;

    // Parse command line arguments
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port number: %s\n", argv[1]);
            return 1;
        }
    }

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
        return 1;
    }

    // Bind socket to port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Failed to bind socket to port %d: %s\n", port, strerror(errno));
        close(sockfd);
        return 1;
    }

    printf("UDP Object Detection Receiver\n");
    printf("Listening on port %d...\n", port);
    printf("----------------------------------------\n");
    fflush(stdout);

    // Main receive loop
    while (1) {
        recv_len = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0,
                           (struct sockaddr *)&client_addr, &client_len);

        if (recv_len < 0) {
            fprintf(stderr, "Error receiving data: %s\n", strerror(errno));
            continue;
        }

        ParsedDetection det;
        if (parse_detection(buffer, recv_len, &det) == 0) {
            // Output detection in a structured format for GUI parsing
                // Print detection with object and optional id first, confidence next, location, then timestamp at end
                if (det.id != -1) {
                    printf("%s id=%d (%.2f%%) at [%d,%d] size [%dx%d] [%s]\n",
                           det.name,
                           det.id,
                           det.confidence * 100.0f,
                           det.x, det.y,
                           det.width, det.height,
                           det.timestamp);
                } else {
                    printf("%s (%.2f%%) at [%d,%d] size [%dx%d] [%s]\n",
                           det.name,
                           det.confidence * 100.0f,
                           det.x, det.y,
                           det.width, det.height,
                           det.timestamp);
                }
            fflush(stdout);
        }
    }

    close(sockfd);
    return 0;
}
