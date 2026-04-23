""" This script convert Dragon Riders Chronicles of Pern .3D file to GLTF file
    that can be read by Blender. .3D files including level and meshes.

    You can do what ever you want with this code, 
    modify and publish it everywhere you want.

    Author: Trololp
"""


import struct
from struct import unpack
import sys
import json
import math
import array
import numpy as np

# This script atleast can export Branth dragon mesh with skinning information.
# some models are not exported with skin information, like Tunnel snake or D'kor.
# This script is not resolve materials true name, and materials is broken.

# put all vertex from .3D to file [done]
# put all indexes from .3D to file [done]
# make bufferviews [done]
# make accessors [done]
# parse materials [done]
# fill materials into gltf [done]
# parse nodes (skeleton) [done]
# fill nodes in GLTF [done]
# skinning information parse [done]
# make basic GLTF file [done]

# fix bones rotated 90 degrees ...


g_3d_model_name = "blank"
g_bin_file_size = 0
g_vertex_count = 0
g_have_skin_information = False
g_inv_matrices_loc = 0
g_inv_matrices_count = 0

g_bone_node_ids = []

NODE_TYPE_MESH = 1
NODE_TYPE_SKIN = 6

def make_blank_gltf_json():
    json_d = {}
    json_d["asset"] = {"generator":"BRUH", "version":"2.0"}
    json_d["scene"] = 0
    json_d["scenes"] = []
    json_d["nodes"] = []
    # miss materials for now
    json_d["meshes"] = []
    json_d["accessors"] = []
    json_d["images"] = []
    json_d["textures"] = []
    json_d["bufferViews"] = []
    json_d["buffers"] = []
    json_d["materials"] = []
    return json_d

# from euclid to quaternion
def to_quat(vec):
    """
    Convert Euler angles (in radians) to quaternion (x, y, z, w)
    Rotation order: XYZ
    """
    x = vec[0]
    y = vec[1]
    z = vec[2]
    cx = math.cos(x / 2)
    sx = math.sin(x / 2)
    cy = math.cos(y / 2)
    sy = math.sin(y / 2)
    cz = math.cos(z / 2)
    sz = math.sin(z / 2)

    qw = cx * cy * cz + sx * sy * sz
    qx = sx * cy * cz - cx * sy * sz
    qy = cx * sy * cz + sx * cy * sz
    qz = cx * cy * sz - sx * sy * cz

    return [qx, qy, qz, qw]



