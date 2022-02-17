#include <stdio.h>
#include <vector>
#include <Windows.h>

//Defines
#define ORIGINAL_SPAWNER_VTBL_OFFSET 0x52DFE0

// Structs
struct Vector3f
{
	float x;
	float y;
	float z;
};

struct Vector4f
{
	float x;
	float y;
	float z;
	float w;
};

struct __declspec(align(4)) Temp_EI
{
	DWORD action_id;
	DWORD seq_id;
	DWORD Flags;
	DWORD action;
	__int16 action1;
	__int16 count;
	DWORD Size;
	void *pData;
	DWORD flags;
};

struct dmg_action
{
	DWORD unk1;
	DWORD unk2;
	float base_hp;
	DWORD unk3;
	DWORD unk4;
	DWORD unk5;
	DWORD inflictor_seq_id;
	DWORD unk_hash;
	float dmg_amount;
};

struct Base_entity_mbrs
{
	DWORD seq_id;
	DWORD ent_id;
	DWORD Flags;
	DWORD Hash1;
	DWORD Hash2;
	DWORD Count;
	void *pData;
	DWORD AnotherFlags;
};

struct Basic_Waypoint_mbrs
{
	DWORD some_flags;
	Vector3f pos;
	DWORD unk;
};

struct Basic_Waypoint_with_rot_mbrs
{
	DWORD packed_vec4;
};

struct Spawner_8039_mini_class_mbrs
{
	DWORD unk_num;
	DWORD unk_num2;
	DWORD unk_num3;
	float min_time;
	float max_time;
	DWORD amt_spawn_allowed;
	DWORD *p_arr_12b;
	DWORD id_starting_waypoint;
	DWORD unk_seq_id_2;
	float timer;
	DWORD unk_counter;
	DWORD amt_something2;
	DWORD unk7;
	DWORD *p_arr_4b_seq_ids;
};

struct Spawner_8039_mini_class
{
	DWORD* vtbl2;
	Spawner_8039_mini_class_mbrs _;
};

struct Spawner_8039_waypoint_class
{
	DWORD* vtbl;
	Base_entity_mbrs BE;
	Basic_Waypoint_mbrs Waypoint;
	Basic_Waypoint_with_rot_mbrs Waypoint_;
	Spawner_8039_mini_class _;
};

struct Spawner_8038_spawn_data2 {
	DWORD seq_id;
	DWORD class_hash;
	DWORD subclass_hash;
	DWORD unk;
	DWORD unk2;
};

//Typedefs

typedef int(__cdecl *_PRINTF_CONSOLE)(const wchar_t* fmt, ...);
typedef bool(__cdecl *_CREATE_CONSOLE_CMD)(const wchar_t* name, DWORD unk, DWORD pFunc, DWORD unk2, DWORD access_lvl);
typedef void(__cdecl *_EXECUTE_SMSG)(int, int, float);
typedef Temp_EI*(__cdecl *_ALLOC_ACT)(__int16, size_t, __int16, void*);
typedef void(__cdecl *_EXEC_ACT)(Temp_EI*, int, char);
typedef void* (__thiscall *_SPAWNER_CONSTRUCTOR)(Spawner_8039_waypoint_class*, int);
typedef DWORD(__thiscall *_SPAWN_FUNC)(Spawner_8039_mini_class*, DWORD, Spawner_8038_spawn_data2*);

//Globals
_CREATE_CONSOLE_CMD g_create_console_cmd;
_EXECUTE_SMSG g_exec_smsg;
_ALLOC_ACT g_allocate_EI;
_EXEC_ACT g_execute_action;
_SPAWNER_CONSTRUCTOR g_spawner_constructor;

_PRINTF_CONSOLE g_ingame_printf;
extern uintptr_t g_engine_base;
extern bool g_exit;

Spawner_8039_waypoint_class* p_spawner;

Vector3f g_player_pos;
Vector3f g_player_orient;
DWORD g_ptr_to_func1 = 0x40000;
float g_spawn_lenght = 5.0f;
bool g_takedmg_toggled = true;
bool g_useless_variable;

std::vector<DWORD> g_unactive_seq_ids;

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

