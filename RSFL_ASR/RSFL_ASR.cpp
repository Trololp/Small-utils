// PACKER unpaker
//

#include "stdafx.h"
//#include "miniz.h" // For unpacking AsuraZlb Archives
#pragma warning( disable: 4996 )

//Defines
#define ASRUNPACK_VER_STRING "ASRUNPACK v0.5 made by Trololp \n"

//Macro
#define SFPS(pos) SetFilePointer(f, pos, 0, FILE_BEGIN)
#define SFPC(pos) SetFilePointer(f, pos, 0, FILE_CURRENT)
#define READ(v) ReadFile(f, &(v), sizeof(v), &a, NULL)
#define READP(p, n) ReadFile(f, p, n, &a, NULL)
#define WRITE(v) WriteFile(f2, &(v), sizeof(v), &a, NULL)
#define WRITEP(p, n) WriteFile(f2, p, n, &a, NULL)
#define BYTE0(dw) (dw & 0xFF)
#define BYTE1(dw) ((dw & 0xFF00) >> 8)
#define BYTE2(dw) ((dw & 0xFF0000) >> 16)
#define BYTE3(dw) ((dw & 0xFF000000) >> 24)

// =============================================== //
// 12/17/2023
// Vhetration's fixes for  trolop unpacker/packer
// i decided to help with some bug i found.

#include <stdlib.h>
#include <Windows.h>
#include <string>
#include <tchar.h>

#pragma once

 // Removes first character from char array.
 // EX: /sounds/xyz --> sounds/xyz

const char* RemoveFirstChar(const char* FLpath)
{

	const char* RETURNEDPATH = FLpath + 1;
	return RETURNEDPATH;
}

//Structs
struct RSFL_entry
{
	const char* file_name;
	DWORD offset;
	DWORD file_size;
	DWORD unk;
};

struct RSCF_hdr
{
	DWORD magic;
	DWORD size;
	DWORD type1;
	DWORD type2;
	DWORD RSCF_type1;
	DWORD RSCF_type2;
	DWORD size_wo_hdr;
};

struct chunk_hdr
{
	DWORD magic;
	DWORD Size;
	DWORD type1;
	DWORD type2;
};

struct chunk_info
{
	DWORD magic;
	bool has_name;
	int offset_of_name;
	bool check_type;
	DWORD type1;
	DWORD type2;
};

struct RSCF_type_ext
{
	DWORD type1;
	DWORD type2;
	const char ext_name[32];
};

//Globals

const int g_amount_check_chunks = 8;

chunk_info g_chunk_info[g_amount_check_chunks] = {
	{ 'STUC', TRUE, 0x24, 1, 22, 0 },
	{ 'VELD', TRUE, 0x10, 5, 0 },
	{ 'NKSH', TRUE, 0x18, 0},
	{ 'MNAH', TRUE, 0x18, 0},
	{ 'TPMH', TRUE, 0x14, 0},
	{ 'BBSH', TRUE, 0x10, 0},
	{ 'LKSH', FALSE, 0x1C,0},
	{ 'DNSH', FALSE, 0x14,1, 4, 1 },
};


// This is information from AVP2010

const int g_amount_check_RSCF_types = 12;

RSCF_type_ext RSCF_types[g_amount_check_RSCF_types] = {
	{0, 4, "\0" },
	{ 0, 6, "\0" },
	{ 0, 7, ".carpet"},
	{ 0, 8, ".botanicals" },
	{ 0, 11, ".enviroment" },
	{ 0, 12, ".mat_response" },
	{ 0, 13, ".decals" },
	{ 0, 14, ".shaders" },
	{ 0, 15, ".model"},
	{ 2, 0, "\0" }, // Image
	{ 3, 0, "\0" }, // sound
	{ 6, 0, "\0" }, // idk
};

RSFL_entry* g_RSFL_entries = nullptr;
DWORD g_RSFL_size = 0;
DWORD g_RSFL_count = 0;
DWORD g_core_segment_addr = 0;
char g_archive_file_name[260] = { 0 };
char g_path[260] = { 0 };


//For packing
HANDLE g_handle_for_pack;
char g_folder_to_pack_path[260] = { 0 };

