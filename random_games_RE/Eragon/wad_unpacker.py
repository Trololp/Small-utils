from struct import unpack
import sys

# Eragon .wad unpacker

def swap_4b(_4b_arr):
    strochencka = bytearray(b'0000')
    strochencka[0] = _4b_arr[3]
    strochencka[1] = _4b_arr[2]
    strochencka[2] = _4b_arr[1]
    strochencka[3] = _4b_arr[0]
    return bytes(strochencka)

def chnk_name(offset, _4b_arr):
    
    name = ""
    
    for i in range(4):
        if _4b_arr[i] == 0:
            name += " "
            continue
        name += chr(_4b_arr[i])
    
    if name == "////":
        name = "text"
    
    if _4b_arr == b'\xa1\x00\x00\xa1':
        name = "A1__"
    
    if _4b_arr == b'\xac\x00\x00\xac':
        name = "AC__"
    
    if _4b_arr == b'\x00\x00\x01\x00':
        name = "UNK_"
    
    if _4b_arr == b'WF\x04\x00':
        name = "WF__"
    
    name = f"{name}_{offset}"
    return name

def dump_chunk(f, size, name):
    data = f.read(size)
    f2 = open(name, 'wb')
    f2.write(data)
    f2.close
    return

def main(argv):
    f = open(argv[0], 'rb');
    hdr = f.read(8)
    #print(hdr)
    if ((hdr[0:4] != b'wad\x00') and (hdr[4:8] != b'pc\x00\x00')): 
        print('this is not a .wad file')
        return
    
    f.read(24)
    
    n_chunks = unpack("I", f.read(4))[0]
    print(f"n_chunks = {n_chunks}")
    
    # make map 
    chunk_info = []
    f.read(8 + 8 * n_chunks) # skip strange table
    for i in range(n_chunks):
        f.read(4) # align factor no need
        chunk_size = unpack("I", f.read(4))[0]
        offset_in_file = unpack("I", f.read(4))[0]
        unk = unpack("I", f.read(4))[0]
        if unk != 0:
            print("unk != 0", offset_in_file, i)
        chunk_info.append({"offset": offset_in_file, "size": chunk_size})
    
    
    
    for chnk in chunk_info:
        f.seek(chnk["offset"], 0)
        signature = f.read(4)
        f.seek(-4, 1)
        size_of_data = chnk["size"]
        print(signature, size_of_data, hex(chnk["offset"]), chnk_name(f.tell(), signature))
        dump_chunk(f, size_of_data, chnk_name(f.tell(), signature))
    
    f.close()

if __name__ == "__main__":
   main(sys.argv[1:])