// PHYS file format
// PHYS file is a collison geometry.
// 

// loader func analyzis
// HfWorld.cpp 
// 4b read version check // (2.9f)
// sub_447B40 (part of HfStaticTree.cpp)
//    4b n_leafs  // 27
//    of n_leafs => alocating buffer array with 0x28 size entires
//    4b n_faces // 292
//    4b n_leafs_with_val // 01
//    if n_leafs_with_val => aloc amunt2 * 21
//    while n_leafs_with_val 
//       4b value_v 9
//       4b amount_v (aloc amt3 * 4) // 1
//    4b n_vertex_buffers // 1
//    while n_vertex_buffers 
//       4b amount_vertexes // 240
//       4b uiVertexSize (must be 12) // 12
//       Nb read amount_vertexes * uiVertexSize
//    
//       sub_447840
//    24b bounding_box // { 3329.0, 5920.0, 1301.5 } { -780.0, -6135.0, -2617.2}
//    4b val // -1
//    1b flg // 0
//    if (flg) {
//        4b n_leaf_faces (9b array) // 2
//	      while n_leaf_faces 
//			2b // 04 02
//		    6b // EC00, ED00, EE00
//		    1b // 00
//     }
//	  4b ulChildrenCount  // 2
//	  while ulChildrenCount
//       reqursive calls to sub_447840
//	
//	4b amt (22b array) // 0	  
//	while amt {
//      4b 
//      4b amt2 (39b array)
//      while amt2 {
//         12b
//         12b
//         12b // vectors
//         
//      }	
//	}
//		  
//		  
//		  
//		  
//		  
//		  
//		  
//		  

typedef UINT unsigned int;

struct Mesh_point {
	WORD unk;
	WORD indexes[3];
	BYTE unk_b;
};

struct Vector3f {
	float x;
	float y;
	float z;
};

struct BoundVolume {
	Vector3f max_;
	Vector3f min_;
};

struct Leaf_t {
	BoundVolume bv;
	UINT index_prob; // eq -1 most of the time
	bool bContainIndexes;
	// if contain then 
	UINT amount_points;
	Mesh_point pts[amount_points];
	// if not then nothing...
	UINT ulChildrenCount; // amount of child leafs...
	
};

struct unk_struct_1 {
	UINT value_v;
	UINT amount_v_leafs;
};

struct static_tree_header {
	UINT n_leafs;
	UINT n_total_faces;
	UINT amount_value_assigned_to_leaf;
	unk_struct_1 unk_struct_1_array[amount_unk1];
	UINT amount_vertex_buffers;
};

struct vertex_buffer_header {
	UINT amount_vertexes_in_buffer;
	UINT uiVertexSize;
};

struct vertex_buffer {
	vertex_buffer_header hdr;
	vertex vertex_array[N];
};

struct header {
	unsigned int unk; // 1
	float version; // version must be 2.9f
};