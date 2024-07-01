#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Psapi.h>

#include "detours/detours.h"

#define PLUGIN_NAME "[Custom Items Game] "

#define CUSTOM_ITEMS_GAME "scripts/items/items_game_custom.txt"
#define CUSTOM_ITEMS_GAME_SIG CUSTOM_ITEMS_GAME ".sig"

#define INTERFACEVERSION_IPLUGIN "ISERVERPLUGINCALLBACKS003"
#define BASEFILESYSTEM_INTERFACE_VERSION "VBaseFileSystem011"

#define SIG(x) Sig({#x, sizeof(#x) - 1})
#define SIG_WILDCARD 0x2A

struct Sig { const char* sig; size_t len; };

class edict_t;
class CCommand;
enum EQueryCvarValueStatus;
typedef void* (*CreateInterfaceFn)(const char* pName, int* pReturnCode);
typedef int QueryCvarCookie_t;
enum PluginResult { PLUGIN_CONTINUE, PLUGIN_OVERRIDE, PLUGIN_STOP };
enum InterfaceResult { IFACE_OK, IFACE_FAILED };

typedef void* FileHandle_t;
enum FileSystemSeek_t { FILESYSTEM_SEEK_HEAD, FILESYSTEM_SEEK_CURRENT, FILESYSTEM_SEEK_TAIL };

struct Color { unsigned char r, g, b, a; };
typedef void (*ConColorMsg)(const Color& clr, const char* msg, ...);
ConColorMsg Log = nullptr;

struct CPlugin
{
	virtual bool			Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
	virtual void			Unload(void);
	virtual void			Pause(void) {}
	virtual void			UnPause(void) {}
	virtual const char* GetPluginDescription(void) { return "Custom Items Game"; }
	virtual void			LevelInit(char const* pMapName) {}
	virtual void			ServerActivate(edict_t* pEdictList, int edictCount, int clientMax) {}
	virtual void			GameFrame(bool simulating) {}
	virtual void			LevelShutdown(void) {}
	virtual void			ClientActive(edict_t* pEntity) {}
	virtual void			ClientDisconnect(edict_t* pEntity) {}
	virtual void			ClientPutInServer(edict_t* pEntity, char const* playername) {}
	virtual void			SetCommandClient(int index) {}
	virtual void			ClientSettingsChanged(edict_t* pEdict) {}
	virtual PluginResult	ClientConnect(bool* bAllowConnect, edict_t* pEntity, const char* pszName, const char* pszAddress, char* reject, int maxrejectlen) { return PLUGIN_CONTINUE; }
	virtual PluginResult	ClientCommand(edict_t* pEntity, const CCommand& args) { return PLUGIN_CONTINUE; }
	virtual PluginResult	NetworkIDValidated(const char* pszUserName, const char* pszNetworkID) { return PLUGIN_CONTINUE; }
	virtual void			OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t* pPlayerEntity, EQueryCvarValueStatus eStatus, const char* pCvarName, const char* pCvarValue) {}
	virtual void			OnEdictAllocated(edict_t* edict) {}
	virtual void			OnEdictFreed(const edict_t* edict) {}
} plugin;

struct IBaseFileSystem
{
	virtual int				Read(void* pOutput, int size, FileHandle_t file) = 0;
	virtual int				Write(void const* pInput, int size, FileHandle_t file) = 0;
	virtual FileHandle_t	Open(const char* pFileName, const char* pOptions, const char* pathID = 0) = 0;
	virtual void			Close(FileHandle_t file) = 0;
	virtual void			Seek(FileHandle_t file, int pos, FileSystemSeek_t seekType) = 0;
	virtual unsigned int	Tell(FileHandle_t file) = 0;
	virtual unsigned int	Size(FileHandle_t file) = 0;
	virtual unsigned int	Size(const char* pFileName, const char* pPathID = 0) = 0;
	virtual void			Flush(FileHandle_t file) = 0;
	virtual bool			Precache(const char* pFileName, const char* pPathID = 0) = 0;
	virtual bool			FileExists(const char* pFileName, const char* pPathID = 0) = 0;
} *filesystem = NULL;

