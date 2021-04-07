// Rebuilder.cpp
// Rebuilder for lower size of asura compressed Alien misson to retrieve only alien data


#include "stdafx.h"
#include <Windows.h>

#define SFPS(pos) SetFilePointer(f, pos, 0, FILE_BEGIN)
#define SFPC(pos) SetFilePointer(f, pos, 0, FILE_CURRENT)
#define READ(v) ReadFile(f, &(v), sizeof(v), &a, NULL)
#define READP(p, n) ReadFile(f, p, n, &a, NULL)
#define WRITE(v) WriteFile(f2, &(v), sizeof(v), &a, NULL)
#define WRITEP(p, n) WriteFile(f2, p, n, &a, NULL)

#pragma warning(disable : 4996)

struct Chunk_header
{
	DWORD chunk_name;
	DWORD chunk_size;
	DWORD type1;
	DWORD type2;
};
struct RSCF_HDR
{
	DWORD magic;
	DWORD size;
	DWORD type1;
	DWORD type2;
	DWORD RSCF_type1;
	DWORD RSCF_type2;
	DWORD size_wo_hdr;
};

struct MARE_ENTRY
{
	DWORD mat_hash;
	unsigned long int num;
	DWORD Hash1;
	DWORD Hash2;
	DWORD Hash3;
	DWORD Hash4;
	DWORD Hash5;
	float a;
	float b[4];
	DWORD c[3];
	DWORD Hash6;
	DWORD d;
	DWORD e[4];
	DWORD f[2];
	DWORD h[3][4];
	DWORD g;
	DWORD i[6];
	DWORD j[3];
	DWORD l[6];
	DWORD Default_mat_id;
	DWORD Hash7;
	DWORD m;
	DWORD Hash8;
	DWORD n;
	DWORD Hash9;
	DWORD Hash10;
	DWORD k[4];
	DWORD pad[16];
};


DWORD g_skip_group[7] = {
	'ITNE', // Entity
	'GSMS', // Messages will be overwriten anyway
	'STUC', // Cutscene data
	'CATC', // ...
	'TATC', // ...
	'VETC', // ...
	'RTTC'  // ...
};

