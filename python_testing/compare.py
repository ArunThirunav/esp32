def compare_bin_files(file1, file2):
    with open(file1, 'rb') as f1, open(file2, 'rb') as f2:
        pos = 0
        diff_count = 0
        while True:
            b1 = f1.read(1)
            b2 = f2.read(1)

            if not b1 and not b2:
                break  # Reached end of both files

            if b1 != b2:
                print(f"Difference at byte {pos}: {b1.hex()} != {b2.hex()}")
                diff_count += 1

            pos += 1

        print(f"\nTotal differences: {diff_count}")
        print(f"Total bytes compared: {pos}")

# Example usage
compare_bin_files('in.txt', 'out.txt')
