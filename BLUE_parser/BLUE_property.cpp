#include "stdafx.h"
#include "BLUE_property.h"
#include "Common.h"

#pragma warning( disable: 4996 )

extern bool g_try_unscramble;

struct asura_hdr {
	DWORD magic;
	DWORD file_size;
	DWORD type1;
	DWORD type2;
};

BLUE_property::BLUE_property()
{
}

BLUE_property::BLUE_property(BLUE_property_type prop_type, DWORD name_hash, char* name)
{
	m_property_name_hash = name_hash;
	m_property_name = name;
	m_type = prop_type;


	switch (prop_type) {
	case BLUE_PROPERTY_FLOAT:
		m_float_val = 0.0f;
		break;
	case BLUE_PROPERTY_BOOL:
		m_bool_val = false;
		break;
	case BLUE_PROPERTY_STR:
		m_str_val = nullptr;
		m_str_val_hash = 0;
		m_str_val_hash2 = 0;
		break;
	case BLUE_PROPERTY_INT:
		m_int_val = 0;
		break;
	case BLUE_PROPERTY_BLOB:
	default:
		printf("not supported (%d)\n", m_type);
		break;
	}
}

char* BLUE_property::BLUE_read_str_field(HANDLE f)
{
	DWORD a = 0;

	READ(m_str_val_hash2); // after "." symbol
	READ(m_str_val_hash);

	if (!m_str_val_hash)
	{
		return nullptr;
	}

	char* result = read_padded_str_f(f);

	if (g_try_unscramble && !result)
	{
		char* str_p = search_hash(m_str_val_hash);
		if (str_p)
		{
			return str_copy(str_p);
		}
	}

	return result;
}

void BLUE_property::BLUE_write_str_field(HANDLE f2)
{
	DWORD a = 0;

	WRITE(m_str_val_hash2); // after "." symbol
	WRITE(m_str_val_hash);

	if (!m_str_val_hash) return;
	write_padded_str_f(m_str_val, f2);
}

void * BLUE_property::BLUE_read_blob_field(HANDLE f)
{
	DWORD a = 0;
	READ(m_blob_size);
	if (m_blob_size)
	{
		char* mem = (char*)malloc(m_blob_size);
		READP(mem, m_blob_size);

		DWORD file_pos = SFPC(0);
		SFPC(-(int)file_pos & 3);
		
		return mem;
	}
	return nullptr;
}

void BLUE_property::BLUE_write_blob_field(HANDLE f2)
{
	DWORD a = 0;
	HANDLE f = f2;
	WRITE(m_blob_size);
	if (m_blob_size) {
		WRITEP(m_blob_val, m_blob_size);

		DWORD file_pos = SFPC(0);
		WRITEP("\0\0\0\0", -(int)file_pos & 3);
	}
}

bool BLUE_property::BLUE_set_value_str(char * str, char* second_str)
{
	if (m_type != BLUE_PROPERTY_STR) {
		printf("this property is not string type ! \n");
		return false;
	}
	m_str_val = str;

	if (str && *str) {
		m_str_val_hash = hash_f(str);
		if (second_str && *second_str) {
			m_str_val_hash2 = hash_f(second_str);
		}
		else {
			m_str_val_hash2 = m_str_val_hash;
		}
		

		//char* ch_p = strrchr(str, '.'); // Additional logics need here!
		//if (*(ch_p + 1) == 'a' || *(ch_p + 1) == 'A') {
		//	ch_p = strrchr(ch_p - 1, '\\');
		//
		//}
		//if (ch_p) {
		//	m_str_val_hash2 = hash_f(ch_p + 1);
		//}
		//else {
		//	m_str_val_hash2 = m_str_val_hash;
		//}
		return true;
	}
	else {
		m_str_val_hash = 0;
		m_str_val_hash2 = 0;
		return false;
	}
}

bool BLUE_property::BLUE_set_value_str(DWORD hash1, DWORD hash2)
{
	if (m_type != BLUE_PROPERTY_STR) {
		printf("this property is not string type ! \n");
		return false;
	}

	m_str_val = nullptr;
	m_str_val_hash = hash1;
	m_str_val_hash2 = hash2;
	return true;
}