// For RSCF textures save files
const int g_amount_resources_to_save = 196;
const char* g_save_resources[g_amount_resources_to_save] =
{
	"\\graphics\\characters\\alien_warrior\\alien_hud_mouth_colour.tga",
	"\\graphics\\characters\\alien_warrior\\alien_hud_mouth_normal.tga",
	"\\graphics\\characters\\alien_warrior\\alien_warrior_hud_arms_colour.tga",
	"\\graphics\\characters\\alien_warrior\\alien_warrior_body_colour.tga",
	"\\graphics\\characters\\alien_warrior\\alien_warrior_body_normal.tga",
	"\\graphics\\characters\\alien_warrior\\alien_warrior_chunk_bits_colour.tga",
	"\\graphics\\characters\\alien_warrior\\alien_warrior_chunk_bits_normal.tga",
	"\\graphics\\characters\\alien_warrior\\alien_warrior_head_colour.tga",
	"\\graphics\\characters\\alien_warrior\\alien_warrior_head_normal.tga",
	"\\graphics\\characters\\alien_warrior\\a_w_chunk_cap_colour.tga",
	"\\graphics\\characters\\alien_warrior\\a_w_chunk_cap_normal.tga",
	"\\graphics\\characters\\alien_warrior\\a_w_chunk_cap_tranny.tga",
	"\\graphics\\characters\\alien_warrior\\a_w_gib_cap_tranny.tga",
	"\\graphics\\characters\\alien_warrior\\dismemberedalien_innards_col.tga",
	"\\graphics\\characters\\alien_warrior\\dismemberedalien_innards_norm.tga",
	"\\graphics\\characters\\number_6\\alien_warrior_head_bit_colour.tga",
	"\\graphics\\characters\\number_6\\alien_warrior_head_bit_illumination.tga",
	"\\graphics\\characters\\number_6\\alien_warrior_head_bit_normal.tga",
	"\\graphics\\characters\\praetorian\\praetorian_body_colour.tga",
	"\\graphics\\characters\\praetorian\\praetorian_body_normal.tga",
	"\\graphics\\characters\\praetorian\\praetorian_head_colour.tga",
	"\\graphics\\characters\\praetorian\\praetorian_head_normal.tga",
	"\\graphics\\characters\\praetorian\\praetorian_hud_arm_colour.tga",
	"\\graphics\\characters\\praetorian\\praetorian_hud_arm_normal.tga",
	"\\graphics\\specialfx\\pfx\\animated\\pfx_alien_saliva.dds",
	"\\graphics\\specialfx\\pfx\\animated\\pfx_blood_alien_splatter_set1.dds",
	"\\graphics\\specialfx\\pfx\\animated\\pfx_blood_android_splatter_set1.dds",
	"\\graphics\\specialfx\\pfx\\animated\\pfx_blood_predator_splatter_set1.dds",
	"\\graphics\\specialfx\\pfx\\animated\\pfx_blood_splatter_set1.dds",
	"\\graphics\\specialfx\\pfx\\animated\\pfx_blood_splatter_setav.dds",
	"\\graphics\\specialfx\\pfx\\animated\\pfx_blood_splatter_sethv.dds",
	"\\graphics\\specialfx\\pfx\\animated\\pfx_blood_splatter_sethv2.dds",
	"\\graphics\\specialfx\\pfx\\animated\\pfx_dust_motes.dds",
	"\\graphics\\specialfx\\pfx\\animated\\pfx_electbolts.dds",
	"\\graphics\\specialfx\\pfx\\animated\\pfx_fireheat_256_4frames.dds",
	"\\graphics\\specialfx\\pfx\\animated\\pfx_fire_256_4frames.dds",
	"\\graphics\\specialfx\\pfx\\animated\\pfx_muzzle_distortion_512_4frames.dds",
	"\\graphics\\specialfx\\pfx\\animated\\pfx_plasma_ball_512_4frames.dds",
	"\\graphics\\specialfx\\pfx\\animated\\pfx_smoke-dust-steam_1024x512_32frames.dds",
	"\\graphics\\specialfx\\pfx\\animated\\pfx_smoke_512_4frames.dds",
	"\\graphics\\specialfx\\pfx\\animated\\pfx_smoke_thin_16frames.dds",
	"\\graphics\\specialfx\\pfx\\hud\\acid_creep_01.tga",
	"\\graphics\\specialfx\\pfx\\hud\\alien_haze_glows2.tga",
	"\\graphics\\specialfx\\pfx\\hud\\alien_mess_haze.tga",
	"\\graphics\\specialfx\\pfx\\hud\\alien_mess_haze2.tga",
	"\\graphics\\specialfx\\pfx\\hud\\alien_rays01.tga",
	"\\graphics\\specialfx\\pfx\\hud\\alien_warp01.tga",
	"\\graphics\\specialfx\\pfx\\hud\\marine_box_text_01.tga",
	"\\graphics\\specialfx\\pfx\\hud\\marine_small_box_01.tga",
	"\\graphics\\specialfx\\pfx\\hud\\pred_mine_01.tga",
	"\\graphics\\specialfx\\pfx\\hud\\pred_mine_02.tga",
	"\\graphics\\specialfx\\pfx\\hud\\pred_pfx_01_dds.dds",
	"\\graphics\\specialfx\\pfx\\hud\\pred_target_02_dds.dds",
	"\\graphics\\specialfx\\pfx\\static\\character_scanlines_blue.tga",
	"\\graphics\\specialfx\\pfx\\static\\pfx_blade_blood_alien.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_blade_blood_android.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_blade_blood_human.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_blob_cloudysoft_small_128.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_blood_circlespray.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_blood_splatter3.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_bullet_set.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_cloud_mammatus2.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_cloud_static_2.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_cloud_stratahilights.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_crosslensflare-starburst.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_flare_horizontal.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_glass_shards.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_impact_sparks.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_largeglaresoft_brightcentre.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_leaves-bark-512.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_lensflare_softcircular.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_lightning.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_muzzle_flash_set1.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_muzzle_flash_set2.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_muzzle_hot.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_radiating_mess.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_ringexplosion.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_rock_debris_impacts.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_skybox_cloud.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_sky_planet_red.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_smokeheavy1_256.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_smoke_layered.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_softbrokensphere_small.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_thick_cloud.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_water_splashes.dds",
	"\\graphics\\specialfx\\pfx\\static\\pfx_white_dot_64x64.dds",
	"\\graphics\\specialfx\\displacement\\alien_distort_normal_v10_dds.dds",
	"\\graphics\\specialfx\\displacement\\alien_distort_normal_v6_dds.dds",
	"\\graphics\\specialfx\\displacement\\alien_land_512_3_8dds.dds",
	"Alien_Hud",
	"Alien_Warrior",
	"Alien_Warrior_Arm_Left",
	"Alien_Warrior_Arm_Right",
	"Alien_Warrior_Face",
	"Alien_Warrior_Head",
	"Alien_Warrior_Leg_Left",
	"Alien_Warrior_Leg_Right",
	"Alien_Warrior_Pipes_Top_Left",
	"Alien_Warrior_Pipes_Top_Right",
	"Alien_Warrior_Tail_01",
	"Alien_Warrior_Tail_02",
	"L1#Alien_Warrior",
	"L1#Number_6_Alien_Warrior",
	"L2#Alien_Warrior",
	"L2#Number_6_Alien_Warrior",
	"Number_6_Alien_Warrior",
	"Praetorian_Hud",
	"\\characters\\alien_warrior\\alien_hud_mouth_colour",
	"\\characters\\alien_warrior\\alien_hud_mouth_normal",
	"\\characters\\alien_warrior\\alien_warrior_hud_arms_colour",
	"\\characters\\alien_warrior\\alien_warrior_body_colour",
	"\\characters\\alien_warrior\\alien_warrior_body_normal",
	"\\characters\\alien_warrior\\alien_warrior_chunk_bits_colour",
	"\\characters\\alien_warrior\\alien_warrior_chunk_bits_normal",
	"\\characters\\alien_warrior\\alien_warrior_head_colour",
	"\\characters\\alien_warrior\\alien_warrior_head_normal",
	"\\characters\\alien_warrior\\a_w_chunk_cap_colour",
	"\\characters\\alien_warrior\\a_w_chunk_cap_normal",
	"\\characters\\alien_warrior\\a_w_chunk_cap_tranny",
	"\\characters\\alien_warrior\\a_w_gib_cap_tranny",
	"\\characters\\alien_warrior\\dismemberedalien_innards_col",
	"\\characters\\alien_warrior\\dismemberedalien_innards_norm",
	"\\characters\\number_6\\alien_warrior_head_bit_colour",
	"\\characters\\number_6\\alien_warrior_head_bit_illumination",
	"\\characters\\number_6\\alien_warrior_head_bit_normal",
	"\\characters\\praetorian\\praetorian_body_colour",
	"\\characters\\praetorian\\praetorian_body_normal",
	"\\characters\\praetorian\\praetorian_head_colour",
	"\\characters\\praetorian\\praetorian_head_normal",
	"\\characters\\praetorian\\praetorian_hud_arm_colour",
	"\\characters\\praetorian\\praetorian_hud_arm_normal",
	"\\specialfx\\pfx\\animated\\pfx_alien_saliva",
	"\\specialfx\\pfx\\animated\\pfx_blood_alien_splatter_set1",
	"\\specialfx\\pfx\\animated\\pfx_blood_android_splatter_set1",
	"\\specialfx\\pfx\\animated\\pfx_blood_predator_splatter_set1",
	"\\specialfx\\pfx\\animated\\pfx_blood_splatter_set1",
	"\\specialfx\\pfx\\animated\\pfx_blood_splatter_setav",
	"\\specialfx\\pfx\\animated\\pfx_blood_splatter_sethv",
	"\\specialfx\\pfx\\animated\\pfx_blood_splatter_sethv2",
	"\\specialfx\\pfx\\animated\\pfx_dust_motes",
	"\\specialfx\\pfx\\animated\\pfx_electbolts",
	"\\specialfx\\pfx\\animated\\pfx_fireheat_256_4frames",
	"\\specialfx\\pfx\\animated\\pfx_fire_256_4frames",
	"\\specialfx\\pfx\\animated\\pfx_muzzle_distortion_512_4frames",
	"\\specialfx\\pfx\\animated\\pfx_plasma_ball_512_4frames",
	"\\specialfx\\pfx\\animated\\pfx_smoke-dust-steam_1024x512_32frames",
	"\\specialfx\\pfx\\animated\\pfx_smoke_512_4frames",
	"\\specialfx\\pfx\\animated\\pfx_smoke_thin_16frames",
	"\\specialfx\\pfx\\hud\\acid_creep_01",
	"\\specialfx\\pfx\\hud\\alien_haze_glows2",
	"\\specialfx\\pfx\\hud\\alien_mess_haze",
	"\\specialfx\\pfx\\hud\\alien_mess_haze2",
	"\\specialfx\\pfx\\hud\\alien_rays01",
	"\\specialfx\\pfx\\hud\\alien_warp01",
	"\\specialfx\\pfx\\hud\\marine_box_text_01",
	"\\specialfx\\pfx\\hud\\marine_small_box_01",
	"\\specialfx\\pfx\\hud\\pred_mine_01",
	"\\specialfx\\pfx\\hud\\pred_mine_02",
	"\\specialfx\\pfx\\hud\\pred_pfx_01_dds",
	"\\specialfx\\pfx\\hud\\pred_target_02_dds",
	"\\specialfx\\pfx\\static\\character_scanlines_blue",
	"\\specialfx\\pfx\\static\\pfx_blade_blood_alien",
	"\\specialfx\\pfx\\static\\pfx_blade_blood_android",
	"\\specialfx\\pfx\\static\\pfx_blade_blood_human",
	"\\specialfx\\pfx\\static\\pfx_blob_cloudysoft_small_128",
	"\\specialfx\\pfx\\static\\pfx_blood_circlespray",
	"\\specialfx\\pfx\\static\\pfx_blood_splatter3",
	"\\specialfx\\pfx\\static\\pfx_bullet_set",
	"\\specialfx\\pfx\\static\\pfx_cloud_mammatus2",
	"\\specialfx\\pfx\\static\\pfx_cloud_static_2",
	"\\specialfx\\pfx\\static\\pfx_cloud_stratahilights",
	"\\specialfx\\pfx\\static\\pfx_crosslensflare-starburst",
	"\\specialfx\\pfx\\static\\pfx_flare_horizontal",
	"\\specialfx\\pfx\\static\\pfx_glass_shards",
	"\\specialfx\\pfx\\static\\pfx_impact_sparks",
	"\\specialfx\\pfx\\static\\pfx_largeglaresoft_brightcentre",
	"\\specialfx\\pfx\\static\\pfx_leaves-bark-512",
	"\\specialfx\\pfx\\static\\pfx_lensflare_softcircular",
	"\\specialfx\\pfx\\static\\pfx_lightning",
	"\\specialfx\\pfx\\static\\pfx_muzzle_flash_set1",
	"\\specialfx\\pfx\\static\\pfx_muzzle_flash_set2",
	"\\specialfx\\pfx\\static\\pfx_muzzle_hot",
	"\\specialfx\\pfx\\static\\pfx_radiating_mess",
	"\\specialfx\\pfx\\static\\pfx_ringexplosion",
	"\\specialfx\\pfx\\static\\pfx_rock_debris_impacts",
	"\\specialfx\\pfx\\static\\pfx_skybox_cloud",
	"\\specialfx\\pfx\\static\\pfx_sky_planet_red",
	"\\specialfx\\pfx\\static\\pfx_smokeheavy1_256",
	"\\specialfx\\pfx\\static\\pfx_smoke_layered",
	"\\specialfx\\pfx\\static\\pfx_softbrokensphere_small",
	"\\specialfx\\pfx\\static\\pfx_thick_cloud",
	"\\specialfx\\pfx\\static\\pfx_water_splashes",
	"\\specialfx\\pfx\\static\\pfx_white_dot_64x64",
	"\\specialfx\\displacement\\alien_distort_normal_v10_dds",
	"\\specialfx\\displacement\\alien_distort_normal_v6_dds",
	"\\specialfx\\displacement\\alien_land_512_3_8dds"
};


