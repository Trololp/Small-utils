// Compressors testing
//

#include "stdafx.h"
//#include <cstdlib>
//#include <algorithm>
#include <Windows.h>
#include <vector>

#pragma warning( disable: 4996 )

//Defines
typedef unsigned int UINT;
typedef unsigned char UCHAR;
typedef unsigned long long int ULLINT;
typedef int(*__compar_d_fn_t) (const void *, const void *, void *);

//Macro
#define swap_uint(a, b) {UINT tmp = (a) ; (a) = (b); (b) = tmp;}
#define SFPS(pos) SetFilePointer(f, pos, 0, FILE_BEGIN)
#define SFPC(pos) SetFilePointer(f, pos, 0, FILE_CURRENT)
#define READ(v) ReadFile(f, &(v), sizeof(v), &a, NULL)
#define READP(p, n) ReadFile(f, p, n, &a, NULL)
#define WRITE(v) WriteFile(f2, &(v), sizeof(v), &a, NULL)
#define WRITEP(p, n) WriteFile(f2, p, n, &a, NULL)

//Structs

//Globals


UCHAR* gimme_random_array(UCHAR count)
{
	UCHAR* mem = (UCHAR*)malloc(count * sizeof(UCHAR));

	for (int i = 0; i < count; i++)
	{
		mem[i] = (UCHAR)rand();
	}
	return mem;
}

UCHAR* gimme_inverse_array(UCHAR count)
{
	UCHAR* mem = (UCHAR*)malloc(count * sizeof(UCHAR));

	for (int i = 0; i < count; i++)
	{
		mem[i] = count - i;
	}
	return mem;
}

UCHAR* gimme_sorted_array(UCHAR count)
{
	UCHAR* mem = (UCHAR*)malloc(count * sizeof(UCHAR));

	for (int i = 0; i < count; i++)
	{
		mem[i] = count;
	}
	return mem;
}

UCHAR* gimme_bunch_of_zeros_array(UCHAR count)
{
	UCHAR* mem = (UCHAR*)malloc(count * sizeof(UCHAR));

	memset(mem, 0, count * sizeof(UCHAR));

	return mem;
}

void randomize_array(UCHAR* arr, UCHAR count)
{
	for (int i = 0; i < count; i++)
	{
		arr[i] = rand();
	}
}

void read_cycles(ULLINT* cycles)
{
	UINT tmp_v1, tmp_v2;
	__asm {
		xor eax, eax
		cpuid
		rdtsc
		mov tmp_v1, eax
		mov tmp_v2, edx
	}

	*cycles = tmp_v2;
	*cycles <<= 32;
	*cycles += tmp_v1;
}



//  Compressing\Decompressing algorithms
//  Template
//  out_array* Compressor(input_array*, UINT size_of_data, UINT* size_of_compressed_data)
//  out_array* Decompressor(input_array*, UINT size_of_compressed_data, UINT size_of_decompressed_data)

// Delta encoding is
// [val1][val2] -> [val1][val2-val1]
// it helps to compress linear sequences
UCHAR* Delta_encode(UCHAR* input, UINT size_of_data, UINT* size_of_compressed_data)
{
	UCHAR* input_ptr = input;
	UCHAR saved_val = *input_ptr;
	input_ptr++;
	while (input_ptr <= (input + size_of_data))
	{
		UCHAR tmp = *input_ptr;
		*input_ptr = *input_ptr - saved_val;
		input_ptr++;
		saved_val = tmp;
	}

	return input;
}

// [val1][val2-val1]
//
UCHAR* Delta_decode(UCHAR* input, UINT size_of_compressed_data, UINT size_of_decompressed_data)
{
	UCHAR* input_ptr = input;
	UCHAR saved_val = *input_ptr;
	input_ptr++;
	while (input_ptr <= (input + size_of_compressed_data))
	{
		*input_ptr = *input_ptr + saved_val;
		saved_val = *input_ptr;
		input_ptr++;
	}

	return input;
}

