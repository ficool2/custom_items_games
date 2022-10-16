#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Psapi.h>
#include "detours.h"

#include "iplugin.h"

#include "helpers.h"
#include "module.h"
#include "address.h"
#include "hook.h"

class CPlugin : public IPlugin
{
public:
	virtual bool			Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
	virtual void			Unload(void);
	virtual void			Pause(void) {}
	virtual void			UnPause(void) {}
	virtual const char*		GetPluginDescription(void) { return "Schema Sig Bypasser"; }
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

	bool					FindLog();
	bool					FindModules();
	bool					FindAddresses();
	void					LoadDetours();
	void					UnloadDetours();
};

CPlugin g_Plugin;

extern "C" __declspec(dllexport) void* CreateInterface(const char* pName, int* pReturnCode)
{ 
	if (!strcmp(INTERFACEVERSION_IPLUGIN, pName))
	{
		if (pReturnCode)
			*pReturnCode = IFACE_OK;
		return static_cast<IPlugin*>(&g_Plugin);
	}

	if (pReturnCode)
		*pReturnCode = IFACE_FAILED;
	return nullptr;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return TRUE;
}

bool CPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
	if (!FindLog())
		return false;

	if (!FindModules())
		return false;

	if (!FindAddresses())
		return false;

	LoadDetours();

	Log(Color(0, 255, 0, 255), "Loaded Schema Sig Bypasser plugin successfully\n");

	return true;
}

void CPlugin::Unload()
{
	UnloadDetours();
}

bool CPlugin::FindLog()
{
	HMODULE Handle = LoadLibrary("tier0.dll");
	if (!Handle)
		return false;

	Log = (LogFunc)GetProcAddress(Handle, "?ConColorMsg@@YAXABVColor@@PBDZZ");
	if (!Log)
		return false;

	FreeLibrary(Handle);
	return true;
}

bool CPlugin::FindModules()
{
	bool failed = false;
	for (ModuleInfo& mod : modules)
		failed |= !mod.Find();
	return !failed;
}

bool CPlugin::FindAddresses()
{
	bool failed = false;
	for (AddressBase* addr : addresses)
		failed |= !addr->Find();
	return !failed;
}

void CPlugin::LoadDetours()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DETOUR_LOAD(crypto_verifySignature);
	DETOUR_LOAD(gcUpdateItemSchema_runJob);
	DETOUR_LOAD_GAME(econItemSystem_parseItemSchemaFile);

	DetourTransactionCommit();
}

void CPlugin::UnloadDetours()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DETOUR_UNLOAD(crypto_verifySignature);
	DETOUR_UNLOAD(gcUpdateItemSchema_runJob);
	DETOUR_UNLOAD_GAME(econItemSystem_parseItemSchemaFile);

	DetourTransactionCommit();
}