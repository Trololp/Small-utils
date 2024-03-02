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

def create_PHYS_vertex_file(f):
    global g_VertexBufferLenght
    global g_min_bounds
    global g_max_bounds
    f.seek(16,0)
    amt2 =    unpack("I", f.read(4))[0]
    for i in range(amt2):
        f.seek(8,1)
    
    n_vertex_buffers = unpack("I", f.read(4))[0]
    if n_vertex_buffers != 1:
        print("n_vertex_buffers != 1")
    
    amt_vertex = unpack("I", f.read(4))[0]
    uiVertexSize = unpack("I", f.read(4))[0]
    g_VertexBufferLenght = amt_vertex * uiVertexSize
    f_data = f.read(amt_vertex * uiVertexSize)
    
    g_max_bounds = unpack("fff", f.read(12))
    g_min_bounds = unpack("fff", f.read(12))
    
    f2 = open("PHYS_vtx.bin", "wb")
    f2.write(f_data)
    f2.close
    return



def reqursive_build_node_array(f, node_arr):
    f.seek(28, 1) # skip boundvolume n value
    flag_v = unpack("?", f.read(1))[0]
    indexes_fileoffset = f.tell() + 4
    n_faces = 0
    
    if flag_v == True:
        n_faces = unpack("I", f.read(4))[0]
        f.seek(9 * n_faces, 1)
    
    
    childs_cnt = unpack("I", f.read(4))[0]
    
    childs_n_arr = []
    
    for i in range(childs_cnt):
        childs_n_arr.append(reqursive_build_node_array(f, node_arr))
    
    node_t = {"number": len(node_arr), 
              "bHaveMesh": flag_v,  
              "Childs" : childs_n_arr,
              "n_faces" : n_faces,
              "mesh_offset": indexes_fileoffset,
              "in_bin_file_offset" : 0}
    
    node_arr.append(node_t)
    return node_t["number"]
    
    

def create_nodes_array_from_file(f):
    #will have mesh struct, offset to indexes, number, childs
    nodes = []
    f.seek(16,0)
    amt2 =    unpack("I", f.read(4))[0]
    for i in range(amt2):
        f.seek(8,1)
    
    n_vertex_buffers = unpack("I", f.read(4))[0]
    if n_vertex_buffers != 1:
        print("n_vertex_buffers != 1")
    
    amt_vertex = unpack("I", f.read(4))[0]
    uiVertexSize = unpack("I", f.read(4))[0]
    f.seek(amt_vertex * uiVertexSize, 1)
    reqursive_build_node_array(f, nodes)
    return nodes
    
def from_nodes_array_to_index_file(f, nodes):
    global g_IndexBufferLenght
    f.seek(0, 0)
    f2 = open("PHYS_indexes.bin", "wb")
    
    for node in nodes:
        if node["bHaveMesh"]:
            f.seek(node["mesh_offset"], 0)
            node["in_bin_file_offset"] = f2.tell()
            for i in range(node["n_faces"]):
                f.seek(2, 1)
                indexes_d = f.read(6)
                f.seek(1, 1)
                f2.write(indexes_d)
    
    g_IndexBufferLenght = f2.tell()
    
    f2.close()
    return
    
def make_gltf_json(json_d, nodes):
    global g_IndexBufferLenght
    global g_VertexBufferLenght
    global g_min_bounds
    global g_max_bounds
    json_d["buffers"] = [{"byteLength": g_VertexBufferLenght, "uri": "PHYS_vtx.bin"},
                         {"byteLength": g_IndexBufferLenght, "uri": "PHYS_indexes.bin"}]
    
    
    json_d["bufferViews"].append({"buffer":0, "byteLength": g_VertexBufferLenght, "byteOffset": 0, "target":34962})
    
    for node in nodes:
        if node["bHaveMesh"]:
            buf_view = {"buffer": 1, "byteLength": node["n_faces"] * 6, "byteOffset": node["in_bin_file_offset"], "target":34963}
            json_d["bufferViews"].append(buf_view)
    
    json_d["accessors"].append({"bufferView": 0, 
                                "componentType":5126,
                                "count":g_VertexBufferLenght // 12,
                                "max": [g_max_bounds[0], g_max_bounds[1], g_max_bounds[2]],
                                "min": [g_min_bounds[0], g_min_bounds[1], g_min_bounds[2]],
                                "type":"VEC3"})
    
    buf_view_n = 1
    for node in nodes:
        if node["bHaveMesh"]:
            acc_t = {"bufferView": buf_view_n,
                     "componentType": 5123,
                     "count": node["n_faces"] * 3,
                     "type":"SCALAR"}
            buf_view_n += 1
            json_d["accessors"].append(acc_t)
    
    buf_view_n = 1
    for node in nodes:
        if node["bHaveMesh"]:
            mesh_t = {"name": f"node_{node['number']}",
                      "primitives": [ {
                           "attributes" : {"POSITION":0},
                           "indices": buf_view_n,
                           "material": 0
                      } ]}
            buf_view_n += 1
            json_d["meshes"].append(mesh_t)
    mesh_n = 0
    
    for node in nodes:
        node_t = {"name": f"node_{node['number']}"}
        if node["bHaveMesh"]:
            node_t["mesh"] = mesh_n
            mesh_n += 1
        if len(node["Childs"]) > 0:
            node_t["children"] = node["Childs"]
        json_d["nodes"].append(node_t)
    
    json_d["scenes"] = [{"name":"Scene",
	                    "nodes":[len(nodes) - 1]}]
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
    
    create_PHYS_vertex_file(f)
    nodes = create_nodes_array_from_file(f)
    from_nodes_array_to_index_file(f, nodes)
    
    json_d = make_blank_gltf_json()
    make_gltf_json(json_d, nodes)
    
    f2 = open("PHYS_f.gltf", "w")
    json.dump(json_d, f2)
    f2.close()
    
    f.close()

if __name__ == "__main__":
   main(sys.argv[1:])