bool BLUE_property::BLUE_set_value_float(float new_v)
{
	if (m_type != BLUE_PROPERTY_FLOAT) {
		printf("this property is not float type ! \n");
		return false;
	}

	m_float_val = new_v;

	return true;
}

bool BLUE_property::BLUE_set_value_bool(bool new_v)
{
	if (m_type != BLUE_PROPERTY_BOOL) {
		printf("this property is not bool type ! \n");
		return false;
	}
	m_bool_val = new_v;

	return true;
}

bool BLUE_property::BLUE_set_value_int(DWORD new_v)
{
	if (m_type != BLUE_PROPERTY_INT) {
		printf("this property is not default type ! \n");
		return false;
	}

	m_default_val = new_v;

	return true;
}

void BLUE_property::Print_property()
{
	printf("Property %s (%08x) \n", m_property_name ? m_property_name : "", m_property_name_hash);

	switch (m_type)
	{
	case BLUE_PROPERTY_FLOAT:
		printf("type: float, value = %f \n", m_float_val);
		break;
	case BLUE_PROPERTY_BOOL:
		printf("type: boolean, value = %s \n", m_bool_val ? "true" : "false");
		break;
	case BLUE_PROPERTY_STR:
		printf("type: string, value = %s full (%08x) short (%08x)\n", m_str_val, m_str_val_hash, m_str_val_hash2);
		break;
	case BLUE_PROPERTY_BLOB:
		printf("type: blob, size = %d \n", m_blob_size);
		Dump_hex(m_blob_val, m_blob_size);
		break;
	case BLUE_PROPERTY_INT:
		printf("type: int, value = %d \n", m_int_val);
		break;
	default:
		printf("type: unknown (%d), value = %d \n", m_type, m_default_val);
		break;
	}
}

bool BLUE_property::BLUE_store_property(HANDLE f2, DWORD type)
{
	DWORD a = 0;
	DWORD no_str_marker = 0x00FFFF00;

	WRITE(m_property_name_hash);
	if (m_property_name) { write_padded_str_f(m_property_name, f2); }
	else { WRITE(no_str_marker); }
	if (m_property_group_str) { write_padded_str_f(m_property_group_str, f2); }
	else { WRITE(no_str_marker); }

	WRITE(m_unk2);
	WRITE(m_unk3);
	WRITE(m_type);

	//DWORD bool_dw = m_bool_val;

	switch (type)
	{
	case 8:
		switch (m_type)
		{
		case BLUE_PROPERTY_INT:
			WRITE(m_int_val);
			break;
		case BLUE_PROPERTY_FLOAT:
			WRITE(m_float_val);
			break;
		case BLUE_PROPERTY_BOOL:
			WRITE(m_bool_val);
			break;
		case BLUE_PROPERTY_STR:
			BLUE_write_str_field(f2);
			break;
		case BLUE_PROPERTY_BLOB:
			BLUE_write_blob_field(f2);
			break;
		default:
			WRITE(m_default_val);
			break;
		}
		break;
	case 7:
	case 6:
		switch (m_type)
		{
		case BLUE_PROPERTY_INT:
			WRITE(m_int_val);
			break;
		case BLUE_PROPERTY_FLOAT:
			WRITE(m_float_val);
			break;
		case BLUE_PROPERTY_BOOL:
			WRITE(m_bool_val);
			break;
		case BLUE_PROPERTY_STR:
			BLUE_write_str_field(f2);
			break;
		default:
			WRITE(m_default_val);
			break;
		}
		break;
	case 5:
		printf("subclass type 5 not supported !\n");
		break;
	default:
		printf("subclass type %d not supported !\n", type);
		break;
	}

	return false;
}