void entity_call_action(int act_id, int enti_seq_id, int invoker_seq_id, void* pData, size_t size)
{
	Temp_EI* TEI = g_allocate_EI(act_id, size, 0, 0);
	if (TEI)
	{
		TEI->action_id = enti_seq_id;
		TEI->seq_id = invoker_seq_id;
		if (size && pData)
		{
			memcpy(TEI->pData, pData, size);
		}
		g_execute_action(TEI, 1, 1);
	}

}

bool __cdecl entity_call_action_cmd(wchar_t *arg, bool no_arg, bool get_description)
{
	if (get_description || no_arg)
	{
		g_ingame_printf(L"act (act_id) (seq_id) \n");
		return 1;
	}
	if (arg)
	{
		int act_id = 0;
		int seq_id = 0;
		if (swscanf_s(arg, L"%d %d", &act_id, &seq_id) == 2)
		{
			g_ingame_printf(L"act %d %d\n", act_id, seq_id);
			entity_call_action(act_id, seq_id, 0, nullptr, 0);
			return 1;
		}
		g_ingame_printf(L"Incorrect arg \n");
		return 1;
	}
	return 0;
}

void call_smsg(int smsg_id, int invoker_id)
{
	g_exec_smsg(smsg_id, invoker_id, 0.0f);
}

bool __cdecl call_smsg_cmd(wchar_t *arg, bool no_arg, bool get_description)
{
	if (get_description || no_arg)
	{
		g_ingame_printf(L"SMSG (SMSG number) \n");
		return 1;
	}
	if (arg)
	{
		int smsg_id = 0;
		if (swscanf_s(arg, L"%d", &smsg_id))
		{
			g_ingame_printf(L"SMSG %d \n", smsg_id);
			call_smsg(smsg_id, 0);
		}

		return 1;
	}
	return 0;
}

bool __cdecl test_cmd(wchar_t *arg, bool no_arg, bool get_description)
{
	//printf("I am a test cmd call me to test your injection work properly\n");
	if (get_description)
	{
		g_ingame_printf(L"I am a \"Hello world !\" func\n");
		return 1;
	}
	if (no_arg)
	{
		g_ingame_printf(L"Hello world !\n");
		return 1;
	}
	if (arg)
	{
		g_ingame_printf(L"Arg is %s \n", arg);
		return 1;
	}
	return 0;
}

bool __cdecl dmg_entity(wchar_t *arg, bool no_arg, bool get_description)
{
	if (get_description || no_arg)
	{
		g_ingame_printf(L"dmg_ent (seq_id) (amount)\n");
		return 1;
	}
	if (arg)
	{
		float dmg_amount = 0.0f;
		int seq_id = 0;
		if (swscanf_s(arg, L"%d %f", &seq_id, &dmg_amount) == 2)
		{

			Temp_EI* TEI = g_allocate_EI(0x8055, 36, 0, 0);
			if (TEI)
			{
				TEI->action_id = seq_id;
				TEI->seq_id = 0;
				if (TEI->pData)
				{
					DWORD invoker_seq_id = 0;
					dmg_action dmg_act = { 2, 0, 100.0f, 1, 2, 1, invoker_seq_id, 0x1b2be34, dmg_amount };
					memcpy(TEI->pData, &dmg_act, 36);
				}
				g_execute_action(TEI, 1, 1);
				return 1;
			}
			g_ingame_printf(L"Unallocated TEI \n");
		}
		g_ingame_printf(L"Incorrect arg \n");
		return 1;
	}
	return 0;
}

bool recreate_spawner()
{
	if (g_spawner_constructor(p_spawner, 999))
	{
		printf("spawner created :) addr: %x\n", p_spawner);
		p_spawner->_._.min_time = 0.0f;
		p_spawner->_._.max_time = 1.0f;

		std::vector<DWORD>().swap(g_unactive_seq_ids);
		//g_unactive_seq_ids.swap()

		return true;
	}
	else
	{
		printf("spawner not created :(\n");
		p_spawner = nullptr;
		std::vector<DWORD>().swap(g_unactive_seq_ids);
		return false;
	}
}

// return seq_id
DWORD call_spawn_8039_entity_func(Vector3f pos, DWORD hash)
{
	if (p_spawner)
	{

		if ((DWORD)p_spawner->vtbl != (g_engine_base + ORIGINAL_SPAWNER_VTBL_OFFSET))
		{
			bool res = recreate_spawner();
			if (!res) return 999;
		}

		p_spawner->Waypoint.pos = pos;
		Spawner_8038_spawn_data2 spawn_data;
		spawn_data.class_hash = 0x766161D2;
		spawn_data.subclass_hash = hash;
		spawn_data.seq_id = 999;
		spawn_data.unk = 999;
		spawn_data.unk2 = 0;

		DWORD seq_id = ((_SPAWN_FUNC)(*(p_spawner->_.vtbl2 + 3)))(&(p_spawner->_), p_spawner->BE.seq_id, &spawn_data);
		return seq_id;
	}
	return 999;
}

