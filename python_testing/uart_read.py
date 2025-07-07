import serial

# Replace with your serial port and baud rate
SERIAL_PORT = "/dev/ttyUSB0"   # Example for Linux. On Windows, it might be "COM3"
BAUD_RATE = 115200

def main():
    try:
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
            print(f"Listening on {SERIAL_PORT} at {BAUD_RATE} baud rate.")
            while True:
                data = ser.read(500)  # Reads up to 512 bytes or until timeout
                if data:
                    for i in range(0, len(data), 30):
                        row = data[i:i+30]
                        print(' '.join(f'{b:02X}' for b in row))
    except serial.SerialException as e:
        print(f"Serial exception: {e}")

if __name__ == "__main__":
    main()