int read_padded_str(HANDLE file, char* dest)
{
	bool check = 1;
	DWORD bytes_readen = 0;
	int len = 4;
	int data;
	DWORD offset = SetFilePointer(file, NULL, NULL, FILE_CURRENT);
	ReadFile(file, &data, 4, &bytes_readen, NULL);
	for (; ((data >> 24) && ((data >> 16) & 0xFF) && ((data >> 8) & 0xFF) && (data & 0xFF));)
	{
		ReadFile(file, &data, 4, &bytes_readen, NULL);
		len += 4;
	}
	SetFilePointer(file, offset, NULL, FILE_BEGIN);
	if (!ReadFile(file, dest, len, &bytes_readen, NULL))
	{
		printf("Read padded str error %d\n", GetLastError());
		return 0;
	}
	return 1;
}

bool TryCreateDirectory(char *path)
{
	char* path_ = path;
	char* pos;
	char folder_name[260];

	if (path[1] == ':') // this is "C:\\..." like path
	{
		path_ = path + 4;
	}

	while (pos = strchr(path_, '\\'))
	{
		memcpy(folder_name, path, pos - path);
		folder_name[pos - path] = '\0';
		path_ += pos - path_ + 1;


		if (!(CreateDirectoryA(folder_name, NULL) || GetLastError() == ERROR_ALREADY_EXISTS))
		{
			printf("Error create subdirectory  %s \n", folder_name);
			printf("Error: %d \n", GetLastError());
			return false;
		}
	}
	return true;
}

int get_resource_name(chunk_hdr* hdr)
{
	for (int i = 0; i < g_amount_check_chunks; i++)
	{
		if (hdr->magic == g_chunk_info[i].magic)
		{
			if (g_chunk_info[i].check_type)
			{
				if ((hdr->type1 == g_chunk_info[i].type1) && (hdr->type2 == g_chunk_info[i].type2))
					return g_chunk_info[i].offset_of_name;
			}
			else
			{
				return g_chunk_info[i].offset_of_name;
			}
		}
	}
	return 0;
}


const char* get_RSCF_ext(DWORD type1, DWORD type2)
{
	for (int i = 0; i < g_amount_check_RSCF_types; i++)
	{
		if ((RSCF_types[i].type1 == type1) && (RSCF_types[i].type2 == type2))
		{
			return (RSCF_types[i].ext_name);
		}
	}
	return "\0";
}


