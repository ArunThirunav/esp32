# file_size = os.path.getsize(BIN_FILE)
# print(f"Sending {file_size} bytes in {chunk_size}-byte chunks...")
# with open(BIN_FILE, "rb") as f:
#     index = 0
#     while True:
#         chunk = f.read(chunk_size)
#         if not chunk:
#             break
#         print(f"Sent chunk {index + 1} ({len(chunk)} bytes) \n {chunk} \n")
#         index += 1
import asyncio
from bleak import BleakClient, BleakScanner
import os

# === CONFIGURATION ===
DEVICE_NAME = "nimble_prph"  # or use the MAC address like "AA:BB:CC:DD:EE:FF"
CHAR_UUID = "00000006-8c26-476f-89a7-a108033a69c7"  # Change to your characteristic UUID
BIN_FILE = "bleprph.bin"
chunk_size = 500

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
            while True:
                chunk = f.read(chunk_size)
                if not chunk:
                    break

                await client.write_gatt_char(char_uuid, chunk, response=False)
                print(f"Sent chunk {index + 1} ({len(chunk)} bytes)")
                index += 1

        print("File sent successfully.")

async def main():
    print("Scanning for BLE devices...")
    devices = await BleakScanner.discover()
    target_device = None

    for d in devices:
        if DEVICE_NAME.lower() in (d.name or "").lower():
            target_device = d
            break

    if not target_device:
        print(f"Device '{DEVICE_NAME}' not found.")
        return

    await send_file_in_chunks(target_device.address, CHAR_UUID, BIN_FILE, chunk_size)

if __name__ == "__main__":
    asyncio.run(main())