// Run Lenght Encode
// Scheme
// [amt] [val] [amt] [val] [val]
//  amt < 254
//  [amt] byte [value] 
//  if amt == 255 next byte is size of data bytes following next
//  [255] byte [size] [data_bytes][][][]....
UCHAR* RLE_Compress(UCHAR* input, UINT size_of_data, UINT* size_of_compressed_data)
{
	UCHAR* output = (UCHAR*)malloc(size_of_data);
	UCHAR* output_ptr = output;
	UCHAR* input_ptr = input;

	while (input_ptr <= (input + size_of_data))
	{
		UINT b1 = *input_ptr;
		input_ptr++;
		UINT b2 = *input_ptr;
		input_ptr++;

		UCHAR run_lenght = 0;

		if (b1 == b2)
		{
			run_lenght = 2;

			while (*input_ptr == b1 && run_lenght < 254)
			{
				run_lenght++;
				input_ptr++;
			}

			*output_ptr = run_lenght;
			output_ptr++;
			*output_ptr = b1;
			output_ptr++;
		}
		else
		{
			run_lenght = 2;

			UINT saved_val = b2;
			UCHAR* saved_input_ptr = input_ptr - 2;

			while (*input_ptr != saved_val && run_lenght < 254)
			{
				run_lenght++;
				saved_val = *input_ptr;
				input_ptr++;
			}

			*output_ptr = 255;
			output_ptr++;
			*output_ptr = run_lenght;
			output_ptr++;

			while (saved_input_ptr != input_ptr)
			{
				*output_ptr = *saved_input_ptr;
				output_ptr++;
				saved_input_ptr++;
			}
		}
	}

	*output_ptr = 0;

	*size_of_compressed_data = output_ptr - output;

	printf("RLE Compression done: %d : %d\n", size_of_data, *size_of_compressed_data);

	return output;
}

UCHAR* RLE_Decompress(UCHAR* input, UINT size_of_compressed_data, UINT size_of_decompressed_data)
{
	UCHAR* output = (UCHAR*)malloc(size_of_decompressed_data);
	UCHAR* output_ptr = output;
	UCHAR* input_ptr = input;

	while (output_ptr <= (output + size_of_decompressed_data))
	{
		UINT amt = *input_ptr;
		input_ptr++;
		UINT val = *input_ptr;
		input_ptr++;

		if (amt == 255)
		{
			for (int i = 0; i < val; i++)
			{
				*output_ptr = *input_ptr;
				input_ptr++;
				output_ptr++;
			}

			continue;
		}

		if (amt != 0)
		{
			for (int i = 0; i < amt; i++)
			{
				*output_ptr = val;
				output_ptr++;
			}
			continue;
		}

		if (size_of_decompressed_data != (output_ptr - output))
		{
			printf("Error in decompression, incorrect decompression size\n");
			return output;
		}

	}

	return output;
}

//RLE + Delta-encoding
UCHAR* DeltaRLE_Compress(UCHAR* input, UINT size_of_data, UINT* size_of_compressed_data)
{
	Delta_encode(input, size_of_data, size_of_compressed_data);
	return RLE_Compress(input, size_of_data, size_of_compressed_data);
}

UCHAR* DeltaRLE_Decompress(UCHAR* input, UINT size_of_compressed_data, UINT size_of_decompressed_data)
{
	UCHAR* decompressed_data = RLE_Decompress(input, size_of_compressed_data, size_of_decompressed_data);
	Delta_decode(decompressed_data, size_of_decompressed_data, size_of_decompressed_data);
	return decompressed_data;
}

// Huffman code compression
struct Huffman_Tree_node {
	UINT weight;

	// contain symbols
	UINT count;
	UCHAR symbols[256];

	Huffman_Tree_node* p_left = nullptr;
	Huffman_Tree_node* p_right = nullptr;

};


void Dump_node(Huffman_Tree_node* p_node, UINT hops)
{
	for (int i = 0; i < hops; i++)
	{
		printf("\t");
	}

	printf("%08x count - %d left %08x right %08x \n", p_node, p_node->count, p_node->p_left, p_node->p_right);
}