DWORD g_hashes[g_amount_resources_to_save] = { 0 };

DWORD g_Nulls[256] = { 0 };

//I just paste it from decompiler
DWORD hash_from_str(DWORD init, char* str)
{
	DWORD result; // eax@1
	char *v3; // esi@1
	char i; // cl@2
	int v5; // edx@7
	int v6; // eax@7

	result = init;
	v3 = str;
	if (str)
	{
		for (i = *str; i; result = v5 + v6)
		{
			if ((i - 'A') > 0x19u)
			{
				if (i == '\\')
					i = '/';
			}
			else
			{
				i += 32;
			}
			v5 = 31 * result;
			v6 = i;
			i = (v3++)[1];
		}
	}
	return result;
}

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
		printf("Error", "Read padded str error %d\n", GetLastError());
		return 0;
	}
	return 1;
}

bool check_asura_sign(char* sign)
{
	if (sign[0] == 'A' &&
		sign[1] == 's' &&
		sign[2] == 'u' &&
		sign[3] == 'r' &&
		sign[4] == 'a' &&
		sign[5] == 'C' &&
		sign[6] == 'm' &&
		sign[7] == 'p')
	{
		printf("Compressed not support \n");
		return 0;
	}

	if (sign[0] == 'A' &&
		sign[1] == 's' &&
		sign[2] == 'u' &&
		sign[3] == 'r' &&
		sign[4] == 'a' &&
		sign[5] == 'Z' &&
		sign[6] == 'l' &&
		sign[7] == 'b')
	{
		printf("Compressed not support \n");
		return 0;
	}

	if (sign[0] != 'A' ||
		sign[1] != 's' ||
		sign[2] != 'u' ||
		sign[3] != 'r' ||
		sign[4] != 'a' ||
		sign[5] != ' ' ||
		sign[6] != ' ' ||
		sign[7] != ' ')
	{
		printf("Not Asura arch \n");
		return 0;
	}
	return 1;
}

