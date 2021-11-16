// png2bmp
// uses "Miniz" as Zlib library.
// see https://github.com/richgel999/miniz
#include "stdafx.h"
#include <stdlib.h>
//#include "crc32.hpp"
#include <Windows.h>
#include <vector>

#include "miniz.h"

#pragma warning(disable : 4996)

//Macro
#define SFPS(pos) SetFilePointer(f, pos, 0, FILE_BEGIN)
#define SFPC(pos) SetFilePointer(f, pos, 0, FILE_CURRENT)
#define READ(v) ReadFile(f, &(v), sizeof(v), &a, NULL)
#define READBE4(v) ReadFile(f, &(v), sizeof(v), &a, NULL); \
					v = SwapFourBytes((v))
#define READP(p, n) ReadFile(f, p, n, &a, NULL)
#define WRITE(v) WriteFile(f2, &(v), sizeof(v), &a, NULL)
#define WRITEP(p, n) WriteFile(f2, p, n, &a, NULL)
#define SwapFourBytes(data)   \
( (((data) >> 24) & 0x000000FF) | (((data) >>  8) & 0x0000FF00) | \
  (((data) <<  8) & 0x00FF0000) | (((data) << 24) & 0xFF000000) ) 

//typedef unsigned int UINT;

typedef struct {
	UINT len;
	UINT type;
	UINT start_at;
	UINT crc;
} png_chunk_t;

enum PNG_COLOR_TYPE
{
	PNG_COLOR_GRAYSCALE,
	PNG_COLOR_RGB = 2,
	PNG_COLOR_PALETTE,
	PNG_COLOR_GRAYSCALE_ALPHA,
	PNG_COLOR_RGBA = 6
};

enum PNG_INTERLACE_METHOD
{
	PNG_INTERLACE_NO,
	PNG_INTERLACE_ADAM7
};

enum PNG_FILTER_METHOD
{
	PNG_FILTER_NONE,
	PNG_FILTER_SUB,
	PNG_FILTER_UP,
	PNG_FILTER_AVG,
	PNG_FILTER_PAETH
};

typedef struct {
	UINT height;
	UINT width;
	BYTE bit_depth;
	BYTE color_type;
	BYTE comp_method; // only deflate
	BYTE filter_method; // only one
	BYTE interlace_method;
} png_image_hdr_t;

typedef struct {
	BYTE r;
	BYTE g;
	BYTE b;
} rgb_vec_t;

BYTE* g_pallete = nullptr;
BYTE* g_decompressed_IDAT_bytestream = nullptr;
BYTE* g_compressed_IDAT_bytestream = nullptr;
BYTE* g_raw_image = nullptr;
BYTE* g_bmp24_raw_image = nullptr;
bool g_debug_filters = false;
std::vector<PNG_FILTER_METHOD> g_vec_filters;

void Pad_lines(BYTE* lines_new, BYTE* lines_old, UINT width, UINT height)
{
	UINT line_num = 0;
	UINT new_width = (3 * width + 3) & ~3;

	while (line_num < height)
	{
		memcpy(lines_new + new_width * line_num, lines_old + 3 * width * line_num, 3 * width);
		line_num++;
	}
}

bool BMP_24_create_from_bytes(char* file_name, png_image_hdr_t* png_hdr, BYTE* bmp_data)
{
	DWORD a = 0;
	HANDLE f2 = CreateFileA(file_name, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (GetLastError() && (GetLastError() != ERROR_ALREADY_EXISTS))
	{
		printf("Error create bmp file: %d \n", GetLastError());
		return false;
	}

	// 14b BITMAPFILEHEADER
	// 40b BITMAPINFOHEADER
	// Data ?

	UINT bmp_size = 54 + 3 * png_hdr->width * png_hdr->height;

	BITMAPFILEHEADER bmp_hdr;
	bmp_hdr.bfType = 0x4D42; // BM
	bmp_hdr.bfSize = bmp_size;
	bmp_hdr.bfReserved1 = 0;
	bmp_hdr.bfReserved2 = 0;
	bmp_hdr.bfOffBits = 54;

	BITMAPINFOHEADER bmp_info_hdr;
	ZeroMemory(&bmp_info_hdr, sizeof(BITMAPINFOHEADER));
	bmp_info_hdr.biSize = 40;
	bmp_info_hdr.biWidth = png_hdr->width;
	bmp_info_hdr.biHeight = -(int)png_hdr->height;
	bmp_info_hdr.biPlanes = 1;
	bmp_info_hdr.biBitCount = 24;
	bmp_info_hdr.biCompression = 0;
	bmp_info_hdr.biSizeImage = bmp_size - 54;

	WRITE(bmp_hdr);
	WRITE(bmp_info_hdr);

	// probably most bad code here
	// REPLACE IT !!! IT IS BAD FOR PERFORMANCE
	//reverseBytes(bmp_data, bmp_size - 54);
	//reverse_lines_rgb((rgb_vec_t*)bmp_data, png_hdr->width, png_hdr->height);
	if (png_hdr->width % 4 != 0)
	{
		BYTE* bmp_padded_data = (BYTE*)malloc(((3 * png_hdr->width + 3) & ~3) * png_hdr->height);
		Pad_lines(bmp_padded_data, bmp_data, png_hdr->width, png_hdr->height);

		UINT pad_bytes_size = (((3 * png_hdr->width  + 3) & ~3) - 3 * png_hdr->width) * png_hdr->height;

		WRITEP(bmp_padded_data, bmp_size - 54 + pad_bytes_size);

		free(bmp_padded_data);
		return true;
	}

	WRITEP(bmp_data, bmp_size - 54);


	return true;
}

void BMP_raw_grayscale_to_raw_bgr(BYTE* raw_in, BYTE* raw_out, UINT bitdepth, UINT width, UINT height)
{
	BYTE bit2_to_8bit_color[] = {0x00, 0x55, 0xAA, 0xFF};
	BYTE bit4_to_8bit_color[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

	UINT pix_amt = width * height;
	BYTE* raw_in_row_p = raw_in;

	switch (bitdepth)
	{
	case 1:

		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				BYTE pix_val = (raw_in_row_p[j / 8] & (0x80 >> (j % 8))) != 0 ? 0xFF : 0x00;
				raw_out[3 * (i*width + j) + 2] = pix_val;
				raw_out[3 * (i*width + j) + 1] = pix_val;
				raw_out[3 * (i*width + j) + 0] = pix_val;
			}

			raw_in_row_p = raw_in + ((width + 7) >> 3) * i;
		}

		return;
	case 2:

		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				BYTE val = raw_in_row_p[j / 4] & (0xC0 >> (2 * (j % 4)));
				BYTE pix_val = bit2_to_8bit_color[val >> (6 - 2 * (j % 4))];

				raw_out[3 * (i*width + j) + 2] = pix_val;
				raw_out[3 * (i*width + j) + 1] = pix_val;
				raw_out[3 * (i*width + j) + 0] = pix_val;
			}

			raw_in_row_p = raw_in + ((width + 3) >> 2) * i;
		}

		return;
	case 4:

		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				BYTE val = raw_in_row_p[j / 2] & (0xF0 >> (4 * (j % 2)));
				BYTE pix_val = bit4_to_8bit_color[val >> (4 - 4 * (j % 2))];

				raw_out[3 * (i*width + j) + 2] = pix_val;
				raw_out[3 * (i*width + j) + 1] = pix_val;
				raw_out[3 * (i*width + j) + 0] = pix_val;
			}

			raw_in_row_p = raw_in + ((width + 1) >> 1) * i;
		}

		return;
	case 8:
		for (int i = 0; i < pix_amt; i++)
		{
			BYTE pix_val = raw_in[i];
			raw_out[3 * i + 0] = pix_val;
			raw_out[3 * i + 1] = pix_val;
			raw_out[3 * i + 2] = pix_val;
		}
		return;
	case 16:
		for (int i = 0; i < pix_amt; i++)
		{
			BYTE pix_val = (BYTE)((float)((WORD*)raw_in)[i] / 65535.0f * 255.0f);
			raw_out[3 * i + 0] = pix_val;
			raw_out[3 * i + 1] = pix_val;
			raw_out[3 * i + 2] = pix_val;
		}
		return;
	default:
		printf("Error bitdepth = %d \n", bitdepth);
		return;
	}
}

