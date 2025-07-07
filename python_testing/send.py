import asyncio
from bleak import BleakClient, BleakScanner
import os
from datetime import datetime

# === CONFIGURATION ===
# BLE_DEVICE_ADDRESS = "A0:85:E3:F1:87:CE"  # Replace with your ESP32's MAC
# BLE_DEVICE_ADDRESS = "A0:85:E3:F0:FB:82"  # Replace with your ESP32's MAC
# BLE_DEVICE_ADDRESS = "A0:85:E3:F1:8F:2A"  # Replace with your ESP32's MAC
BLE_DEVICE_ADDRESS = "A0:85:E3:F0:76:16"  # Replace with your ESP32's MAC
# BLE_DEVICE_ADDRESS = "A0:85:E3:F1:8C:C6"  # Replace with your ESP32's MAC

CHAR_UUID = "A2BD0011-AD84-44BE-94BB-B289C6D34F32"
# CHAR_UUID = "FF01"
CHUNK_SIZE = 500
# BIN_FILE = "test_20mb.bin"
BIN_FILE = "nexus_cfg_main.cfg"


async def send_file_in_chunks(address, char_uuid, file_path, chunk_size):
    async with BleakClient(address) as client:
        if not client.is_connected:
            print("Failed to connect.")
            return

        print(f"Connected to {address}")
        file_size = os.path.getsize(file_path)
        print(f"Sending {file_size} bytes in {chunk_size}-byte chunks...")

        with open(file_path, "rb") as f:
            index = 0
            start_time = datetime.now()
            while True:
                chunk = f.read(chunk_size)
                if not chunk:
                    break

                await client.write_gatt_char(char_uuid, chunk, response=False)
                # print(f"Sent chunk {index + 1} ({len(chunk)} bytes)")
                index += 1
        end_time = datetime.now()
        time_diff = end_time - start_time
        print("File sent successfully.")
        print("Time Taken: ", time_diff)


async def main():
    await send_file_in_chunks(BLE_DEVICE_ADDRESS, CHAR_UUID, BIN_FILE, CHUNK_SIZE)

if __name__ == "__main__":
    asyncio.run(main())
