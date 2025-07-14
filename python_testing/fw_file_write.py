import asyncio
import binascii
from bleak import BleakClient
import time

BLE_DEVICE_ADDRESS = "A0:85:E3:F0:76:16"  # Replace with your ESP32's MAC
CHAR_UUID = "A2BD0011-AD84-44BE-94BB-B289C6D34F32"
REQ_CHAR_UUID = "A2BD0013-AD84-44BE-94BB-B289C6D34F32"
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

def create_packet(payload: bytes) -> bytes:
    header = bytearray()
    header.append(0xA5)
    header.append(0x21)
    header += len(payload).to_bytes(4, byteorder='little')

    crc = crc32_iso(header + payload)
    crc_bytes = crc.to_bytes(4, byteorder='big')

    return header + payload + crc_bytes

def handle_notification(sender, data):
    print("data: ", data)
    if data == b'\x01' or data == b'\x00':
        ack_event.set()    

async def send_binary_file_ble(file_path: str, address: str, char_uuid: str):
    async with BleakClient(address) as client:
        print(f"Connected: {client.is_connected}")
        await client.start_notify(REQ_CHAR_UUID, handle_notification)
        start_time = time.time()
        total_bytes_sent = 0
        start_packet = bytearray([
            0xA5, 0x20, 0x00, 0x00, 0x00, 0x00, 
            0x39, 0xD2,0x0E, 0xFA
        ])

        await client.write_gatt_char(REQ_CHAR_UUID, start_packet, response=False)
        await ack_event.wait()

        with open(file_path, 'rb') as file:
            chunk_num = 0
            while True:
                chunk = file.read(CHUNK_SIZE)
                if not chunk:
                    break

                packet = create_packet(chunk)
                await client.write_gatt_char(char_uuid, packet, response=False)

                print(f"Sent chunk {chunk_num}, size {(len(packet)-10)*chunk_num} bytes")
                chunk_num += 1
                total_bytes_sent += len(packet)
                await asyncio.sleep(0.001)  # Slight delay to avoid flooding
        
        end_time = time.time()
        elapsed_time = end_time - start_time

        # Speed Calculation
        speed_bps = total_bytes_sent / elapsed_time  # Bytes per second
        speed_mbps = (speed_bps * 8) / 1_000_000     # Convert to Megabits per second

        print(f"\nTransfer Complete:")
        print(f"Total bytes sent: {total_bytes_sent} bytes")
        print(f"Total time taken: {elapsed_time:.2f} seconds")
        print(f"Transfer speed: {speed_bps:.2f} Bytes/sec ({speed_mbps:.2f} Mbps)")
        
        await asyncio.sleep(5)  # Slight delay to avoid flooding
        
        end_packet = bytearray([
            0xA5, 0x22, 0x00, 0x00, 0x00, 0x00,
            0x43, 0x12, 0x5D, 0x9A
        ])

        await client.write_gatt_char(REQ_CHAR_UUID, end_packet, response=False)


if __name__ == "__main__":
    # file_path = "fw_0_0_28_0_e42d49c7.bin"
    file_path = "test_1mb.bin"
    # file_path = "nexus.cfg"

    asyncio.run(send_binary_file_ble(
        file_path,
        BLE_DEVICE_ADDRESS,
        CHAR_UUID
    ))