bool __cdecl set_spawn_lenght(wchar_t *arg, bool no_arg, bool get_description)
{
	//printf("I am a test cmd call me to test your injection work properly\n");
	if (get_description || no_arg)
	{
		g_ingame_printf(L"sets spawn lenght\n");
		return 1;
	}
	if (arg)
	{
		//g_ingame_printf(L"Arg is %s \n", arg);

		while (iswspace(*arg)) arg++;

		float len;

		if (swscanf_s(arg, L"%f", &len) != 1)
		{
			g_ingame_printf(L"Incorrect number?\n");
			return 0;
		}

		if (len >= 100.0f || len < 0.5f)
		{
			g_ingame_printf(L"Limits is [0.5 .. 100.0]\n");
			return 0;
		}

		g_spawn_lenght = len;

		return 1;
	}
	return 0;
}

void refresh_player_pos()
{
	//if ((g_engine_base + 0x5E9d60))
	//{
	//	if (*(DWORD*)(*(DWORD*)(g_engine_base + 0x5E9d60) + 0xC) == NULL) {
	//		g_player_pos_addr = nullptr;
	//		g_player_orient_addr = nullptr;
	//		return;
	//	}
	//
	//
	//	g_player_pos_addr = (Vector3f*)(*(DWORD*)(*(DWORD*)(g_engine_base + 0x5E9d60) + 0xC) + 0x60);
	//	g_player_orient_addr = (Vector3f*)(*(DWORD*)(*(DWORD*)(g_engine_base + 0x5E9d60) + 0xC) + 0x50);
	//
	//	printf("player pos: %f %f %f \n", g_player_pos_addr->x, g_player_pos_addr->y, g_player_pos_addr->z);
	//	return;
	//}

	//g_player_pos_addr = nullptr;
	//g_player_orient_addr = nullptr;
	//return;

	g_player_pos = *(Vector3f*)(g_engine_base + 0x5ff600);
	Vector4f q = *(Vector4f*)(g_engine_base + 0x5ff60C);

	//not working quaternion magick
	g_player_orient = { -2 * q.x*q.z + 2 * q.y*q.w, -2 * q.y*q.z + 2 * q.x*q.w, -1 + 2 * (q.x*q.x + q.y*q.y) };

	printf("player pos: %f %f %f \ncosines: %f %f %f \n", g_player_pos.x, g_player_pos.y, g_player_pos.z, 
		g_player_orient.x, g_player_orient.y, g_player_orient.z);

}

Vector3f get_spawn_position()
{
	Vector3f vec = { 0, 0, 0 };

	refresh_player_pos();

	//if (!g_player_pos_addr || !g_player_orient_addr)
	//	return vec;

	// get player actor pos + looking vector?
	// no collision check yet
	Vector3f new_pos = {
		g_player_pos.x + g_player_orient.x * g_spawn_lenght,
		g_player_pos.y + g_player_orient.y * g_spawn_lenght,
		g_player_pos.z + g_player_orient.z * g_spawn_lenght
	};

	printf("spawn_pos: %f %f %f \n", new_pos.x, new_pos.y, new_pos.z);

	return new_pos;
}

DWORD actor_hash_from_arg(wchar_t *arg, UINT limit)
{

	wchar_t *end_p = arg;

	
	if (*arg == L'\"')
	{
		end_p++;
		while (end_p - arg < limit) { if (*end_p == L'\"') break; end_p++; }
		char actor_name[256] = { 0 };

		end_p--;
		

		UINT name_size = end_p - arg;
		for (int i = 0; i < name_size; i++)
			actor_name[i] = ((char*)(arg + 1))[2 * i];

		actor_name[name_size] = 0;

		return hash_from_str(0, actor_name);
	}
}