bool chunk_in_skip_group(DWORD chunk_name, char* RSCF_name)
{
	if (chunk_name != 'FCSR')
	{
		for (int i = 0; i < sizeof(g_skip_group) / 4; i++)
		{
			if (chunk_name == g_skip_group[i])
				return true;
		}
		return false;
	}
	else
	{
		DWORD hash = hash_from_str(0, RSCF_name);
		for (int i = 0; i < g_amount_resources_to_save; i++)
		{
			if (hash == g_hashes[i]) // Save files with given name
				return false;
		}
		return true;
	}

}

void Print_procentage(DWORD Readen, DWORD Size)
{
	if (Readen == 0)
	{
		printf("%d of %d bytes completed (%f procent) \n", 0, Size, 0.0f);
		return;
	}

	int ratio = Size / Readen;
	float result = 100.0f / (float)ratio;
	if (ratio == 1)
	{
		if (Readen - Size / 2 > 0)
		{
			if ((Readen - Size) == 0)
			{
				printf("%d of %d bytes completed (%f procent) \n", Size, Size, 100.0f);
			}
			ratio = Size / (Size - Readen);
			result = 100.0f - 100.0f / (float)ratio;
		}
	}
	printf("%d of %d bytes completed (%f procent) \n", Readen, Size, result);
}

