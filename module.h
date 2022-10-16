#pragma once

enum ModuleName
{
	MOD_INVALID = -1,

	MOD_CLIENT,
	MOD_SERVER,
	//MOD_ENGINE,

	MOD_MAX,
};

struct ModuleInfo
{
	const char* name;
	intptr_t base;
	intptr_t size;

	bool Find()
	{
		char modName[MAX_PATH];
		sprintf(modName, "%s.%s", name, "dll");

		MODULEINFO modinfo = { 0 };
		HMODULE module = GetModuleHandle(modName);

		if (!module)
		{
			Log(Color(255, 0, 0, 255), "Failed to get module info for %s\n", modName);
			return false;
		}

		GetModuleInformation(GetCurrentProcess(), module, &modinfo, sizeof(MODULEINFO));
		base = (intptr_t)modinfo.lpBaseOfDll;
		size = (intptr_t)modinfo.SizeOfImage;

		Log(Color(255, 100, 200, 255), "Found module info for %s: Start: 0x%X End: 0x%X (%X)\n",
			modName, base, base + size, size);
		return true;
	}
};

#define MODULE(name) #name, 0, 0

ModuleInfo modules[MOD_MAX] =
{
	MODULE(client),
	MODULE(server),
	//MODULE(engine),
};