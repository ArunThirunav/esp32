import asyncio
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.backends import default_backend
from bleak import BleakClient

BLE_DEVICE = "A0:85:E3:F0:76:16"  # Your ESP32 BLE MAC
CHALLENGE_UUID = "A2BD0014-AD84-44BE-94BB-B289C6D34F32"
RESPONSE_UUID = "A2BD0014-AD84-44BE-94BB-B289C6D34F32"

# Load private key
with open("rsa_private.pem", "rb") as f:
    private_key = serialization.load_pem_private_key(
        f.read(), password=None, backend=default_backend())

async def run():
    async with BleakClient(BLE_DEVICE) as client:
        print("Connected")

        # Read challenge
        challenge = await client.read_gatt_char(CHALLENGE_UUID)
        print("Challenge:", challenge.hex())

        # Sign it
        signature = private_key.sign(
            challenge,
            padding.PKCS1v15(),
            hashes.SHA256()
        )

        # Send signature
        await client.write_gatt_char(RESPONSE_UUID, signature)
        print("Signature sent ✅")

asyncio.run(run())