bool save_resource_by_hash(DWORD hash)
{
	for (int i = 0; i < g_amount_resources_to_save; i++)
	{
		if (hash == g_hashes[i]) // Save files with given name
			return true;
	}
	return false;
}

int Rebuild_arch(char* File_name)
{
	DWORD a = 0;
	DWORD Processed_bytes = 0;
	HANDLE f = CreateFileA(File_name, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (GetLastError() != ERROR_SUCCESS)
	{
		printf("Opening file Error: %d\n", GetLastError());
		CloseHandle(f);
		return 0;
	}

	char* sign = (char*)malloc(8);
	READP(sign, 8);

	if (!check_asura_sign(sign))
	{
		CloseHandle(f);
		return 0;
	}

	DWORD File_size = GetFileSize(f, NULL);

	char new_path[260] = { 0 };
	strcpy(new_path, File_name);
	char* extension_str = strchr(new_path, '.');
	strcpy(extension_str, "_stripped.asr");
	printf("%s\n", new_path);

	HANDLE f2 = CreateFileA(new_path, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if ((GetLastError() != ERROR_SUCCESS) && (GetLastError() != ERROR_ALREADY_EXISTS))
	{
		printf("Create file Error: %d\n", GetLastError());
		CloseHandle(f);
		return 0;
	}

	WRITEP(sign, 8);

	free(sign);

	bool b_has_chunk = 0;
	char tmp_str[260] = { 0 };

	do
	{
		if (Processed_bytes + 16 >= File_size)
			break;

		Print_procentage(Processed_bytes, File_size);
		// Chunk read here
		Chunk_header hdr;
		READ(hdr);
		
		// Check if it chunk
		b_has_chunk = (hdr.chunk_name == 0) ? 0 : 1;

		if (!b_has_chunk)
			break;

		DWORD RSCF_type1, RSCF_type2, RSCF_resrc_size = 0;


		if (hdr.chunk_name == 'FCSR')
		{
			READ(RSCF_type1);
			READ(RSCF_type2);
			READ(RSCF_resrc_size);
			READP(tmp_str, hdr.chunk_size - RSCF_resrc_size - 28);
			if (RSCF_type1 == 2 || ((RSCF_type1 == 0) && (RSCF_type2 == 0xF)))
			{
				if (chunk_in_skip_group(hdr.chunk_name, tmp_str))
				{
					SFPC(RSCF_resrc_size);
					continue;
				}
			}

			// Code that copyes chunk into new file here
			WRITE(hdr);
			WRITE(RSCF_type1);
			WRITE(RSCF_type2);
			WRITE(RSCF_resrc_size);
			WRITEP(tmp_str, hdr.chunk_size - RSCF_resrc_size - 28);

			void* mem2 = malloc(RSCF_resrc_size); // Potentialy unoptimized way to do that
			READP(mem2, RSCF_resrc_size);
			WRITEP(mem2, RSCF_resrc_size);
			free(mem2);

			Processed_bytes += hdr.chunk_size;

			continue;
		}

		if (hdr.chunk_name == 'ERAM')
		{
			WRITE(hdr);
			MARE_ENTRY* mem = (MARE_ENTRY*)malloc(hdr.chunk_size - sizeof(hdr) - 4); // Potentialy unoptimized way to do that
			int count = 0;

			READ(count);
			READP(mem, hdr.chunk_size - sizeof(hdr) - 4);

			printf("MARE %d count \n", count);

			//Reparse MARE

			MARE_ENTRY* save_mem = (MARE_ENTRY*)malloc(hdr.chunk_size - sizeof(hdr) - 4);
			ZeroMemory(save_mem, hdr.chunk_size - sizeof(hdr) - 4);

			int saved_materials = 0;

			for (int i = 0; i < count; i++)
			{
				DWORD hash1 = mem[i].Hash1;
				DWORD hash2 = mem[i].Hash2;
				DWORD hash3 = mem[i].Hash3;
				DWORD hash4 = mem[i].Hash4;
				DWORD hash5 = mem[i].Hash5;
				
				if (save_resource_by_hash(hash1) ||
					save_resource_by_hash(hash2) ||
					save_resource_by_hash(hash3) ||
					save_resource_by_hash(hash4) ||
					save_resource_by_hash(hash5))
				{
					memcpy(&(save_mem[saved_materials]), &(mem[i]), sizeof(MARE_ENTRY));
					saved_materials++;
				}

			}

			WRITE(saved_materials);
			WRITEP(save_mem, hdr.chunk_size - sizeof(hdr) - 4);
			free(mem);
			free(save_mem);

			Processed_bytes += hdr.chunk_size;

			continue;
		}

		if (chunk_in_skip_group(hdr.chunk_name, nullptr))
		{
			SFPC(hdr.chunk_size - sizeof(hdr));
			continue;
		}

		WRITE(hdr);
		void* mem = malloc(hdr.chunk_size); // Potentialy unoptimized way to do that
		READP(mem, hdr.chunk_size - sizeof(hdr));
		WRITEP(mem, hdr.chunk_size - sizeof(hdr));
		free(mem);

		Processed_bytes += hdr.chunk_size;

	} while (b_has_chunk);
	
	CloseHandle(f);
	CloseHandle(f2);
	return 1;
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf(" Usage: \n program asura_arch.asr\n");
		return 0;
	}

	//Init hashes
	for (int i = 0; i < g_amount_resources_to_save; i++)
	{
		g_hashes[i] = hash_from_str(0, (char*)g_save_resources[i]);
	}

	if (!Rebuild_arch(argv[1]))
	{
		printf(" Something went wrong ! \n");
		return 0;
	}


	//system("PAUSE");
	return 0;
}