def make_gltf_json(json_d, meshes, nodes, materials):
    global g_have_skin_information
    global g_inv_matrices_loc
    global g_inv_matrices_count
    global g_bone_node_ids
    json_d["buffers"] = [{"byteLength": g_bin_file_size, "uri": g_3d_model_name + ".bin"}]

    i = 0

    if g_have_skin_information:
        json_d["bufferViews"].append({"buffer":0, "byteLength": g_inv_matrices_count * 64, \
        "byteOffset": g_inv_matrices_loc, "byteStride": 64, "target":34962})
        i += 1

    for m in meshes:
        # vertex buffer

        json_d["bufferViews"].append({"buffer":0, "byteLength": m["vertex_buffer_size"], \
            "byteOffset": m["vertex_buffer_pos"], "byteStride": 64 if g_have_skin_information else 44, "target":34962})

        m["vtx_buffer_id"] = i
        i += 1


        # index buffers in primitives
        for p in m["primitives"]:
            json_d["bufferViews"].append({"buffer":0, "byteLength": p["index_buffer_size"], \
            "byteOffset": p["index_buffer_pos"], "target":34962})
            p["idx_buffer_id"] = i
            i += 1

    acc_id = 0

    if g_have_skin_information:
        json_d["accessors"].append({"bufferView": 0, "componentType":5126, \
            "count": g_inv_matrices_count, "byteOffset": 0, "type":"MAT4"})
        acc_id += 1

    for m in meshes:

        vtx_count = m["vertex_buffer_size"] // (64 if g_have_skin_information else 44)
        vtx_buff = m["vtx_buffer_id"]
        # vertices pos
        json_d["accessors"].append({"bufferView": vtx_buff, "componentType":5126, \
            "count": vtx_count, "byteOffset": 0, \
            "max": [1.0, 1.0, 1.0], "min": [-1.0, -1.0, -1.0], "type":"VEC3"})

        # normals
        json_d["accessors"].append({"bufferView": vtx_buff, "componentType":5126, \
            "count":vtx_count, "byteOffset": 12, \
            "max": [1.0, 1.0, 1.0], "min": [-1.0, -1.0, -1.0], "type":"VEC3"})

        # colors
        json_d["accessors"].append({"bufferView": vtx_buff, "componentType":5121, \
            "count":vtx_count, "byteOffset": 24, \
            "max": [1.0, 1.0, 1.0, 1.0], "min": [0.0, 0.0, 0.0, 0.0], "type":"VEC4"})

        # uv1
        json_d["accessors"].append({"bufferView": vtx_buff, "componentType":5126, \
            "count":vtx_count, "byteOffset": 28, \
            "max": [1.0, 1.0], "min": [0.0, 0.0], "type":"VEC2"})

        # uv2
        json_d["accessors"].append({"bufferView": vtx_buff, "componentType":5126, \
            "count":vtx_count, "byteOffset": 36, \
            "max": [1.0, 1.0], "min": [0.0, 0.0], "type":"VEC2"})

        if g_have_skin_information:
            # joints
            json_d["accessors"].append({"bufferView": vtx_buff, "componentType":5121, \
            "count":vtx_count, "byteOffset": 44, \
            "max": [255, 255, 255, 255], "min": [0, 0, 0, 0], "type":"VEC4"})
            # weights
            json_d["accessors"].append({"bufferView": vtx_buff, "componentType":5126, \
            "count":vtx_count, "byteOffset": 48, \
            "max": [1.0, 1.0, 1.0, 1.0], "min": [0.0, 0.0, 0.0, 0.0], "type":"VEC4"})

        m["acc_id"] = acc_id
        acc_id += 7 if g_have_skin_information else 5

        for p in m["primitives"]:
            acc_t = {"bufferView": p["idx_buffer_id"], "componentType": 5123, \
                "count": p["lod_idx_count"], "type":"SCALAR"}
            acc_id += 1
            json_d["accessors"].append(acc_t)

    view_n = 0
    for m in meshes:
        vn = m["acc_id"]
        view_n = (vn + 7) if g_have_skin_information else (vn + 5)
        primitives = []
        for k in range(len(m["primitives"])):
            if g_have_skin_information:
                prim_t = {"attributes" : {"POSITION":vn, "NORMAL": vn + 1, "COLOR_0": vn + 2, "TEXCOORD_0": vn + 3, "TEXCOORD_1": vn + 4, \
                    "JOINTS_0": vn + 5, "WEIGHTS_0": vn + 6},
                  "indices": view_n,
                  "material": m["primitives"][k]["mat_id"],
                  "mode" : 4} # 4
            else:
                prim_t = {
                  "attributes" : {"POSITION":vn, "NORMAL": vn + 1, "COLOR_0": vn + 2, "TEXCOORD_0": vn + 3, "TEXCOORD_1": vn + 4 },
                  "indices": view_n,
                  "material": m["primitives"][k]["mat_id"],
                  "mode" : 4} # 4
            view_n += 1
            primitives.append(prim_t)

        mesh_t = {"name": m["name"],
                  "primitives": primitives }
        json_d["meshes"].append(mesh_t)


    for n in nodes:
        node_t = {"name" : n["name"], "children": n["children"], \
            "translation": n["pos"], "scale": n["scale"], "rotation": n["rot"]}
        if n["type"] == NODE_TYPE_MESH: # contain mesh data
            #print(n["name"])
            if g_have_skin_information:
                node_t = {"name" : n["name"], "mesh": n["data_id"], "skin": 0, \
            "translation": n["pos"], "scale": n["scale"], "rotation":n["rot"]}
            else:
                node_t = {"name" : n["name"], "mesh": n["data_id"], \
                "translation": n["pos"], "scale": n["scale"], "rotation": n["rot"]}

        json_d["nodes"].append(node_t)

    joints = []

    for b in g_bone_node_ids:
        joints.append(b)

    skel_id = 0
    for i, n in enumerate(nodes):
        if n["type"] == NODE_TYPE_SKIN:
            skel_id = i
            break

    if g_have_skin_information:
        json_d["skins"] = []
        json_d["skins"].append({"inverseBindMatrices": 0, \
            "joints": joints, "skeleton": skel_id})

    json_d["scenes"] = [{"name":"Scene",
	                    "nodes":[len(nodes) - 1]}]
    json_d["scene"] = 0

    i = 0

    if len(materials) > 0:
        for m in materials:
            json_d["images"].append({"uri": f"{m["dmap"].upper()}.DDS"})
            json_d["textures"].append({"source": i})
            mat_t =         {
                "doubleSided":False,
                "name":m["name"],
                "pbrMetallicRoughness":{
                    #"baseColorFactor": m["diff_color"],
                    "baseColorTexture":{
                        "index": i
                    },
                    "metallicFactor":0.0,
                    "roughnessFactor":0.5
                }
            }
            json_d["materials"].append(mat_t)
            i += 1
    else:
        mat_t =  {
                "doubleSided":False,
                "name": "default",
                "pbrMetallicRoughness":{
                    "baseColorFactor": [1.0, 1.0, 1.0, 1.0],
                    "metallicFactor":0.0,
                    "roughnessFactor":0.5
                }
        }
        json_d["materials"].append(mat_t)

    return

