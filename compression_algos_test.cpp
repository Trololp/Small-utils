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
typedef unsigned long long int QWORD;
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

#define HUFFMAN_MAX_SYMBOLS_COUNT 256

// Huffman code compression
struct Huffman_Tree_node {
	UINT weight;

	// contain symbols
	UINT count;
	UCHAR symbols[HUFFMAN_MAX_SYMBOLS_COUNT];

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



void Dump_bit_string(UINT val, UINT len) {
	for (int i = 0; i < len; i++) {
		printf(val & (1 << i) ? "1" : "0");
	}
}

void Dump_bit_table(UINT* bit_codes, UINT* bit_counts) {

	printf("Symbol_table: \n");

	for (int i = 0; i < HUFFMAN_MAX_SYMBOLS_COUNT; i++) {
		printf("%02x : len %d (", i, bit_counts[i]);
		Dump_bit_string(bit_codes[i], bit_counts[i]);
		printf(") \n");
	}
}

int quick_sort_pairs_partition(std::vector<std::pair<BYTE, BYTE>>& p_v, int low, int high)
{
	BYTE pivot = p_v[high].first; // pivot
	int i = (low - 1); // Index of smaller element and indicates the right position of pivot found so far

	for (int j = low; j <= high - 1; j++)
	{
		// If current element is smaller than the pivot
		if (p_v[j].first < pivot)
		{
			i++; // increment index of smaller element

			std::pair<BYTE, BYTE> tmp = p_v[i];
			p_v[i] = p_v[j];
			p_v[j] = tmp;
		}
	}
	std::pair<BYTE, BYTE> tmp = p_v[i + 1];
	p_v[i + 1] = p_v[high];
	p_v[high] = tmp;

	return (i + 1);
}

void quick_sort_pairs(std::vector<std::pair<BYTE, BYTE>>& p_v, int low, int high) {
	if (low < high)
	{
		/* pi is partitioning index, arr[p] is now
		at right place */
		int pi = quick_sort_pairs_partition(p_v, low, high);

		// Separately sort elements before
		// partition and after partition
		quick_sort_pairs(p_v, low, pi - 1);
		quick_sort_pairs(p_v, pi + 1, high);
	}
}

UCHAR* Huffman_Compress(UCHAR* input, UINT size_of_data, UINT* size_of_compressed_data)
{
	UCHAR* input_ptr = input;
	//Fill frequency table
	static UINT g_frequency_table[HUFFMAN_MAX_SYMBOLS_COUNT] = { 0 };
	static UINT g_symbol_bits_table[HUFFMAN_MAX_SYMBOLS_COUNT] = { 0 };
	static UINT g_bit_count_table[HUFFMAN_MAX_SYMBOLS_COUNT] = { 0 };
	static bool g_used_symbols[HUFFMAN_MAX_SYMBOLS_COUNT] = { 0 };

	while (input_ptr <= (input + size_of_data))
	{
		g_frequency_table[*input_ptr]++;
		g_used_symbols[*input_ptr] = true;
		input_ptr++;
	}

	printf("g_frequency_table built\n");

	std::vector <Huffman_Tree_node*> ht_nodes;



	for (int i = 0; i < HUFFMAN_MAX_SYMBOLS_COUNT; i++)
	{
		if (!g_used_symbols[i]) continue;
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
	for (UINT symbol = 0; symbol < HUFFMAN_MAX_SYMBOLS_COUNT; symbol++)
	{
		if (!g_used_symbols[symbol]) continue;

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

	DWORD symbol_used_amt = 0;
	for (auto used : g_used_symbols) { if (used) symbol_used_amt++; }

	// 3 tables [symb][sybol codes][symbol len] - [byte][word][byte]
	DWORD additional_info_size = symbol_used_amt * 4 + sizeof(WORD);

	UCHAR* output = (UCHAR*)malloc(size_of_data + additional_info_size); 
	memset(output, 0, size_of_data + additional_info_size);
	UCHAR* output_ptr = output;
	input_ptr = input;


	// Store tables

	// there need some kind of sorting for tables
	// from lowest bit count to higher.

	std::vector<std::pair<BYTE, BYTE>> increasing_order; // number of bits and ID

	for (int i = 0; i < HUFFMAN_MAX_SYMBOLS_COUNT; i++) {
		if (!g_used_symbols[i]) continue;
		increasing_order.push_back({ g_bit_count_table[i], i });
	}

	quick_sort_pairs(increasing_order, 0, increasing_order.size() - 1);

	// storing amount of symbols used
	*(WORD*)output_ptr = symbol_used_amt;
	output_ptr += 2;

	// symbols table
	for (int i = 0; i < symbol_used_amt; i++) {
		*(BYTE*)output_ptr = increasing_order[i].second;
		output_ptr++;
	}

	// symbol code table
	for (int i = 0; i < symbol_used_amt; i++)
	{
		*((DWORD*)output_ptr) = g_symbol_bits_table[increasing_order[i].second];
		output_ptr += 4;
	}
	// bit amount table
	for (int i = 0; i < symbol_used_amt; i++)
	{
		*((BYTE*)output_ptr) = g_bit_count_table[increasing_order[i].second];
		output_ptr += 1;
	}

	int bit_counter = 0;
	// bit hell
	while (input_ptr <= (input + size_of_data))
	{
		QWORD val = *((QWORD*)output_ptr);
		//val = 0;


		while (bit_counter < 32)
		{
			UCHAR symbol = *input_ptr;
			input_ptr++;

			val |= ((QWORD)g_symbol_bits_table[symbol] << bit_counter);
			bit_counter += g_bit_count_table[symbol];

		}

		*((QWORD*)output_ptr) = val;

		if (bit_counter >= 32)
		{
			output_ptr += 4;
			bit_counter -= 32;
		}
	}

	*size_of_compressed_data = output_ptr - output + 1;

	printf("Huffman Compression done: %d : %d\n", size_of_data, *size_of_compressed_data);
	printf("Huffman Compression tables space coeff: %f %%\n", 100.0f * (float)additional_info_size
		/ (float) (*size_of_compressed_data - additional_info_size));


	return output;

}



UCHAR* Huffman_Decompress(UCHAR* input, UINT size_of_compressed_data, UINT size_of_decompressed_data)
{
	UCHAR* input_end = input + size_of_compressed_data;


	static UINT g_symbol_bits_table[HUFFMAN_MAX_SYMBOLS_COUNT] = { 0 };
	static UINT g_bit_count_table[HUFFMAN_MAX_SYMBOLS_COUNT] = { 0 };
	static BYTE g_char_table[HUFFMAN_MAX_SYMBOLS_COUNT] = { 0 };
	static DWORD g_bitmask_table[HUFFMAN_MAX_SYMBOLS_COUNT] = { 0 };

	DWORD symbol_used_amt = 0;
	symbol_used_amt = *(WORD*)input;
	input += sizeof(WORD);

	for (int i = 0; i < symbol_used_amt; i++) {
		g_char_table[i] = *(BYTE*)input;
		input++;
	}

	// read immportant tables
	for (int i = 0; i < symbol_used_amt; i++) {
		g_symbol_bits_table[i] = *(DWORD*)input;
		input += 4;
	}

	for (int i = 0; i < symbol_used_amt; i++) {
		g_bit_count_table[i] = *(BYTE*)input;
		input++;
	}

	int symb_id = 0;
	while (symb_id < symbol_used_amt) {
		DWORD bit_mask = 0;
		for (int i = 0; i < g_bit_count_table[symb_id]; i++) bit_mask |= 1 << i;
		g_bitmask_table[symb_id] = bit_mask;
		symb_id++;
	}

	//Dump_bit_table(g_symbol_bits_table, g_bit_count_table);
	
	//return input;


	UCHAR* decompressed_data = (UCHAR*)malloc(size_of_decompressed_data);
	memset(decompressed_data, 0, size_of_decompressed_data);

	UCHAR* output_ptr = decompressed_data;
	UCHAR* output_end = decompressed_data + size_of_decompressed_data;

	int bit_counter = 0;
	DWORD tmp_val;

	// Decoder :(
	while (output_ptr <= output_end && input <= input_end)
	{
		while (bit_counter < 32) {
			DWORD val = *((QWORD*)input) >> bit_counter;
			//val = 0;
			int symb_id = 0;


			// probably should use something tree like structures
			// instead of iterating through every symbol ??
			// probably will be better in perf but anyway not speed testing this algos
			while (symb_id < symbol_used_amt) {
				
				DWORD tmp_val = val ^ g_symbol_bits_table[symb_id];

				if ((tmp_val & g_bitmask_table[symb_id]) == 0) {
					*output_ptr = g_char_table[symb_id];
					output_ptr++;
					break;
				}
				symb_id++;
			}

			bit_counter += g_bit_count_table[symb_id];
		}

		if (bit_counter >= 32) {
			input += 4;
			bit_counter -= 32;
		}
	}

	return decompressed_data;
}

// Liv-Zempel? like algorithm 
// just dictionary algorithm of some sort
// with sliding window
// !!! NOT WORKING NOW

#define LZ_SLIDING_WINDOW_SIZE 0x8000
#define LZ_MAX_SEQUENCE_LENGHT 0xFF
#define LZ_MAX_COPY_DATA_LENGHT 0x7F

struct lz_op_code {
	//USHORT distance; // if distance < LZ_SLIDING_WINDOW_SIZE this correct
	UCHAR dist1; //higner if > 0x80 then copy
	UCHAR dist0; //lower part
	UCHAR lenght;
	UCHAR symbol;
};

// unused
/*
struct lz_op_code_data {
	UCHAR lenght; // len = this byte - 0x80 + 1
	UCHAR data; // data bytes ahead...
};*/

// Fills lps[] for given patttern pat[0..M-1]
void lz_computeLPSArray(UCHAR* pat, int M, UCHAR* lps)
{
	// length of the previous longest prefix suffix
	UCHAR len = 0;

	lps[0] = 0; // lps[0] is always 0

				// the loop calculates lps[i] for i = 1 to M-1
	int i = 1;
	while (i < M) {
		if (pat[i] == pat[len]) {
			len++;
			lps[i] = len;
			i++;
		}
		else // (pat[i] != pat[len])
		{
			// This is tricky. Consider the example.
			// AAACAAAA and i = 7. The idea is similar
			// to search step.
			if (len != 0) {
				len = lps[len - 1];

				// Also, note that we do not increment
				// i here
			}
			else // if (len == 0)
			{
				lps[i] = 0;
				i++;
			}
		}
	}
}

// Uses Knuth–Morris–Pratt algorithm
USHORT lz_find_best_match(UCHAR* buf, USHORT buf_len, UCHAR* seq, USHORT seq_len, USHORT* max_len_found) {
	int M = seq_len;
	int N = buf_len;

	*max_len_found = 0;

	// create lps[] that will hold the longest prefix suffix
	// values for pattern
	//int* lps = (int*)malloc(sizeof(int)*M);

	static UCHAR lps[LZ_MAX_SEQUENCE_LENGHT] = { 0 };

	//int lps[M];

	// Preprocess the pattern (calculate lps[] array)
	lz_computeLPSArray(seq, M, lps);

	int i = 0; // index for txt[]
	int j = 0; // index for pat[]
	while (i < N) {
		*max_len_found = j;
		if (seq[j] == buf[i]) {
			j++;
			i++;
		}

		if (j == M) {
			//free(lps);
			return i - j;
			//printf("Found pattern at index %d ", i - j);
			//j = lps[j - 1];
		}

		// mismatch after j matches
		else if (i < N && seq[j] != buf[i]) {
			// Do not match lps[0..lps[j-1]] characters,
			// they will match anyway
			if (j != 0)
				j = lps[j - 1];
			else
				i = i + 1;
		}
	}
	//free(lps);
	return -1;
}



UCHAR* lz_compress(UCHAR* input, UINT size_of_data, UINT* size_of_compressed_data) {

	// first fill buffer with known information
	UCHAR* output = (UCHAR*)malloc(size_of_data);
	UCHAR* output_ptr = output;


	static_assert(LZ_MAX_COPY_DATA_LENGHT <= 0x7F, "LZ_MAX_COPY_DATA_LENGHT must be <= 0x7F");

	*output_ptr = 0x80 + LZ_MAX_COPY_DATA_LENGHT;
	output_ptr++;

	// Optimize this code for small data ?
	if (size_of_data < LZ_MAX_COPY_DATA_LENGHT) {
		memcpy(output_ptr, input, size_of_data);
		output_ptr += size_of_data;

		*size_of_compressed_data = output_ptr - output;

		return output_ptr;
	}
	else {
		memcpy(output_ptr, input, LZ_MAX_COPY_DATA_LENGHT);
		output_ptr += LZ_MAX_COPY_DATA_LENGHT;
	}
	

	UCHAR* dict_ptr = input;
	UCHAR* input_ptr = input + LZ_MAX_COPY_DATA_LENGHT;

	UCHAR* end_ptr = input + size_of_data;

	while (input_ptr < end_ptr) {
		// find best match!
		// IMPORTANT!!!
		// this is bad way to do it!
		// There should be way to optimize best matches \n


		if (input_ptr + LZ_MAX_SEQUENCE_LENGHT + 1 < end_ptr) {
			UINT seq_lenght = LZ_MAX_SEQUENCE_LENGHT;
			USHORT max_len_found;
			USHORT distance = 0xFFFF;
			// find lower limit first
			while (distance == 0xFFFF) {
				
				distance = lz_find_best_match(dict_ptr, input_ptr - dict_ptr, input_ptr, seq_lenght, &max_len_found);
				//printf("dist %d seq_len %d dict: %08x seq: %08x\n", distance, seq_lenght, dict_ptr, input_ptr);
				seq_lenght = max_len_found;
				if (seq_lenght == 0) break;
			}
			
			// now match found
			if (distance == 0xFFFF)
			{
				*output_ptr = 0x80 + LZ_MAX_COPY_DATA_LENGHT;
				output_ptr++;
				memcpy(output_ptr, input_ptr, LZ_MAX_COPY_DATA_LENGHT);
				output_ptr += LZ_MAX_COPY_DATA_LENGHT;

				// updata pointers
				input_ptr += LZ_MAX_COPY_DATA_LENGHT;
				if (input_ptr - dict_ptr >= LZ_SLIDING_WINDOW_SIZE)
					dict_ptr = input_ptr - LZ_SLIDING_WINDOW_SIZE;


			}

			// contruct lz opcode
			lz_op_code opcode;
			opcode.dist0 = distance & 0xFF;
			opcode.dist1 = (distance & 0xFF00) >> 8;
			//opcode.distance = distance;
			opcode.lenght = seq_lenght;
			opcode.symbol = *(input_ptr + seq_lenght);

			*(lz_op_code*)output_ptr = opcode;
			output_ptr += sizeof(lz_op_code);

			// update pointers

			input_ptr += seq_lenght;
			if (input_ptr - dict_ptr >= LZ_SLIDING_WINDOW_SIZE)
				dict_ptr = input_ptr - LZ_SLIDING_WINDOW_SIZE;

		}
		else {
			// end data should be copied? or better other way?
			//
			//


			if (end_ptr - input_ptr <= LZ_MAX_COPY_DATA_LENGHT) {
				*output_ptr = 0x80 + end_ptr - input_ptr;
				output_ptr++;
				memcpy(output_ptr, input_ptr, end_ptr - input_ptr);
				output_ptr += end_ptr - input_ptr;
			}
			else {
				*output_ptr = 0x80 + LZ_MAX_COPY_DATA_LENGHT;
				output_ptr++;
				memcpy(output_ptr, input_ptr, LZ_MAX_COPY_DATA_LENGHT);
				output_ptr += LZ_MAX_COPY_DATA_LENGHT;
				*output_ptr = 0x80 + end_ptr - input_ptr - LZ_MAX_COPY_DATA_LENGHT;
				output_ptr++;
				memcpy(output_ptr, input_ptr, end_ptr - input_ptr - LZ_MAX_COPY_DATA_LENGHT);
				output_ptr += end_ptr - input_ptr - LZ_MAX_COPY_DATA_LENGHT;
			}

			*size_of_compressed_data = output_ptr - output;

			return output;
		}


	}
	*size_of_compressed_data = output_ptr - output;
	return output;
}

UCHAR* lz_decompress(UCHAR* input, UINT size_of_compressed_data, UINT size_of_decompressed_data) {

	UCHAR* output = (UCHAR*)malloc(size_of_decompressed_data);
	UCHAR* output_ptr = output;
	UCHAR* end_ptr = output + size_of_decompressed_data;


	UCHAR* input_ptr = input;
	UCHAR* dict_ptr = output_ptr;
	// Optimize this code for small data ?
	if (size_of_compressed_data < LZ_MAX_COPY_DATA_LENGHT) {
		memcpy(output_ptr, input, size_of_compressed_data);
		return output_ptr;
	}
	else {
		memcpy(output_ptr, input_ptr, LZ_MAX_COPY_DATA_LENGHT);
		output_ptr += LZ_MAX_COPY_DATA_LENGHT;
		input_ptr += LZ_MAX_COPY_DATA_LENGHT;
	}
	
	while (output_ptr < end_ptr) {

		// is data
		if (*(BYTE*)input_ptr < 0) {
			UINT data_len = *(UCHAR*)input_ptr - 0x80;
			input_ptr++;

			memcpy(output_ptr, input_ptr, data_len);
			output_ptr += data_len;
			input_ptr += data_len;
		}
		else { // is reference
			lz_op_code* opcode_p = (lz_op_code*)input_ptr;
			input_ptr += sizeof(lz_op_code);

			UINT distance = opcode_p->dist0 + (opcode_p->dist1 << 8);

			// check if memcpy can be broken in case of (output_ptr + opcode_p->lenght > output_ptr - opcode_p->distance)
			memcpy(output_ptr, dict_ptr + distance, opcode_p->lenght);
			output_ptr += opcode_p->lenght;
			*output_ptr = opcode_p->symbol;
			output_ptr++;
		}
		if (output_ptr - dict_ptr >= LZ_SLIDING_WINDOW_SIZE)
			dict_ptr = output_ptr - LZ_SLIDING_WINDOW_SIZE;

	}

	return output;
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

		f2 = CreateFileA("decompressed.bin", GENERIC_WRITE, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
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

	//printf("LZ compression \n");
	//Test_decompression_algorithm(lz_compress, lz_decompress, argv[1], compressed_file_name);

	system("PAUSE");
	return 0;
}

