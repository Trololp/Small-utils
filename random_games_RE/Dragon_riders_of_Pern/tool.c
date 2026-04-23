// Tool to list and decompress files inside .pfi and .pdf file
// for game Dragon Riders Chronicles of Pern
//
// Author: Trololp



#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>


// download dynamite library sources: https://github.com/twogood/dynamite
// it used to decompress some data
#include "libdynamite.h"

#pragma pack(push, 1)

typedef struct {
    char tag[2];
    uint16_t name_len;
    uint32_t offset_in_pfs;
} vfsx_entry_fixed;

typedef struct {
    uint32_t magic_val;
    uint32_t val1;
    uint32_t val2;
    uint32_t p_first_entry;
} vfsx_header;

typedef struct {
    char tag[2];
    uint16_t unk;
    uint32_t self; // pointer to self
    uint32_t p_next_file; // pointer to next file in directory
    uint32_t p_data; // pointer to data
    uint32_t size_decompressed;
    uint16_t compression_type;
    uint16_t name_size;
    uint32_t size_compressed;
    uint32_t crc_prob;
    // char name[name_size];
} vfs_entry_fixed;

#pragma pack(pop)

typedef struct _Cookie
{
  void* in_buffer;
  size_t in_buffer_size;
  uint32_t in_curr_p;
  void* out_buffer;
  size_t out_buffer_size;
  uint32_t out_curr_p;
} Cookie;

static size_t reader(void* buffer, size_t size, void* cookie)
{
    //return fread(buffer, 1, size, ((Cookie*)cookie)->input_file);

    Cookie* p_cookie = ((Cookie*)cookie);

    if(size + p_cookie->in_curr_p > p_cookie->in_buffer_size) {
        printf("reader out of bounds!\n");
        return 0;
    }

    memcpy(buffer, p_cookie->in_buffer + p_cookie->in_curr_p, size);
    p_cookie->in_curr_p += size;
    return size;
}

static size_t writer(void* buffer, size_t size, void* cookie)
{
  //return fwrite(buffer, 1, size, ((Cookie*)cookie)->output_file);
    Cookie* p_cookie = ((Cookie*)cookie);

    if(size + p_cookie->out_curr_p > p_cookie->out_buffer_size) {
        printf("writer out of bounds!\n");
        return 0;
    }

    memcpy(p_cookie->out_buffer + p_cookie->out_curr_p, buffer, size);
    p_cookie->out_curr_p += size;
    return size;
}

//
bool decompress(void* in, void* out, size_t in_size, size_t out_size) {
    printf("decomp logic!\n");
    Cookie cookie;
    cookie.in_buffer = in;
    cookie.out_buffer = out;
    cookie.in_buffer_size = in_size;
    cookie.out_buffer_size = out_size;
    cookie.in_curr_p = 0;
    cookie.out_curr_p = 0;


    DynamiteResult result = dynamite_explode(reader, writer, &cookie);

    if(result == DYNAMITE_SUCCESS) {
        return true;
    }

    printf("Dynamite lib error: %d\n", result);

    return false;
}

// ------------------------------------------------------------
// Utility: find offset in content.txt
// ------------------------------------------------------------
int find_offset(const char* filename, uint32_t* out_offset) {
    FILE* fp = fopen("content.txt", "r");
    if (!fp) {
        perror("content.txt");
        return 0;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        char name[256];
        unsigned int offset;

        if (sscanf(line, "%255s 0x%X", name, &offset) == 2) {
            if (strcmp(name, filename) == 0) {
                *out_offset = offset;
                fclose(fp);
                return 1;
            }
        }
    }

    fclose(fp);
    return 0;
}

// ------------------------------------------------------------
// Extract file from data.pfs
// ------------------------------------------------------------
int extract_file(const char* filename) {
    uint32_t offset;

    if (!find_offset(filename, &offset)) {
        printf("File not found in content.txt\n");
        return 1;
    }

    FILE* fp = fopen("data.pfs", "rb");
    if (!fp) {
        perror("data.pfs");
        return 1;
    }

    fseek(fp, offset, SEEK_SET);

    vfs_entry_fixed entry;
    if (fread(&entry, sizeof(entry), 1, fp) != 1) {
        printf("Failed to read vfs_entry\n");
        fclose(fp);
        return 1;
    }

    if(1) {
        printf("entry: size_decomp %d size_comp %d p_data 0x%x\n", entry.size_decompressed, \
        entry.size_compressed, entry.p_data);
    }


    // Read name
    char* name = malloc(entry.name_size);
    fread(name, 1, entry.name_size, fp);
    name[entry.name_size - 1] = '\0';

    printf("Extracting: %s\n", name);

    // Open output file
    FILE* out = fopen(name, "wb");
    if (!out) {
        perror("output file");
        free(name);
        fclose(fp);
        return 1;
    }

    if (entry.compression_type == 1) {
        // Uncompressed
        uint8_t* buffer = malloc(entry.size_decompressed);

        fseek(fp, entry.p_data, SEEK_SET);
        fread(buffer, 1, entry.size_decompressed, fp);

        fwrite(buffer, 1, entry.size_decompressed, out);
        free(buffer);
    }
    else if (entry.compression_type == 2) {
        // Compressed
        uint8_t* comp = malloc(entry.size_compressed);
        uint8_t* decomp = malloc(entry.size_decompressed);

        fseek(fp, entry.p_data, SEEK_SET);
        fread(comp, 1, entry.size_compressed, fp);

        if (!decompress(comp, decomp,
                        entry.size_compressed,
                        entry.size_decompressed)) {
            printf("Decompression failed\n");
            free(comp);
            free(decomp);
            fclose(out);
            fclose(fp);
            free(name);
            return 1;
        }

        fwrite(decomp, 1, entry.size_decompressed, out);

        free(comp);
        free(decomp);
    }
    else {
        printf("Unknown compression type: %u\n", entry.compression_type);
    }

    fclose(out);
    fclose(fp);
    free(name);

    printf("Done.\n");
    return 0;
}

// ------------------------------------------------------------
// Original listing logic
// ------------------------------------------------------------
void generate_content_txt(void) {
    FILE *fp = fopen("data.pfi", "rb");
    FILE *out = fopen("content.txt", "w");

    vfsx_header header;
    fread(&header, sizeof(header), 1, fp);

    fseek(fp, header.p_first_entry, SEEK_SET);

    while (1) {
        vfsx_entry_fixed entry;

        if (fread(&entry, sizeof(entry), 1, fp) != 1)
            break;

        char *name = malloc(entry.name_len);
        fread(name, 1, entry.name_len, fp);
        name[entry.name_len - 1] = '\0';

        if (entry.tag[0] == 'F' && entry.tag[1] == '!') {
            fprintf(out, "%s 0x%08X\n", name, entry.offset_in_pfs);
        }

        free(name);
    }

    fclose(fp);
    fclose(out);

    printf("Generated content.txt\n");
}

// ------------------------------------------------------------
// MAIN
// ------------------------------------------------------------
int main(int argc, char** argv) {
    if (argc == 3 && strcmp(argv[1], "-e") == 0) {
        return extract_file(argv[2]);
    }

    // Default behavior: generate content.txt
    generate_content_txt();
    return 0;
}
