#include "stdafx.h"
#include "String_extraction.h"
#include "Common.h"
#include <map>
//#include <string>

#pragma warning( disable: 4996 )

std::map<DWORD, char*> g_extractor_map;

String_extraction::String_extraction()
{
}

void String_extraction::add(char * str, size_t len)
{
	if (!str || len == 0) return;

	DWORD hash = hash_f(str);

	if (g_extractor_map[hash]) {
		return;
	}

	char* new_str = (char*)malloc(len + 1);
	strcpy(new_str, str);
	g_extractor_map[hash] = new_str;

	return;
}

void String_extraction::store_to_file(const char * file_name)
{
	FILE* f = fopen(file_name, "a+");

	if (!f || f == INVALID_HANDLE_VALUE)
	{
		printf("Error opening names list !!!\n");
		return;
	}

	for (auto mbr : g_extractor_map) {
		if (mbr.second) {
			size_t l = strlen(mbr.second);
			if (!fwrite(mbr.second, l, 1, f)) {
				printf("Error writing lines\n");
				return;
			}
			fwrite("\n", 1, 1, f);

		}
	}

	fclose(f);
	return;
}


String_extraction::~String_extraction()
{
	//for (auto mbr : g_extractor_map) {
	//	if (mbr.second) {
	//		free(mbr.second);
	//	}
	//}
}
