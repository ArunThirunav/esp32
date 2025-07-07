import asyncio
from bleak import BleakClient
import sys
from datetime import datetime
# BLE_DEVICE_ADDRESS = "A0:85:E3:F0:FB:82"  # Replace with your ESP32's MAC
BLE_DEVICE_ADDRESS = "A0:85:E3:F0:76:16"  # Replace with your ESP32's MAC
CHAR_UUID = "A2BD0011-AD84-44BE-94BB-B289C6D34F32"
CHUNK_SIZE = 500

ack_event = asyncio.Event()

def handle_notification(sender, data):
    if data == b'\xAA':
        # print("Received ACK")
        ack_event.set()

async def send_file(file_path):
    async with BleakClient(BLE_DEVICE_ADDRESS) as client:
        await client.start_notify(CHAR_UUID, handle_notification)

        with open(file_path, "rb") as f:
            start_time = datetime.now()
            while chunk := f.read(CHUNK_SIZE):
                ack_event.clear()
                await client.write_gatt_char(CHAR_UUID, chunk)
                # print(f"Sent {len(chunk)} bytes. Waiting for ACK...")
                await ack_event.wait()

        end_time = datetime.now()
        time_diff = end_time - start_time
        print("Time Taken: ", time_diff)
        print("File sent successfully.")
        await client.stop_notify(CHAR_UUID)

if __name__ == "__main__":
    asyncio.run(send_file("test_100kb.bin"))