BLUE_property::BLUE_property(HANDLE f, DWORD type)
{
	DWORD a = 0;

	READ(m_property_name_hash);
	m_property_name = read_padded_str_f(f);
	m_property_group_str = read_padded_str_f(f);
	READ(m_unk2);
	READ(m_unk3);
	//SFPC(8);
	READ(m_type);

	switch (type)
	{
	case 8:
		switch (m_type)
		{
		case BLUE_PROPERTY_INT:
			READ(m_int_val);
			break;
		case BLUE_PROPERTY_FLOAT:
			READ(m_float_val);
			break;
		case BLUE_PROPERTY_BOOL:
			READ(m_bool_val);
			break;
		case BLUE_PROPERTY_STR:
			m_str_val = BLUE_read_str_field(f);
			break;
		case BLUE_PROPERTY_BLOB:
			m_blob_val = BLUE_read_blob_field(f);
			break;
		default:
			//m_type = BLUE_PROPERTY_INT;
			READ(m_default_val);
			break;
		}
		break;
	case 7:
	case 6:
		switch (m_type)
		{
		case BLUE_PROPERTY_INT:
			READ(m_int_val);
			break;
		case BLUE_PROPERTY_FLOAT:
			READ(m_float_val);
			break;
		case BLUE_PROPERTY_BOOL:
			READ(m_bool_val);
			break;
		case BLUE_PROPERTY_STR:
			m_str_val = BLUE_read_str_field(f);
			break;
		default:
			//m_type = BLUE_PROPERTY_DEFAULT;
			READ(m_default_val);
			break;
		}
		break;
	case 5:
		printf("subclass type 5 not supported !\n");
		break;
	default:
		printf("subclass type %d not supported !\n", type);
		break;
	}

}


BLUE_property::~BLUE_property()
{
	if (m_property_name) free(m_property_name);
	if (m_property_group_str) free(m_property_group_str);

	if (m_type == BLUE_PROPERTY_STR) {
		free(m_str_val);
	}

	if (m_type == BLUE_PROPERTY_BLOB) {
		free(m_blob_val);
	}
}

BLUE_subclass::BLUE_subclass()
{
}

BLUE_subclass::BLUE_subclass(DWORD parent_hash, DWORD class_name_hash, char * class_name)
{
	m_subclass_name = class_name;
	m_subclass_name_hash = class_name_hash;
	m_unk = m_subclass_name_hash; // idk
	m_type = 8;
	m_parent_name_hash = parent_hash;
}

BLUE_subclass::BLUE_subclass(HANDLE f)
{
	DWORD a = 0;

	READ(m_subclass_name_hash);
	READ(m_type);
	READ(m_unk);
	READ(m_parent_name_hash);
	m_subclass_name = read_padded_str_f(f);

	if (!m_subclass_name && g_try_unscramble) {
		char* subclass_name_p = search_hash(m_subclass_name_hash);
		if(subclass_name_p) m_subclass_name = str_copy(subclass_name_p);
	}

	DWORD prop_cnt;
	READ(prop_cnt);

	m_properties.reserve(prop_cnt * sizeof(BLUE_property*));

	for (int i = 0; i < prop_cnt; i++) {
		m_properties.push_back(new BLUE_property(f, m_type));
	}
}

BLUE_subclass::~BLUE_subclass()
{
	if (m_subclass_name) free(m_subclass_name);

	for (auto obj : m_properties) {
		delete obj;
	}
}

BLUE_property * BLUE_subclass::Get_property(DWORD hash)
{
	for (auto prop_p : m_properties) {
		if (prop_p->m_property_name_hash == hash) {
			return prop_p;
		}
	}

	return nullptr;
}

BLUE_property * BLUE_subclass::Get_property(char * name)
{
	DWORD hash = hash_f(name);

	for (auto prop_p : m_properties) {
		if (prop_p->m_property_name_hash == hash) {
			return prop_p;
		}
	}

	return nullptr;
}

bool BLUE_subclass::Add_property(BLUE_property * new_prop)
{
	m_properties.push_back(new_prop);
	return true;
}

bool BLUE_subclass::Delete_property(DWORD hash)
{
	for (auto prop_p : m_properties) {
		if (prop_p->m_property_name_hash == hash) {
			prop_p->m_bDeleted = true;
			return true;
		}
	}
	return false;
}

