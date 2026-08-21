#!/usr/bin/env python3
"""
Test script to send simulated object detection data to the UDP receiver.
This helps testing the inference tab without needing actual object detection.
"""

import socket
import struct
import time
import random

# Configuration
UDP_IP = "127.0.0.1"
UDP_PORT = 12347
START_DELIMITER = 0xDDCCBBAA

# Sample object classes
CLASSES = ["person", "car", "dog", "cat", "bicycle", "bird", "bottle", "chair"]

def create_detection_packet(x, y, width, height, name, confidence):
    """
    Create a detection packet matching the C receiver's expected format.
    """
    packet = bytearray()
    
    # Delimiter (4 bytes, little-endian)
    packet.extend(struct.pack('<I', START_DELIMITER))
    
    # Coordinates and dimensions (4 * 4 bytes)
    packet.extend(struct.pack('<i', x))
    packet.extend(struct.pack('<i', y))
    packet.extend(struct.pack('<i', width))
    packet.extend(struct.pack('<i', height))
    
    # Name length and name string
    name_bytes = name.encode('utf-8')
    packet.extend(struct.pack('<B', len(name_bytes)))
    packet.extend(name_bytes)
    
    # Confidence (4 bytes float)
    packet.extend(struct.pack('<f', confidence))
    
    return bytes(packet)

def send_test_detections(count=10, interval=1.0):
    """
    Send simulated detection packets to the UDP receiver.
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    print(f"Sending {count} test detections to {UDP_IP}:{UDP_PORT}")
    print("Make sure the UDP receiver is running!")
    print("-" * 60)
    
    for i in range(count):
        # Generate random detection
        x = random.randint(0, 1920)
        y = random.randint(0, 1080)
        width = random.randint(50, 300)
        height = random.randint(50, 400)
        name = random.choice(CLASSES)
        confidence = random.uniform(0.5, 0.99)
        
        # Create and send packet
        packet = create_detection_packet(x, y, width, height, name, confidence)
        sock.sendto(packet, (UDP_IP, UDP_PORT))
        
        print(f"[{i+1}/{count}] Sent: {name} ({confidence:.2%}) at [{x},{y}] size [{width}x{height}]")
        
        time.sleep(interval)
    
    sock.close()
    print("-" * 60)
    print("Test completed!")

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="Send test detection packets to UDP receiver")
    parser.add_argument("-c", "--count", type=int, default=10, help="Number of detections to send")
    parser.add_argument("-i", "--interval", type=float, default=1.0, help="Interval between packets (seconds)")
    parser.add_argument("-p", "--port", type=int, default=12347, help="UDP port")
    
    args = parser.parse_args()
    UDP_PORT = args.port
    
    try:
        send_test_detections(args.count, args.interval)
    except KeyboardInterrupt:
        print("\nInterrupted by user")
    except Exception as e:
        print(f"Error: {e}")