bool __cdecl spawn_cmd(wchar_t *arg, bool no_arg, bool get_description)
{
	//printf("I am a test cmd call me to test your injection work properly\n");
	if (get_description || no_arg)
	{
		g_ingame_printf(L"Spawn hash(0x12345678) or Spawn ActorName\n");
		return 1;
	}
	if (arg)
	{
		g_ingame_printf(L"Arg is %s \n", arg);

		while (iswspace(*arg)) arg++;

		DWORD actor_hash = 0;

		//it is hash?
		if (*arg == L'0' && *(arg + 1) == L'x')
		{
			if (swscanf_s(arg + 2, L"%x", &actor_hash) != 1)
			{
				g_ingame_printf(L"Incorrect number?\n");
				return 0;
			}


		} else if (*arg == L'\"')
		{
			actor_hash = actor_hash_from_arg(arg, 256);  
		} else
		{
			g_ingame_printf(L"Incorrect arg\n");
			return 0;
		}

		g_ingame_printf(L"Hash: %X\n", actor_hash);

		DWORD seq_id = call_spawn_8039_entity_func(get_spawn_position(), actor_hash);
		if (seq_id == 999)
		{
			g_ingame_printf(L"Not spawned unlucky\n");
		}
		else
		{
			g_ingame_printf(L"Spawned seq_id = %d\n", seq_id);
		}
		return 1;
	}
	return 0;
}

bool __cdecl spawn_unactive_cmd(wchar_t *arg, bool no_arg, bool get_description)
{
	//printf("I am a test cmd call me to test your injection work properly\n");
	if (get_description || no_arg)
	{
		g_ingame_printf(L"spawn_unactive hash(0x12345678) or spawn_unactive ActorName\n");
		return 1;
	}
	if (arg)
	{
		g_ingame_printf(L"Arg is %s \n", arg);

		while (iswspace(*arg)) arg++;

		DWORD actor_hash = 0;

		//it is hash?
		if (*arg == L'0' && *(arg + 1) == L'x')
		{
			if (swscanf_s(arg + 2, L"%x", &actor_hash) != 1)
			{
				g_ingame_printf(L"Incorrect number?\n");
				return 0;
			}


		}
		else if (*arg == L'\"')
		{
			actor_hash = actor_hash_from_arg(arg, 256);
		}
		else
		{
			g_ingame_printf(L"Incorrect arg\n");
			return 0;
		}

		g_ingame_printf(L"Hash: %X\n", actor_hash);

		DWORD seq_id = call_spawn_8039_entity_func(get_spawn_position(), actor_hash);
		if (seq_id == 999)
		{
			g_ingame_printf(L"Not spawned unlucky\n");
		}
		else
		{
			g_ingame_printf(L"Spawned seq_id = %d\n", seq_id);
			entity_call_action(4, seq_id, 0, nullptr, 0);
			g_unactive_seq_ids.push_back(seq_id);
		}
		return 1;
	}
	return 0;
}

bool __cdecl spawn_activate_cmd(wchar_t *arg, bool no_arg, bool get_description)
{
	//printf("I am a test cmd call me to test your injection work properly\n");
	if (get_description)
	{
		g_ingame_printf(L"spawn_activate");
		return 1;
	}

	for (DWORD seq_id : g_unactive_seq_ids)
	{
		entity_call_action(3, seq_id, 0, nullptr, 0);
	}

	std::vector<DWORD>().swap(g_unactive_seq_ids);

	return 1;
}

bool __cdecl toggle_invincible_cmd(wchar_t *arg, bool no_arg, bool get_description)
{
	//printf("I am a test cmd call me to test your injection work properly\n");
	if (get_description)
	{
		g_ingame_printf(L"on/off invincibility");
		return 1;
	}

	g_takedmg_toggled = !g_takedmg_toggled;

	g_ingame_printf(L"you now %s\n", g_takedmg_toggled ? L"not invincible" : L"invincible");

	int act_id = 0x26; // toggle can take dmg
	int seq_id = 10000000; // player have constant id

	entity_call_action(act_id, seq_id, 0, &g_takedmg_toggled, 1);

	return 1;
}

bool __cdecl toggle_invincible_entity_cmd(wchar_t *arg, bool no_arg, bool get_description)
{
	//printf("I am a test cmd call me to test your injection work properly\n");
	if (get_description)
	{
		g_ingame_printf(L"invincible_ent seq_id 1/0");
		return 1;
	}

	int toggle = 0;
	int seq_id = 0;
	if (swscanf_s(arg, L"%d %d", &seq_id, &toggle) == 2)
	{

		int act_id = 0x26; // toggle can take dmg
		g_useless_variable = toggle;
		entity_call_action(act_id, seq_id, 0, &g_useless_variable, 1);

		return 1;
	}

	g_ingame_printf(L"Incorrect args\n");
	return 0;
}

