import serial

# Replace with your serial port and baud rate
SERIAL_PORT = "/dev/ttyUSB0"   # Example for Linux. On Windows, it might be "COM3"
BAUD_RATE = 1000000
OUTPUT_FILE = "output.bin"     # Name of the output binary file
def main():
    length = 0
    try:
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
            print(f"Listening on {SERIAL_PORT} at {BAUD_RATE} baud rate.")
            with open(OUTPUT_FILE, "wb") as f: 
                while True:
                    data = ser.read((256*1024))  # Reads up to 512 bytes or until timeout
                    if data:
                        f.write(data)
                        length += len(data)
                        print("Len of data: ", length)
    except serial.SerialException as e:
        print(f"Serial exception: {e}")

if __name__ == "__main__":
    main()
