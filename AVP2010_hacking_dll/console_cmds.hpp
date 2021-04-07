#include <stdio.h>
#include <vector>
#include <Windows.h>

// Structs
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
//Typedefs

typedef int(__cdecl *_PRINTF_CONSOLE)(const wchar_t* fmt, ...);
typedef bool(__cdecl *_CREATE_CONSOLE_CMD)(const wchar_t* name, DWORD unk, DWORD pFunc, DWORD unk2, DWORD access_lvl);
typedef void(__cdecl *_EXECUTE_SMSG)(int, int, float);
typedef Temp_EI*(__cdecl *_ALLOC_ACT)(__int16, size_t, __int16, void*);
typedef void(__cdecl *_EXEC_ACT)(Temp_EI*, int, char);

//Globals
_CREATE_CONSOLE_CMD g_create_console_cmd;
_EXECUTE_SMSG g_exec_smsg;
_ALLOC_ACT g_allocate_EI;
_EXEC_ACT g_execute_action;

_PRINTF_CONSOLE g_ingame_printf;
extern uintptr_t g_engine_base;



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

DWORD g_ptr_to_func1 = 0x40000;

bool Init_console()
{
	g_ingame_printf = (_PRINTF_CONSOLE)(g_engine_base + 0x18A7D0);
	g_create_console_cmd = (_CREATE_CONSOLE_CMD)(g_engine_base + 0x18C250);
	g_exec_smsg = (_EXECUTE_SMSG)(g_engine_base + 0x1283B0);
	g_allocate_EI = (_ALLOC_ACT)(g_engine_base + 0x19D810);
	g_execute_action = (_EXEC_ACT)(g_engine_base + 0x19E340);
	printf("g_ingame_printf found at addr: %x\n", g_ingame_printf);
	printf("g_create_console_cmd found at addr: %x\n", g_create_console_cmd);
	printf("g_exec_smsg found at addr: %x\n", g_exec_smsg);
	printf("g_allocate_EI found at addr: %x\n", g_allocate_EI);
	printf("g_execute_action found at addr: %x\n", g_execute_action);



	g_ptr_to_func1 = (DWORD)&test_cmd;

	if (g_create_console_cmd(L"Test_command", 6, (DWORD)(&g_ptr_to_func1), 0, -1))
	{
		printf("Succesfull load test console command at addr: %x\n", &test_cmd);
	}

	g_ptr_to_func1 = (DWORD)&call_smsg_cmd;

	if (g_create_console_cmd(L"smsg", 6, (DWORD)(&g_ptr_to_func1), 0, -1))
	{
		printf("Succesfull load smsg cmd at addr: %x\n", &call_smsg_cmd);
	}

	g_ptr_to_func1 = (DWORD)&entity_call_action_cmd;

	if (g_create_console_cmd(L"act", 6, (DWORD)(&g_ptr_to_func1), 0, -1))
	{
		printf("Succesfull load smsg cmd at addr: %x\n", &entity_call_action_cmd);
	}

	g_ptr_to_func1 = (DWORD)&dmg_entity;

	if (g_create_console_cmd(L"dmg_ent", 6, (DWORD)(&dmg_entity), 0, -1))
	{
		printf("Succesfull load smsg cmd at addr: %x\n", &dmg_entity);
	}

	return 0;
}