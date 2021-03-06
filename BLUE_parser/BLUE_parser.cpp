// ConsoleApplication6.cpp: blue parser
//

#include "stdafx.h"
#include "Common.h"
#include "BLUE_property.h"
#include "String_extraction.h"
//#include <map>



//Defines

//Macro



//Structs
struct asura_hdr {
	DWORD magic;
	DWORD file_size;
	DWORD type1;
	DWORD type2;
};



//Globals
bool g_extract_strings = false;
bool g_try_unscramble = true;
bool g_hierarchy_only = false;

char g_UserInput[1024];

String_extraction g_str_storage;

void Read_str_val_type_3(HANDLE f, bool print)
{
	DWORD a = 0;
	DWORD hash_of_substring; // after "." symbol
	DWORD hash_of_string;
	READ(hash_of_substring);
	READ(hash_of_string);
	if (!hash_of_string)
	{
		if (print)
			printf("(value: None) \n");
		return;
	}
	char* val_str = (char*)malloc(1024);
	read_padded_str(f, val_str);

	size_t str_length = strlen(val_str);

	if (g_try_unscramble && (str_length == 0))
	{
		char* str_p = search_hash(hash_of_string);
		if (str_p)
		{
			if (print)
				printf("(value: \"%s\" , %08x) \n", str_p, hash_of_substring);
		}	
		else
		{
			if (print)
				printf("(value: Unknown (%08x, %08x)) \n", hash_of_string, hash_of_substring);
		}
	}
	else
	{
		if (print)
			printf("(value: \"%s\") \n", val_str);
	}

	if (g_extract_strings) {
		if (str_length) {
			g_str_storage.add(val_str, str_length);
		}
	}

	//dbgprint("%s\n", val_str);
	free(val_str);
	return;
}

void Read_str_val_type_4(HANDLE f, bool print)
{
	DWORD a = 0;
	DWORD size;
	READ(size);
	if (size)
	{
		char* mem = (char*)malloc(size + 1);
		READP(mem, size);

		mem[size] = '\0';

		DWORD file_pos = SFPC(0);

		SFPC(-(int)file_pos & 3); // ...

		//printf("%08x compiler hack2 \n", file_pos);

		file_pos = SFPC(0);
		//printf("%08x compiler hack2 ! \n", file_pos);

		if (print)
			printf("(value: \"%s\") \n", mem);
		free(mem);
		return;
	}

	printf("(value: None) \n");

}

