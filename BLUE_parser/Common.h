#pragma once

#define SFPS(pos)    SetFilePointer(f, pos, 0, FILE_BEGIN)
#define SFPC(pos)    SetFilePointer(f, pos, 0, FILE_CURRENT)
#define READ(v)      readf(f, &(v), sizeof(v))
#define READP(p, n)  readpf(f, p, n)
#define READLE(v)      ReadFile(f, &(v), sizeof(v), &a, NULL)
#define READPLE(p, n)  ReadFile(f, p, n, &a, NULL)
#define WRITE(v)     WriteFile(f2, &(v), sizeof(v), &a, NULL)
#define WRITEP(p, n) WriteFile(f2, p, n, &a, NULL)

void Dump_hex(void* data, unsigned int count);

bool set_file_state_BE(bool isBE);

int readf(HANDLE f, void* pVal, size_t Val_size, DWORD* bytesReaden = nullptr);
int readpf(HANDLE f, void* p, size_t size, DWORD* bytesReaden = nullptr);


int read_padded_str(HANDLE f, char* dest);
char* read_padded_str_f(HANDLE f);
void  write_padded_str_f(char* str, HANDLE f2);
void  skip_padded_str_f(HANDLE f);
DWORD hash_f(char* name);
void add_hash(DWORD hash, char* str);
char* search_hash(DWORD hash);
void Init_hashs(char* file_name);

char* str_copy(char* str_from);

char* str_to_lower(char* str);