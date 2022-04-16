#pragma once
#include <vector>

enum BLUE_property_type {
	BLUE_PROPERTY_INT = 0,
	BLUE_PROPERTY_FLOAT = 1,
	BLUE_PROPERTY_BOOL = 2,
	BLUE_PROPERTY_STR = 3,
	BLUE_PROPERTY_BLOB = 4,
	//BLUE_PROPERTY_DEFAULT = 5
};

class BLUE_property
{
public:
	bool m_bDeleted = false;

	char* m_property_name = nullptr;
	DWORD m_property_name_hash;

	char* m_property_group_str = nullptr;

	BLUE_property_type m_type;
	union {
		float m_float_val;
		char* m_str_val;
		bool m_bool_val;
		void* m_blob_val;
		DWORD m_int_val;
		DWORD m_default_val;
	};

	DWORD m_str_val_hash;
	DWORD m_str_val_hash2;

	DWORD m_blob_size;

	DWORD m_unk2;
	DWORD m_unk3;

	size_t m_container_size;

	BLUE_property();
	BLUE_property(BLUE_property_type prop_type, DWORD name_hash, char* name = nullptr);
	BLUE_property(HANDLE f, DWORD type);
	~BLUE_property();

	char* BLUE_read_str_field(HANDLE f);
	void  BLUE_write_str_field(HANDLE f2);

	void* BLUE_read_blob_field(HANDLE f);
	void BLUE_write_blob_field(HANDLE f2);

	bool BLUE_set_value_str(char* str, char* second_str);
	bool BLUE_set_value_str(DWORD hash1, DWORD hash2);
	bool BLUE_set_value_float(float new_v);
	bool BLUE_set_value_bool(bool new_v);
	bool BLUE_set_value_int(DWORD new_v);

	void Print_property();

	bool BLUE_store_property(HANDLE f, DWORD type);

};

class BLUE_subclass {
public:

	bool m_bDeleted = false;

	char* m_subclass_name;
	DWORD m_subclass_name_hash;
	DWORD m_parent_name_hash;
	
	UINT m_type;
	DWORD m_unk;

	std::vector<BLUE_property*> m_properties;

	BLUE_subclass();
	BLUE_subclass(DWORD parent_hash, DWORD class_name_hash, char* class_name = nullptr);
	BLUE_subclass(HANDLE f);
	~BLUE_subclass();

	BLUE_property* Get_property(DWORD hash);
	BLUE_property* Get_property(char* name);

	bool Add_property(BLUE_property* new_prop);
	bool Delete_property(DWORD hash);


	void BLUE_subclass_store(HANDLE f2);

};

class BLUE_file {
public:

	bool m_bBad = false;

	char* m_class_name;
	DWORD m_class_name_hash;

	size_t m_amt_classes;
	DWORD m_type;

	std::vector<BLUE_subclass*> m_subclasses;

	BLUE_file();
	BLUE_file(HANDLE f);
	~BLUE_file();

	BLUE_subclass* Get_subclass(char* name);
	BLUE_subclass* Get_subclass(DWORD hash);

	bool Delete_class(DWORD hash);
	bool Add_class(BLUE_subclass* new_class);

	void BLUE_file_store(HANDLE f2);

};