import struct

with open(r'BUILD\ft33.bin', 'rb+', 0) as f:
    checksum = 0
    for _ in range(7):
        checksum += struct.unpack('i', f.read(4))[0]
    checksum = -checksum
    f.write(struct.pack('1i', checksum))