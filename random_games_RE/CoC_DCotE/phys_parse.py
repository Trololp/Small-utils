from struct import unpack
import sys

def print_leaf(f, current_leaf_n, current_req_lvl):
    
    print(f"Leaf {current_leaf_n}")
    
    bound_max_d = unpack("fff", f.read(12))
    bound_min_d = unpack("fff", f.read(12))
    
    _a = "\t" * current_req_lvl
    
    print(_a + "Boundvolume,", bound_max_d, bound_min_d)
    
    value_tf = unpack("i", f.read(4))[0]
    flag_v = unpack("?", f.read(1))[0]
    print(_a + f"value = {value_tf}")
    if flag_v == True:
        amount_faces = unpack("I", f.read(4))[0]
        print(_a + f"amount_faces = {amount_faces}")
        for i in range(amount_faces):
            unk_w = unpack("H", f.read(2))[0]
            index_0 = unpack("H", f.read(2))[0]
            index_1 = unpack("H", f.read(2))[0]
            index_2 = unpack("H", f.read(2))[0]
            unk_b = unpack("B", f.read(1))[0]
            print(_a, unk_w, index_0, index_1, index_2, unk_b)
    
    childs_cnt = unpack("I", f.read(4))[0]
    print(_a + f"childs_cnt = {childs_cnt}")
    
    for i in range(childs_cnt):
        current_leaf_n = print_leaf(f, current_leaf_n + 1, current_req_lvl + 1)
    
    return current_leaf_n;

def main(argv):
    f = open(argv[0], 'rb')
    idk = unpack("I", f.read(4))[0]
    version_f = unpack("f", f.read(4))[0]
    print(f"version_f = {version_f}")
    if version_f != 2.9000000953674316: 
        print('this is not PHYS v2.9')
        return
    
    n_leafs = unpack("I", f.read(4))[0]
    n_total_faces     = unpack("I", f.read(4))[0]
    amt2 =    unpack("I", f.read(4))[0]
    print(f"n_leafs = {n_leafs}\nn_total_faces = {n_total_faces}\namt2 = {amt2}")
    
    for i in range(amt2):
        unk1 = unpack("I", f.read(4))[0]
        unk2 = unpack("I", f.read(4))[0]
        print(f"\t[{unk1}] [{unk2}]")
    
    n_vertex_buffers = unpack("I", f.read(4))[0]
    print(f"n_vertex_buffers = {n_vertex_buffers}")
    
    for i in range(n_vertex_buffers):
        amt_vertex = unpack("I", f.read(4))[0]
        uiVertexSize = unpack("I", f.read(4))[0]
        print(f"amt_vertex = {amt_vertex}")
        f.seek(amt_vertex * uiVertexSize, 1)
    
    print("Leafs...\n\n")
    
    amt_leaf_total = print_leaf(f, 0, 1)
    
    print(f"amt_leaf_total = {amt_leaf_total}")
    
    n_unk =  unpack("I", f.read(4))[0]
    print(f"n_unk = {n_unk}")
    
    f.close()

if __name__ == "__main__":
   main(sys.argv[1:])