void Read_BLUE_subclass_property(HANDLE f, DWORD type)
{
	DWORD a = 0;

	DWORD property_name_hash; // property name hash
	DWORD unk2;
	DWORD unk3; // some common hash value or hash of some common name?
	DWORD type2_or_count;
	char* str1 = (char*)malloc(1024); // Property name
	char* str2 = (char*)malloc(1024); // Class of property ? (Skin/Model, health, etc)


	READ(property_name_hash);
	read_padded_str(f, str1);
	read_padded_str(f, str2);
	READ(unk2);
	READ(unk3);
	READ(type2_or_count);

	if (!g_hierarchy_only)
	{
		if (g_try_unscramble && (strlen(str1) == 0))
		{
			//hash_tree_node2* hn_p = search_by_hash2(g_hashs, property_name_hash);
			char* str_prop_p = search_hash(property_name_hash);
			if (str_prop_p)
			{
				printf("\t\t %s(%08x) %s type: %d ", \
					str_prop_p, property_name_hash, str2, type2_or_count);
			}
			else
			{
				printf("\t\t unk(%08x) %s type: %d ", \
					property_name_hash, str2, type2_or_count);
			}
		}
		else
		{
			printf("\t\t %s %s type: %d ", \
				str1, str2, type2_or_count);
		}
	}

	if (g_extract_strings) {
		size_t str1_len = strlen(str1);
		if (str1_len) {
			g_str_storage.add(str1, str1_len);
		}

		size_t str2_len = strlen(str2);
		if (str2_len) {
			g_str_storage.add(str2, str1_len);
		}
	}

	free(str1);
	free(str2);

	if (!g_hierarchy_only)
	{
		switch (type)
		{
		case 8:
			switch (type2_or_count)
			{
			case 1:
				float val;
				READ(val);
				printf("(value : %f) \n", val);
				break;
			case 2:
				BYTE val2;
				READ(val2);
				if (val2)
				{
					printf("(value : True ) \n");
				}
				else
				{
					printf("(value : False) \n");
				}
				break;
			case 3:
				Read_str_val_type_3(f, 1);
				break;
			case 4:
				Read_str_val_type_4(f, 1);
				break;
			default:
				DWORD val4;
				READ(val4);
				printf("((default branch) value : %08x) \n", val4);
				break;
			}
			break;
		case 7:
		case 6:
			switch (type2_or_count)
			{
			case 1:
				float val;
				READ(val);
				printf("(value : %f) \n", val);
				break;
			case 2:
				BYTE val2;
				READ(val2);
				if (val2)
				{
					printf("(value : True ) \n");
				}
				else
				{
					printf("(value : False) \n");
				}
				break;
			case 3:
				Read_str_val_type_3(f, 1);
				break;
			default:
				DWORD val4;
				READ(val4);
				printf("((default branch) value : %08x) \n", val4);
				break;
			}
			break;
		case 5:
			for (DWORD i = 0; i < type2_or_count; i++)
			{
				DWORD some_type;
				READ(some_type);
				switch (some_type)
				{
				case 1:
					float val;
					READ(val);
					printf("(value : %f) \n", val);
					break;
				case 2:
					BYTE val2;
					READ(val2);
					if (val2)
					{
						printf("(value : True ) \n");
					}
					else
					{
						printf("(value : False) \n");
					}
					break;
				case 3:
					Read_str_val_type_3(f, 1);
					break;
				default:
					DWORD val4;
					READ(val4);
					printf("((default branch) value : %08x) \n", val4);
					break;
				}
			}
		}
	}
	else
	{
		switch (type)
		{
		case 8:
			switch (type2_or_count)
			{
			case 1:
				float val;
				READ(val);
				//printf("(value : %f) \n", val);
				break;
			case 2:
				BYTE val2;
				READ(val2);
				if (val2)
				{
					//printf("(value : True ) \n");
				}
				else
				{
					//printf("(value : False) \n");
				}
				break;
			case 3:
				Read_str_val_type_3(f, 0);
				break;
			case 4:
				Read_str_val_type_4(f, 0);
				break;
			default:
				DWORD val4;
				READ(val4);
				//printf("((default branch) value : %08x) \n", val4);
				break;
			}
			break;
		case 7:
		case 6:
			switch (type2_or_count)
			{
			case 1:
				float val;
				READ(val);
				//printf("(value : %f) \n", val);
				break;
			case 2:
				BYTE val2;
				READ(val2);
				if (val2)
				{
					//printf("(value : True ) \n");
				}
				else
				{
					//printf("(value : False) \n");
				}
				break;
			case 3:
				Read_str_val_type_3(f, 0);
				break;
			default:
				DWORD val4;
				READ(val4);
				//printf("((default branch) value : %08x) \n", val4);
				break;
			}
			break;
		case 5:
			for (DWORD i = 0; i < type2_or_count; i++)
			{
				DWORD some_type;
				READ(some_type);
				switch (some_type)
				{
				case 1:
					float val;
					READ(val);
					//printf("(value : %f) \n", val);
					break;
				case 2:
					BYTE val2;
					READ(val2);
					if (val2)
					{
						//printf("(value : True ) \n");
					}
					else
					{
						//printf("(value : False) \n");
					}
					break;
				case 3:
					Read_str_val_type_3(f, 0);
					break;
				default:
					DWORD val4;
					READ(val4);
					//printf("((default branch) value : %08x) \n", val4);
					break;
				}
			}
		}
	}
	
}

