import serial
import time
import sys

# Configuration
PORT = '/dev/ttyUSB0'  # Change to your serial port (COM3, /dev/ttyUSB0, etc.)
BAUDRATE = 460800
PACKET_SIZE = 10 * 1024  # 5KB

def generate_dummy_packet():
    """Generate 256 bytes alternating between A-Z and a-z"""
    result = bytearray()
    j = 0
    for i in range(1, PACKET_SIZE):
        if(i % 120 == 0):
            j += 1
        else:
            result.append(65 + j)
    # print(result)
    return bytes(result)

def send_uart_packet():
    try:
        # Initialize serial port
        ser = serial.Serial(
            port=PORT,
            baudrate=BAUDRATE,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            bytesize=serial.EIGHTBITS,
            timeout=1
        )
        
        # Generate dummy data
        dummy_data = generate_dummy_packet()
        print(f"Generated {len(dummy_data)} dummy packet")
        
        # Send data
        print(f"Sending to {PORT}...")
        start_time = time.time()
        ser.write(dummy_data)
        ser.flush()
        elapsed = time.time() - start_time
        
        # Calculate throughput
        kbps = (PACKET_SIZE * 8 / 1000) / elapsed
        print(f"Sent {len(dummy_data)} bytes in {elapsed:.2f}s ({kbps:.1f}kbps)")
        
        # Close port
        ser.close()
        
    except Exception as e:
        print(f"Error: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    send_uart_packet()