void BMP_raw_palette_to_raw_bgr(BYTE* raw_in, BYTE* raw_out, UINT width, UINT height, UINT bit_depth, BYTE* palette)
{

	UINT pix_amt = width * height;
	BYTE* raw_in_row_p = raw_in;

	if (!palette) return;

	switch (bit_depth)
	{
	case 1:

		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				BYTE idx_val = (raw_in_row_p[j / 8] & (0x80 >> (j % 8))) != 0 ? 0xFF : 0x00;
				raw_out[3 * (i*width + j) + 2] = palette[idx_val * 3 + 0];
				raw_out[3 * (i*width + j) + 1] = palette[idx_val * 3 + 1];
				raw_out[3 * (i*width + j) + 0] = palette[idx_val * 3 + 2];
			}

			raw_in_row_p = raw_in + ((width + 7) >> 3) * i;
		}

		return;
	case 2:

		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				BYTE val = raw_in_row_p[j / 4] & (0xC0 >> (2 * (j % 4)));
				BYTE idx_val = val >> (6 - 2 * (j % 4));

				raw_out[3 * (i*width + j) + 2] = palette[idx_val * 3 + 0];
				raw_out[3 * (i*width + j) + 1] = palette[idx_val * 3 + 1];
				raw_out[3 * (i*width + j) + 0] = palette[idx_val * 3 + 2];
			}

			raw_in_row_p = raw_in + ((width + 3) >> 2) * i;
		}

		return;
	case 4:

		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				BYTE val = raw_in_row_p[j / 2] & (0xF0 >> (4 * (j % 2)));
				BYTE idx_val = val >> (4 - 4 * (j % 2));

				raw_out[3 * (i*width + j) + 2] = palette[idx_val * 3 + 0];
				raw_out[3 * (i*width + j) + 1] = palette[idx_val * 3 + 1];
				raw_out[3 * (i*width + j) + 0] = palette[idx_val * 3 + 2];
			}

			raw_in_row_p = raw_in + ((width + 1) >> 1) * i;
		}

		return;
	case 8:
		for (int i = 0; i < pix_amt; i++)
		{
			BYTE idx_val = raw_in[i];
			raw_out[3 * i + 2] = palette[idx_val * 3 + 0];
			raw_out[3 * i + 1] = palette[idx_val * 3 + 1];
			raw_out[3 * i + 0] = palette[idx_val * 3 + 2];
		}
		return;
	case 16:
	default:
		printf("Error bitdepth = %d \n", bit_depth);
		return;
	}
}

