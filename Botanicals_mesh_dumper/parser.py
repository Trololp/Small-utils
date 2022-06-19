from struct import unpack
from struct import pack
from os import makedirs

def get_int(file):
	return unpack("<I", file.read(4))[0]

def get_float(file):
	return unpack("f", file.read(4))[0]

def get_byte(file):
	return unpack("B", file.read(1))[0]

def get_vec3f(file):
	return unpack("fff", file.read(12))

def get_vec2f(file):
	return unpack("ff", file.read(8))

def parse_boundvolume(file):
	return unpack("ffffff", file.read(24))

def report_position(file):
	pos = file.seek(0, 1)
	print ("pos: %08x" % pos)
	return



def dump_mesh(material_hash, name, f):
	amt_vertex = get_int(f)
	
	try:
		makedirs('mesh_obj\\')
	except FileExistsError:
		a = 2
	
	
	vertices_pos = []
	normals = []
	text_coords = []
	
	for i in range(amt_vertex):
		vertices_pos.append(get_vec3f(f))
		normals.append(get_vec3f(f))
		f.seek(4, 1)
		text_coords.append(get_vec2f(f))
		
		f.seek(36, 1) # skip other stuff
	
	index_amt = get_int(f)
	
	indexes = []
	for i in range(index_amt):
		indexes.append(get_int(f))
	
	#pos_str = "%08x_mesh.dmp" % file.seek(0, 1)
	
	f2 = open("mesh_obj\\" + name + '.obj', "w")
	
	f2.write("# botanicals mesh export\n\n")
	f2.write("mtllib Materials\\Materials.mtl\n")
	f2.write("# vertices \ng\n")
	
	for i in range(len(vertices_pos)):
		f2.write("v %f %f %f \n" % (vertices_pos[i][0], vertices_pos[i][1], vertices_pos[i][2]))
		f2.write("vt %f %f \n" % (text_coords[i][0], (1 - text_coords[i][1])))
		f2.write("vn %f %f %f \n" % (normals[i][0], normals[i][1], normals[i][2]))
	
	f2.write("#faces \ng %s\n" % name)
	f2.write("usemtl material_%08x \n" % material_hash)
	
	for i in range(len(indexes) // 3):
		f2.write("f %d/%d/%d %d/%d/%d %d/%d/%d\n" % (indexes[i*3 + 0] + 1, indexes[i*3 + 0] + 1, indexes[i*3 + 0] + 1, \
		indexes[i*3 + 2] + 1, indexes[i*3 + 2] + 1, indexes[i*3 + 2] + 1, \
		indexes[i*3 + 1] + 1, indexes[i*3 + 1] + 1, indexes[i*3 + 1] + 1))
	
	f2.close()

	

def parse_bot(f):
	Bot_hdr1 = get_int(f)
	Bot_hdr2 = get_int(f)
	
	if Bot_hdr1 != 0xF0000003 or Bot_hdr2 != 0x52800001:
		print("Botanicals magic values is not correct")
		return
	
	f.seek(4, 1) # 1
	table_size = get_int(f)
	f.seek(table_size * 8, 1) # some table idk
	
	Amount_1 = get_int(f) # amount meshes
	
	for idk in range(Amount_1):
		
		print("new chunks")
		report_position(f)
		
		mesh_id = get_int(f)
		
		print("mesh id: %d" % mesh_id)
		f.seek(28, 1)
		
		
		b1 = get_byte(f)
		if b1:
			f.seek(48, 1) # some struct
		
		f.seek(20, 1)
		
		#amt3 = get_int(f)
		#f.seek(get_int(f) * 8, 1) # hashes?
		if get_int(f) != 0:
			print('not null at amt3 hashes')
			report_position(f)
			return
	
		if get_int(f) != 0:
			print('not null at amt4')
			report_position(f)
			return
		
		f.seek(29, 1)
		
		f.seek(get_int(f) * 8, 1)
		f.seek(get_int(f) * 6, 1)
		
		if get_int(f) != 1:
			print('not 1 at amt9')
			report_position(f)
			return
		
		f.seek(get_int(f) * 4, 1)
		f.seek(get_int(f) * 2, 1)
		
		if get_int(f) != 1:
			print('not 1 at amt9 -> amt3')
			report_position(f)
			return
		
		amt_stuffs2 = get_int(f)
		
		for i in range(amt_stuffs2):
			f.seek(28, 1)
			mat_hash = get_int(f)
			#print(hex())
			dump_mesh(mat_hash, "id_%d_lod%d_var1" % (mesh_id, i), f)
		
		
		
		#if get_int(f) != 0:
		#	print('not null at atm9 -> amt3 amt')
		#	report_position(f)
		#	return
		
		amt_stuffs = get_int(f)
		
		for i in range(amt_stuffs):
			f.seek(28, 1)
			mat_hash = get_int(f)
			#print(hex(get_int(f))) # hash?
			dump_mesh(mat_hash, "id_%d_lod%d_var2" % (mesh_id, i), f)
		
		
		if get_int(f) != 0:
			print('not null at atm9 -> amt3 amt3')
			report_position(f)
			return	
			
		if get_int(f) != 0:
			print('not null at atm9 -> amt3 amt4')
			report_position(f)
			return
	
	print("entries pos:")
	report_position(f)
	
	amt_entries = get_int(f)
	print("amt entries %d" % amt_entries)
	
	f2 = open("positions.txt", "w")
	f2.write("%d\n" % amt_entries)
	
	for _ in range(amt_entries):
		type = get_int(f)
		#print("type: %d" % get_int(f))
		f.seek(24, 1) # bbox?
		amt_instances = get_int(f)
		f2.write("%d\n" % amt_instances)
		f2.write("type:%d\n" % type)
		for __ in range(amt_instances):
			vec3 = get_vec3f(f)
			f2.write("%f %f %f \n" % (vec3[0], vec3[1], vec3[2]))
			#print(get_vec3f(f))
			f.seek(29, 1)

	f2.close()
	
	return

def parse_RSCF(f):
	hdr = f.read(4)
	if hdr != b'RSCF':
		print ("is not RSCF file")
		return
		
	f_size = get_int(f)
	f_ver = get_int(f)
	f_subver = get_int(f)
	RSCF_type = get_int(f)
	RSCF_subtype = get_int(f)
	
	if f_ver != 4:
		print("RSCF file ver != 4")
		return
	if f_subver != 0:
		print("RSCF file subver != 0")
		return
	if RSCF_type != 0:
		print("RSCF file RSCF_type != 0")
		return
	if RSCF_subtype != 8:
		print("RSCF file RSCF_subtype != 8 (botanicals)")
		return
	
	f.seek(4, 1) # resource size
	f.seek(4, 1) # Bot\0 ASCII string
	
	parse_bot(f)
	
	f.close()
	
	return
	
	
file = open("Bot.RSCF", 'rb')

parse_RSCF(file)
