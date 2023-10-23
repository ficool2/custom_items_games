#define CUSTOM_ITEMS_GAME "scripts/items/items_game_custom.txt"
#define CUSTOM_ITEMS_GAME_SIG CUSTOM_ITEMS_GAME ".sig"

extern IBaseFileSystem* filesystem;

typedef intptr_t (*econItemSystem)();
ADDR(
	econItemSystem,
	"CTFItemSystem",
	MOD_SERVER,
	\xA1\x2A\x2A\x2A\x2A\x85\xC0\x75\x2A\x56,
	x????xxx?x
);

bool customItemsGameFound = false;

bool helper_check_custom_itemsgame()
{
	bool foundCustom = true;
	if (!filesystem->FileExists(CUSTOM_ITEMS_GAME))
	{
		Log(Color(255, 0, 127, 255), "Server: %s not found, loading default items_game.txt ...\n", CUSTOM_ITEMS_GAME);
		foundCustom = false;
	}
	if (!filesystem->FileExists(CUSTOM_ITEMS_GAME_SIG))
	{
		Log(Color(255, 0, 127, 255), "Server: %s not found, loading default items_game.txt ...\n", CUSTOM_ITEMS_GAME_SIG);
		foundCustom = false;
	}
	customItemsGameFound = foundCustom;
	return foundCustom;
}

typedef bool (*crypto_verifySignature)(uint8_t*, uint8_t, uint8_t*, uint8_t, uint8_t*, uint8_t*);
ADDR_GAME(
	crypto_verifySignature, 
	"CCrypto::RSAVerifySignatureSHA256", 
	\x55\x8B\xEC\x6A\xFF\x68\x2A\x2A\x2A\x2A\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x81\xEC\xCC\x00\x00\x00,
	xxxxxx????xxxxxxxxxxxxxxxxxxxx
);
bool hook_crypto_verifySignature(uint8_t*, uint8_t, uint8_t*, uint8_t, uint8_t*, uint8_t*)
{
	return true;
}

typedef void (__fastcall *econItemSystem_parseItemSchemaFile)(intptr_t, void*, const char*);
ADDR_GAME(
	econItemSystem_parseItemSchemaFile,
	"CEconItemSystem::ParseItemSchemaFile",
	\x55\x8B\xEC\x83\xEC\x14\x8B\x41\x04\x8D\x55\xEC\x83\xC1\x04\xC7\x45\xEC\x00\x00\x00\x00\x52\x68\x2A\x2A\x2A\x2A,
	xxxxxxxxxxxxxxxxxxxxxxxx????
);
void __fastcall hook_server_econItemSystem_parseItemSchemaFile(intptr_t thisptr, void* edx, const char* filename)
{
	if (helper_check_custom_itemsgame())
	{
		filename = CUSTOM_ITEMS_GAME;
		Log(Color(0, 255, 127, 255), "Server: loading %s...\n", filename);
	}

	address_econItemSystem_parseItemSchemaFile[MOD_SERVER](thisptr, edx, filename);
}
void __fastcall hook_client_econItemSystem_parseItemSchemaFile(intptr_t thisptr, void* edx, const char* filename)
{
	if (helper_check_custom_itemsgame())
	{
		filename = CUSTOM_ITEMS_GAME;
		hook_server_econItemSystem_parseItemSchemaFile(address_econItemSystem[MOD_SERVER](), edx, filename);
		Log(Color(0, 255, 127, 255), "Client: Loading %s...\n", filename);
	}
	address_econItemSystem_parseItemSchemaFile[MOD_CLIENT](thisptr, edx, filename);
}

bool is_demo_branch()
{
	FileHandle_t f = filesystem->Open("steam.inf", "r");
	if (!f)
	{
		Log(Color(255, 0, 0, 255), "Failed to open steam.inf\n");
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

typedef bool (__fastcall* gcUpdateItemSchema_runJob)(intptr_t, void*, void*);
ADDR_GAME(
	gcUpdateItemSchema_runJob,
	"CGCUpdateItemSchema::BYieldingRunGCJob",
	\x55\x8B\xEC\x83\xEC\x1C\x53\x57\x8B\xF9\x8D\x4D\xE4,
	xxxxxxxxxxxxx
);
void __fastcall hook_gcUpdateItemSchema_runJob(intptr_t thisptr, void* edx, void* packet)
{
	if (customItemsGameFound)
	{
		Log(Color(0, 255, 127, 255), "Blocked item schema update from GC\n");
	}
	else if (is_demo_branch())
	{
		Log(Color(0, 255, 127, 255), "Forcefully blocked item schema update from GC (demo branch detected)\n");
	}
	else
	{
		address_gcUpdateItemSchema_runJob.Resolve(Deref(thisptr))(thisptr, edx, packet);
	}
}

#pragma optimize("", on)