extern "C" __declspec(dllexport) void* CreateInterface(const char* pName, int* pReturnCode)
{
	if (!strcmp(INTERFACEVERSION_IPLUGIN, pName))
	{
		if (pReturnCode) *pReturnCode = IFACE_OK;
		return &plugin;
	}
	if (pReturnCode) *pReturnCode = IFACE_FAILED;
	return NULL;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return TRUE;
}

static bool FindLog()
{
	HMODULE Handle = LoadLibrary("tier0.dll");
	if (!Handle)
		return false;
#ifdef _WIN64
	Log = (ConColorMsg)GetProcAddress(Handle, "?ConColorMsg@@YAXAEBVColor@@PEBDZZ");
#else
	Log = (ConColorMsg)GetProcAddress(Handle, "?ConColorMsg@@YAXABVColor@@PBDZZ");
#endif
	FreeLibrary(Handle);
	return Log != NULL;
}

static void* FindSignature(const MODULEINFO& info, const Sig& sig)
{
	uintptr_t start = (uintptr_t)info.lpBaseOfDll;
	uintptr_t end = start + info.SizeOfImage - sig.len;

	for (uintptr_t i = start; i < end; i++)
	{
		bool found = true;
		for (size_t j = 0; j < sig.len; j++)
			found &= (sig.sig[j] == SIG_WILDCARD) || (sig.sig[j] == *(char*)(i + j));
		if (found)
			return (void*)i;
	}

	return NULL;
}

static bool IsDemoBranch()
{
	FileHandle_t f = filesystem->Open("steam.inf", "r");
	if (!f)
	{
		Log({ 255, 0, 0, 255 }, PLUGIN_NAME "Failed to open steam.inf\n");
		return false;
	}

	bool demo_branch = false;
	bool patch_version = false;

	char buffer[256];
	filesystem->Read(buffer, sizeof(buffer), f);

	char* p = strtok(buffer, "=\r\n");
	while (p)
	{
		if (patch_version)
		{
			if (atoi(p) <= 8207200)
				demo_branch = true;
			break;
		}

		if (!strcmp(p, "PatchVersion"))
			patch_version = true;

		p = strtok(NULL, "=\r\n");
	}

	filesystem->Close(f);
	return demo_branch;
}

static bool CheckCustomItemsGame()
{
	static bool foundCustom = true, check = true;
	if (check)
	{
		check = false;
		if (!filesystem->FileExists(CUSTOM_ITEMS_GAME))
		{
			Log({ 255, 0, 127, 255 },
				PLUGIN_NAME "Server: %s not found, loading default items_game.txt ...\n", CUSTOM_ITEMS_GAME);
			foundCustom = false;
		}
		if (!filesystem->FileExists(CUSTOM_ITEMS_GAME_SIG))
		{
			Log({ 255, 0, 127, 255 },
				PLUGIN_NAME "Server: %s not found, loading default items_game.txt ...\n", CUSTOM_ITEMS_GAME_SIG);
			foundCustom = false;
		}
	}
	return foundCustom;
}

typedef void* (*func_econItemSystem)();
func_econItemSystem server_econItemSystem = NULL;

typedef bool (*func_crypto_verifySignature)(uint8_t*, uint8_t, uint8_t*, uint8_t, uint8_t*, uint8_t*);
func_crypto_verifySignature client_crypto_verifySignature = NULL;
func_crypto_verifySignature server_crypto_verifySignature = NULL;
bool hook_crypto_verifySignature(uint8_t*, uint8_t, uint8_t*, uint8_t, uint8_t*, uint8_t*)
{
	return true;
}

#ifdef _WIN64
typedef void(__fastcall* func_econItemSchema_init)(void*, const char*, const char*, void*);
func_econItemSchema_init client_econItemSchema_init = NULL;
func_econItemSchema_init server_econItemSchema_init = NULL;
void __fastcall hook_client_econItemSchema_init(void* thisptr, const char* filename, const char* pathid, void* errors)
{
	if (CheckCustomItemsGame())
		filename = CUSTOM_ITEMS_GAME;
	client_econItemSchema_init(thisptr, filename, pathid, errors);
}
void __fastcall hook_server_econItemSchema_init(void* thisptr, const char* filename, const char* pathid, void* errors)
{
	if (CheckCustomItemsGame())
		filename = CUSTOM_ITEMS_GAME;
	server_econItemSchema_init(thisptr, filename, pathid, errors);
}