void Read_BLUE(HANDLE f)
{
	DWORD a = 0;
	SFPS(0);

	DWORD hdr_magic_v;
	READ(hdr_magic_v);

	if (hdr_magic_v == 'BLUE') {
		set_file_state_BE(true);
	}
	else {
		set_file_state_BE(false);
	}

	SFPS(0);

	asura_hdr file_hdr;
	READ(file_hdr.magic);
	READ(file_hdr.file_size);
	READ(file_hdr.type1);
	READ(file_hdr.type2);

	if (file_hdr.magic != 'EULB')
	{
		printf("bad BLUE header \n");
		return;
	}

	DWORD BLUE_type;
	READ(BLUE_type);

	DWORD amount_of_entries;
	READ(amount_of_entries);
	printf("Detected BLUE\n type: %d \n unk: %d \n", BLUE_type, amount_of_entries);

	if (BLUE_type == 2 || BLUE_type == 1)
	{
		for (DWORD i = 0; i < amount_of_entries; i++)
		{
			DWORD Class_name_hash = 0;
			char* Class_name = (char*)malloc(1024);
			READ(Class_name_hash);
			read_padded_str(f, Class_name);
			DWORD count_of_subclasses = 0;
			READ(count_of_subclasses);

			if (g_try_unscramble && (strlen(Class_name) == 0))
			{
				//hash_tree_node2* hn_p = search_by_hash2(g_hashs, Class_name_hash);
				char* class_name_p = search_hash(Class_name_hash);
				if (class_name_p)
					printf("%s class (%d subclasses)\n", class_name_p, count_of_subclasses);
				else
					printf("(%08x) class (%d subclasses)\n", Class_name_hash, count_of_subclasses);
			}
			else
			{
				printf("%s class (%d subclasses)\n", Class_name, count_of_subclasses);
			}

			if (g_extract_strings) {
				size_t class_name_len = strlen(Class_name);
				if (class_name_len) {
					g_str_storage.add(Class_name, class_name_len);
				}
			}

			for (DWORD j = 0; j < count_of_subclasses; j++)
			{
				DWORD subclass_name_hash = 0;
				DWORD type_of_subclass = 0;
				DWORD parent_class_hash = 0;
				char* subclass_name = (char*)malloc(1024);
				DWORD properies_count = 0;
				READ(subclass_name_hash);
				READ(type_of_subclass);
				SFPC(4);
				READ(parent_class_hash);
				read_padded_str(f, subclass_name);
				READ(properies_count);

				if (g_extract_strings) {
					size_t subclass_name_len = strlen(subclass_name);
					if (subclass_name_len) {
						g_str_storage.add(subclass_name, subclass_name_len);
					}
				}

				if (g_try_unscramble && (strlen(subclass_name) == 0))
				{
					//hash_tree_node2* hn_p = search_by_hash2(g_hashs, subclass_name_hash);
					char* subclass_name_p = search_hash(subclass_name_hash);
					if (subclass_name_p)
					{
						//hash_tree_node2* hn_p2 = search_by_hash2(g_hashs, unk_hash);
						char* parent_name_p = search_hash(parent_class_hash);
						if (parent_name_p)
						{
							if (!g_hierarchy_only)
							{
								printf("\t- %s (parent: %s type: %d properties_count: %d) \n", \
									subclass_name_p, parent_name_p, type_of_subclass, properies_count);
							}
							else
							{
								printf("%s:%s\n", subclass_name_p, parent_name_p);
							}
						}
						else
						{
							if (!g_hierarchy_only)
							{
								printf("\t- %s (parent: unknown (%08x) type: %d  properties_count: %d) \n", \
									subclass_name_p, parent_class_hash, type_of_subclass, properies_count);
							}
							else
							{
								printf("%s:(%08x)\n", subclass_name_p, parent_class_hash);
							}
							
						}

					}
					else
					{
						//hash_tree_node2* hn_p2 = search_by_hash2(g_hashs, unk_hash);
						char* parent_name_p = search_hash(parent_class_hash);
						if (parent_name_p)
						{
							if (!g_hierarchy_only)
							{
								printf("\t- unknown (%08x) (parent: %s type: %d properties_count: %d) \n", \
									subclass_name_hash, parent_name_p, type_of_subclass, properies_count);
							}
							else
							{
								printf("(%08x):%s\n", subclass_name_hash, parent_name_p);
							}
							
						}
						else
						{
							if (!g_hierarchy_only)
							{
								printf("\t- unknown (%08x) (parent: unknown (%08x) type: %d properties_count: %d) \n", \
									subclass_name_hash, parent_class_hash, type_of_subclass, properies_count);
							}
							else
							{
								printf("(%08x):(%08x)\n", subclass_name_hash, parent_class_hash);
							}
						}

					}
				}
				else
				{
					if (g_try_unscramble)
					{
						char* parent_name_p = search_hash(parent_class_hash);
						//hash_tree_node2* hn_p2 = search_by_hash2(g_hashs, unk_hash);
						if (parent_name_p)
						{
							if (!g_hierarchy_only)
							{
								printf("\t- %s (parent: %s type: %d properties_count: %d) \n", 
									subclass_name, parent_name_p, type_of_subclass, properies_count);
							}
							else
							{
								printf("%s:%s\n", subclass_name, parent_name_p);
							}
						}
						else
						{
							if (!g_hierarchy_only)
							{
								printf("\t- %s (parent: unknown (%08x) type: %d properties_count: %d) \n", 
									subclass_name, parent_class_hash, type_of_subclass, properies_count);
							}
							else
							{
								printf("%s:(%08x)\n", subclass_name, parent_class_hash);
							}
						}
					}
					else
					{
						if (!g_hierarchy_only)
						{
							if ((strlen(subclass_name) == 0))
							{
								printf("\t- %08x (type: %d parent: (hash: %08x) properties_count: %d) \n", \
									subclass_name_hash, type_of_subclass, parent_class_hash, properies_count);
							}
							else
							{
								printf("\t- %s (type: %d parent: (hash: %08x) properties_count: %d) \n", \
									subclass_name, type_of_subclass, parent_class_hash, properies_count);
							}
							
						}
						else
						{
							if ((strlen(subclass_name) == 0))
							{
								printf("(%08x):(%08x)\n", subclass_name_hash, parent_class_hash);
							}
							else
							{
								printf("%s:(%08x)\n", subclass_name, parent_class_hash);
							}
						}
						
					}
				}

				for (DWORD k = 0; k < properies_count; k++)
				{
					Read_BLUE_subclass_property(f, type_of_subclass);
				}
			}
			printf("Reached Point after subclasses \n");
			DWORD again_some_count = 0;
			READ(again_some_count);

			printf("again some count = %d \n", again_some_count);

			// conditional can not exist
			//for loop again here
		}
	}
	else if (BLUE_type == 0)
	{
		for (DWORD i = 0; i < amount_of_entries; i++)
		{
			DWORD Class_name_hash = 0;
			DWORD count_of_subclasses = 0;
			char* Class_name = (char*)malloc(1024);
			READ(Class_name_hash);
			READ(count_of_subclasses);
			read_padded_str(f, Class_name);

			if (g_extract_strings) {
				size_t Class_name_len = strlen(Class_name);
				if (Class_name_len) {
					g_str_storage.add(Class_name, Class_name_len);
				}
			}

			printf("%s class (%d subclasses)\n", Class_name, count_of_subclasses);
			//dbgprint("%s\n", Class_name);

			for (DWORD j = 0; j < count_of_subclasses; j++)
			{
				//DWORD subclass_name_hash = 0;
				DWORD type_of_subclass = 0;
				DWORD unk_hash = 0;
				char* subclass_name = (char*)malloc(1024);
				DWORD properies_count = 0;
				//READ(subclass_name_hash);
				READ(type_of_subclass);
				SFPC(4);
				READ(unk_hash);
				read_padded_str(f, subclass_name);
				READ(properies_count);

				if (g_extract_strings) {
					size_t subclass_name_len = strlen(subclass_name);
					if (subclass_name_len) {
						g_str_storage.add(subclass_name, subclass_name_len);
					}
				}

				printf("\t- %s type: %d unk_hash: %08x properties_count: %d \n", \
					subclass_name, type_of_subclass, unk_hash, properies_count);
				//dbgprint("%s\n", subclass_name);

				for (DWORD k = 0; k < properies_count; k++)
				{
					Read_BLUE_subclass_property(f, type_of_subclass);
				}
			}
			printf("Reached Point after subclasses \n");
			DWORD again_some_count = 0;
			READ(again_some_count);

			printf("again some count = %d \n", again_some_count);

			// conditional can not exist
			//for loop again here
		}
	}
	else
	{
		printf("Unknown BLUEprint type!, I am work with Aliens vs Predator (2010) BLUEprints !\n");
	}
}