UCHAR* Huffman_Compress(UCHAR* input, UINT size_of_data, UINT* size_of_compressed_data)
{
	UCHAR* input_ptr = input;
	//Fill frequency table
	UINT g_frequency_table[256] = { 0 };
	UINT g_symbol_bits_table[256] = { 0 };
	UINT g_bit_count_table[256] = { 0 };

	while (input_ptr <= (input + size_of_data))
	{
		g_frequency_table[*input_ptr]++;
		input_ptr++;
	}

	printf("g_frequency_table built\n");

	std::vector <Huffman_Tree_node*> ht_nodes;

	for (int i = 0; i < 256; i++)
	{
		Huffman_Tree_node* p_node = new Huffman_Tree_node;
		p_node->weight = g_frequency_table[i];
		p_node->count = 1;
		p_node->symbols[0] = i;

		ht_nodes.push_back(p_node);
	}

	// building tree
	while (ht_nodes.size() > 1)
	{

		int id_least1 = 0;
		int id_least2 = 0;
		UINT least_weight1 = -1;
		UINT least_weight2 = -1;

		// find 2 least weight nodes
		for (int i = 0; i < ht_nodes.size(); i++)
		{
			if (least_weight1 >= ht_nodes[i]->weight)
			{
				least_weight1 = ht_nodes[i]->weight;
				id_least1 = i;
			}

			if (least_weight2 >= ht_nodes[i]->weight && i != id_least1)
			{
				least_weight2 = ht_nodes[i]->weight;
				id_least2 = i;
			}
		}

		//Construct new node
		Huffman_Tree_node* p_node = new Huffman_Tree_node;
		p_node->weight = least_weight1 + least_weight2;
		p_node->count = ht_nodes[id_least1]->count + ht_nodes[id_least2]->count;
		p_node->p_right = ht_nodes[id_least1]; // the least one is right branch
		p_node->p_left = ht_nodes[id_least2];

		//printf("p_node->weight: %d \n", p_node->weight);
		//printf("p_node->count: %d \n", p_node->count);

		UINT cnt = 0;
		// Filling symbols
		while (cnt < p_node->count)
		{
			if (cnt < ht_nodes[id_least1]->count)
				p_node->symbols[cnt] = ht_nodes[id_least1]->symbols[cnt];
			else
				p_node->symbols[cnt] = ht_nodes[id_least2]->symbols[cnt - ht_nodes[id_least1]->count];

			cnt++;
		}

		// changes to new node at old one and removes second old node.
		


		ht_nodes[id_least2] = p_node;
		ht_nodes.erase(ht_nodes.begin() + id_least1);

		//printf("Nodes remaining %d\n", ht_nodes.size());
	}

	printf("Huffman Tree built\n");

	

	// builds code table
	for (UINT symbol = 0; symbol < 256; symbol++)
	{
		UINT hops = 0;
		UINT symbol_code = 0;
		Huffman_Tree_node* p_node = ht_nodes[0];


		while (p_node)
		{
			//Dump_node(p_node, hops);

			// node contain symbol ?
			UCHAR cnt = 0;
			bool is_left = false;
			bool is_contain = false;
			if (p_node->p_left)
			{
				while (cnt < p_node->p_left->count)
				{
					if (p_node->p_left->symbols[cnt] == symbol)
					{
						is_left = true;
						is_contain = true;
						p_node = p_node->p_left;
						break;
					}
					cnt++;
				}
			}

			if (is_left)
			{
				hops += is_contain;
				continue;
			}

			if (p_node->p_right)
			{
				cnt = 0;
				while (cnt < p_node->p_right->count)
				{
					if (p_node->p_right->symbols[cnt] == symbol)
					{
						p_node = p_node->p_right;
						is_contain = true;
						break;
					}
					cnt++;
				}
			}

			if (!is_contain)
				break;

			
			//             [d] [a] [b]    |
			//	d=00000000   \  \0  / 1   } 2 - hop
			//	a=10000000   0|  [c]      |
			//	b=11000000    \  / 1	  } 1 - hop
			//				 [a]		  |
			symbol_code |= ((!is_left) << hops);
			hops += is_contain;

		}
		g_symbol_bits_table[symbol] = symbol_code;
		g_bit_count_table[symbol] = hops;
	}

	printf("code tables built\n");

	UCHAR* output = (UCHAR*)malloc(size_of_data + 768);
	memset(output, 0, size_of_data + 768);
	UCHAR* output_ptr = output;
	input_ptr = input;
	int bit_counter = 0;

	// Store tables
	// symbol code table
	for (int i = 0; i < 256; i++)
	{
		*((WORD*)output_ptr) = g_symbol_bits_table[i];
		output_ptr += 2;
	}
	// bit amount table
	for (int i = 0; i < 256; i++)
	{
		*((BYTE*)output_ptr) = g_bit_count_table[i];
		output_ptr += 1;
	}

	// bit hell
	while (input_ptr <= (input + size_of_data))
	{
		DWORD val = *((DWORD*)output_ptr);
		//val = 0;


		while (bit_counter < 16)
		{
			UCHAR symbol = *input_ptr;
			input_ptr++;

			val |= (g_symbol_bits_table[symbol] << bit_counter);
			bit_counter += g_bit_count_table[symbol];

		}

		*((DWORD*)output_ptr) = val;

		if (bit_counter >= 16)
		{
			output_ptr += 2;
			bit_counter -= 16;
		}
	}

	*size_of_compressed_data = output_ptr - output + 1;

	printf("Huffman Compression done: %d : %d\n", size_of_data, *size_of_compressed_data);

	return output;

}

