import asyncio
import zlib
from bleak import BleakClient

# --------- Constants ---------
HEADER_START_BYTE = 0xA5
HEADER_TYPE_BYTE = 0x54
BLOCK_SIZE = 500  # Maximum payload size per packet

# Replace with your BLE device address and characteristic UUID
BLE_DEVICE_ADDRESS = "A0:85:E3:F0:76:16"  # Replace with your ESP32's MAC
CHARACTERISTIC_UUID = "A2BD0012-AD84-44BE-94BB-B289C6D34F32"

# --------- Functions ---------

def create_payload_block(data_block: bytes) -> bytes:
    # Build header
    length_bytes = len(data_block).to_bytes(4, 'little')  # 4 bytes, little-endian
    header = bytes([HEADER_START_BYTE, HEADER_TYPE_BYTE]) + length_bytes

    # Combine header + payload
    full_payload = header + data_block

    # Calculate CRC32-ISO over header + payload
    crc = zlib.crc32(full_payload) & 0xFFFFFFFF
    crc_bytes = crc.to_bytes(4, 'big')

    # Final packet: header + payload + CRC
    return full_payload + crc_bytes


async def send_file_over_ble(file_path: str, ble_address: str, char_uuid: str):
    async with BleakClient(ble_address) as client:
        print(f"Connected: {await client.is_connected()}")

        # Read file and send in chunks
        with open(file_path, 'rb') as file:
            while True:
                data_block = file.read(BLOCK_SIZE)
                if not data_block:
                    break

                packet = create_payload_block(data_block)
                print(f"Sending packet of size {len(packet)} bytes")
                
                # Send the packet over the BLE characteristic
                await client.write_gatt_char(char_uuid, packet)
                await asyncio.sleep(0.05)  # small delay between packets (adjust as needed)

        print("File transfer complete.")


# --------- Main ---------

if __name__ == "__main__":
    file_path = "nexuscfg.cfg"  # Replace with your file path
    asyncio.run(send_file_over_ble(file_path, BLE_DEVICE_ADDRESS, CHARACTERISTIC_UUID))
