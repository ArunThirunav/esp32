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

# Test
# data = bytes([0xA5, 0x50, 0x00])
value = 0xA55000
byte_array = value.to_bytes(3, byteorder='big')  # 3 bytes for 24 bits
print(f"CRC32 (ISO): 0x{crc32_iso(byte_array):08X}")

# value = 0xA55000
# byte_array = value.to_bytes(3, byteorder='big')  # 3 bytes for 24 bits
# print(byte_array)           # Output: b'\xa5P\x00'
# a = crc32(byte_array)
# crc = zlib.crc32(byte_array) & 0xFFFFFFFF
# print(f"CRC32 (ISO): 0x{crc:08X}")
# print(hex(a))