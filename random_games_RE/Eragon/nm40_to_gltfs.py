from struct import unpack
import sys
import json

# put all vertex from PHYS to file [done]
# put all indexes from PHYS structures to file [done]
# make bufferviews
# make accessors
# make ierahical node stucture inside GLTF
# make basic GLTF file [done]
# add materials?
# make a function that will return array of node objects from file [done]

g_VertexBufferLenght = 0
g_IndexBufferLenght = 0
g_min_bounds = []
g_max_bounds = []

g_model_info = []

def make_blank_gltf_json():
    json_d = {}
    json_d["asset"] = {"generator":"Bruh v0.1", "version":"2.0"}
    json_d["scene"] = 0 
    json_d["scenes"] = []
    json_d["nodes"] = []
    # miss materials for now
    json_d["meshes"] = []
    json_d["accessors"] = []
    json_d["bufferViews"] = []
    json_d["buffers"] = []
    return json_d

def create_PCRD_vertex_n_index_file(f, offset):
    global g_VertexBufferLenght
    global g_min_bounds
    global g_max_bounds
    global g_model_info
    
    f.seek(offset + 12,0)
    amt_idx = unpack("I", f.read(4))[0]
    amt_vtx = unpack("I", f.read(4))[0]
    offset_idx = unpack("I", f.read(4))[0]
    offset_vtx = unpack("I", f.read(4))[0]
    

    
    f.seek(offset_idx, 0)
    
    f2 = open(f"PCRD_indexes_{offset}.bin", "wb")
    f2.write(f.read(amt_idx*2))
    f2.close()
    
    f.seek(offset_vtx)
    f2 = open(f"PCRD_vtx_{offset}.bin", "wb")
    f2.write(f.read(amt_vtx*0x34))
    f2.close()
    
    f.seek(offset_vtx) 
    
    g_max_bounds = [-9999.0, -9999.0, -9999.0]
    g_min_bounds = [+9999.0, +9999.0, +9999.0]
    
    for i in range(amt_vtx):
        f.seek(offset_vtx + 0x34 * i, 0)
        f1 = unpack("f", f.read(4))[0]
        f2 = unpack("f", f.read(4))[0]
        f3 = unpack("f", f.read(4))[0]
        if g_max_bounds[0] < f1:
            g_max_bounds[0] = f1
        if g_max_bounds[1] < f2:
            g_max_bounds[1] = f2
        if g_max_bounds[2] < f3:
            g_max_bounds[2] = f3
        
        if g_min_bounds[0] > f1:
            g_min_bounds[0] = f1
        if g_min_bounds[1] > f2:
            g_min_bounds[1] = f2
        if g_min_bounds[2] > f3:
            g_min_bounds[2] = f3
        
        
    
    

    
    model_info_t = {"offset": offset, "idx_amt": amt_idx, "vtx_amt" : amt_vtx, "len_vtx" : amt_vtx*0x34, \
        "len_idx":  amt_idx*2, "max_bounds": g_max_bounds , "min_bounds":  g_min_bounds      }
    g_model_info.append(model_info_t)
    
    
    return

    
def make_gltf_json(json_d, num_model):
    global g_model_info
    global g_min_bounds
    global g_max_bounds
    
    model_info = g_model_info[num_model]
    
    json_d["buffers"] = [{"byteLength": model_info["len_vtx"], "uri": f"PCRD_vtx_{model_info['offset']}.bin"},
                         {"byteLength": model_info["len_idx"], "uri": f"PCRD_indexes_{model_info['offset']}.bin"}]
    
    
    json_d["bufferViews"].append({"buffer":0, "byteStride": 0x34, "byteLength": model_info["len_vtx"], "byteOffset": 0, "target":34962})
    json_d["bufferViews"].append({"buffer":1, "byteLength": model_info["len_idx"], "byteOffset": 0, "target":34963})
    
    
    json_d["nodes"].append({"mesh" : 0, "name" : "Model"})
    
    mesh_t = {"name": "model",
                      "primitives": [ {
                           "attributes" : {"POSITION":0, "NORMAL": 2, "TEXCOORD_0": 3},
                           "indices": 1,
                           "material": 0
                      } ]}
    json_d["meshes"].append(mesh_t)
    
    
    
    json_d["accessors"].append({"bufferView": 0, 
                                "componentType":5126,
                                "count": model_info["vtx_amt"],
                                "max": [model_info["max_bounds"][0], model_info["max_bounds"][1], model_info["max_bounds"][2]],
                                "min": [model_info["min_bounds"][0], model_info["min_bounds"][1], model_info["min_bounds"][2]],
                                "type":"VEC3"})
    
    json_d["accessors"].append({"bufferView": 1,
                     "componentType": 5123,
                     "count": model_info["idx_amt"],
                     "type":"SCALAR"})
    
    json_d["accessors"].append({"bufferView": 0, 
                                "componentType":5126,
                                "count": model_info["vtx_amt"],
                                "max": [1, 1, 1],
                                "min": [-1, -1, -1],
                                "type":"VEC3"})
    
    json_d["accessors"].append({"bufferView": 0, 
                                "componentType":5126,
                                "count": model_info["vtx_amt"],
                                "type":"VEC2"}) 
    
    json_d["scenes"] = [{"name":"Scene",
	                    "nodes":[0]}]
    json_d["scene"] = 0
    
    json_d["materials"] = [
        {
            "doubleSided":True,
            "name":"Material",
            "pbrMetallicRoughness":{
                "baseColorFactor":[
                    0.800000011920929,
                    0.800000011920929,
                    0.800000011920929,
                    1.0
                ],
                "metallicFactor":0.0,
                "roughnessFactor":0.5
            }
        }
    ]
    
    return
    
    
def main(argv):
    f = open(argv[0], 'rb')
    
    # check if this is NM40 file
    if f.read(4) != b'NM40':
        print("not a NM40 file")
        return 
    
    
    model_PCRD_offsets = []
    
    # map all PCRD headers
    f.seek(0x24, 0) # amt entry1
    amt_entry1 = unpack("H", f.read(2))[0]
    
    f.seek(0x34, 0) # offset to array entry1 
    offset_entry1 = unpack("I", f.read(4))[0]
    
    for i in range(amt_entry1):
        f.seek(i*8 + 2 + offset_entry1, 0)
        amt_entry2 = unpack("H", f.read(2))[0]
        offset_entrys2 = unpack("I", f.read(4))[0]
        for j in range(amt_entry2):
            f.seek(j*16 + 12 + offset_entrys2, 0)
            model_PCRD_offsets.append(unpack("I", f.read(4))[0])
    
    
    for n in range(len(model_PCRD_offsets)):
        create_PCRD_vertex_n_index_file(f, model_PCRD_offsets[n])
        json_d = make_blank_gltf_json()
        make_gltf_json(json_d, n)
    
        f2 = open(f"PCRD_f_{n}.gltf", "w")
        json.dump(json_d, f2)
        f2.close()

    f.close()

if __name__ == "__main__":
   main(sys.argv[1:])