UCHAR* Huffman_Decompress(UCHAR* input, UINT size_of_compressed_data, UINT size_of_decompressed_data)
{
	return input;
}

void Test_decompression_algorithm(
	UCHAR* (*compressor_fn)(UCHAR* input, UINT size_of_data, UINT* size_of_compressed_data),
	UCHAR* (*decompressor_fn)(UCHAR* input, UINT size_of_compressed_data, UINT size_of_decompressed_data),
	char* data_file_name,
	char* compressed_file_name
)
{
	DWORD a = 0;
	HANDLE f = CreateFileA(data_file_name, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (GetLastError())
	{
		printf("File open Error: %d\n", GetLastError());
		return;
	}

	DWORD file_size = GetFileSize(f, NULL);
	UCHAR* file_data = (UCHAR*)malloc(file_size);
	UCHAR* file_data_copy = (UCHAR*)malloc(file_size);
	READP(file_data, file_size);

	memcpy(file_data_copy, file_data, file_size);

	UINT size_of_compressed_data;

	UCHAR* compressed_data = compressor_fn(file_data_copy, file_size, &size_of_compressed_data);

	HANDLE f2 = CreateFileA(compressed_file_name, GENERIC_WRITE, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (GetLastError() != ERROR_ALREADY_EXISTS && GetLastError())
	{
		printf("File create Error: %d\n", GetLastError());
		return;
	}

	WRITE(size_of_compressed_data);
	WRITEP(compressed_data, size_of_compressed_data);

	CloseHandle(f);
	CloseHandle(f2);

	UCHAR* decompressed_data = decompressor_fn(compressed_data, size_of_compressed_data, file_size);

	if (!memcmp(file_data, decompressed_data, file_size))
	{
		float compression_ratio = (float)file_size / (float)size_of_compressed_data;

		printf("Test passed, compress ratio %f\n", compression_ratio);
	}
	else
	{
		printf("Test not passed\n");

		f2 = CreateFileA("test.bin", GENERIC_WRITE, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		WRITEP(decompressed_data, file_size);
		CloseHandle(f2);
	}

	free(file_data);
	free(compressed_data);
	//free(decompressed_data);

	return;
}

int main(int argc, char** argv)
{
	bool run_tests = 0;

	if (argc < 2)
		return 0;

	char compressed_file_name[MAX_PATH];
	char* dot_ch;

	strcpy(compressed_file_name, argv[1]);
	dot_ch = strrchr(compressed_file_name, '.');
	strcat(dot_ch, "_compressed.cmp");

	//printf("RLE compression\n");
	//Test_decompression_algorithm(RLE_Compress, RLE_Decompress, argv[1], compressed_file_name);

	//printf("RLE+delta-encoding compression\n");
	//Test_decompression_algorithm(DeltaRLE_Compress, DeltaRLE_Decompress, argv[1], compressed_file_name);

	printf("Huffman compression \n");
	Test_decompression_algorithm(Huffman_Compress, Huffman_Decompress, argv[1], compressed_file_name);

	system("PAUSE");
	return 0;
}