bool __cdecl attack_entity_cmd(wchar_t *arg, bool no_arg, bool get_description)
{
	if (get_description)
	{
		g_ingame_printf(L"attack_target seq_id target_seq_id type");
		return 1;
	}

	int seq_id = 0;
	int target_seq_id = 0;
	int type = 0;
	if (swscanf_s(arg, L"%d %d %d", &seq_id, &target_seq_id, &type) == 3)
	{

		int act_id = 0x80E4; // attack action?
		
		struct attack_data_struct {
			DWORD type;
			DWORD seq_id_target;
		};

		attack_data_struct* ds = new attack_data_struct;

		ds->seq_id_target = target_seq_id;
		ds->type = type;
		
		entity_call_action(act_id, seq_id, seq_id, ds, 8);

		delete ds;

		return 1;
	}

	g_ingame_printf(L"Incorrect args\n");
	return 0;
}

void make_function(const wchar_t* name, DWORD func_ptr)
{
	g_ptr_to_func1 = (DWORD)func_ptr;

	if (g_create_console_cmd(name, 6, (DWORD)(&g_ptr_to_func1), 0, -1))
	{
		printf("Succesfull load %ws console command at addr: %x\n", name, func_ptr);
	}
}

// must be ingame while injecting?
bool Init_console()
{



	if (!*(unsigned char*)(g_engine_base + 0x666850))
	{
		printf("You not in game \n");
		g_exit = true;
		return 1;
	}

	// ultra debugging session for 10hrs required!!!

	g_ingame_printf = (_PRINTF_CONSOLE)(g_engine_base + 0x18A7D0);
	g_create_console_cmd = (_CREATE_CONSOLE_CMD)(g_engine_base + 0x18C250);
	g_exec_smsg = (_EXECUTE_SMSG)(g_engine_base + 0x1283B0);
	g_allocate_EI = (_ALLOC_ACT)(g_engine_base + 0x19D810);
	g_execute_action = (_EXEC_ACT)(g_engine_base + 0x19E340);
	g_spawner_constructor = (_SPAWNER_CONSTRUCTOR)(g_engine_base + 0x367b80);


	refresh_player_pos();
	

	p_spawner = (Spawner_8039_waypoint_class*)malloc(sizeof(Spawner_8039_waypoint_class));
	memset(p_spawner, 0, sizeof(Spawner_8039_waypoint_class));

	recreate_spawner();
	

	printf("g_ingame_printf found at addr: %x\n", g_ingame_printf);
	printf("g_create_console_cmd found at addr: %x\n", g_create_console_cmd);
	printf("g_exec_smsg found at addr: %x\n", g_exec_smsg);
	printf("g_allocate_EI found at addr: %x\n", g_allocate_EI);
	printf("g_execute_action found at addr: %x\n", g_execute_action);



	make_function(L"Test_command", (DWORD)&test_cmd);
	make_function(L"smsg", (DWORD)&call_smsg_cmd);
	make_function(L"act", (DWORD)&entity_call_action_cmd);
	make_function(L"spawn", (DWORD)&spawn_cmd);
	make_function(L"spawn_set_len", (DWORD)&set_spawn_lenght);
	make_function(L"spawn_unactive", (DWORD)&spawn_unactive_cmd);
	make_function(L"spawn_activate", (DWORD)&spawn_activate_cmd);
	make_function(L"invincible", (DWORD)&toggle_invincible_cmd);
	make_function(L"invincible_ent", (DWORD)&toggle_invincible_entity_cmd);
	make_function(L"attack_target", (DWORD)&attack_entity_cmd);

	printf("call_spawn_8039_entity_func: %x\n", &call_spawn_8039_entity_func);

	// that crashes.

	//g_ptr_to_func1 = (DWORD)&dmg_entity;
	//
	//if (g_create_console_cmd(L"dmg_ent", 6, (DWORD)(&dmg_entity), 0, -1))
	//{
	//	printf("Succesfull load smsg cmd at addr: %x\n", &dmg_entity);
	//}



	return 0;
}