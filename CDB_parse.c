// ForTestingOnly.cpp: определяет точку входа для консольного приложения.
//


#include "stdafx.h"
#include <stdio.h>

char operation_code(char ch)
{
	int group_code = (ch >> 5) & 0x7;
	switch (group_code)
	{
	case 0:
		printf("6 byte command\n");
		break;
	case 1:
	case 2:
		printf("10 byte command\n");
		break;
	case 3:
		printf("Reserved or 0x7F code\n");
		break;
	case 4:
		printf("16 byte command\n");
		break;
	case 5:
		printf("12 byte command\n");
		break;
	case 6:
	case 7:
		printf("Vendor specific\n");
		break;
	default:
		printf("%d \n", group_code);
	}

	int command_code = ch & 0x1F;
	printf("Command code: %x \n", command_code);
	return group_code;
}

void control_byte_dump(char ch)
{

}

void CDB_6bytes_print(unsigned char* cdb)
{
	// 1st byte skipped cuz of its already showed
	char lba1 = cdb[1];
	char lba2 = cdb[2];
	char lba3 = cdb[3];
	unsigned int LBA = lba3 + 0x100*lba2  + 0x10000*(lba1 & 0x1F);
	char some_CDB_info = lba1 >> 5;
	char some_lenght = cdb[4];
	char control_byte = cdb[5];

	printf("Miscellaneous CDB information: %d \n", some_CDB_info);
	printf("Lba (21-bit): %d (0x%06x) \n", LBA, LBA);
	printf("Len: %d (0x%x)\n", some_lenght, some_lenght);
	printf("Control: %d (0x%x)\n", control_byte, control_byte);
}

void CDB_10bytes_print(unsigned char* cdb)
{
	// 1st byte skipped cuz of its already showed

	char ch2 = cdb[1];
	unsigned int LBA = *(DWORD*)(cdb + 2);
	char some_CDB_info = ch2 >> 5;
	char SA = ch2 & 0x1F;
	char some_CDB_info2 = cdb[6];
	WORD some_lenght = *(WORD*)(cdb + 7);
	char control_byte = cdb[9];

	printf("Miscellaneous CDB information 1: %d \n", some_CDB_info);
	printf("Miscellaneous CDB information 2: %d \n", some_CDB_info2);
	printf("Service action: %d (0x%x) \n", SA, SA);
	printf("Lba (32-bit): %d (0x%08x) \n", LBA, LBA);
	printf("Len: %d (0x%04x)\n", some_lenght, some_lenght);
	printf("Control: %d (0x%x)\n", control_byte, control_byte);
}

void CDB_12bytes_print(unsigned char* cdb)
{
	// 1st byte skipped cuz of its already showed

	char ch2 = cdb[1];
	unsigned int LBA = *(DWORD*)(cdb + 2);
	char some_CDB_info = ch2 >> 5;
	char SA = ch2 & 0x1F;
	DWORD some_lenght = *(DWORD*)(cdb + 6);
	char some_CDB_info2 = cdb[10];
	char control_byte = cdb[11];

	printf("Miscellaneous CDB information 1: %d \n", some_CDB_info);
	printf("Miscellaneous CDB information 2: %d \n", some_CDB_info2);
	printf("Service action: %d (0x%x) \n", SA, SA);
	printf("Lba (32-bit): %d (0x%08x) \n", LBA, LBA);
	printf("Len: %d (0x%08x)\n", some_lenght, some_lenght);
	printf("Control: %d (0x%x)\n", control_byte, control_byte);
}

void CDB_16bytes_print(unsigned char* cdb)
{
	// 1st byte skipped cuz of its already showed
	char some_CDB_info = cdb[1];
	long long unsigned int LBA = *(long long unsigned int*)(cdb + 2);
	DWORD some_lenght = *(DWORD*)(cdb + 10);
	char some_CDB_info2 = cdb[14];
	char control_byte = cdb[15];

	printf("Miscellaneous CDB information 1: %d \n", some_CDB_info);
	printf("Miscellaneous CDB information 2: %d \n", some_CDB_info2);
	printf("Lba (32-bit): %lld (0x%016x) \n", LBA, LBA);
	printf("Len: %d (0x%08x)\n", some_lenght, some_lenght);
	printf("Control: %d (0x%x)\n", control_byte, control_byte);
}


// from offset 0x30 '0 1 2 3 4 5 6 7 8 9 @ a b c d ...'
int hash_map_nible[64] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
	0, 0, 0, 0, 0, 0, 0, 10, 11, 12,
	13, 14, 15, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 10,
	11, 12, 13, 14, 15, 0, 0, 0, 0, 0,
	0, 0, 0, 0
};

unsigned char hex2byte(char nible1, char nible2)
{
	return hash_map_nible[(nible1 - 0x30) & 0x3F] + 0x10*hash_map_nible[(nible2 - 0x30) & 0x3F];
}

void hex2char(char* hexstr, unsigned char* str, unsigned int size)
{
	for (unsigned int i = 0; i < size; i++)
	{
		str[i] = hex2byte(hexstr[2 * i + 1], hexstr[2 * i ]);
	}
}

int main(int argc, char** argv)
{
	//printf("%x \n", hex2byte(argv[1][0], argv[1][1]));
	if (argc != 2)
	{
		printf("Usage: \n");
		printf("cdb_decoder byte_string\n");
		system("PAUSE");
		return 0;
	}
	printf("%s\n", argv[1]);

	

	int str_lenght = strlen((const char*)argv[1]) / 2;
	int group_code;

	if (str_lenght > 16)
		str_lenght = 16;

	if (str_lenght > 0)
	{
		group_code = operation_code(hex2byte(argv[1][1], argv[1][0]));
		unsigned char cdb[16] = { 0 };
		hex2char(argv[1], cdb, str_lenght);
		for (int i = 0; i < str_lenght; i++)
			printf("%02x \n", cdb[i]);
		switch (group_code)
		{
		case 0:
			CDB_6bytes_print(cdb);
			break;
		case 1:
		case 2:
			CDB_10bytes_print(cdb);
			break;
		case 3:
			//print("Reserved or 0x7F code\n");
			break;
		case 4:
			CDB_16bytes_print(cdb);
			break;
		case 5:
			CDB_12bytes_print(cdb);
			break;
		case 6:
		case 7:
			//print("Vendor specific\n");
			break;
		}
	}

	//system("PAUSE");
	return 0;
}