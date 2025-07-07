CHUNK_SIZE = 1  # 1 MB
total_size = 1 * 1024 * 1024  # 20 MB
pattern = b'\x31' * CHUNK_SIZE  # Fill with '1' character (0x31)

with open("test_.bin", "wb") as f:
    for _ in range(total_size // CHUNK_SIZE):
        f.write(pattern)