printable = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!"#$%&\'()*+,-./:;<=>?@[\\]^_`{|}~ '

def u32(f):
    return unpack("I", f.read(4))[0]

def f32(f):
    return unpack("f", f.read(4))[0]

def vec4f(f):
    return list(unpack("ffff", f.read(16)))

def vec3f(f):
    return list(unpack("fff", f.read(12)))

def print_vec3f(vec):
    return "%.03f %.03f %.03f" % (vec[0], vec[1], vec[2])

def print_vec4f(vec):
    return "%.03f %.03f %.03f %.03f" % (vec[0], vec[1], vec[2], vec[3])

def validate_string(tha_string):
    if len(tha_string) == 0:
        return " "

    if all(c in set(printable) for c in tha_string):
        return tha_string
    else:
        return "invalid_string"

def parse_container(f, content_size_limit):

    posible_tag = f.read(4)
    is_shit = False
    try:
        tag = posible_tag.decode("ASCII")
    except UnicodeDecodeError:
        is_shit = True

    if is_shit or validate_string(tag) == "invalid_string":
        print("something went wrong! tag is not ASCII (at 0x%x)\n" % f.tell(), posible_tag)
        sys.exit("error")

    content_size = u32(f)

    if content_size > content_size_limit:
        print("something went wrong! content size > limit\n")
        sys.exit("error")

    content_p = f.tell()
    f.seek(content_size, 1)

    return {"tag": tag, "size": content_size, "content_p": content_p}

# assuming it is called for MATL, NODE, MESH, _3D_, or SKIN container
def get_containers(f, limit_size):
    parsed_len = 0
    containers = []
    while parsed_len < limit_size:
        containers.append(parse_container(f, limit_size - parsed_len))
        parsed_len += containers[-1]["size"] + 8
    return containers


def find_containers_with_tag(containers, tag):
    return [c for c in containers if c["tag"] == tag]

def get_file_info(f):
    version = u32(f)
    if version != 3:
        print("INFO version is not 3")
        sys.exit(0)

    material_count = u32(f)
    node_count = u32(f)
    mesh_count = u32(f)
    lights_count = u32(f)
    f.seek(8, 1)
    occlusions_count = u32(f)
    skinning_count = u32(f)
    f.seek(24, 1)
    return {"mat_count": material_count, "node_count": node_count, "mesh_count": mesh_count, \
        "lights_count": lights_count, "skin_count": skinning_count}

def get_mesh_primitive(f):
    material_id = u32(f)
    vertex_count = u32(f)
    vertex_ids = f.tell(); # store pointer to vertex_ids
    f.seek(vertex_count * 2, 1)

    # this is lod information?
    # lower dist the better is mesh. right?
    lowest_lod_dist = 9999.0
    lowest_lod_indexes = 0
    lowest_lod_index_count = 0
    lod_cnt = u32(f)
    for i in range(lod_cnt):
        lod_dist = f32(f)
        f.seek(4, 1)
        index_count = u32(f)
        indexes = f.tell()
        if(lod_dist < lowest_lod_dist):
            lowest_lod_dist = lod_dist
            lowest_lod_indexes = indexes
            lowest_lod_index_count = index_count
        f.seek(2*index_count, 1)

    return {"mat_id": material_id, "vtx_count": vertex_count, "vtx_id_buff": vertex_ids, \
            "lod_indexes": lowest_lod_indexes, "lod_idx_count": lowest_lod_index_count, \
            "index_buffer_pos": 0, "index_buffer_size": 0}