void Debug_fill_filter_colors(BYTE* raw_in, png_image_hdr_t* png_hdr)
{
	UINT row_size = ((png_hdr->bit_depth + 7) >> 3) * png_hdr->width * 3;
	UINT line_n = 0;
	UINT line_size = 10;

	if (g_vec_filters.size() < png_hdr->height)
	{
		printf("g_vec_filters.size() < png_hdr->height\n");
		return;
	}


	while (line_n < png_hdr->height)
	{
		PNG_FILTER_METHOD mtd = g_vec_filters[line_n];

		BYTE* line_p = raw_in + row_size * line_n;

		switch (mtd)
		{
		case PNG_FILTER_NONE:
			memset(line_p, 0, line_size*3);
			break;
		case PNG_FILTER_SUB:
			for (int i = 0; i < line_size * 3; i++) { line_p[i] = !(2 - i % 3) ? 0xFF : 0x00; } // red
			break;
		case PNG_FILTER_UP:
			for (int i = 0; i < line_size * 3; i++) { line_p[i] = !(i % 3) ? 0xFF : 0x00; } // blue
			break;
		case PNG_FILTER_AVG:
			for (int i = 0; i < line_size * 3; i++) { line_p[i] = !(1 - i % 3) ? 0xFF : 0x00; } // green
			break;
		case PNG_FILTER_PAETH:
			for (int i = 0; i < line_size * 3; i++) { line_p[i] = !(1 - i % 3) ? 0x00 : 0xFF; } // purple
			break;
		default:
			break;
		}

		line_n++;
	}
}

bool BMP_decode_raw_png_to_raw_bgr_bmp(BYTE* raw_in, BYTE* raw_out, png_image_hdr_t* png_hdr, rgb_vec_t background_clr = { 255, 255, 255 }, BYTE* pallete = nullptr)
{
	BYTE ct = png_hdr->color_type;
	//UINT raw_in_size = bpp * png_hdr->width * png_hdr->height;

	switch (ct)
	{
	case PNG_COLOR_GRAYSCALE:
		BMP_raw_grayscale_to_raw_bgr(raw_in, raw_out, png_hdr->bit_depth, png_hdr->width, png_hdr->height);
		break;

	case PNG_COLOR_RGB:
		if (png_hdr->bit_depth == 8)
		{
			//memcpy(raw_out, raw_in, 3 * png_hdr->width * png_hdr->height);
			for (int i = 0; i < png_hdr->width * png_hdr->height; i++)
			{
				rgb_vec_t pix = ((rgb_vec_t*)raw_in)[i];
				((rgb_vec_t*)raw_out)[i] = { pix.b, pix.g, pix.r };
			}

			break;
		}
		if (png_hdr->bit_depth == 16)
		{
			for (int i = 0; i < png_hdr->width * png_hdr->height; i++)
			{
				//raw_out[i] = (BYTE)((float)((WORD*)raw_in)[i] / 65535.0f * 255.0f);

				BYTE pix_r = (BYTE)((float)((WORD*)raw_in)[3 * i + 0] / 255.0f);
				BYTE pix_g = (BYTE)((float)((WORD*)raw_in)[3 * i + 1] / 255.0f);
				BYTE pix_b = (BYTE)((float)((WORD*)raw_in)[3 * i + 2] / 255.0f);

				((rgb_vec_t*)raw_out)[i] = { pix_b, pix_g, pix_r };
			}
			break;
		}
		printf("error PNG_COLOR_RGB bit depth = %d", png_hdr->bit_depth);
		return false;

	case PNG_COLOR_PALETTE:

		BMP_raw_palette_to_raw_bgr(raw_in, raw_out, png_hdr->width, png_hdr->height, png_hdr->bit_depth, pallete);
		break;
	case PNG_COLOR_GRAYSCALE_ALPHA:
		if (png_hdr->bit_depth == 8)
		{
			for (int i = 0; i < png_hdr->width * png_hdr->height; i++)
			{
				// output = alpha * foreground + (1-alpha) * background
				float alpha = (float)raw_in[2 * i + 1] / 255.0f;
				float foreground = (float)raw_in[2 * i + 0] / 255.0f;

				BYTE clr_r = (BYTE)((alpha * foreground + (1 - alpha) * (float)background_clr.r / 255.0f) * 255.0f);
				BYTE clr_g = (BYTE)((alpha * foreground + (1 - alpha) * (float)background_clr.g / 255.0f) * 255.0f);
				BYTE clr_b = (BYTE)((alpha * foreground + (1 - alpha) * (float)background_clr.b / 255.0f) * 255.0f);
																											
				raw_out[3 * i + 2] = clr_r;
				raw_out[3 * i + 1] = clr_g;
				raw_out[3 * i + 0] = clr_b;
			}
			break;
		}
		if (png_hdr->bit_depth == 16)
		{
			for (int i = 0; i < png_hdr->width * png_hdr->height; i++)
			{
				// output = alpha * foreground + (1-alpha) * background
				float alpha = (float)((WORD*)raw_in)[2 * i + 1] / 65535.0f;
				float foreground = (float)((WORD*)raw_in)[2 * i + 0] / 65535.0f;

				BYTE clr_r = (BYTE)((alpha * foreground + (1 - alpha) * (float)background_clr.r / 255.0f) * 255.0f);
				BYTE clr_g = (BYTE)((alpha * foreground + (1 - alpha) * (float)background_clr.g / 255.0f) * 255.0f);
				BYTE clr_b = (BYTE)((alpha * foreground + (1 - alpha) * (float)background_clr.b / 255.0f) * 255.0f);

				raw_out[3 * i + 2] = clr_r;
				raw_out[3 * i + 1] = clr_g;
				raw_out[3 * i + 0] = clr_b;
			}
			break;
		}
		printf("error PNG_COLOR_GRAYSCALE_ALPHA bit depth = %d", png_hdr->bit_depth);
		return false;

	case PNG_COLOR_RGBA:
		if (png_hdr->bit_depth == 8)
		{
			for (int i = 0; i < png_hdr->width * png_hdr->height; i++)
			{
				// output = alpha * foreground + (1-alpha) * background
				float alpha = (float)raw_in[4 * i + 3] / 255.0f;
				float foreground_r = (float)raw_in[4 * i + 0] / 255.0f;
				float foreground_g = (float)raw_in[4 * i + 1] / 255.0f;
				float foreground_b = (float)raw_in[4 * i + 2] / 255.0f;

				BYTE clr_r = (BYTE)((alpha * foreground_r + (1 - alpha) * (float)background_clr.r / 255.0f) * 255.0f);
				BYTE clr_g = (BYTE)((alpha * foreground_g + (1 - alpha) * (float)background_clr.g / 255.0f) * 255.0f);
				BYTE clr_b = (BYTE)((alpha * foreground_b + (1 - alpha) * (float)background_clr.b / 255.0f) * 255.0f);

				raw_out[3 * i + 2] = clr_r;
				raw_out[3 * i + 1] = clr_g;
				raw_out[3 * i + 0] = clr_b;
			}
			break;
		}
		if (png_hdr->bit_depth == 16)
		{
			for (int i = 0; i < png_hdr->width * png_hdr->height; i++)
			{
				// output = alpha * foreground + (1-alpha) * background
				float alpha = (float)((WORD*)raw_in)[4 * i + 3] / 65535.0f;
				float foreground_r = (float)((WORD*)raw_in)[4 * i + 0] / 65535.0f;
				float foreground_g = (float)((WORD*)raw_in)[4 * i + 1] / 65535.0f;
				float foreground_b = (float)((WORD*)raw_in)[4 * i + 2] / 65535.0f;

				BYTE clr_r = (BYTE)((alpha * foreground_r + (1 - alpha) * (float)background_clr.r / 255.0f) * 255.0f);
				BYTE clr_g = (BYTE)((alpha * foreground_g + (1 - alpha) * (float)background_clr.g / 255.0f) * 255.0f);
				BYTE clr_b = (BYTE)((alpha * foreground_b + (1 - alpha) * (float)background_clr.b / 255.0f) * 255.0f);

				raw_out[3 * i + 2] = clr_r;
				raw_out[3 * i + 1] = clr_g;
				raw_out[3 * i + 0] = clr_b;
			}
			break;
		}
		printf("error PNG_COLOR_RGBA bit depth = %d", png_hdr->bit_depth);
		return false;

	default:
		printf("unknown (%d)\n", ct);
		return false;
	}

	if (g_debug_filters)
		Debug_fill_filter_colors(raw_out, png_hdr);

	return true;
}