// filename arg is inlined
typedef void(__fastcall* func_econItemSystem_parseItemSchemaFile)(void*);
func_econItemSystem_parseItemSchemaFile server_econItemSystem_parseItemSchemaFile = NULL;
func_econItemSystem_parseItemSchemaFile client_econItemSystem_parseItemSchemaFile = NULL;
void __fastcall hook_server_econItemSystem_parseItemSchemaFile(void* thisptr)
{
	if (CheckCustomItemsGame())
	{
		Log({ 0, 255, 127, 255 }, PLUGIN_NAME "Server: loading %s...\n", CUSTOM_ITEMS_GAME);
	}
	server_econItemSystem_parseItemSchemaFile(thisptr);
}
void __fastcall hook_client_econItemSystem_parseItemSchemaFile(void* thisptr)
{
	if (CheckCustomItemsGame())
	{
		hook_server_econItemSystem_parseItemSchemaFile(server_econItemSystem());
		Log({ 0, 255, 127, 255 }, PLUGIN_NAME "Client: Loading %s...\n", CUSTOM_ITEMS_GAME);
	}
	client_econItemSystem_parseItemSchemaFile(thisptr);
}

typedef bool(__fastcall* func_gcUpdateItemSchema_runJob)(void*, void*);
func_gcUpdateItemSchema_runJob client_gcUpdateItemSchema_runJob = NULL;
func_gcUpdateItemSchema_runJob server_gcUpdateItemSchema_runJob = NULL;
bool __fastcall hook_client_gcUpdateItemSchema_runJob(void* thisptr, void* packet)
{
	if (CheckCustomItemsGame())
		Log({ 0, 255, 127, 255 }, PLUGIN_NAME "Blocked item schema update from GC\n");
	else if (IsDemoBranch())
		Log({ 0, 255, 127, 255 }, PLUGIN_NAME "Forcefully blocked item schema update from GC (demo branch detected)\n");
	else
		client_gcUpdateItemSchema_runJob(thisptr, packet);
	return true;
}
bool __fastcall hook_server_gcUpdateItemSchema_runJob(void* thisptr, void* packet)
{
	if (CheckCustomItemsGame())
		Log({ 0, 255, 127, 255 }, PLUGIN_NAME "Blocked item schema update from GC\n");
	else if (IsDemoBranch())
		Log({ 0, 255, 127, 255 }, PLUGIN_NAME "Forcefully blocked item schema update from GC (demo branch detected)\n");
	else
		server_gcUpdateItemSchema_runJob(thisptr, packet);
	return true;
}
#else
typedef void(__fastcall* func_econItemSystem_parseItemSchemaFile)(void*, void*, const char*);
func_econItemSystem_parseItemSchemaFile server_econItemSystem_parseItemSchemaFile = NULL;
func_econItemSystem_parseItemSchemaFile client_econItemSystem_parseItemSchemaFile = NULL;
void __fastcall hook_server_econItemSystem_parseItemSchemaFile(void* thisptr, void* edx, const char* filename)
{
	if (CheckCustomItemsGame())
	{
		filename = CUSTOM_ITEMS_GAME;
		Log({ 0, 255, 127, 255 }, PLUGIN_NAME "Server: loading %s...\n", filename);
	}
	server_econItemSystem_parseItemSchemaFile(thisptr, edx, filename);
}
void __fastcall hook_client_econItemSystem_parseItemSchemaFile(void* thisptr, void* edx, const char* filename)
{
	if (CheckCustomItemsGame())
	{
		filename = CUSTOM_ITEMS_GAME;
		Log({ 0, 255, 127, 255 }, PLUGIN_NAME "Client: Loading %s...\n", filename);
		hook_server_econItemSystem_parseItemSchemaFile(server_econItemSystem(), edx, filename);
	}
	client_econItemSystem_parseItemSchemaFile(thisptr, edx, filename);
}

