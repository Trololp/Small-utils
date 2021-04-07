// Parser for asura BLUE chunks. BLUE - bluprints
//

#include "stdafx.h"

#pragma warning( disable: 4996 )

//Defines

//Macro
#define SFPS(pos) SetFilePointer(f, pos, 0, FILE_BEGIN)
#define SFPC(pos) SetFilePointer(f, pos, 0, FILE_CURRENT)
#define READ(v) ReadFile(f, &(v), sizeof(v), &a, NULL)
#define READP(p, n) ReadFile(f, p, n, &a, NULL)
#define WRITE(v) WriteFile(f2, &(v), sizeof(v), &a, NULL)
#define WRITEP(p, n) WriteFile(f2, p, n, &a, NULL)
#define SwapFourBytes(data)   \
( (((data) >> 24) & 0x000000FF) | (((data) >>  8) & 0x0000FF00) | \
  (((data) <<  8) & 0x00FF0000) | (((data) << 24) & 0xFF000000) ) 

//Structs
struct asura_hdr {
	DWORD magic;
	DWORD file_size;
	DWORD type1;
	DWORD type2;
};

struct hash_tree_node2
{
	DWORD hash = 0;
	void* ptr = nullptr;
	hash_tree_node2* bigger = nullptr;
	hash_tree_node2* smaller = nullptr;
};

//Globals
bool g_try_unscramble = 0;
hash_tree_node2* g_hashs;
char g_dump_hex_line[81] = { 0 };
char g_dump_hex_number[5] = { 0 };


void Dump_hex(void* data, unsigned int count)
{
	//dbgprint("Dump_hex_invoced", "data: %X count: %d\n", data, count);
	if (count == 0 || data == nullptr)
	{
		return;
	}
	if (count < 16)
	{
		memset(g_dump_hex_line, 0, 81);
		for (unsigned int i = 0; i < count; i++)
		{
			//dbgprint("test", "%X\n", ((unsigned char*)data)[i]);
			sprintf(g_dump_hex_number, "%02X ", ((unsigned char*)data)[i]);
			g_dump_hex_number[4] = '\0';
			strcat(g_dump_hex_line, g_dump_hex_number);
		}
		g_dump_hex_line[80] = '\0';
		printf("%s \n", g_dump_hex_line);
	}
	else
	{
		int lines = count / 16;
		for (int i = 0; i < lines; i++)
		{
			printf("%08x %08x %08x %08x \n", SwapFourBytes(((DWORD*)data)[i * 4]), \
				SwapFourBytes(((DWORD*)data)[i * 4 + 1]), SwapFourBytes(((DWORD*)data)[i * 4 + 2]), \
				SwapFourBytes(((DWORD*)data)[i * 4 + 3]));
		}
		if (count - lines * 16 > 0)
		{
			memset(g_dump_hex_line, 0, 81);
			for (unsigned int i = 0; i < count - lines * 16; i++)
			{
				sprintf(g_dump_hex_number, "%02x ", ((unsigned char*)data)[i + lines * 16]);
				g_dump_hex_number[4] = '\0';
				strcat(g_dump_hex_line, g_dump_hex_number);
			}
			g_dump_hex_line[80] = '\0';
			printf("%s \n", g_dump_hex_line);
		}
	}
}

//I just paste it from decompiler
DWORD hash_from_str(DWORD init, char* str)
{
	DWORD result; // eax@1
	char *v3; // esi@1
	char i; // cl@2
	int v5; // edx@7
	int v6; // eax@7

	result = init;
	v3 = str;
	if (str)
	{
		for (i = *str; i; result = v5 + v6)
		{
			if ((i - 'A') > 0x19u)
			{
				if (i == '\\')
					i = '/';
			}
			else
			{
				i += 32;
			}
			v5 = 31 * result;
			v6 = i;
			i = (v3++)[1];
		}
	}
	return result;
}

void add_hash2(hash_tree_node2* h_n, hash_tree_node2* new_node)
{
	if (!new_node->hash)
		return;
	if (!h_n->hash)
	{
		h_n->hash = new_node->hash;
		h_n->ptr = new_node->ptr;
		return;
	}
	if (h_n->hash == new_node->hash)
		return;
	if (h_n->hash > new_node->hash)
	{
		if (!h_n->smaller)
		{
			h_n->smaller = new_node;
			return;
		}
		else
		{
			add_hash2(h_n->smaller, new_node);
		}
	}
	else
	{
		if (!h_n->bigger)
		{
			h_n->bigger = new_node;
			return;
		}
		else
		{
			add_hash2(h_n->bigger, new_node);
		}
	}
}

hash_tree_node2* search_by_hash2(hash_tree_node2* h_n, DWORD hash)
{
	if (!hash)
		return 0;
	hash_tree_node2* hn = h_n;
	while (hn)
	{
		if (hn->hash >= hash)
		{
			if (hn->hash == hash)
				return hn;
			hn = hn->smaller;
		}
		else
		{
			hn = hn->bigger;
		}
	}
	return 0;
}

