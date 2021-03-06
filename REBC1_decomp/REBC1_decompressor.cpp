// ConsoleApplication5.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include "huffman.hpp"
#pragma warning( disable: 4996 )

int main(int argc, char** argv)
{	
	if (argc < 2)
		return 0;

	HANDLE f = CreateFileA(argv[1], GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (GetLastError() != ERROR_SUCCESS)
	{
		printf("File open error: %d\n", GetLastError());
		CloseHandle(f);
		return 0;
	}
	DWORD compr_file_size = GetFileSize(f, nullptr);
	void* mem = malloc(compr_file_size);

	DWORD a = 0;
	ReadFile(f, mem, compr_file_size, &a, NULL);
	if (a != compr_file_size)
	{
		printf("Something went wrong: readen %d of %d bytes(%d)\n", a, compr_file_size, GetLastError());
		CloseHandle(f);
		free(mem);
		return 0;
	}

	DWORD decomp_size = ((HuffmanPackage*)mem)->UncompressedDataSize + 16;
	char* mem2 = HuffmanDecompress((HuffmanPackage*)mem);

	CloseHandle(f);
	free(mem);



	HANDLE f2 = CreateFileA(strcat(argv[1], "dec"), GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if ((GetLastError() != ERROR_SUCCESS) && (GetLastError() != ERROR_ALREADY_EXISTS))
	{
		printf("File create error: %d\n", GetLastError());
		CloseHandle(f2);
		return 0;
	}

	WriteFile(f2, mem2, decomp_size, &a, NULL);
	if (a != decomp_size)
	{
		printf("Something went wrong: writen %d of %d bytes(%d)\n", a, decomp_size, GetLastError());
		CloseHandle(f2);
		free(mem2);
		return 0;
	}

	CloseHandle(f2);
	free(mem2);

	system("PAUSE");
    return 0;
}

