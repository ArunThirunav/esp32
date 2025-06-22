import asyncio
from bleak import BleakClient
import sys

BLE_DEVICE_ADDRESS = "A0:85:E3:F0:FB:82"  # Replace with your ESP32's MAC
CHAR_UUID = "A2BD0013-AD84-44BE-94BB-B289C6D34F32"
CHUNK_SIZE = 500

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
        await client.write_gatt_char(CHAR_UUID, byte_array)
        print(f"Sent {len(byte_array)} bytes. Waiting for ACK...")

if __name__ == "__main__":
    byte_array = bytes([0xA5, 0x50, 0x00, 0x00, 0x00, 0x02, 0xAB, 0xCC])
    asyncio.run(send_request(byte_array, crc32_iso(byte_array)))

'''
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
BBBBBBBBBBBB
'''    