void* get_value_by_hash(hash_tree_node2* h_n, DWORD hash)
{
	char not_found[10] = "Not found";
	hash_tree_node2* hn_p = search_by_hash2(h_n, hash);
	if (hn_p)
		return hn_p->ptr;
	return not_found;
}

void delete_nodes2(hash_tree_node2* hn)
{
	if (hn)
	{
		if (hn->bigger)
			delete_nodes2(hn->bigger);
		if (hn->smaller)
			delete_nodes2(hn->smaller);
		delete hn;
	}
}

void Init_hashs(char* file_name)
{
	FILE* f = fopen(file_name, "rt");

	printf("Init hashs\n");

	if (f == INVALID_HANDLE_VALUE)
	{
		printf("Error opening names list !!!\n");
		return;
	}

	g_hashs = new hash_tree_node2;
	g_hashs->hash = 0;
	g_hashs->bigger = nullptr;
	g_hashs->smaller = nullptr;
	g_hashs->ptr = nullptr;

	char _name[512];

	while (!feof(f))
	{
		char* name = new char[512];
		ZeroMemory(name, 512);
		fgets(_name, 512, f);

		//Dump_hex(_name, 512);
		//exit(0);

		//sscanf(_name, "%s", name);


		strcpy(name, _name);

		name[strlen(name) - 1] = '\0';

		//Dump_hex(name, 512);
		//exit(0);

		if (strlen(name) == 0)
			break;

		//printf("str: %s\n", name);

		DWORD hash = hash_from_str(0, name);

		if (search_by_hash2(g_hashs, hash))
		{
			//printf("found repited str: %s\n", name);
			delete[] name;
			continue;
		}

		hash_tree_node2* hn = new hash_tree_node2;
		hn->hash = hash;
		hn->ptr = name;
		add_hash2(g_hashs, hn);
	}
	printf("Init hashs end\n");

	fclose(f);
	return;
}

void dbgprint(const char* fmt, ...)
{
	wchar_t* cwd = _wgetcwd(0, 0);
	wchar_t file_path[260];
	wsprintf(file_path, L"%ws\\debug.txt", cwd);
	FILE* file = _wfopen(file_path, L"a+");
	if (!file)
	{
		MessageBox(nullptr, L"Cannot open debug log !", L"Error", MB_OK);
		return;
	}
	//SYSTEMTIME st;
	//GetLocalTime(&st);
	//fprintf(file, "%02d.%02d.%d %02d:%02d (%s) | ", st.wDay, st.wMonth,
	//	st.wYear, st.wHour, st.wMinute, debug_note);

	va_list args;
	va_start(args, fmt);

	vfprintf(file, fmt, args);

	va_end(args);
	fclose(file);
}

int read_padded_str(HANDLE f, char* dest)
{
	bool check = 1;
	DWORD a = 0;
	int len = 4;
	int data;
	DWORD offset = SFPC(0);
	READ(data);
	for (; ((data >> 24) && ((data >> 16) & 0xFF) && ((data >> 8) & 0xFF) && (data & 0xFF));)
	{
		READ(data);
		len += 4;
	}
	SFPS(offset);
	if (!(READP(dest, len)))
	{
		printf("Read padded str error %d\n", GetLastError());
		return 0;
	}
	return 1;
}




void Compiler_hack(HANDLE f)
{
	DWORD a = 0;
	DWORD hash_of_substring; // after "." symbol
	DWORD hash_of_string;
	READ(hash_of_substring);
	READ(hash_of_string);
	if (!hash_of_string)
	{
		printf("(value: None) \n");
		return;
	}
	char* val_str = (char*)malloc(1024);
	read_padded_str(f, val_str);

	if (g_try_unscramble && (strlen(val_str) == 0))
	{
		hash_tree_node2* hn_p = search_by_hash2(g_hashs, hash_of_string);
		if (hn_p)
			printf("(value: \"%s\") \n", hn_p->ptr);
		else
			printf("(value: Unknown (%08x)) \n", hash_of_string);
	}
	else
	{
		printf("(value: \"%s\") \n", val_str);
	}
	dbgprint("%s\n", val_str);
	free(val_str);
	return;
}