bool Unpack_Asura_arch(HANDLE f)
{
	DWORD a = 0;
	SFPS(0);
	char Arch_magic[9] = "";
	Arch_magic[8] = '\0';

	char unpacked_folder[260] = { 0 };
	strcpy(unpacked_folder, g_archive_file_name);
	strcat(unpacked_folder, "_unpacked\\");
	TryCreateDirectory(unpacked_folder);

	char unpacked_file[260] = { 0 };

	READP(Arch_magic, 8);

	printf("Unpacking... \n");

	UINT32 unnamed_chunk_count = 0;
	char named_filled_name[260] = { 0 };

	if (!strcmp(Arch_magic, "Asura   "))
	{


		chunk_hdr hdr;
		READ(hdr);

		while (hdr.magic != NULL)
		{
			ZeroMemory(unpacked_file, 260);
			DWORD file_pos_in_arch = SFPC(0) - sizeof(hdr);


			if (hdr.magic == 'FCSR') // resource file need handle specially
			{
				DWORD RSCF_data_size;
				DWORD RSCF_type1;
				DWORD RSCF_type2;
				READ(RSCF_type1);// RSCF_type1
				READ(RSCF_type2);// RSCF_type2
				READ(RSCF_data_size);// RSFC_size_wo_hdr
				read_padded_str(f, named_filled_name); // Resource name
				sprintf(unpacked_file, "%s%s%s", unpacked_folder, named_filled_name, get_RSCF_ext(RSCF_type1, RSCF_type2));

				//temp_fdn[1] = '\0';
				char temp_fdn1[260] = { 0 };
				strcpy(temp_fdn1, unpacked_file);
				char* temp_fdn = strrchr(temp_fdn1, '\\');

				if (temp_fdn) // contains path in name
				{
					temp_fdn[1] = '\0';
					TryCreateDirectory(temp_fdn1);
				}

				HANDLE f2 = CreateFileA(unpacked_file, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

				printf("%s \n", unpacked_file);

				if ((GetLastError() != ERROR_SUCCESS) && (GetLastError() != ERROR_ALREADY_EXISTS))
				{
					printf("File create error: %d (%s)\n", GetLastError(), unpacked_file);
					CloseHandle(f2);
					return 1;
				}

				if (RSCF_type1 == 0)
				{
					RSCF_data_size = hdr.Size;
					SFPS(file_pos_in_arch);
				}

				void* RSCF_data = malloc(RSCF_data_size);
				READP(RSCF_data, RSCF_data_size);

				WRITEP(RSCF_data, RSCF_data_size);

				free(RSCF_data);
				CloseHandle(f2);

				READ(hdr);
				continue;
			}

			char chunk_ext[6] = { 0 };
			sprintf(chunk_ext, ".%c%c%c%c", BYTE0(hdr.magic), BYTE1(hdr.magic), BYTE2(hdr.magic), BYTE3(hdr.magic));

			DWORD name_offset = get_resource_name(&hdr);

			char chunk_dir[260] = { 0 };
			strcpy(chunk_dir, unpacked_folder);
			strcat(chunk_dir, chunk_ext + 1);
			strcat(chunk_dir, "_chunk\\");

			TryCreateDirectory(chunk_dir);

			if (name_offset)
			{
				SFPC(name_offset - sizeof(hdr));
				read_padded_str(f, named_filled_name);
				strcat(named_filled_name, chunk_ext);
				sprintf(unpacked_file, "%s%s", chunk_dir, named_filled_name);
			}
			else
			{
				sprintf(named_filled_name, "%d", unnamed_chunk_count);
				strcat(named_filled_name, chunk_ext);
				sprintf(unpacked_file, "%s%s", chunk_dir, named_filled_name);
				unnamed_chunk_count++;
			}

			HANDLE f2 = CreateFileA(unpacked_file, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			printf("%s \n", unpacked_file);

			if ((GetLastError() != ERROR_SUCCESS) && (GetLastError() != ERROR_ALREADY_EXISTS))
			{
				printf("File create error: %d (%s)\n", GetLastError(), unpacked_file);
				CloseHandle(f2);
				return 1;
			}

			void* file_data = malloc(hdr.Size);
			SFPS(file_pos_in_arch);
			READP(file_data, hdr.Size);

			WRITEP(file_data, hdr.Size);

			CloseHandle(f2);
			free(file_data);

			READ(hdr); // Next file
		}

		printf("Done !!! \n");

	}
	else
	{
		printf("Compressed, try \"decompress\" option first !\n");
	}
	return 0;
}

bool Read_RSFL(HANDLE f)
{
	printf("reading RSFL\n");
	DWORD a = 0;
	struct header_asura
	{
		DWORD magic;
		DWORD size_file;
		DWORD type;
		DWORD type2;
		DWORD count;
	};

	header_asura hdr;
	READ(hdr);

	if (hdr.magic != 'LFSR')
	{
		printf("bad RSFL header \n");
		return 0;
	}

	g_RSFL_size = hdr.size_file;
	printf("RSFL_size: %x \n", g_RSFL_size);
	g_RSFL_count = hdr.count;
	g_RSFL_entries = (RSFL_entry*)malloc(sizeof(RSFL_entry)*hdr.count);
	DWORD offset;
	DWORD size_file;
	DWORD unk;

	for (int i = 0; i < hdr.count; i++)
	{
		char* str = (char*)malloc(260);
		read_padded_str(f, str);
		READ(offset);
		READ(size_file);
		READ(unk);

		g_RSFL_entries[i].file_name = str;
		g_RSFL_entries[i].offset = offset;
		g_RSFL_entries[i].file_size = size_file;
		g_RSFL_entries[i].unk = unk;

		//printf("%08X | %08d | %d | %s \n", offset, size_file, unk, str);
	}
	return 1;
}



void Unpack_Asura_arch_with_RSFL(HANDLE f)
{
	printf("Unpacking Asura arch with RSFL info \n");
	DWORD a = 0;


	//This part will dump first segment
	char core_file_path[260];
	strcpy(core_file_path, g_path);
	strcat(core_file_path, "\\");
	strcat(core_file_path, g_archive_file_name);
	strcat(core_file_path, "_core.asr");

	DWORD first_offset = g_RSFL_entries[0].offset + g_RSFL_size;
	DWORD core_size = first_offset - g_core_segment_addr;
	printf("first offset: 0x%x ; Core_addr: 0x%x \n", first_offset, g_core_segment_addr);
	SFPS(g_core_segment_addr);

	void* temp_mem2 = malloc(core_size);
	READP(temp_mem2, core_size);

	HANDLE f3 = CreateFileA(core_file_path, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if ((GetLastError() != ERROR_SUCCESS) && (GetLastError() != ERROR_ALREADY_EXISTS))
	{
		printf("File create error: %d (%s)\n", GetLastError(), core_file_path);
		free(temp_mem2);
		CloseHandle(f3);
		return;
	}

	WriteFile(f3, "Asura   ", 8, &a, NULL);
	WriteFile(f3, temp_mem2, core_size, &a, NULL);
	WriteFile(f3, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16, &a, NULL);

	free(temp_mem2);
	CloseHandle(f3);

	//This part will dump named segments of archive

	char dump_path[260];
	strcpy(dump_path, g_path);
	strcat(dump_path, "\\");
	strcat(dump_path, g_archive_file_name);
	strcat(dump_path, "_decomposed\\");



	for (int i = 0; i < g_RSFL_count; i++)
	{
		printf("Unpacking %s \n", g_RSFL_entries[i].file_name);
		printf("File pos: %x, Entry_offset: %x \n", g_RSFL_entries[i].offset + g_RSFL_size, g_RSFL_entries[i].offset);
		DWORD file_offset = g_RSFL_entries[i].offset + g_RSFL_size;
		SFPS(file_offset);


		DWORD file_size = g_RSFL_entries[i].file_size;

		void* temp_mem = malloc(file_size);
		READP(temp_mem, file_size);

		char file_dump_name[260] = { 0 };
		strcpy(file_dump_name, dump_path);
		strcat(file_dump_name, g_RSFL_entries[i].file_name);

		char folder_dump_name[260] = { 0 };
		strcpy(folder_dump_name, g_archive_file_name);
		strcat(folder_dump_name, "_decomposed\\");
		strcat(folder_dump_name, g_RSFL_entries[i].file_name);
		char* temp_fdn = strrchr(folder_dump_name, '\\');
		temp_fdn[1] = '\0';

		if (TryCreateDirectory(folder_dump_name))
		{
			HANDLE f2 = CreateFileA(file_dump_name, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			if ((GetLastError() != ERROR_SUCCESS) && (GetLastError() != ERROR_ALREADY_EXISTS))
			{
				printf("File create error: %d (%s)\n", GetLastError(), file_dump_name);
				free(temp_mem);
				CloseHandle(f2);
				continue;
			}

			WRITEP("Asura   ", 8);
			WRITEP(temp_mem, file_size);
			WRITEP("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16);

			CloseHandle(f2);
		}
		else
		{
			printf("Directory create error: %d (%s)\n", GetLastError(), folder_dump_name);
		}

		free(temp_mem);

	}

}

DWORD find_RSFL_in_arch(HANDLE f, int attemps_in_chunks)
{
	DWORD a = 0;
	SFPS(0);
	char Arch_magic[9] = "";
	Arch_magic[8] = '\0';

	READP(Arch_magic, 8);

	printf("Finding RSFL \n");

	if (!strcmp(Arch_magic, "Asura   "))
	{
		struct chunk_hdr
		{
			DWORD Name;
			DWORD Size;
		};

		chunk_hdr hdr;

		for (int i = 0; i < attemps_in_chunks; i++)
		{
			READ(hdr);
			if (hdr.Name == (DWORD)'LFSR')
				return SFPC(0) - sizeof(hdr);
			SFPC(hdr.Size - sizeof(hdr));
		}
	}
	else
	{
		printf("Tf you put compressed arch here !\n");
	}
	return 0;
}

int decompose_func(char* f_name)
{
	GetCurrentDirectoryA(260, (LPSTR)g_path);
	strcpy(g_archive_file_name, f_name);

	HANDLE f = CreateFileA(f_name, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (GetLastError() != ERROR_SUCCESS)
	{
		printf("File open error: %d\n", GetLastError());
		CloseHandle(f);
		return EXIT_FAILURE;
	}


	DWORD RSFL_addr = find_RSFL_in_arch(f, 5);
	if (RSFL_addr == 0)
	{
		printf("Not found RSFL\n");
		return EXIT_FAILURE;
	}
	SFPS(RSFL_addr);

	if (Read_RSFL(f))
	{
		//Unpacking
		g_core_segment_addr = RSFL_addr + g_RSFL_size;
		Unpack_Asura_arch_with_RSFL(f);
	}
}

void Read_and_write_to_archive(const char* file_name)
{
	char file_path[MAX_PATH] = { 0 };
	//sprintf(file_path, "%s\\", g_folder_to_pack_path);
	strcat(file_path, file_name);

	DWORD a = 0;
	HANDLE f2 = g_handle_for_pack;

	//printf("file_name: %s\n", file_path);
	printf("\n\n");
	printf("[ Detected File ] : ");
	printf(file_path);
	printf("\n");

	HANDLE f = CreateFileA(file_path, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (GetLastError() != ERROR_SUCCESS)
	{
		printf("File open error: %d\n", GetLastError());
		CloseHandle(f);
		return;
	}

	DWORD sign = 0;
	READ(sign);

	if (sign != 'rusA')
	{
		if (sign == ' SDD') // graphics
		{
			const char* tmp_01 = file_name + strlen(g_folder_to_pack_path); // to fix a path for files !!!!

			const char* tmp = RemoveFirstChar(tmp_01);
			printf("[ DDS Written ]   : ");
			printf(tmp);



			DWORD size = GetFileSize(f, NULL);
			DWORD real_str_len = strlen(tmp);
			DWORD str_size = (strlen(tmp) & ~3) + 4;

			RSCF_hdr hdr;
			hdr.magic = (DWORD)'FCSR';
			hdr.size_wo_hdr = size;
			hdr.size = size + str_size + 28;
			hdr.type1 = 0;
			hdr.type2 = 0;
			hdr.RSCF_type1 = 2;
			hdr.RSCF_type2 = 0;

			void* mem = malloc(size);
			SFPS(0);
			READP(mem, size);

			WRITE(hdr);
			WRITEP(tmp, real_str_len);
			char null_char = 0;
			for (int i = 0; i < str_size - real_str_len; i++)
			{
				WRITE(null_char);
			}
			WRITEP(mem, size);

			free(mem);
			CloseHandle(f);
			return;
		}
		if (sign == 'FFIR') // sounds
		{
			const char* tmp_01 = file_name + strlen(g_folder_to_pack_path);

			const char* tmp = RemoveFirstChar(tmp_01);
			printf("[ WAV Written ]   : ");
			printf(tmp);


			DWORD size = GetFileSize(f, NULL);
			DWORD real_str_len = strlen(tmp);
			DWORD str_size = (strlen(tmp) & ~3) + 4;

			RSCF_hdr hdr;
			hdr.magic = (DWORD)'FCSR';
			hdr.size_wo_hdr = size;
			hdr.size = size + str_size + 28;
			hdr.type1 = 0;
			hdr.type2 = 0;
			hdr.RSCF_type1 = 3;
			hdr.RSCF_type2 = 0;

			void* mem = malloc(size);
			SFPS(0);
			READP(mem, size);

			WRITE(hdr);
			WRITEP(tmp, real_str_len);
			char null_char = 0;
			for (int i = 0; i < str_size - real_str_len; i++)
			{
				WRITE(null_char);
			}
			WRITEP(mem, size);

			free(mem);
			CloseHandle(f);
			return;
		}

		DWORD chunk_size = 0;
		READ(chunk_size);
		void* mem = malloc(chunk_size);
		SFPS(0);

		READP(mem, chunk_size);
		WRITEP(mem, chunk_size);

		printf("[ Chunk Written ]");

		free(mem);
		CloseHandle(f);
		return;
	}
	else
	{
		DWORD sign2 = 0;
		READ(sign2);
		if (sign2 == '   a')
		{
			// code
			DWORD file_size = GetFileSize(f, NULL);
			void* mem = malloc(file_size - 8 - 16);
			SFPS(8);

			READP(mem, file_size - 8 - 16);
			WRITEP(mem, file_size - 8 - 16);

			free(mem);
			CloseHandle(f);

			return;
		}
		CloseHandle(f);
		return;
	}
}
int Read_files_in_folder(const char* start_folder)
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
			Read_files_in_folder(next_path);
		}
		else
		{
			char path[260] = { 0 };
			strcpy(path, start_folder);
			strcat(path, FindFileData.cFileName);
			Read_and_write_to_archive(path);
		}
	} while (FindNextFileA(hFind, &FindFileData) != 0);

	FindClose(hFind);
	return 1;
}

int Asura_pack(char* folder_name)
{
	strcpy(g_archive_file_name, folder_name);
	strcat(g_archive_file_name, ".asr");

	strcpy(g_folder_to_pack_path, folder_name);

	DWORD a = 0;

	HANDLE f2 = CreateFileA(g_archive_file_name, GENERIC_WRITE, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	g_handle_for_pack = f2;

	if ((GetLastError() != ERROR_SUCCESS) && (GetLastError() != ERROR_ALREADY_EXISTS))
	{
		printf("File create error: %d\n", GetLastError());
		CloseHandle(f2);
		return EXIT_FAILURE;
	}

	WRITEP("Asura   ", 8);

	char temp[260] = { 0 };
	strcpy(temp, g_folder_to_pack_path);
	strcat(temp, "\\");

	Read_files_in_folder(temp);

	WRITEP("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16);

	CloseHandle(f2);
	return EXIT_SUCCESS;
}



int Asura_unpack(char* f_name)
{
	GetCurrentDirectoryA(260, (LPSTR)g_path);
	strcpy(g_archive_file_name, f_name);

	HANDLE f = CreateFileA(f_name, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (GetLastError() != ERROR_SUCCESS)
	{
		printf("File open error: %d\n", GetLastError());
		CloseHandle(f);
		return EXIT_FAILURE;
	}

	char unpacked_folder[260] = { 0 };
	strcpy(unpacked_folder, g_archive_file_name);
	strcat(unpacked_folder, "_unpacked\\");
	TryCreateDirectory(unpacked_folder);

	Unpack_Asura_arch(f);

	return 0;
}

void Usage_func()
{
	printf("Usage: program [option] [name] \n");
	printf(ASRUNPACK_VER_STRING);
	//printf("Miniz unpack library: version %s \n", MZ_VERSION);
	printf("Options: \"decompose\" - decomposes uncompressed asura archive into smaller \n");
	printf("                         ones by using information encoded in RSFL chunk.   \n");
	printf("         \"decompress\"- decompress compressed by zlib asura archives.      \n");
	printf("         \"unpack\"    - unpacks chunks from uncompresses asura archive.    \n");
	printf("         \"pack\"      - pack chunks back into asura archive, use folder    \n");
	printf("                         name in [name] field.                              \n");
	return;
}

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		Usage_func();
		return 0;
	}



	if (!strcmp(argv[1], "decompose"))
	{
		decompose_func(argv[2]);
	}
	else if (!strcmp(argv[1], "decompress"))
	{
		printf("It not implemented yet, use BMS script \n");
	}
	else if (!strcmp(argv[1], "unpack"))
	{
		Asura_unpack(argv[2]);
	}
	else if (!strcmp(argv[1], "pack"))
	{
		if (Asura_pack(argv[2]))
		{
			printf("Something went wrong at packing ! \n");
		}
	}
	else
	{
		Usage_func();
		return 0;
	}


	//system("PAUSE");
    return 0;
}