typedef bool(__fastcall* func_gcUpdateItemSchema_runJob)(void*, void*, void*);
func_gcUpdateItemSchema_runJob client_gcUpdateItemSchema_runJob = NULL;
func_gcUpdateItemSchema_runJob server_gcUpdateItemSchema_runJob = NULL;
bool __fastcall hook_client_gcUpdateItemSchema_runJob(void* thisptr, void* edx, void* packet)
{
	if (CheckCustomItemsGame())
		Log({ 0, 255, 127, 255 }, PLUGIN_NAME "Blocked item schema update from GC\n");
	else if (IsDemoBranch())
		Log({ 0, 255, 127, 255 }, PLUGIN_NAME "Forcefully blocked item schema update from GC (demo branch detected)\n");
	else
		client_gcUpdateItemSchema_runJob(thisptr, edx, packet);
	return true;
}
bool __fastcall hook_server_gcUpdateItemSchema_runJob(void* thisptr, void* edx, void* packet)
{
	if (CheckCustomItemsGame())
		Log({ 0, 255, 127, 255 }, PLUGIN_NAME "Blocked item schema update from GC\n");
	else if (IsDemoBranch())
		Log({ 0, 255, 127, 255 }, PLUGIN_NAME "Forcefully blocked item schema update from GC (demo branch detected)\n");
	else
		server_gcUpdateItemSchema_runJob(thisptr, edx, packet);
	return true;
}
#endif

static bool FindSignatures()
{
	Log({ 0, 255, 0, 255 }, PLUGIN_NAME "Finding signatures...\n");

	const char* client_name = "client.dll", *server_name = "server.dll";
	MODULEINFO client_info, server_info;
	HMODULE client, server;
	
	client = GetModuleHandle(client_name);
	if (!client)
	{
		Log({ 255, 0, 0, 255 }, PLUGIN_NAME "Failed to get module info for %s\n", client_name);
		return false;
	}

	server = GetModuleHandle(server_name);
	if (!server)
	{
		Log({ 255, 0, 0, 255 }, PLUGIN_NAME "Failed to get module info for %s\n", server_name);
		return false;
	}

	GetModuleInformation(GetCurrentProcess(), client, &client_info, sizeof(client_info));
	GetModuleInformation(GetCurrentProcess(), server, &server_info, sizeof(server_info));

#ifdef _WIN64
	Sig sig_econItemSystem = SIG(\x48\x83\xEC\x2A\x48\x8B\x05\x2A\x2A\x2A\x2A\x48\x85\xC0\x75\x2A\xB9);
	Sig sig_crypto_verifySignature = SIG(\x48\x89\x5C\x24\x2A\x4C\x89\x44\x24\x2A\x89\x54\x24\x2A\x48\x89\x4C\x24);
	Sig sig_econItemSchema_init = SIG(\x48\x89\x5C\x24\x2A\x48\x89\x74\x24\x2A\x48\x89\x7C\x24\x2A\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x2A\x2A\x2A\x2A\x48\x81\xEC\x2A\x2A\x2A\x2A\x48\x8B\x01\x49\x8B\xD9);
	Sig sig_econItemSystem_parseItemSchemaFile = SIG(\x48\x89\x5C\x24\x2A\x57\x48\x83\xEC\x2A\x48\x8B\x41\x2A\x4C\x8D\x4C\x24);
	Sig sig_gcUpdateItemSchema_runJob = SIG(\x48\x8B\xC4\x48\x83\xEC\x2A\x48\x89\x58\x2A\x48\x89\x68\x2A\x48\x8B\xEA\x48\x89\x70\x2A\x48\x89\x78);
#else
	Sig sig_econItemSystem = SIG(\xA1\x2A\x2A\x2A\x2A\x85\xC0\x75\x2A\x56);
	Sig sig_crypto_verifySignature = SIG(\x55\x8B\xEC\x6A\xFF\x68\x2A\x2A\x2A\x2A\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x81\xEC\xCC\x00\x00\x00);
	Sig sig_econItemSystem_parseItemSchemaFile = SIG(\x55\x8B\xEC\x83\xEC\x14\x8B\x41\x04\x8D\x55\xEC\x83\xC1\x04\xC7\x45\xEC\x00\x00\x00\x00\x52\x68\x2A\x2A\x2A\x2A);
	Sig sig_gcUpdateItemSchema_runJob = SIG(\x55\x8B\xEC\x83\xEC\x1C\x53\x57\x8B\xF9\x8D\x4D\xE4);
#endif

	server_econItemSystem = (func_econItemSystem)FindSignature(server_info, sig_econItemSystem);

	client_crypto_verifySignature = (func_crypto_verifySignature)FindSignature(client_info, sig_crypto_verifySignature); 
	server_crypto_verifySignature = (func_crypto_verifySignature)FindSignature(server_info, sig_crypto_verifySignature);
#ifdef _WIN64
	client_econItemSchema_init = (func_econItemSchema_init)FindSignature(client_info, sig_econItemSchema_init);
	server_econItemSchema_init = (func_econItemSchema_init)FindSignature(server_info, sig_econItemSchema_init);
#endif
	client_econItemSystem_parseItemSchemaFile = (func_econItemSystem_parseItemSchemaFile)FindSignature(client_info, sig_econItemSystem_parseItemSchemaFile);
	server_econItemSystem_parseItemSchemaFile = (func_econItemSystem_parseItemSchemaFile)FindSignature(server_info, sig_econItemSystem_parseItemSchemaFile);
	client_gcUpdateItemSchema_runJob = (func_gcUpdateItemSchema_runJob)FindSignature(client_info, sig_gcUpdateItemSchema_runJob); 
	server_gcUpdateItemSchema_runJob = (func_gcUpdateItemSchema_runJob)FindSignature(server_info, sig_gcUpdateItemSchema_runJob);

	if (!server_econItemSystem
		|| !client_crypto_verifySignature || !server_crypto_verifySignature
#ifdef _WIN64
		|| !client_econItemSchema_init || !server_econItemSchema_init
#endif
		|| !client_econItemSystem_parseItemSchemaFile || !server_econItemSystem_parseItemSchemaFile
		|| !client_gcUpdateItemSchema_runJob || !server_gcUpdateItemSchema_runJob
	)
	{
		Log({ 255, 0, 0, 255 }, PLUGIN_NAME "Failed to find required signatures\n");
		return false;
	}

	return true;
}

