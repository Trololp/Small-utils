// ConsoleApplication5.cpp: определяет точку входа для консольного приложения.
// is not complete yet
// Unpacker that unpacks Asura archives with special RSFL chunk. will unpack zlib, huffman, xmemcompr... etc...

#include "stdafx.h"
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


//Structs
struct RSFL_entry
{
	const char* file_name;
	DWORD offset;
	DWORD file_size;
	DWORD unk;
};

struct chunk_info
{
	DWORD magic;
	bool has_name;
	int offset_of_name;
	DWORD type1;
	DWORD type2;
};

//Globals
chunk_info g_chunk_info[100] = {
	{ 'FCSR', TRUE, 0x1C, 2, 0 },
	{ 'CATC', TRUE, 0x18 },
	{ 'TATC', TRUE, 0x14 },
	{ 'FCSR', FALSE, 0x0 },
	{ 'FCSR', FALSE, 0x0 },
	{ 'FCSR', FALSE, 0x0 },
	{ 'FCSR', FALSE, 0x0 },
	{ 'FCSR', FALSE, 0x0 },
	{ 'FCSR', FALSE, 0x0 },
	{ 'FCSR', FALSE, 0x0 },
	{ 'FCSR', FALSE, 0x0 },
	{ 'FCSR', FALSE, 0x0 },
};

RSFL_entry* g_RSFL_entries = nullptr;
DWORD g_RSFL_size = 0;
DWORD g_RSFL_count = 0;
DWORD g_core_segment_addr = 0;
char g_archive_file_name[260] = { 0 };
char g_path[260] = { 0 };

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
			return false;
		}
	}
	return true;
}

bool Recource_have_name(DWORD magic)
{

}

bool Unpack_Asura_arch(HANDLE f)
{
	DWORD a = 0;
	SFPS(0);
	char Arch_magic[9] = "";
	Arch_magic[8] = '\0';

	READP(Arch_magic, 8);

	printf("Unpacking... \n");

	if (!strcmp(Arch_magic, "Asura   "))
	{
		struct chunk_hdr
		{
			DWORD magic;
			DWORD Size;
		};

		chunk_hdr hdr;

		do
		{
			READ(hdr);

			if (Recource_have_name(hdr.magic))
			{

			}

		} while (hdr.magic != NULL);

		

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

	}
	else if (!strcmp(argv[1], "unpack"))
	{

	}
	else if (!strcmp(argv[1], "pack"))
	{

	}
	else
	{
		Usage_func();
		return 0;
	}


	system("PAUSE");
    return 0;
}

