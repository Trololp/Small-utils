from struct import unpack
import sys

# for debug purposes to list vertex contents

VRTX_OFFSET = 0x16E0
VRTX_AMT = 0xCB

f = open("NM40_5353472", "rb")

f.seek(VRTX_OFFSET, 0)

for i in range(VRTX_AMT):
    f1 = unpack("f", f.read(4))[0]
    f2 = unpack("f", f.read(4))[0]
    f3 = unpack("f", f.read(4))[0]
    f4 = unpack("f", f.read(4))[0]
    f5 = unpack("f", f.read(4))[0]
    f6 = unpack("f", f.read(4))[0]
    f7 = unpack("f", f.read(4))[0]
    f8 = unpack("f", f.read(4))[0]
    b1 = unpack("B", f.read(1))[0]
    b2 = unpack("B", f.read(1))[0]
    b3 = unpack("B", f.read(1))[0]
    b4 = unpack("B", f.read(1))[0]
    f9  = unpack("f", f.read(4))[0]
    f10 = unpack("f", f.read(4))[0]
    i1  = unpack("I", f.read(4))[0]
    i2  = unpack("I", f.read(4))[0]
    #print(f1, f2, f3, f4, f5, f6, f7, f8, b1, b2, b3, b4, f9, f10, i1, i2)
    print(f"{f4: .2f}, {f5: .2f}, {f6: .2f}, {f7: .2f}, {f8: .2f}", b1, b2, b3, b4)


    