void Compiler_hack2(HANDLE f)
{
	DWORD a = 0;
	DWORD size;
	READ(size);
	void* mem = malloc((size + 3) & ~3);
	READP(mem, (size + 3) & ~3);
	Dump_hex(mem, size);
	free(mem);
	return;
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

	if (g_try_unscramble && (strlen(str1) == 0))
	{
		hash_tree_node2* hn_p = search_by_hash2(g_hashs, property_name_hash);
		if (hn_p)
		{
			printf("\t\t %s %s unk2: %08x unk3: %08x type_or_count: %d ", \
				hn_p->ptr, str2, unk2, unk3, type2_or_count);
		}
		else
		{
			printf("\t\t unknown %s unk2: %08x unk3: %08x type_or_count: %d ", \
				str2, unk2, unk3, type2_or_count);
		}
	}
	else
	{
		printf("\t\t %s %s unk2: %08x unk3: %08x type_or_count: %d ", \
			str1, str2, unk2, unk3, type2_or_count);
	}

	dbgprint("%s\n", str1);

	free(str1);
	free(str2);

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
			Compiler_hack(f);
			break;
		case 4:
			Compiler_hack2(f);
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
			Compiler_hack(f);
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
				Compiler_hack(f);
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

void Read_BLUE(HANDLE f)
{
	DWORD a = 0;
	SFPS(0);
	asura_hdr file_hdr;
	READ(file_hdr);

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

	if (BLUE_type == 2)
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
				hash_tree_node2* hn_p = search_by_hash2(g_hashs, Class_name_hash);
				if (hn_p)
					printf("%s class (%d subclasses)\n", hn_p->ptr, count_of_subclasses);
				else
					printf("Unknown class (%d subclasses)\n", count_of_subclasses);
			}
			else
			{
				printf("%s class (%d subclasses)\n", Class_name, count_of_subclasses);
			}

			for (DWORD j = 0; j < count_of_subclasses; j++)
			{
				DWORD subclass_name_hash = 0;
				DWORD type_of_subclass = 0;
				DWORD unk_hash = 0;
				char* subclass_name = (char*)malloc(1024);
				DWORD properies_count = 0;
				READ(subclass_name_hash);
				READ(type_of_subclass);
				SFPC(4);
				READ(unk_hash);
				read_padded_str(f, subclass_name);
				READ(properies_count);

				if (g_try_unscramble && (strlen(subclass_name) == 0))
				{
					hash_tree_node2* hn_p = search_by_hash2(g_hashs, subclass_name_hash);
					if (hn_p)
					{
						hash_tree_node2* hn_p2 = search_by_hash2(g_hashs, unk_hash);
						if (hn_p2)
						{
							printf("\t- %s (parent: %s type: %d properties_count: %d) \n", \
								hn_p->ptr, hn_p2->ptr, type_of_subclass, properies_count);
						}
						else
						{
							printf("\t- %s (parent: unknown (%08x) type: %d  properties_count: %d) \n", \
								hn_p->ptr, unk_hash, type_of_subclass, properies_count);
						}

					}
					else
					{
						hash_tree_node2* hn_p2 = search_by_hash2(g_hashs, unk_hash);
						if (hn_p2)
						{
							printf("\t- unknown (parent: %s type: %d properties_count: %d) \n", \
								hn_p2->ptr, type_of_subclass, properies_count);
						}
						else
						{
							printf("\t- unknown (parent: unknown (%08x) type: %d properties_count: %d) \n", \
								unk_hash, type_of_subclass, properies_count);
						}

					}
				}
				else
				{
					printf("\t- %s (type: %d parent: (hash: %08x) properties_count: %d) \n", \
											subclass_name, type_of_subclass, unk_hash, properies_count);
					//printf("%s\n", subclass_name);
				}


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
		printf("Unknown BLUEprint type!, i work with Aliens vs Predator (2010) BLUEprints !\n");
	}




}

void Usage_func()
{
	printf("Usage: program [file_name] [optional]\n");
	printf("BLUE parse test\n");
	printf("optional: \"unscramble\" - Try unscramble Hashs \n");
	return;
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		Usage_func();
		return 0;
	}

	printf("Main \n");

	if (argv[2])
	{
		if (!strcmp(argv[2], "unscramble"))
		{
			g_try_unscramble = true;
			printf("Call make Init hashs\n");
			Init_hashs((char*)"names.txt");
			//printf("%s \n", search_by_hash2(g_hashs, 0x3BF145E)->ptr);
		}

		if (!strcmp(argv[2], "test_hash"))
		{
			Init_hashs((char*)"names.txt");
			printf("%s \n", get_value_by_hash(g_hashs, 1801878019));
			printf("%s \n", get_value_by_hash(g_hashs, 3428676664));
			printf("%s \n", get_value_by_hash(g_hashs, 1670523056));
			printf("%s \n", get_value_by_hash(g_hashs, 562750696));
			printf("%s \n", get_value_by_hash(g_hashs, 315656744));
			printf("%s \n", get_value_by_hash(g_hashs, 4211224973));
			printf("%s \n", get_value_by_hash(g_hashs, 340768193));
			printf("%s \n", get_value_by_hash(g_hashs, 1150004599));
		}
	}

	HANDLE f = CreateFileA(argv[1], GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (GetLastError() != ERROR_SUCCESS)
	{
		printf("File open error: %d\n", GetLastError());
		CloseHandle(f);
		return EXIT_FAILURE;
	}
	printf("call Read BLUE\n");
	Read_BLUE(f);

	//system("PAUSE");
	return 0;
}