/* report a zlib or i/o error */
void zerr(int ret)
{
	//fputs("zpipe: ", stderr);
	switch (ret) {
	case Z_ERRNO:
		if (ferror(stdin))
			fputs("error reading stdin\n", stdout);
		if (ferror(stdout))
			fputs("error writing stdout\n", stdout);
		break;
	case Z_STREAM_ERROR:
		fputs("invalid compression level\n", stdout);
		break;
	case Z_DATA_ERROR:
		fputs("invalid or incomplete deflate data\n", stdout);
		break;
	case Z_MEM_ERROR:
		fputs("out of memory\n", stdout);
		break;
	case Z_VERSION_ERROR:
		fputs("zlib version mismatch!\n", stdout);
	}
}


int zlib_decompress_IDAT(BYTE* in_data, int size_in, BYTE* out_data, int size_out)
{
	if (!in_data) return Z_ERRNO;


	int ret;
	z_stream strm;
	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = size_in;
	strm.next_in = in_data;
	ret = inflateInit(&strm);
	if (ret != Z_OK)
		return ret;

	strm.avail_out = size_out;
	strm.next_out = out_data;

	ret = inflate(&strm, Z_NO_FLUSH);
	//printf("inflate ret %d \n", ret);
	zerr(ret);

	/* clean up and return */
	(void)inflateEnd(&strm);
	//return Z_OK;

	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

void Paeth_filter_func(BYTE* row, BYTE* prev_row, UINT bpp, UINT width)
{
	UINT row_bytes_amt = width * bpp;
	BYTE* rp_end = row + bpp;

	while (row < rp_end)
	{
		BYTE a = *row + *prev_row++;
		*row++ = a;
	}
	

	rp_end += row_bytes_amt - bpp;

	while (row < rp_end)
	{
		int a, b, c, pa, pb, pc, p;

		c = *(prev_row - bpp);
		a = *(row - bpp);
		b = *prev_row++;

		p = b - c;
		pc = a - c;

		pa = abs(p);
		pb = abs(pc);
		pc = abs(p + pc);

		if (pb < pa)
		{
			pa = pb; a = b;
		}
		if (pc < pa) a = c;

		a += *row;
		*row++ = a;

	}

}



void Unfilter_data(BYTE* data_in, BYTE* data_out, UINT width, UINT height, UINT bpp, UINT bitdepth)
{
	BYTE* curr_ptr_in = data_in;
	BYTE* curr_ptr_out = data_out;

	//UINT bpp = (png_hdr->bit_depth + 7) >> 3;
	UINT row_bytes_amt = width * bpp;
	if (bitdepth == 4) row_bytes_amt = (row_bytes_amt + 1) / 2;
	else if (bitdepth == 2) row_bytes_amt = (row_bytes_amt + 3) / 4;
	else if (bitdepth == 1) row_bytes_amt = (row_bytes_amt + 7) / 8;

	UINT in_data_size = (row_bytes_amt + 1) * height;

	BYTE* row_ptr_out;
	BYTE* tmp_prev_row_p;

	BYTE* prev_row = (BYTE*)malloc(width * bpp);
	ZeroMemory(prev_row, width);

	while (curr_ptr_in < data_in + in_data_size)
	{
		if (g_debug_filters)
		{
			g_vec_filters.push_back((PNG_FILTER_METHOD)*curr_ptr_in);
		}

		switch (*curr_ptr_in)
		{
		case PNG_FILTER_NONE:
			//printf("Filter (None, %08x, %08x) \n", curr_ptr_in - data_in, curr_ptr_out - data_out);

			memcpy(curr_ptr_out, curr_ptr_in + 1, row_bytes_amt);
			memcpy(prev_row, curr_ptr_out, row_bytes_amt);
			curr_ptr_out += row_bytes_amt;
			curr_ptr_in += row_bytes_amt + 1;

			break;
		case PNG_FILTER_SUB:
			//printf("Filter (Sub, %08x, %08x) \n", curr_ptr_in - data_in, curr_ptr_out - data_out);
			memcpy(curr_ptr_out, curr_ptr_in + 1, row_bytes_amt);
			curr_ptr_in += row_bytes_amt + 1;

			row_ptr_out = curr_ptr_out;

			curr_ptr_out += bpp;
			for (int i = bpp; i < row_bytes_amt; i++)
			{
				*curr_ptr_out = (*curr_ptr_out + *(curr_ptr_out - bpp)) & 0xff;
				curr_ptr_out++;
			}
			memcpy(prev_row, row_ptr_out, row_bytes_amt);
			

			break;
		case PNG_FILTER_UP:
			//printf("Filter (Up, %08x, %08x) \n", curr_ptr_in - data_in, curr_ptr_out - data_out);
			memcpy(curr_ptr_out, curr_ptr_in + 1, row_bytes_amt);
			curr_ptr_in += row_bytes_amt + 1;

			row_ptr_out = curr_ptr_out;
			tmp_prev_row_p = prev_row;

			for (int i = 0; i < row_bytes_amt; i++)
			{
				*curr_ptr_out = (*curr_ptr_out + (*tmp_prev_row_p++)) & 0xff;
				curr_ptr_out++;
			}
			memcpy(prev_row, row_ptr_out, row_bytes_amt);


			break;
		case PNG_FILTER_AVG:
			//printf("Filter (Avg, %08x, %08x) \n", curr_ptr_in - data_in, curr_ptr_out - data_out);
			memcpy(curr_ptr_out, curr_ptr_in + 1, row_bytes_amt);
			curr_ptr_in += row_bytes_amt + 1;

			row_ptr_out = curr_ptr_out;
			tmp_prev_row_p = prev_row;

			for (int i = 0; i < bpp; i++)
			{
				*curr_ptr_out = (*curr_ptr_out + (*tmp_prev_row_p++) / 2) & 0xff;
				curr_ptr_out++;
			}

			for (int i = 0; i < row_bytes_amt - bpp; i++)
			{
				*curr_ptr_out = (*curr_ptr_out + 
					(*tmp_prev_row_p++ + *(curr_ptr_out - bpp)) / 2) & 0xff;
				curr_ptr_out++;
			}

			memcpy(prev_row, row_ptr_out, row_bytes_amt);

			break;
		case PNG_FILTER_PAETH:
			//printf("Filter (Paeth, %08x, %08x) \n", curr_ptr_in - data_in, curr_ptr_out - data_out);
			memcpy(curr_ptr_out, curr_ptr_in + 1, row_bytes_amt);
			curr_ptr_in += row_bytes_amt + 1;

			row_ptr_out = curr_ptr_out;

			Paeth_filter_func(curr_ptr_out, prev_row, bpp, width);
			curr_ptr_out += row_bytes_amt;

			memcpy(prev_row, row_ptr_out, row_bytes_amt);

			break;
		default:
			printf("Error not existing filter method %d \n", *curr_ptr_in);
			return;
		}
	}

	free(prev_row);

}

void Unfilter_data_interlaced(BYTE* data_in, BYTE* data_out, UINT width, UINT height, UINT bpp, UINT bitdepth)
{

	UINT pass_cols[7] = { 0 };
	UINT pass_rows[7] = { 0 };
	UINT start_row[7] = { 0, 0, 4, 0, 2, 0, 1 };
	UINT start_col[7] = { 0, 4, 0, 2, 0, 1, 0 };
	UINT row_inc[7] = { 8, 8, 8, 4, 4, 2, 2 };
	UINT col_inc[7] = { 8, 8, 4, 4, 2, 2, 1 };

	pass_cols[0] = ((width + 7) / 8); pass_rows[0] = (height + 7) / 8;
	pass_cols[1] = ((width + 3) / 8); pass_rows[1] = (height + 7) / 8;
	pass_cols[2] = ((width + 3) / 4); pass_rows[2] = (height + 3) / 8;
	pass_cols[3] = ((width + 1) / 4); pass_rows[3] = (height + 3) / 4;
	pass_cols[4] = ((width + 1) / 2); pass_rows[4] = (height + 1) / 4;
	pass_cols[5] = (width / 2);		  pass_rows[5] = (height + 1) / 2;
	pass_cols[6] = (width);			  pass_rows[6] = (height) / 2;

	UINT stop_row[7] = {};
	UINT stop_col[7] = {};
	UINT fpass_size[7] = {};
	UINT pass_size[7] = {};
	for (int i = 0; i < 7; i++) stop_row[i] = start_row[i] + row_inc[i] * pass_rows[i];
	for (int i = 0; i < 7; i++) stop_col[i] = start_col[i] + col_inc[i] * pass_cols[i];
	for (int i = 0; i < 7; i++) fpass_size[i] = (pass_cols[i] * bpp + 1) * pass_rows[i];
	for (int i = 0; i < 7; i++) pass_size[i] = (pass_cols[i] * bpp) * pass_rows[i];

	BYTE* curr_ptr_in = data_in;
	BYTE* curr_ptr_out = data_out;


	UINT curr_col = start_col[0];
	UINT curr_row = start_row[0];
	UINT pass = 0;

	while (pass < 7)
	{
		BYTE* filtered_pass = (BYTE*)malloc(fpass_size[pass]);
		BYTE* unfiltered_pass = (BYTE*)malloc(pass_size[pass]);
		memcpy(filtered_pass, curr_ptr_in, fpass_size[pass]);
		curr_ptr_in += fpass_size[pass];
		
		Unfilter_data(filtered_pass, unfiltered_pass, pass_cols[pass], pass_rows[pass], bpp, bitdepth);
		BYTE* unfiltered_curr_ptr = unfiltered_pass;
		curr_row = start_row[pass];

		while (curr_row < stop_row[pass])
		{
			curr_ptr_out = data_out + bpp * start_col[pass] + curr_row * width * bpp;
			curr_col = start_col[pass];

			while (curr_col < stop_col[pass])
			{

				for (int i = 0; i < bpp; i++)
				{
					*curr_ptr_out = *unfiltered_curr_ptr;
					curr_ptr_out++;
					unfiltered_curr_ptr++;
				}
				curr_ptr_out += bpp * (col_inc[pass] - 1);

				curr_col += col_inc[pass];
			}
			curr_row += row_inc[pass];
		}

		free(filtered_pass);
		free(unfiltered_pass);

		pass++;
	}

}

bool is_hdr_valid(png_image_hdr_t* hdr_p)
{
	
	// big size
	if (hdr_p->width > 8192 || hdr_p->height > 8192)
		return false;
	
	if (hdr_p->width == 0 || hdr_p->height == 0)
		return false;

	BYTE bd = hdr_p->bit_depth;


	// checking all fields limits
	if (!(bd == 1 || bd == 2 || bd == 4 || bd == 8 || bd == 16))
		return false;

	BYTE ct = hdr_p->color_type;

	if (!(ct == PNG_COLOR_GRAYSCALE ||
		ct == PNG_COLOR_RGB ||
		ct == PNG_COLOR_PALETTE ||
		ct == PNG_COLOR_GRAYSCALE_ALPHA ||
		ct == PNG_COLOR_RGBA))
		return false;

	if (hdr_p->comp_method != 0 || hdr_p->filter_method != 0)
		return false;

	if (!(hdr_p->interlace_method == 0 || hdr_p->interlace_method == 1))
		return false;

	// cheking combination of color type and bit depth

	switch (ct)
	{
	case PNG_COLOR_GRAYSCALE:
		break;
	case PNG_COLOR_RGB:
		if (bd == 1 || bd == 2 || bd == 4)
			return false;
		break;
	case PNG_COLOR_PALETTE:
		if (bd == 16)
			return false;
		break;
	case PNG_COLOR_GRAYSCALE_ALPHA:
		if (bd == 1 || bd == 2 || bd == 4)
			return false;
		break;
	case PNG_COLOR_RGBA:
		if (bd == 1 || bd == 2 || bd == 4)
			return false;
		break;
	default:
		printf("error! ct is not in range somehow ? ct %d", ct);
		break;
	}

	// valid then
	return true;
}

void hdr_sweet_print(png_image_hdr_t* hdr_p)
{
	printf("Width x Height: %dx%d\n", hdr_p->width, hdr_p->height);
	printf("Bitdepth: %d bits\n", hdr_p->bit_depth);
	BYTE ct = hdr_p->color_type;
	printf("Color scheme: ");
	switch (ct)
	{
	case PNG_COLOR_GRAYSCALE:
		printf("grayscale \n");
		break;
	case PNG_COLOR_RGB:
		printf("RGB \n");
		break;
	case PNG_COLOR_PALETTE:
		printf("palette \n");
		break;
	case PNG_COLOR_GRAYSCALE_ALPHA:
		printf("grayscale + alpha \n");
		break;
	case PNG_COLOR_RGBA:
		printf("RGA + alpha \n");
		break;
	default:
		printf("unknown (%d)\n", ct);
		break;
	}
	printf("Compression method: %d\n", hdr_p->comp_method);
	printf("Filter method: %d\n", hdr_p->filter_method);
	if (hdr_p->interlace_method == 0)
		printf("Interlace method: None \n");
	else if (hdr_p->interlace_method == 1)
		printf("Interlace method: Adam7\n");
	else
		printf("Interlace method: %d \n", hdr_p->interlace_method);
	
	return;
}

int PNG_interlaced_image_size(UINT width, UINT height)
{
	//         1 6 4 6 2 6 4 6
	//         7 7 7 7 7 7 7 7
	//         5 6 5 6 5 6 5 6
	//         7 7 7 7 7 7 7 7
	//         3 6 4 6 3 6 4 6
	//         7 7 7 7 7 7 7 7
	//         5 6 5 6 5 6 5 6
	//         7 7 7 7 7 7 7 7

	UINT pass_1, pass_2, pass_3, pass_4, pass_5, pass_6, pass_7;

	pass_1 = ((width + 7) / 8 + 1) * (height + 7) / 8;
	pass_2 = ((width + 3) / 8 + 1) * (height + 7) / 8;
	pass_3 = ((width + 3) / 4 + 1) * (height + 3) / 8;
	pass_4 = ((width + 1) / 4 + 1) * (height + 3) / 4;
	pass_5 = ((width + 1) / 2 + 1) * (height + 1) / 4;
	pass_6 = (width / 2 + 1) * (height + 1) / 2;
	pass_7 = (width + 1) * (height) / 2;

	return pass_1 + pass_2 + pass_3 + pass_4 + pass_5 + pass_6 + pass_7;
}

bool ReadPng(char* file_name)
{
	DWORD a = 0;

	HANDLE f = CreateFileA(file_name, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	UINT png_file_size = GetFileSize(f, 0);

	if (GetLastError())
	{
		printf("Error open file: %d \n", GetLastError());
		return false;
	}

	char magic_buff[9];
	READP(magic_buff, 8);
	magic_buff[8] = '\0';


	if (strcmp(magic_buff, "\x89PNG\x0D\x0A\x1A\x0A"))
	{
		printf("This is not PNG file\n");
		return false;
	}

	//Reading chunks

	std::vector<png_chunk_t> png_chunks;

	while (SFPC(0) <= png_file_size - 12)
	{
		UINT len_chunk;
		READBE4(len_chunk);
		UINT type_chunk;
		READBE4(type_chunk);
		
		UINT start_at = SFPC(0);
		SFPC(len_chunk);

		UINT crc_chunk;
		READBE4(crc_chunk);

		png_chunk_t png_chunk;
		png_chunk.crc = crc_chunk;
		png_chunk.len = len_chunk;
		png_chunk.start_at = start_at;
		png_chunk.type = type_chunk;

		png_chunks.push_back(png_chunk);
	}

	bool crit_chunks[] = { 0, 0, 0 };

	for (png_chunk_t chunk : png_chunks)
	{
		switch (chunk.type)
		{
		case 'IHDR': crit_chunks[0] = true; break;
		case 'IDAT': crit_chunks[1] = true; break;
		case 'IEND': crit_chunks[2] = true; break;
		default:
			break;
		}
	}


	if (crit_chunks[0] + crit_chunks[1] + crit_chunks[2] != 3)
	{
		if (!crit_chunks[0])
			printf("PNG file dont contain IHDR\n");
		if (!crit_chunks[1])
			printf("PNG file dont contain IDAT\n");
		if (!crit_chunks[2])
			printf("PNG file dont contain IEND\n");
		return false;
	}

	if (png_chunks[0].type != 'IHDR')
	{
		printf("IHDR must be first chunk \n");
		return false;
	}

	//debug
	printf("Chunks \n");
	for (png_chunk_t chunk : png_chunks)
	{
		UINT type = chunk.type;
		printf("'%c%c%c%c' : %db lenght at 0x%08x \n", (type >> 24) & 0xFF, (type >> 16) & 0xFF, (type >> 8) & 0xFF, type & 0xFF,
			chunk.len, chunk.start_at);
	}


	png_image_hdr_t png_hdr;
	SFPS(png_chunks[0].start_at);
	READBE4(png_hdr.width);
	READBE4(png_hdr.height);
	READ(png_hdr.bit_depth);
	READ(png_hdr.color_type);
	READ(png_hdr.comp_method);
	READ(png_hdr.filter_method);
	READ(png_hdr.interlace_method);

	// header printf
	hdr_sweet_print(&png_hdr);

	if (!is_hdr_valid(&png_hdr))
	{
		printf("Invalid png header \n");
		return false;
	}

	if (g_debug_filters)
	{
		// image too small
		if (png_hdr.width < 25)
			g_debug_filters = false;
	}

	bool b_contain_plte = false;
	UINT palette_start_at = 0;
	UINT palette_entries_amt = 0;
	for (png_chunk_t chunk : png_chunks) { 
		if (chunk.type == 'PLTE') { 
		b_contain_plte = true;
		palette_start_at = chunk.start_at;
		palette_entries_amt = chunk.len / 3;
		break;
	}
	}

	if ((png_hdr.color_type == PNG_COLOR_PALETTE) && !b_contain_plte)
	{
		printf("File don't contain palette for palette color sheme \n");
		return false;
	}

	if (png_hdr.color_type == PNG_COLOR_PALETTE)
	{
		g_pallete = (BYTE*)malloc(768);
		ZeroMemory(g_pallete, 768);
		SFPS(palette_start_at);
		READP(g_pallete, 3 * palette_entries_amt);
	}

	bool b_have_bKGD_chunk = false;
	UINT png_bKGD_start_at = 0;
	rgb_vec_t png_background_color = { 255, 255, 255 };
	for (png_chunk_t chunk : png_chunks)
	{ if (chunk.type == 'bKGD') { b_have_bKGD_chunk = true; png_bKGD_start_at = chunk.start_at; break; } }

	// get background color
	if (b_have_bKGD_chunk)
	{
		SFPS(png_bKGD_start_at);

		switch (png_hdr.color_type)
		{
		case PNG_COLOR_GRAYSCALE:
		case PNG_COLOR_GRAYSCALE_ALPHA:
			WORD gray_w;
			READ(gray_w);
			png_background_color.r = (BYTE)((float)gray_w / 255.0f);
			png_background_color.g = (BYTE)((float)gray_w / 255.0f);
			png_background_color.b = (BYTE)((float)gray_w / 255.0f);
			break;
		case PNG_COLOR_RGB:
		case PNG_COLOR_RGBA:
			WORD red_w;
			WORD green_w;
			WORD blue_w;
			READ(red_w);
			READ(green_w);
			READ(blue_w);
			png_background_color.r = (BYTE)((float)red_w / 255.0f);
			png_background_color.g = (BYTE)((float)green_w / 255.0f);
			png_background_color.b = (BYTE)((float)blue_w / 255.0f);
			break;
		case PNG_COLOR_PALETTE:
			BYTE idx;
			READ(idx);
			png_background_color.r = g_pallete[idx*3 + 0];
			png_background_color.g = g_pallete[idx*3 + 1];
			png_background_color.b = g_pallete[idx*3 + 2];
			break;
		default:
			printf("bKGD ct %d\n", png_hdr.color_type);
			break;
		}

	}

	// read IDAT
	UINT bytestream_size = 0;
	for (png_chunk_t chunk : png_chunks) {if (chunk.type == 'IDAT') bytestream_size += chunk.len; }

	g_compressed_IDAT_bytestream = (BYTE*)malloc(bytestream_size);

	UINT bpp = (png_hdr.bit_depth + 7) >> 3;

	switch (png_hdr.color_type)
	{
	case PNG_COLOR_GRAYSCALE:
		break;
	case PNG_COLOR_RGB:
		bpp *= 3;
		break;
	case PNG_COLOR_PALETTE:
		bpp = 1;
		break;
	case PNG_COLOR_GRAYSCALE_ALPHA:
		bpp *= 2;
		break;
	case PNG_COLOR_RGBA:
		bpp *= 4;
		break;
	default:
		break;
	}

	UINT raw_image_size = png_hdr.height * png_hdr.width * bpp;

	//that need to be calculated differently for adam7 algorithm
	UINT decompressed_IDAT_size = 0;

	if (png_hdr.interlace_method == PNG_INTERLACE_ADAM7)
	{
		decompressed_IDAT_size = PNG_interlaced_image_size(png_hdr.width, png_hdr.height) * bpp;
		//printf("Interlaced IDAT size = %d\n", decompressed_IDAT_size);
	}
	else
	{
		decompressed_IDAT_size = raw_image_size + 1 * png_hdr.height; // +1 byte filter type per scanline
	}

	g_decompressed_IDAT_bytestream = (BYTE*)malloc(decompressed_IDAT_size);
	ZeroMemory(g_decompressed_IDAT_bytestream, decompressed_IDAT_size);

	UINT bytestream_ptr = 0;
	for (png_chunk_t chunk : png_chunks)
	{
		if (chunk.type == 'IDAT')
		{
			SFPS(chunk.start_at);
			READP(g_compressed_IDAT_bytestream + bytestream_ptr, chunk.len);
			bytestream_ptr += chunk.len;
		}
	}
	/*
	HANDLE f2 = CreateFileA("IDAT_compressed.bin", GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (GetLastError() && (GetLastError() != ERROR_ALREADY_EXISTS))
	{
		printf("Error create file: %d \n", GetLastError());
		free(g_compressed_IDAT_bytestream);
		free(g_decompressed_IDAT_bytestream);
		g_compressed_IDAT_bytestream = nullptr;
		g_decompressed_IDAT_bytestream = nullptr;
		return false;
	}
	WRITEP(g_compressed_IDAT_bytestream, bytestream_size);
	CloseHandle(f2);
	*/
	//printf("%d\n", decompressed_IDAT_size);

	printf("Decompressing\n");

	if (zlib_decompress_IDAT(g_compressed_IDAT_bytestream, bytestream_size, g_decompressed_IDAT_bytestream, decompressed_IDAT_size) != Z_OK)
	{
		printf("something went wrong with decompression of IDAT\n");
		free(g_compressed_IDAT_bytestream);
		free(g_decompressed_IDAT_bytestream);
		g_compressed_IDAT_bytestream = nullptr;
		g_decompressed_IDAT_bytestream = nullptr;

		return false;
	}



	HANDLE f2 = CreateFileA("IDAT_decompressed.bin", GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (GetLastError() && (GetLastError() != ERROR_ALREADY_EXISTS))
	{
		printf("Error create file: %d \n", GetLastError());
		free(g_compressed_IDAT_bytestream);
		free(g_decompressed_IDAT_bytestream);
		g_compressed_IDAT_bytestream = nullptr;
		g_decompressed_IDAT_bytestream = nullptr;
		return false;
	}
	WRITEP(g_decompressed_IDAT_bytestream, decompressed_IDAT_size);
	CloseHandle(f2);
	
	g_raw_image = (BYTE*)malloc(raw_image_size);
	g_bmp24_raw_image = (BYTE*)malloc(3 * png_hdr.width * png_hdr.height); // 3 bytes per pixel RGB

	ZeroMemory(g_raw_image, raw_image_size);

	printf("Defiltering\n");

	if (png_hdr.interlace_method == PNG_INTERLACE_ADAM7)
	{
		Unfilter_data_interlaced(g_decompressed_IDAT_bytestream, g_raw_image, png_hdr.width, png_hdr.height, bpp, png_hdr.bit_depth);
	}
	else
	{
		Unfilter_data(g_decompressed_IDAT_bytestream, g_raw_image, png_hdr.width, png_hdr.height, bpp, png_hdr.bit_depth);
	}

	printf("Decode raw image data to rgb\n");

	BMP_decode_raw_png_to_raw_bgr_bmp(g_raw_image, g_bmp24_raw_image, &png_hdr, png_background_color, g_pallete);

	printf("Writing BMP file...\n");

	BMP_24_create_from_bytes((char*)"test.bmp", &png_hdr, g_bmp24_raw_image);

	
	//debug output of decompressed data
	f2 = CreateFileA("raw.raw", GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (GetLastError() && (GetLastError() != ERROR_ALREADY_EXISTS))
	{
		printf("Error create file: %d \n", GetLastError());
		free(g_compressed_IDAT_bytestream);
		free(g_decompressed_IDAT_bytestream);
		g_compressed_IDAT_bytestream = nullptr;
		g_decompressed_IDAT_bytestream = nullptr;
		return false;
	}

	WRITEP(g_raw_image, raw_image_size);
	CloseHandle(f2);


	



	CloseHandle(f);
	free(g_compressed_IDAT_bytestream);
	free(g_decompressed_IDAT_bytestream);
	free(g_bmp24_raw_image);
	free(g_raw_image);
	if(g_pallete)
		free(g_pallete);
	g_compressed_IDAT_bytestream = nullptr;
	g_decompressed_IDAT_bytestream = nullptr;

	return true;
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("Usage:\n png2bmp [Optional] (png_file)\n");
		printf("Options: debug_filters - will draw lines that \n");
		printf("  represent filter type on image (>25 pix wide)\n");
		printf("  * Black  - filter none\n");
		printf("  * Red    - filter substraction\n");
		printf("  * Blue   - filter up row\n");
		printf("  * Green  - filter average\n");
		printf("  * Purple - filter Paeth\n");
		return 0;
	}

	if (argc == 2) {
		if (!ReadPng(argv[1]))
		{
			return 1;
		}
	}
	else
	{
		if (!strcmp(argv[1], "debug_filters"))
		{
			g_debug_filters = true;

			if (ReadPng(argv[2]))
			{
				return 1;
			}
			return 0;
		}


		printf("unknown option\n");
	}

	

	//system("PAUSE");
	return 1;
}