static void LoadDetours()
{
	Log({ 0, 255, 0, 255 }, PLUGIN_NAME "Loading detours...\n");

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourAttach(&client_crypto_verifySignature, hook_crypto_verifySignature);
	DetourAttach(&server_crypto_verifySignature, hook_crypto_verifySignature);
#ifdef _WIN64
	DetourAttach(&client_econItemSchema_init, hook_client_econItemSchema_init);
	DetourAttach(&server_econItemSchema_init, hook_server_econItemSchema_init);
#endif
	DetourAttach(&client_econItemSystem_parseItemSchemaFile, hook_client_econItemSystem_parseItemSchemaFile);
	DetourAttach(&server_econItemSystem_parseItemSchemaFile, hook_server_econItemSystem_parseItemSchemaFile);
	DetourAttach(&client_gcUpdateItemSchema_runJob, hook_client_gcUpdateItemSchema_runJob);
	DetourAttach(&server_gcUpdateItemSchema_runJob, hook_server_gcUpdateItemSchema_runJob);

	DetourTransactionCommit();
}

static void UnloadDetours()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourDetach(&client_crypto_verifySignature, hook_crypto_verifySignature);
	DetourDetach(&server_crypto_verifySignature, hook_crypto_verifySignature);
#ifdef _WIN64
	DetourDetach(&client_econItemSchema_init, hook_client_econItemSchema_init);
	DetourDetach(&server_econItemSchema_init, hook_server_econItemSchema_init);
#endif
	DetourDetach(&client_econItemSystem_parseItemSchemaFile, hook_client_econItemSystem_parseItemSchemaFile);
	DetourDetach(&server_econItemSystem_parseItemSchemaFile, hook_server_econItemSystem_parseItemSchemaFile);
	DetourDetach(&client_gcUpdateItemSchema_runJob, hook_client_gcUpdateItemSchema_runJob);
	DetourDetach(&server_gcUpdateItemSchema_runJob, hook_server_gcUpdateItemSchema_runJob);

	DetourTransactionCommit();
}

bool CPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
	if (!FindLog())
		return false;

	filesystem = (IBaseFileSystem*)interfaceFactory(BASEFILESYSTEM_INTERFACE_VERSION, NULL);
	if (!filesystem)
	{
		Log({ 255, 0, 0, 255 }, PLUGIN_NAME "Failed to find filesystem\n");
		return false;
	}

	FindSignatures();

	LoadDetours();

	Log({ 0, 255, 0, 255 }, PLUGIN_NAME "Loaded plugin successfully\n");
	return true;
}

void CPlugin::Unload()
{
	UnloadDetours();
}