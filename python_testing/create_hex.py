# Size in bytes
size_kb = 5 * 1024  # 5 KB = 5120 bytes

# Create a byte array with some pattern
data = bytearray([i % 256 for i in range(size_kb)])

# Write to a binary file
with open('output.hex', 'wb') as f:
    f.write(data)

print("5KB hex file created as 'output.hex'")
