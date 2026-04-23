#pragma author Trololp
#pragma description Dragon Riders Chronicles of Pern .3D file
#pragma magic [ 5F 33 44 5F ] @ 0x00

// Dragon Riders Chronicles of Pern - ImHex pattern for 3d models and maps.
// (.3D) files



#include <std/mem.pat>

import std.io;

struct vec4f {
 float x;
 float y;
 float z;
 float w;
 } [[format("vector4f_formater")]];

struct vec3f {
float x;
float y;
float z;
} [[format("vector3f_formater")]];

struct vec2f {
float x;
float y;
} [[format("vector2f_formater")]];

fn vector4f_formater(vec4f vec) {
    return std::format("{:.3f} {:.3f} {:.3f} {:.3f} ", vec.x, vec.y, vec.z, vec.w);
};

fn vector3f_formater(vec3f vec) {
    return std::format("{:.3f} {:.3f} {:.3f} ", vec.x, vec.y, vec.z);
};

fn vector2f_formater(vec2f vec) {
    return std::format("{:.3f} {:.3f} ", vec.x, vec.y);
};

struct PNTS_s {
  u32 vrtx_id;
  u32 norm_id;
  u32 cols_id;
  u32 uv1_id;
  u32 uv2_id;
};

struct info_s {
  u32 version; // should be 3
  u32 material_count;
  u32 node_count;
  u32 mesh_count;
  u32 lights_count;
  u32 unk3[2];
  u32 occlusions_count;
  u32 skinning_count;
  vec4f some_floats;
  u32 unk4;
  u8 unk5[4];
};

struct lod_prim {
    float lod_distance;
    u32 index_count_old;
    u32 count;
    u8 indexes[count*2];
};

struct mesh_primitive {
    u32 material_index;
    u32 vertex_count;
    u16 indexes[vertex_count];
    u32 cnt;
    lod_prim lods[cnt];
};

struct G3DM_s {
    u32 prim_count;
    mesh_primitive sm[prim_count];
    std::mem::AlignTo<4>;
};

struct vertex_influence {
    s32 delta; // is delta, vtx_pos = vtx_pos + delta
    if (delta == -1) break;
    if (delta >= 0) {
        float weight;
        vec3f bone_local_pos;
    } else {
        u32 unk;
    }
};

u32 G3DS_chunk_end = 0;

struct inv_position {
    s32 delta; // is delta, vtx_pos = vtx_pos + delta
    if(delta == -1) break;
    vec3f inv_bone_pos; // probably
};

struct bone_influnce {
    s32 size_of_array1;
    if (size_of_array1 == -1) break;
    vertex_influence v_influence[while($ < G3DS_chunk_end)]; // while(!std::mem::eof())
    s32 size_of_array2;
    inv_position inv_pos[while($ < G3DS_chunk_end)];
    break;
};

struct primitive_bone_influnce {
    s32 primitive_id;
    if (primitive_id == -1) break;
    bone_influnce b[while($ < G3DS_chunk_end)];
};



struct G3DS_s {
  padding[4];
  primitive_bone_influnce pr[while($ < G3DS_chunk_end)];
  padding[4];
};

struct container {
    char tag[4];
    u32 size;
    u32 end_p = $ + size;
    G3DS_chunk_end = end_p;
    match(tag) {
        ("INFO") : info_s data;
        ("MATL" | "NODE" | "MESH" | "_3D_" | "SKIN") : container children[while ($ < end_p)];
        ("NAME" | "DMAP") : char data[size];
        ("FLAG" | "DIDX" | "TYPE" | "PIDX") : u32 data;
        ("POS " | "ROT " | "SCAL") : vec3f vec;
        ("DIFF" | "AMBT" | "SPEC" | "ILUM") : vec4f data;
        ("POWR" | "TRAN" | "REFL") : float data;
        ("G3DM") : G3DM_s data;
        ("G3DS") : G3DS_s data;
        (_) : u8 data[size];
    }
} [[format("cont_formater")]];

fn cont_formater(container ct) {
    match (ct.tag) {
    ("FLAG" | "DIDX" | "TYPE" | "PIDX") : return std::format("{:4s} {:d} ", ct.tag, ct.data);
    ("NAME" | "DMAP") : return std::format("{:4s} {:s} ", ct.tag, ct.data);
     ("POS " | "ROT " | "SCAL") : return std::format("{:4s} {:s} ", ct.tag, vector3f_formater(ct.vec));
     (_) : return std::format("{:4s} ", ct.tag);
    }
};

// starts with _3D_
container file3d @ $;
