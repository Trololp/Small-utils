// add_header.cpp
// ADD Asura RSCF header to files (made to conver .dds & .tga into chunk format)


#include "stdafx.h"
#include <Windows.h>

#define SFPS(pos) SetFilePointer(f, pos, 0, FILE_BEGIN)
#define SFPC(pos) SetFilePointer(f, pos, 0, FILE_CURRENT)
#define READ(v) ReadFile(f, &(v), sizeof(v), &a, NULL)
#define READP(p, n) ReadFile(f, p, n, &a, NULL)
#define WRITE(v) WriteFile(f2, &(v), sizeof(v), &a, NULL)
#define WRITEP(p, n) WriteFile(f2, p, n, &a, NULL)

#pragma warning(disable : 4996)

struct RSCF_IMG_HDR
{
	DWORD magic;
	DWORD size;
	DWORD type1;
	DWORD type2;
	DWORD RSCF_type1;
	DWORD RSCF_type2;
	DWORD size_wo_hdr;
};

DWORD g_Nulls[256] = { 0 };

void load_texture(char* path)
{
	printf("Processing %s \n", path);
	DWORD a = 0;
	HANDLE f = CreateFileA(path, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (GetLastError() != ERROR_SUCCESS)
	{
		printf("Opening file Error: %d\n", GetLastError());
		CloseHandle(f);
		return;
	}

	// READ ENTIRE FILE !!!!!!

	DWORD size = GetFileSize(f, NULL);
	DWORD real_str_len = strlen(path);
	DWORD str_size = (strlen(path) & ~3) + 4;

	RSCF_IMG_HDR hdr;
	hdr.magic = (DWORD)'FCSR';
	hdr.size_wo_hdr = size;
	hdr.size = size + str_size + 28;
	hdr.type1 = 0;
	hdr.type2 = 0;
	hdr.RSCF_type1 = 2;
	hdr.RSCF_type2 = 0;
	
	void* mem = malloc(size);
	READP(mem, size);

	CloseHandle(f);

	char new_path[260] = { 0 };
	strcpy(new_path, path);
	char* extension_str = strchr(new_path, '.');
	strcpy(extension_str, ".RSCF");
	printf("%s\n", new_path);

	HANDLE f2 = CreateFileA(new_path, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (GetLastError() != ERROR_SUCCESS)
	{
		printf("Create file Error: %d\n", GetLastError());
		if(mem) free(mem);
		CloseHandle(f2);
		return;
	}

	WRITE(hdr);
	WRITEP(path, real_str_len);
	char null_char = 0;
	for (int i = 0; i < str_size - real_str_len; i++)
	{
		WRITE(null_char);
	}
	WRITEP(mem, size);

	free(mem);
	CloseHandle(f);
}

int Read_textures(const char* start_folder)
{
	char search_path[260] = { 0 };
	strcpy(search_path, start_folder);
	strcat(search_path, "*.*");
	WIN32_FIND_DATAA FindFileData;
	HANDLE hFind = FindFirstFileA(search_path, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		printf("FindFirstFile failed (%d)\n", GetLastError());
		return 0;
	}

	do {
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (!strcmp(FindFileData.cFileName, ".") || !strcmp(FindFileData.cFileName, ".."))
				continue;
			char next_path[260] = { 0 };
			strcpy(next_path, start_folder);
			strcat(next_path, FindFileData.cFileName);
			strcat(next_path, "\\");
			Read_textures(next_path);
		}
		else
		{
			char path[260] = { 0 };
			strcpy(path, start_folder);
			strcat(path, FindFileData.cFileName);
			load_texture(path);
		}
	} while (FindNextFileA(hFind, &FindFileData) != 0);

	FindClose(hFind);
	return 1;
}



int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("Usage: \n program folder_path");
		return 0;
	}
	Read_textures(argv[1]);



	//system("PAUSE");
	return 0;
}