void get_input_from_user(const char* prompt)
{
	puts(prompt);
	memset(g_UserInput, 0, sizeof(g_UserInput));
	gets_s(g_UserInput, sizeof(g_UserInput));

	str_to_lower(g_UserInput);

	//AnsiLower(g_UserInput);

}

bool get_user_dword_input(DWORD* val) {
	get_input_from_user("value:");
	if (sscanf_s(g_UserInput, "%d", val) == 1) {
		return true;
	}
	else {
		printf("not integer value\n");
		return false;
	}
}

bool get_user_float_input(float* val) {
	get_input_from_user("value:");
	if (sscanf_s(g_UserInput, "%f", val) == 1) {
		return true;
	}
	else {
		printf("not float value\n");
		return false;
	}
}

bool get_user_bool_input(bool* val) {
	get_input_from_user("value:");

	if (g_UserInput[0] == '0') {
		*val = false; return true;
	}

	if (g_UserInput[0] == '1') {
		*val = true; return true;
	}

	if (!strcmp("true", g_UserInput)) {
		*val = true; return true;
	}

	if (!strcmp("false", g_UserInput)) {
		*val = false; return true;
	}

	printf("not bool value\n");
	return false;
}



bool get_user_str_input(char** str) {
	get_input_from_user("value:");
	*str = str_copy(g_UserInput);
	return true;
}



