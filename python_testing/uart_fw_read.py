import serial
import time
import os

PORT = "/dev/ttyUSB0"         # Change this to match your system
BAUDRATE = 1000000            # 1 Mbps
CHUNK_SIZE = 256 * 1024       # 256 KB
TOTAL_CHUNKS = 4              # Change this for more/less chunks
TIMEOUT = 10                  # Timeout per read

def read_uart_chunks():
    try:
        with serial.Serial(PORT, BAUDRATE, timeout=TIMEOUT) as ser:
            print(f"Reading {TOTAL_CHUNKS} chunks of {CHUNK_SIZE} bytes...")

            for chunk_num in range(TOTAL_CHUNKS):
                print(f"\nWaiting for chunk {chunk_num + 1}/{TOTAL_CHUNKS}...")

                buffer = bytearray()
                start_time = time.time()

                while len(buffer) < CHUNK_SIZE:
                    data = ser.read(CHUNK_SIZE - len(buffer))
                    if data:
                        buffer.extend(data)
                    else:
                        print(f"Timeout while reading chunk {chunk_num + 1}")
                        break

                duration = time.time() - start_time
                print(f"Received chunk {chunk_num + 1}: {len(buffer)} bytes in {duration:.2f}s")

                filename = f"chunk_{chunk_num + 1:02}.bin"
                with open(filename, "wb") as f:
                    f.write(buffer)
                print(f"Saved to {filename}")

    except serial.SerialException as e:
        print(f"Serial error: {e}")

if __name__ == "__main__":
    read_uart_chunks()
