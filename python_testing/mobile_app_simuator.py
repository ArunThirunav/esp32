import asyncio
from bleak import BleakClient
import sys

BLE_DEVICE_ADDRESS = "A0:85:E3:F1:87:CE"  # Replace with your ESP32's MAC
# BLE_DEVICE_ADDRESS = "A0:85:E3:F0:FB:82"  # Replace with your ESP32's MAC
# BLE_DEVICE_ADDRESS = "A0:85:E3:F1:8F:2A"  # Replace with your ESP32's MAC


WRITE_CHAR_UUID = "A2BD0013-AD84-44BE-94BB-B289C6D34F32"
READ_CHAR_UUID = "A2BD0012-AD84-44BE-94BB-B289C6D34F32"
CHUNK_SIZE = 500
TOTAL_SIZE = 5312

ack_event = asyncio.Event()

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

async def send_request(byte_array, crc):
    byte_array += crc.to_bytes(4, byteorder='big')
    print(byte_array)
    async with BleakClient(BLE_DEVICE_ADDRESS) as client:
        await client.write_gatt_char(WRITE_CHAR_UUID, byte_array)
        print(f"Sent {len(byte_array)} bytes.")
        index = 0
        received_data = bytearray()
        while len(received_data) < TOTAL_SIZE:
            chunk = await client.read_gatt_char(READ_CHAR_UUID)
            received_data.extend(chunk)
            index += 1
            print(f"Packet#{index}Read {len(chunk)} bytes, Total: {len(received_data)} bytes")

            if len(chunk) < CHUNK_SIZE:
                print("Received chunk smaller than expected — assuming end of data.")
                break
        
        new_list = received_data[6:-4]
        hex_list = [hex(b) for b in new_list]
        # print(hex_list)
        for i in range(0, len(hex_list), 10):
            print(hex_list[i:i+10])
        print("Length: ", len(hex_list))
        print(f"Total received: {len(received_data)} bytes")
        print(f"Data (hex): {received_data.hex()[:100]}...")  # Print first 50 bytes as hex

if __name__ == "__main__":
    byte_array = bytes([0xA5, 0x50, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0xEE, 0x35])
    asyncio.run(send_request(byte_array, crc32_iso(byte_array)))
   