DWORD get_name_or_hash_dialog() {
	get_input_from_user("str or hash (0x123...):");

	if (g_UserInput[0] == '0' && g_UserInput[1] == 'x') {
		DWORD hash = 0;
		if (sscanf_s(g_UserInput + 2, "%08x", &hash) == 1) {
			return hash;
		}
		else {
			printf("hash input incorrect\n");
			return -2;
		}
	}
	return -1;
}

void Print_editor_help() {
	printf("Command list: \n");
	printf("exit - exit from application \n");
	printf("help - print this help \n");
	printf("select_class - select class by hash or name \n");
	printf("select_prop - select class property by hash or name \n");
	printf("print_prop - print property value \n");
	printf("edit_prop - edit property value \n");
	printf("save - save changes to new BLUE file \n");
	printf("add_prop - add property to selected class\n");
	printf("delete_prop - delete property from selected class \n");
	printf("add_class - add class to BLUE file \n");
	printf("delete_class - delete class from BLUE file \n");
}

void Editor_mode(HANDLE f) {

	printf("Editor mode enabled \n");
	Print_editor_help();

	BLUE_file blue_f(f);

	if (blue_f.m_bBad) return;

	bool bCont = true;
	bool bSaveAndExit = false;
	bool bUnsavedChanges = false;

	
	BLUE_subclass* p_selected_subclass = nullptr;
	BLUE_property* p_prop = nullptr;


	while (bCont) {

		get_input_from_user("command: ");

		if (!strncmp("exit", g_UserInput, sizeof("exit"))) {
			if (bUnsavedChanges) {
				printf("You have unsaved changes \n");
				printf("exit(y)/ don't exit (n) / save & exit (s) \n");
				get_input_from_user("y/n/s: ");

				switch (g_UserInput[0]) {
				case 'y':
				case 'Y':
					bCont = false;
					continue;
				case 'n':
				case 'N':
					continue;

				case 's':
				case 'S':
					// Save code
					bSaveAndExit = true;
				}
			}
			bCont = false;
		}

		if (!strncmp("help", g_UserInput, sizeof("help"))) {
			Print_editor_help();
			continue;
		}

		// Selection of class
		if (!strncmp("select_class", g_UserInput, sizeof("select_class"))) {
			DWORD hash = get_name_or_hash_dialog();
			if (hash == -2) continue;
			else if (hash == -1) p_selected_subclass = blue_f.Get_subclass(g_UserInput);
			else p_selected_subclass = blue_f.Get_subclass(hash);

			if (!p_selected_subclass) {
				printf("class not found \n");
			}
			printf("Class %s selected \n", p_selected_subclass->m_subclass_name ? p_selected_subclass->m_subclass_name : "");
			continue;
		}

		if (!strncmp("select_prop", g_UserInput, sizeof("select_prop"))) {
			if (!p_selected_subclass) {
				printf("class not selected use select_class command first\n");
				continue;
			}

			DWORD hash = get_name_or_hash_dialog();

			if (hash == -2) continue;
			else if (hash == -1) p_prop = p_selected_subclass->Get_property(g_UserInput);
			else p_prop = p_selected_subclass->Get_property(hash);

			if (!p_prop) {
				printf("property not found \n");
				continue;
			}
			printf("Property %s selected \n", p_prop->m_property_name ? p_prop->m_property_name : "");
			continue;
		}

		if (!strncmp("print_prop", g_UserInput, sizeof("print_prop"))) {
			if (!p_prop) {
				printf("property not selected use select_prop command first\n");
				continue;
			}

			p_prop->Print_property();
			continue;
		}

		if (!strncmp("edit_prop", g_UserInput, sizeof("edit_prop"))) {
			if (!p_prop) {
				printf("property not selected use select_prop command first\n");
				continue;
			}

			p_prop->Print_property();

			bool bNew_val_set = false;

			float val_f = 0.0;
			bool val_b = false;
			char* str_v = nullptr;
			char* str_v2 = nullptr;
			DWORD hash;
			DWORD str_hash;
			DWORD str_hash2;
			DWORD dw_val = 0;
			bool use_only_hashes = false;

			switch (p_prop->m_type)
			{
			case BLUE_PROPERTY_FLOAT:
				
				if (get_user_float_input(&val_f)) {
					p_prop->BLUE_set_value_float(val_f);
					bNew_val_set = true;
					break;
				}
				break;
			case BLUE_PROPERTY_BOOL:
				
				if (get_user_bool_input(&val_b)) {
					p_prop->BLUE_set_value_bool(val_b);
					bNew_val_set = true;
					break;
				}
				break;
			case BLUE_PROPERTY_STR:
				str_hash = 0;
				str_hash2 = 0;

				hash = get_name_or_hash_dialog();

				if (hash == -2) continue;
				else if (hash == -1) str_v = str_copy(g_UserInput);
				else str_hash = hash;

				printf("Enter short str value\n");

				hash = get_name_or_hash_dialog();

				if (hash == -2) continue;
				else if (hash == -1) str_v2 = str_copy(g_UserInput);
				else str_hash2 = hash;

				if (str_v && str_v2)
				{
					p_prop->BLUE_set_value_str(str_v, str_v2);
					bNew_val_set = true;
					break;
				}else if (str_hash && str_hash2) {
					p_prop->BLUE_set_value_str(str_hash, str_hash2);
					bNew_val_set = true;
					break;
				}
				else {
					printf("incorrect input! \n");
				}
				break;
			case BLUE_PROPERTY_BLOB:
				// not supported yet
				printf("Blobs not supported yet\n");
				break;
			case BLUE_PROPERTY_INT:
				if (get_user_dword_input(&dw_val)) {
					p_prop->BLUE_set_value_int(dw_val);
					bNew_val_set = true;
					break;
				}
				break;

			default:
				break;
			}

			if (bNew_val_set) {
				bUnsavedChanges = true;
				printf("New value is set\n");
			}
			else {
				printf("New value is not set\n");
			}
			continue;
		}

		if (!strncmp("add_prop", g_UserInput, sizeof("add_prop"))) {

			if (!p_selected_subclass) {
				printf("class not selected use select_class command first\n");
				continue;
			}

			BLUE_property* new_prop = nullptr;

			printf("Enter name of property or hash \n");

			char* prop_name = nullptr;

			DWORD hash = get_name_or_hash_dialog();
			DWORD prop_name_hash;

			if (hash == -2) continue;
			else if (hash == -1) {
				prop_name = str_copy(g_UserInput); prop_name_hash = hash_f(prop_name);
			}
			else prop_name_hash = hash;
			

			get_input_from_user("enter type of property (int, float, bool, str)\ntype: ");
			if (!strncmp("int", g_UserInput, sizeof("int"))) { new_prop = new BLUE_property(BLUE_PROPERTY_INT, prop_name_hash, prop_name); }
			else if (!strncmp("float", g_UserInput, sizeof("float"))) { new_prop = new BLUE_property(BLUE_PROPERTY_FLOAT, prop_name_hash, prop_name); }
			else if (!strncmp("bool", g_UserInput, sizeof("bool"))) { new_prop = new BLUE_property(BLUE_PROPERTY_BOOL, prop_name_hash, prop_name); }
			else if (!strncmp("str", g_UserInput, sizeof("str"))) { new_prop = new BLUE_property(BLUE_PROPERTY_STR, prop_name_hash, prop_name); }
			else { printf("incorrect type! \n"); }

			p_selected_subclass->Add_property(new_prop);
			printf("Property added \n");
			continue;
		}

		if (!strncmp("delete_prop", g_UserInput, sizeof("delete_prop"))) {
			if (!p_selected_subclass) {
				printf("class not selected use select_class command first\n");
				continue;
			}
			printf("Enter name of property or hash \n");

			DWORD hash = get_name_or_hash_dialog();
			if (hash == -2) continue;
			if (hash == -1) hash = hash_f(g_UserInput);

			if (p_selected_subclass->Delete_property(hash)) {
				printf("Property deleted \n");
			}
			else {
				printf("Property not found \n");
			}
			continue;
		}

		if (!strncmp("add_class", g_UserInput, sizeof("add_class"))) {

			printf("Enter name of class or hash \n");

			char* class_name = nullptr;

			DWORD hash = get_name_or_hash_dialog();
			DWORD class_name_hash;

			if (hash == -2) continue;
			else if (hash == -1) {
				class_name = str_copy(g_UserInput); class_name_hash = hash_f(class_name);
			}
			else class_name_hash = hash;

			printf("Enter parent class name of class or hash \n");

			char* parent_class_name = nullptr;

			hash = get_name_or_hash_dialog();
			DWORD parent_class_name_hash;

			if (hash == -2) continue;
			else if (hash == -1) {
				parent_class_name = str_copy(g_UserInput); parent_class_name_hash = hash_f(class_name);
			}
			else parent_class_name_hash = hash;

			BLUE_subclass* new_class = new BLUE_subclass(parent_class_name_hash, class_name_hash, class_name);

			blue_f.Add_class(new_class);

			printf("Class added \n");
			continue;
		}

		if (!strncmp("delete_class", g_UserInput, sizeof("delete_class"))) {
			printf("Enter name of class or hash \n");

			DWORD hash = get_name_or_hash_dialog();
			if (hash == -2) continue;
			if (hash == -1) hash = hash_f(g_UserInput);

			if (blue_f.Delete_class(hash)) {
				printf("Class deleted \n");
			}
			else {
				printf("Class not found \n");
			}
			continue;
		}

		if (!strncmp("save", g_UserInput, sizeof("save")) || bSaveAndExit) {
			get_input_from_user("filename: ");

			HANDLE f2 = CreateFileA(g_UserInput, GENERIC_WRITE, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			if (GetLastError() != ERROR_SUCCESS && GetLastError() != ERROR_ALREADY_EXISTS) {
				printf("File create error: %d\n", GetLastError());
				CloseHandle(f2);
				continue;
			}

			blue_f.BLUE_file_store(f2);
			bUnsavedChanges = false;
			printf("File saved! \n");
			continue;
			//if (bSaveAndExit) bCont = false;
		}

		printf("Incorrect Input \n");
	}

	return;
}

void Usage_func()
{
	printf("Usage: program [file_name] [optional] [optional2]\n");
	printf("BLUE parser\n");
	printf("Supported games: AVP2010 only ? \n");
	printf("optional: \"dontunscramble\" - will not try unscramble Hashs \n");

	printf("optional: \"editor\" - will activate editor mode \n");
	printf("optional: \"extract_strings\" - will extract all strings in BLUE file\n");
	printf("optional2: \"hierarchy_only\" - hierarchy of classes inside no data displayed \n");
	return;
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		Usage_func();
		return 0;
	}

	bool bEditor = false;

	if (argv[2])
	{
		if (!strcmp(argv[2], "dontunscramble")) g_try_unscramble = false;

		if (!strcmp(argv[2], "editor")) bEditor = true;

		if (!strcmp(argv[2], "extract_strings")) g_extract_strings = true;

		if (!strcmp(argv[2], "test_hash"))
		{
			Init_hashs((char*)"names.txt");
			printf("%s \n", search_hash(1801878019));
			printf("%s \n", search_hash(0x766161D2));
			printf("%s \n", search_hash(1670523056));
			printf("%s \n", search_hash(562750696));
			printf("%s \n", search_hash(315656744));
			printf("%s \n", search_hash(4211224973));
			printf("%s \n", search_hash(340768193));
			printf("%s \n", search_hash(1150004599));
			exit(0);
		}

		if (!(strcmp(argv[2], "hierarchy_only"))) g_hierarchy_only = true;

		if (argv[3])
		{
			if (!(strcmp(argv[3], "hierarchy_only"))) g_hierarchy_only = true;
		}
	}

	HANDLE f = CreateFileA(argv[1], GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (GetLastError() != ERROR_SUCCESS)
	{
		printf("File open error: %d\n", GetLastError());
		CloseHandle(f);
		return EXIT_FAILURE;
	}

	if (g_try_unscramble) {
		printf("Call make Init hashs\n");
		Init_hashs((char*)"names.txt");
	}

	if (bEditor) {

		Editor_mode(f);
		CloseHandle(f);

		return EXIT_SUCCESS;
	}



	printf("call Read BLUE\n");

	Read_BLUE(f);

	if (g_extract_strings) {
		g_str_storage.store_to_file("extracted.txt");
	}

	CloseHandle(f);
	//system("PAUSE");
	return 0;
}