def get_name(f, size):
    prob_name = f.read(size).rstrip(b'\x00')
    name = ""
    try:
        name = prob_name.decode("ASCII")
    except UnicodeDecodeError:
        print("something went wrong! name is not ASCII (at 0x%x)\n" % f.tell(), prob_name)
        sys.exit("error")
    return name.split(":")[0] # strange names, example: branth_2:Branth_mesh

def get_meshes(f, mesh_conatiners):
    meshes = []
    for mesh_c in mesh_conatiners:
        f.seek(mesh_c["content_p"], 0)
        c = get_containers(f, mesh_c["size"])

        name_str = find_containers_with_tag(c, "NAME")[0]
        f.seek(name_str["content_p"], 0)
        name = get_name(f, name_str["size"])

        vrts_c = find_containers_with_tag(c, "VRTS")
        if len(vrts_c) == 0:
            print("no vertices mesh found!", name)
            continue

        vertex_buffer = find_containers_with_tag(c, "VRTS")[0]

        normals_buffer = find_containers_with_tag(c, "NRMS")[0]
        UVs_buffer = find_containers_with_tag(c, "UVS ")[0]
        COLS_buffer = find_containers_with_tag(c, "COLS")[0]
        PNTS_buffer = find_containers_with_tag(c, "PNTS")[0]

        # parse g3dm structure to get primitives
        g3dm_address = find_containers_with_tag(c, "G3DM")[0]["content_p"]
        f.seek(g3dm_address, 0)
        primitive_count = u32(f)
        primitives = []
        for i in range(primitive_count):
            primitives.append(get_mesh_primitive(f))

        meshes.append({"VRTS" : vertex_buffer, "NRMS": normals_buffer, "UVS": UVs_buffer, \
            "COLS": COLS_buffer, "points": PNTS_buffer, "primitives": primitives, \
            "name": name, "vertex_buffer_pos": 0, "vertex_buffer_size" : 0, "vtx_count": PNTS_buffer["size"] // 20})
    return meshes

def create_data_file(f, meshes, vertex_joints, vertex_weights, inv_matrixes):
    global g_bin_file_size
    global g_have_skin_information
    global g_inv_matrices_loc
    f2 = open(g_3d_model_name + ".bin", "wb")

    for m in meshes:

        f.seek(m["points"]["content_p"], 0)
        points_buff = f.read(m["points"]["size"])

        f.seek(m["VRTS"]["content_p"], 0)
        vertices_buff = f.read(m["VRTS"]["size"])
        f.seek(m["NRMS"]["content_p"], 0)
        normals_buff = f.read(m["NRMS"]["size"])
        f.seek(m["COLS"]["content_p"], 0)
        colors_buff = f.read(m["COLS"]["size"])
        f.seek(m["UVS"]["content_p"], 0)
        uvs_buff = f.read(m["UVS"]["size"])

        points_buffer = bytearray(m["points"]["size"] // 20 * (12+12+4+8+8))


        offset = 0

        chunk_size = 64 if g_have_skin_information else 44
        for v, n, c, u1, u2 in struct.iter_unpack("<5I", points_buff):

            if g_have_skin_information:
                chunk = ( vertices_buff[v*12:v*12+12] +
                normals_buff[n*12:n*12+12] +
                colors_buff[c*4:c*4+4] +
                uvs_buff[u1*8:u1*8+8] +
                uvs_buff[u2*8:u2*8+8] +
                struct.pack("4B", 0, 0, 0, 0) +
                struct.pack("4f", 1.0, 0.0, 0.0, 0.0))
            else:
                chunk = (
                vertices_buff[v*12:v*12+12] +
                normals_buff[n*12:n*12+12] +
                colors_buff[c*4:c*4+4] +
                uvs_buff[u1*8:u1*8+8] +
                uvs_buff[u2*8:u2*8+8])


            points_buffer[offset:offset + chunk_size] = chunk
            offset += chunk_size

        if g_have_skin_information:
            point_number = 0
            for p in m["primitives"]: 
                f.seek(p["vtx_id_buff"], 0)
                vtx_ids = array.array("H")
                vtx_ids.frombytes(f.read(p["vtx_count"] * 2))
                
                for v in vtx_ids:
                    #insane shit
                    offset = v * 64
                    points_buffer[offset + 44:offset + 48] = struct.pack("4B", *vertex_joints[point_number])
                    points_buffer[offset + 48:offset + 64] = struct.pack("4f", *vertex_weights[point_number])
                    point_number += 1
                
        

        m["vertex_buffer_pos"] = f2.seek(0, 1)
        m["vertex_buffer_size"] = len(points_buffer)
        f2.write(points_buffer)

        for p in m["primitives"]:
            f.seek(p["vtx_id_buff"], 0)
            vtx_ids = array.array("H")
            vtx_ids.frombytes(f.read(p["vtx_count"] * 2))

            f.seek(p["lod_indexes"], 0)
            lod_idx = array.array("H")
            lod_idx.frombytes(f.read(p["lod_idx_count"] * 2))

            lod_idx_buff = array.array("H", [0] * p["lod_idx_count"])

            for i in range(p["lod_idx_count"]):
                lod_idx_buff[i] = vtx_ids[lod_idx[i]]


            p["index_buffer_pos"] = f2.seek(0, 1)
            p["index_buffer_size"] = p["lod_idx_count"] * 2 
            f2.write(lod_idx_buff.tobytes()) 

    g_inv_matrices_loc = f2.tell()

    if g_have_skin_information:
        for m in inv_matrixes:
            m2 = m.astype(np.float32)
            m_col_major = m2.T
            f2.write(m_col_major.tobytes())

    g_bin_file_size = f2.tell()
    f2.close()
    return

def read_vector_3f(f):
    x = f32(f)
    y = f32(f)
    z = f32(f)
    return [x, y, z]

def read_vector_4f(f):
    x = f32(f)
    y = f32(f)
    z = f32(f)
    w = f32(f)
    return [x, y, z, w]

def create_nodes_array_from_file(f, node_containers):
    # read nodes build an array
    nodes = []

    for node_c in node_containers:
        f.seek(node_c["content_p"], 0)
        c = get_containers(f, node_c["size"])
        name_str = find_containers_with_tag(c, "NAME")[0]
        f.seek(name_str["content_p"], 0)
        name = get_name(f, name_str["size"])

        f.seek(find_containers_with_tag(c, "PIDX")[0]["content_p"], 0)
        parent = u32(f)

        f.seek(find_containers_with_tag(c, "DIDX")[0]["content_p"], 0)
        DIDX = u32(f)

        f.seek(find_containers_with_tag(c, "TYPE")[0]["content_p"], 0) # 1 mesh, 2 camera, 3 light, 5 occlusion, 6 skin,
        node_type = u32(f)

        f.seek(find_containers_with_tag(c, "POS ")[0]["content_p"], 0)
        position = read_vector_3f(f)

        f.seek(find_containers_with_tag(c, "ROT ")[0]["content_p"], 0)
        rotation = to_quat(read_vector_3f(f)) # convert to quaternion

        f.seek(find_containers_with_tag(c, "SCAL")[0]["content_p"], 0)
        scale = read_vector_3f(f)
        #if node_type == 6:
        #    print("name :", name, "data_id: ", DIDX)
        nodes.append({"name": name, "parent": parent, "type": node_type, \
            "pos": position, "rot": rotation, "scale": scale, "data_id": DIDX, \
            "local_matrix": None, "global_matrix": None, "inverse_bind_matrix": None})

    return nodes

def build_tree(nodes):

    for node in nodes:
        node['children'] = []

    # Build parent -> children relationships
    for i, node in enumerate(nodes):
        parent_index = node['parent']

        if parent_index == 0xFFFFFFFF:
            continue  # root node, no parent

        # Attach this node to its parent
        nodes[parent_index]['children'].append(i)

    return nodes

def translation_matrix(t):
    m = np.eye(4)
    m[:3, 3] = t
    m[3, 3] = 1.0
    return m

def scale_matrix(s):
    m = np.eye(4)
    m[0, 0] = s[0]
    m[1, 1] = s[1]
    m[2, 2] = s[2]
    return m


def quaternion_to_matrix(q):
    #x, y, z, w = q
    x = q[0]
    y = q[1]
    z = q[2]
    w = q[3]
    m = np.eye(4)

    m[0, 0] = 1 - 2*y*y - 2*z*z
    m[0, 1] = 2*x*y - 2*z*w
    m[0, 2] = 2*x*z + 2*y*w

    m[1, 0] = 2*x*y + 2*z*w
    m[1, 1] = 1 - 2*x*x - 2*z*z
    m[1, 2] = 2*y*z - 2*x*w

    m[2, 0] = 2*x*z - 2*y*w
    m[2, 1] = 2*y*z + 2*x*w
    m[2, 2] = 1 - 2*x*x - 2*y*y

    return m

def trs_matrix(position, rotation, scale):
    T = translation_matrix(position)
    R = quaternion_to_matrix(rotation) 
    S = scale_matrix(scale)

    return T @ R @ S

def compute_local_matrices(nodes, node):
    nodes[node]["local_matrix"] = trs_matrix(nodes[node]["pos"], nodes[node]["rot"], nodes[node]["scale"])
    for child in nodes[node]["children"]:
        compute_local_matrices(nodes, child)

def compute_global_matrices(nodes, node, parent_global=np.eye(4)):
    nodes[node]["global_matrix"] = parent_global @ nodes[node]["local_matrix"]
    for child in nodes[node]["children"]:
        compute_global_matrices(nodes, child, nodes[node]["global_matrix"])

def compute_inverse_bind_matrices(nodes, node):
    nodes[node]["inverse_bind_matrix"] = np.linalg.inv(nodes[node]["global_matrix"])
    for child in nodes[node]["children"]:
        compute_inverse_bind_matrices(nodes, child)

def compute_skinning_matrices(nodes, root_node):
    compute_local_matrices(nodes, root_node)
    compute_global_matrices(nodes, root_node)
    compute_inverse_bind_matrices(nodes, root_node)

def get_materials(f, materials_containers):

    materials = []
    i = 0
    for m in materials_containers:
        f.seek(m["content_p"], 0)
        c = get_containers(f, m["size"])

        name_str = find_containers_with_tag(c, "NAME")[0]
        f.seek(name_str["content_p"], 0)
        name = get_name(f, name_str["size"])

        diffuse_map_c = find_containers_with_tag(c, "DMAP")
        dmap_str = "none"
        if len(diffuse_map_c) == 1:
            dmap_str = diffuse_map_c[0] # also exists AMAP, RMAP, SMAP, EMAP?, IMAP?
        f.seek(name_str["content_p"], 0)
        diffuse_map = get_name(f, name_str["size"])

        f.seek(find_containers_with_tag(c, "FLAG")[0]["content_p"], 0)
        flags = u32(f)

        f.seek(find_containers_with_tag(c, "TRAN")[0]["content_p"], 0) # translucency
        tran = f32(f)

        f.seek(find_containers_with_tag(c, "REFL")[0]["content_p"], 0)
        reflectance = f32(f)

        f.seek(find_containers_with_tag(c, "ILUM")[0]["content_p"], 0)
        illuminance = f32(f)

        f.seek(find_containers_with_tag(c, "POWR")[0]["content_p"], 0)
        power = f32(f)

        f.seek(find_containers_with_tag(c, "DIFF")[0]["content_p"], 0)
        diffuse_color = read_vector_4f(f)

        f.seek(find_containers_with_tag(c, "AMBT")[0]["content_p"], 0)
        ambient_color = read_vector_4f(f)

        f.seek(find_containers_with_tag(c, "SPEC")[0]["content_p"], 0)
        specular_color = read_vector_4f(f)

        mat_t = {"name": name, "flags": flags, "tran": tran, "refl": reflectance, \
            "ilum": illuminance, "pow": power, "specular_color": specular_color, \
            "diff_color": diffuse_color, "dmap": diffuse_map, "ambient_color": ambient_color}

        #print("material %d :" % i, mat_t)
        i += 1

        materials.append(mat_t)

    return materials


def find_bone_nodes(nodes):
    real_bones_ids = [i for i, n in enumerate(nodes) if n["type"] == NODE_TYPE_SKIN]

    r, j = real_bones_ids[0], real_bones_ids[-1]
    if r == j:
        return ([r], [r])

    bone_set = set(real_bones_ids)
    no_parent = 0xFFFFFFFF

    for idx in real_bones_ids:
        p = nodes[idx]["parent"]
        # Climb the tree up to the root or until we hit no parent
        while p != no_parent and p != r:
            bone_set.add(p)
            p = nodes[p]["parent"]


    bone_ids = [i for i in range(r, j + 1) if i in bone_set]
    
    return (bone_ids, real_bones_ids)



def get_inv_matrices(nodes, bone_ids):
    matrices = []
    for b in bone_ids:
        if nodes[b]["inverse_bind_matrix"] is None:
            print("inverse bind matrix is None for bone", nodes[b]["name"], "parent is", nodes[nodes[b]["parent"]]["name"])
        matrices.append(nodes[b]["inverse_bind_matrix"])
    
    return matrices


def get_skinning(f, bones_cnt, vertex_count, g3ds_c, data_id_to_bone_id, primitives):

    skin_info = [None] * bones_cnt
    a = 0xFFFFFFFF
    g3ds_counter = 0
    p_len = len(primitives)
    primitve_vtx_start_from = [0] * p_len
    vtx_cntr = 0
    for i, p in enumerate(primitives):
        primitve_vtx_start_from[i] = vtx_cntr
        vtx_cntr += p["vtx_count"]

    for g3ds in g3ds_c:
        f.seek(g3ds["content_p"] + 4, 0)
        primitive_id = u32(f)
        weights = [None] * vertex_count
        while primitive_id != a:
            sz_array_1 = u32(f)
            while(sz_array_1 != a):
                delta = u32(f)
                vtx_id = primitve_vtx_start_from[p_len - 1 - primitive_id] + delta # WHY ARE THEY INVERTED!? no idea
                
                while delta != a:
                    weight = f32(f)
                    vec = vec3f(f)
                    try:
                        weights[vtx_id] = weight
                    except IndexError:
                        print("Skinning info error, vertex index is out of range")
                        print("vertex_count, vtx_id, g3ds_counter", vertex_count, vtx_id, g3ds_counter)
                        print("f pos ", hex(f.tell()))
                        sys.exit(0)

                    delta = u32(f)
                    vtx_id += delta
                sz_array_2 = u32(f)
                i2 = u32(f)
                while i2 != a:
                    vec = vec3f(f) # inverse bind pose for bone
                    i2 = u32(f)
                break
            primitive_id = u32(f)
        
        skin_info[data_id_to_bone_id[g3ds_counter]] = weights
        g3ds_counter += 1

    return skin_info


def convert_weight_info(bone_weights, vtx_count, data_id_to_bone_id):

    vertex_joints = [None] * vtx_count
    vertex_weights = [None] * vtx_count

    for b in data_id_to_bone_id:
        for v in range(vtx_count):
            if bone_weights[b][v] == None:
                    continue
            if vertex_joints[v] is None:
                vertex_joints[v] = [b]
            elif len(vertex_joints[v]) < 4:
                vertex_joints[v].append(b)

            if vertex_weights[v] is None:
                vertex_weights[v] = [bone_weights[b][v]]
            elif len(vertex_weights[v]) < 4:
                vertex_weights[v].append(bone_weights[b][v])

    for i in range(vtx_count):
        if vertex_joints[i] is None:
            vertex_joints[i] = [0]
        if vertex_weights[i] is None:
            vertex_weights[i] = [0.0, 0.0, 0.0, 0.0]
        

    for j in vertex_joints:
        if len(j) < 4:
            j.extend([0] * (4 - len(j)))

    for w in vertex_weights:
        if len(w) < 4:
            w.extend([0.0] * (4 - len(w)))

    return (vertex_joints, vertex_weights)

def get_data_id_for_skinning(nodes, real_bone_ids):
    data_ids = []
    for id in real_bone_ids:
        data_ids.append(nodes[id]["data_id"])

    return data_ids

def get_data_id_to_bone_id(nodes, bone_nodes_id, real_bone_ids):
    indices = []
    data_id_to_bone_id = [0] * len(real_bone_ids)
    for id in real_bone_ids:
        try:
            data_id_to_bone_id[nodes[id]["data_id"]] = id
        except IndexError:
            print("id, data_id", id, nodes[id]["data_id"])
            sys.exit(0)
    

    for val in data_id_to_bone_id:
        indices.append(bone_nodes_id.index(val))
    return indices

def find_node_with_name(nodes, name):
    for i, n in enumerate(nodes):
        if n["name"] == name:
            return i

def main(argv):
    global g_3d_model_name
    global g_have_skin_information
    global g_inv_matrices_count
    global g_bone_node_ids

    file_name = argv[0]

    name_n_ext = file_name.split('.')
    if len(name_n_ext) == 2 and name_n_ext[1] == '3D':
        g_3d_model_name = name_n_ext[0]
    else:
        g_3d_model_name = file_name

    f = open(file_name, 'rb')

    f.seek(0, 0)

    if f.read(4) != b"_3D_":
        print("This is wrong file! magic value mismatch!")
        return

    sz = u32(f)
    f.seek(8, 0)
    file_containers = get_containers(f, sz)

    info_struct = find_containers_with_tag(file_containers, "INFO")

    if len(info_struct) == 0:
        print("can't find INFO structure")
        return

    f.seek(info_struct[0]["content_p"], 0)
    file_info = get_file_info(f)
    if file_info is None:
        return

    print(file_info)

    mesh_conatiners = find_containers_with_tag(file_containers, "MESH")

    meshes = []
    meshes = get_meshes(f, mesh_conatiners)

    materials_containers = find_containers_with_tag(file_containers, "MATL")

    materials = []
    materials = get_materials(f, materials_containers)

    node_containers = find_containers_with_tag(file_containers, "NODE")

    nodes = create_nodes_array_from_file(f, node_containers)

    if len(meshes) < 1:
        print("File contain no mesh information")
        print("Nothing to export")
        f.close()
        sys.exit(0)

    # quirk for tunnel snake mesh
    # Who ever want to fix this
    # the problem is that root node is not type SKIN, and bones are connected to it as if it was skeleton root.
    if (meshes[0]["name"] == "TSnake"):
        file_info["skin_count"] = 0


    build_tree(nodes)

    vertex_joints = None
    vertex_weights = None
    inv_matrixes = None

    if file_info["mesh_count"] > 1 and file_info["skin_count"] > 0:
            print("mesh count above 1 with file containing skin information !")
            print("Its not implemented, fall back to export without skin information")
            file_info["skin_count"] = 0

    if file_info["skin_count"] > 0:
        g_have_skin_information = True
        skin_containers = find_containers_with_tag(file_containers, "G3DS")

        bone_node_ids, real_bone_ids = find_bone_nodes(nodes)
        
        data_id_to_bone_id = get_data_id_to_bone_id(nodes, bone_node_ids, real_bone_ids)

        g_inv_matrices_count = len(bone_node_ids) 

        bone_weights = get_skinning(f, len(bone_node_ids), meshes[0]["vtx_count"], skin_containers, data_id_to_bone_id, meshes[0]["primitives"])

        g_bone_node_ids = bone_node_ids

        vertex_joints, vertex_weights = convert_weight_info(bone_weights, meshes[0]["vtx_count"], data_id_to_bone_id)

        for i, n in enumerate(nodes):
            if n["type"] == NODE_TYPE_SKIN: # skin first node is parent assumption
                compute_skinning_matrices(nodes, i)
                break

        inv_matrixes = get_inv_matrices(nodes, bone_node_ids)


    create_data_file(f, meshes, vertex_joints, vertex_weights, inv_matrixes)

    json_d = make_blank_gltf_json()
    make_gltf_json(json_d, meshes, nodes, materials)

    f2 = open(g_3d_model_name + ".gltf", "w")
    json.dump(json_d, f2)
    f2.close()

    f.close()

if __name__ == "__main__":
   main(sys.argv[1:])
