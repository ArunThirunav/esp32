import serial
import time
import os

# UART configuration
# PORT = "/dev/ttyUSB0"   # Change to "COMx" for Windows
PORT = "COM10"
BAUDRATE = 115200
TIMEOUT = 1000            # seconds
TRIGGER_PACKET_SIZE = 10
RESPONSE_SIZE = 10    # 5 KB

version_req = [0xA5, 0x52, 0x00, 0x00, 0x00, 0x00, 0xBA, 0xC0, 0xBD, 0x55]
config_req = [0xA5, 0x50, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0xEE, 0x35]

def reflect(value, bits):
    result = 0
    for i in range(bits):
        if value & (1 << i):
            result |= 1 << (bits - 1 - i)
    return result

def crc32_iso(data: bytes) -> int:
    poly = 0x04C11DB7
    crc = 0xFFFFFFFF

    for byte in data:
        byte = reflect(byte, 8)
        crc ^= byte << 24

        for _ in range(8):
            if crc & 0x80000000:
                crc = (crc << 1) ^ poly
            else:
                crc <<= 1
            crc &= 0xFFFFFFFF  # Ensure 32-bit range

    crc = reflect(crc, 32)
    return crc ^ 0xFFFFFFFF

def wait_for_packet(ser, size, timeout):
    """Wait until `size` bytes are received or timeout expires."""
    buffer = bytearray()
    start_time = time.time()

    while len(buffer) < size:
        if ser.in_waiting:
            buffer.extend(ser.read(size - len(buffer)))
        if (time.time() - start_time) > timeout:
            print("Timeout while waiting for trigger packet.")
            return None
    return buffer

def main():
    try:
        ser = serial.Serial(PORT, BAUDRATE, timeout=0.1)
        print(f"Listening on {PORT} for 10-byte trigger...")

        # Prepare 5 KB buffer (example: repeating pattern or random)
        response_data = [0xA5, 0x51, 0x0A, 0x00, 0x00, 0x00]
        response_data += bytearray(os.urandom(RESPONSE_SIZE))  # or b'\xAA' * 5120
        print([hex(num) for num in response_data])
        print("\n\n")
        response_data += crc32_iso(response_data).to_bytes(4, byteorder='big')
        print([hex(num) for num in response_data])

        while True:
            packet = wait_for_packet(ser, TRIGGER_PACKET_SIZE, TIMEOUT)
            if packet:
                print(f"Received 10-byte trigger: {packet.hex()}")
                ser.write(response_data)
                print("Sent 5 KB response.")
            else:
                time.sleep(0.1)

    except KeyboardInterrupt:
        print("Interrupted by user.")
    finally:
        ser.close()

if __name__ == "__main__":
    main()
