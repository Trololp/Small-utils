#include "stdafx.h"
#include "Common.h"
#include <map>

#pragma warning( disable: 4996 )

std::map<DWORD, char*> g_hashs;

#define SwapFourBytes(data)   \
( (((data) >> 24) & 0x000000FF) | (((data) >>  8) & 0x0000FF00) | \
  (((data) <<  8) & 0x00FF0000) | (((data) << 24) & 0xFF000000) ) 

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

char* read_padded_str_f(HANDLE f) {

	DWORD a = 0;
	DWORD val;
	READ(val);
	if (val == 0x00FFFF00) {
		return nullptr;
	}
	else {
		SFPC(-4);
	}

	char* dest = (char*)malloc(512);
	if (read_padded_str(f, dest)) {
		return dest;
	}

	free(dest);
	return nullptr;
}

void write_padded_str_f(char* str, HANDLE f2)
{
	HANDLE f = f2;
	DWORD a = 0;
	//UINT file_addr = SFPC(0);

	if (!str) {
		WRITEP("\0\xFF\xFF\0", 4);
		return;
	}

	size_t str_len = strlen(str);

	WRITEP(str, str_len);
	WRITEP("\0\0\0\0", 4 - str_len % 4);
}

void skip_padded_str_f(HANDLE f) {
	DWORD a = 0;
	DWORD data;
	READ(data);
	for (; ((data >> 24) && ((data >> 16) & 0xFF) && ((data >> 8) & 0xFF) && (data & 0xFF));)
	{
		READ(data);
	}
}

DWORD hash_f(char* name) {
	DWORD init = 0;

	DWORD result; // eax@1
	char *v3; // esi@1
	char i; // cl@2
	int v5; // edx@7
	int v6; // eax@7

	result = init;
	v3 = name;
	if (name)
	{
		for (i = *name; i; result = v5 + v6)
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



void add_hash(DWORD hash, char* str) {
	g_hashs[hash] = str;
}

char* search_hash(DWORD hash) {
	return g_hashs[hash];
}

void Init_hashs(char* file_name)
{
	FILE* f = fopen(file_name, "rt");

	printf("Init hashs\n");

	if (!f || f == INVALID_HANDLE_VALUE)
	{
		printf("Error opening names list !!!\n");
		return;
	}

	char _name[512];

	while (!feof(f))
	{
		char* name = new char[512];
		ZeroMemory(name, 512);
		fgets(_name, 512, f);

		strcpy(name, _name);

		name[strlen(name) - 1] = '\0';

		if (strlen(name) == 0)
			break;

		//printf("str: %s\n", name);

		DWORD hash = hash_f(name);

		if (search_hash(hash))
		{
			//printf("found repited str: %s\n", name);
			delete[] name;
			continue;
		}

		add_hash(hash, name);
	}
	printf("Init hashs end\n");

	fclose(f);
	return;
}

char * str_copy(char * str_from)
{
	if (str_from && *str_from) {

		size_t len = strlen(str_from);
		char* result = (char*)malloc(len + 1);
		strcpy(result, str_from);
		return result;
	}
	return nullptr;
}

char * str_to_lower(char * str)
{
	char* str_p = str;

	while (*str) {
		if (('A' <= *str) && (*str <= 'Z')) {
			*str = *str + ('a' - 'A');
		}
		str++;
	}

	return str_p;

}
