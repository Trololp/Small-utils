// MARE_chunk_reparse.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <Windows.h>

#define SFPS(pos) SetFilePointer(f, pos, 0, FILE_BEGIN)
#define SFPC(pos) SetFilePointer(f, pos, 0, FILE_CURRENT)
#define READ(v) ReadFile(f, &(v), sizeof(v), &a, NULL)
#define READP(p, n) ReadFile(f, p, n, &a, NULL)
#define WRITE(v) WriteFile(f2, &(v), sizeof(v), &a, NULL)
#define WRITEP(p, n) WriteFile(f2, p, n, &a, NULL)
#define SFPS2(pos) SetFilePointer(f2, pos, 0, FILE_BEGIN)

#pragma warning(disable : 4996)


struct Chunk_header
{
	DWORD chunk_name;
	DWORD chunk_size;
	DWORD type1;
	DWORD type2;
};

struct MARE_ENTRY
{
	DWORD mat_hash;
	unsigned long int num;
	DWORD Hash1;
	DWORD Hash2;
	DWORD Hash3;
	DWORD Hash4;
	DWORD Hash5;
	float a;
	float b[4];
	DWORD c[3];
	DWORD Hash6;
	DWORD d;
	DWORD e[4];
	DWORD f[2];
	DWORD h[3][4];
	DWORD g;
	DWORD i[6];
	DWORD j[3];
	DWORD l[6];
	DWORD Default_mat_id;
	DWORD Hash7;
	DWORD m;
	DWORD Hash8;
	DWORD n;
	DWORD Hash9;
	DWORD Hash10;
	DWORD k[4];
	DWORD pad[16];
};

// For RSCF textures save files
const int g_amount_resources_to_save = 6;
const char* g_save_resources[g_amount_resources_to_save] =
{
	"\\characters\\praetorian\\praetorian_body_colour",
	"\\characters\\praetorian\\praetorian_body_normal",
	"\\characters\\praetorian\\praetorian_chunk_bits_colour",
	"\\characters\\praetorian\\praetorian_chunk_bits_normal",
	"\\characters\\praetorian\\praetorian_head_colour",
	"\\characters\\praetorian\\praetorian_head_normal"
};

DWORD g_hashes[g_amount_resources_to_save] = { 0 };

DWORD g_Nulls[256] = { 0 };

bool save_resource_by_hash(DWORD hash)
{
	for (int i = 0; i < g_amount_resources_to_save; i++)
	{
		if (hash == g_hashes[i]) // Save files with given name
			return true;
	}
	return false;
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

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf(" Usage: \n program [MARE_file]\n");
		return 0;
	}

	//Init hashes
	for (int i = 0; i < g_amount_resources_to_save; i++)
	{
		g_hashes[i] = hash_from_str(0, (char*)g_save_resources[i]);
	}


	DWORD a = 0;
	HANDLE f = CreateFileA(argv[1], GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (GetLastError() != ERROR_SUCCESS)
	{
		printf("Opening file Error: %d\n", GetLastError());
		CloseHandle(f);
		return 0;
	}

	DWORD File_size = GetFileSize(f, NULL);

	char new_path[260] = { 0 };
	strcpy(new_path, argv[1]);
	char* extension_str = strchr(new_path, '.');
	strcpy(extension_str, "_stripped.MARE");
	printf("%s\n", new_path);

	HANDLE f2 = CreateFileA(new_path, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if ((GetLastError() != ERROR_SUCCESS) && (GetLastError() != ERROR_ALREADY_EXISTS))
	{
		printf("Create file Error: %d\n", GetLastError());
		CloseHandle(f);
		return 0;
	}

	// Chunk read here
	Chunk_header hdr;
	READ(hdr);

	if (hdr.chunk_name == 'ERAM')
	{
		WRITE(hdr);
		SFPS2(0x14);
		MARE_ENTRY* mem = (MARE_ENTRY*)malloc(hdr.chunk_size - sizeof(hdr) - 4); // Potentialy unoptimized way to do that
		int count = 0;

		READ(count);
		READP(mem, hdr.chunk_size - sizeof(hdr) - 4);

		printf("MARE %d count \n", count);

		//Reparse MARE

		MARE_ENTRY* save_mem = (MARE_ENTRY*)malloc(hdr.chunk_size - sizeof(hdr) - 4);
		ZeroMemory(save_mem, hdr.chunk_size - sizeof(hdr) - 4);

		int saved_materials = 0;

		for (int i = 0; i < count; i++)
		{
			DWORD hash1 = mem[i].Hash1;
			DWORD hash2 = mem[i].Hash2;
			DWORD hash3 = mem[i].Hash3;
			DWORD hash4 = mem[i].Hash4;
			DWORD hash5 = mem[i].Hash5;

			if (save_resource_by_hash(hash1) ||
				save_resource_by_hash(hash2) ||
				save_resource_by_hash(hash3) ||
				save_resource_by_hash(hash4) ||
				save_resource_by_hash(hash5))
			{
				WRITEP(&(mem[i]), sizeof(MARE_ENTRY));
				saved_materials++;
			}

		}

		printf("%d materials saved \n", saved_materials);

		DWORD calculated_new_size = 0x14 + sizeof(MARE_ENTRY)*saved_materials;

		SFPS2(0x4);
		WRITE(calculated_new_size);

		SFPS2(0x10);
		WRITE(saved_materials);
		
		free(mem);
		free(save_mem);

		CloseHandle(f);
		CloseHandle(f2);
	}
	CloseHandle(f);
	CloseHandle(f2);

    return 0;
}