void BLUE_subclass::BLUE_subclass_store(HANDLE f2)
{
	DWORD a = 0;
	WRITE(m_subclass_name_hash);
	WRITE(m_type);
	WRITE(m_unk);
	WRITE(m_parent_name_hash);

	write_padded_str_f(m_subclass_name, f2);


	DWORD prop_cnt = m_properties.size();
	WRITE(prop_cnt);

	for (auto prop_p : m_properties) {
		if(!prop_p->m_bDeleted)
			prop_p->BLUE_store_property(f2, m_type);
	}
}

BLUE_file::BLUE_file()
{
}

BLUE_file::BLUE_file(HANDLE f)
{



	DWORD a = 0;
	SFPS(0);
	asura_hdr file_hdr;
	READ(file_hdr.magic);
	READ(file_hdr.file_size);
	READ(file_hdr.type1);
	READ(file_hdr.type2);
	//READ(file_hdr);

	if (file_hdr.magic != 'EULB')
	{
		printf("bad BLUE header \n");
		m_bBad = true;
		return;
	}

	READ(m_type);

	if (m_type != 2) {
		printf("BLUE type %d not supported for editing now \n", m_type);
		m_bBad = true;
		return;
	}

	DWORD amount_of_entries;
	READ(amount_of_entries);

	if (amount_of_entries != 1) {
		printf("BLUE with multiple classes not supported for editing now \n", m_type);
		m_bBad = true;
		return;
	}

	READ(m_class_name_hash);
	m_class_name = read_padded_str_f(f);

	DWORD count_of_subclasses = 0;
	READ(count_of_subclasses);

	m_subclasses.reserve(count_of_subclasses * sizeof(BLUE_subclass*));

	if (g_try_unscramble && !m_class_name)
	{
		//hash_tree_node2* hn_p = search_by_hash2(g_hashs, Class_name_hash);
		char* class_name_p = search_hash(m_class_name_hash);
		m_class_name = str_copy(class_name_p);
	}

	for (int i = 0; i < count_of_subclasses; i++) {
		m_subclasses.push_back(new BLUE_subclass(f));
	}

	return;
}

BLUE_file::~BLUE_file()
{
	if (m_class_name) free(m_class_name);

	for (auto p_subclass : m_subclasses) {
		delete p_subclass;
	}

}

BLUE_subclass * BLUE_file::Get_subclass(char * name)
{
	DWORD hash = hash_f(name);

	for (auto cls_p : m_subclasses) {
		if (cls_p->m_subclass_name_hash == hash) {
			return cls_p;
		}
	}

	return nullptr;
}

BLUE_subclass * BLUE_file::Get_subclass(DWORD hash)
{
	for (auto cls_p : m_subclasses) {
		if (cls_p->m_subclass_name_hash == hash) {
			return cls_p;
		}
	}

	return nullptr;
}

bool BLUE_file::Delete_class(DWORD hash)
{
	for (auto sub_cls : m_subclasses) {
		if (sub_cls->m_subclass_name_hash == hash) {
			sub_cls->m_bDeleted = true;
			return true;
		}
	}
	return false;
}

bool BLUE_file::Add_class(BLUE_subclass * new_class)
{
	m_subclasses.push_back(new_class);
	return true;
}

void BLUE_file::BLUE_file_store(HANDLE f2)
{
	DWORD a = 0;
	asura_hdr hdr;

	hdr.magic = 'EULB';
	hdr.type1 = 0;
	hdr.type2 = 0;

	WRITE(hdr);
	WRITE(m_type);
	DWORD dw = 1;
	WRITE(dw);

	WRITE(m_class_name_hash);
	write_padded_str_f(m_class_name, f2);

	DWORD cnt_subclass = m_subclasses.size();
	WRITE(cnt_subclass);

	for (auto p_subclass : m_subclasses) {
		if(!p_subclass->m_bDeleted)
			p_subclass->BLUE_subclass_store(f2);
	}

	dw = 0;
	WRITE(dw);

	HANDLE f = f2;
	//size_t file_end = SFPC(0);
	//WRITEP("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16 - file_end & 15);

	size_t file_size = SFPC(0);

	hdr.file_size = file_size;
	SFPS(0);
	WRITE(hdr);

	return;
}
