#pragma optimize("", off)

Offset offset_server_econItemSchema = 0x9D2534;

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

typedef void (__fastcall *econItemSystem_parseItemSchemaFile)(void*, void*, const char*);
ADDR_GAME(
	econItemSystem_parseItemSchemaFile,
	"CEconItemSystem::ParseItemSchemaFile",
	\x55\x8B\xEC\x83\xEC\x14\x8B\x41\x04\x8D\x55\xEC\x83\xC1\x04\xC7\x45\xEC\x00\x00\x00\x00\x52\x68\x2A\x2A\x2A\x2A,
	xxxxxxxxxxxxxxxxxxxxxxxx????
);
const char* g_CustomItemsGame = "scripts/items/items_game_custom.txt";
void __fastcall hook_server_econItemSystem_parseItemSchemaFile(void* thisptr, void* edx, const char* name)
{
	name = g_CustomItemsGame;
	Log(Color(0, 255, 127, 255), "[Server] Loading %s...\n", name);
	address_econItemSystem_parseItemSchemaFile[MOD_SERVER](thisptr, edx, name);
}
void __fastcall hook_client_econItemSystem_parseItemSchemaFile(void* thisptr, void* edx, const char* name)
{
	name = g_CustomItemsGame;
	Log(Color(0, 255, 127, 255), "[Client] Loading %s...\n", name);
	address_econItemSystem_parseItemSchemaFile[MOD_CLIENT](thisptr, edx, name);
	hook_server_econItemSystem_parseItemSchemaFile(*((void**)(modules[MOD_SERVER].base + offset_server_econItemSchema)), edx, name);
}

typedef bool (__fastcall* gcUpdateItemSchema_runJob)(void*, void*, void*);
ADDR_GAME(
	gcUpdateItemSchema_runJob,
	"CGCUpdateItemSchema::BYieldingRunGCJob",
	\x55\x8B\xEC\x83\xEC\x1C\x53\x57\x8B\xF9\x8D\x4D\xE4,
	xxxxxxxxxxxxx
);
void __fastcall hook_gcUpdateItemSchema_runJob(void* thisptr, void* edx, const char* name)
{
	Log(Color(0, 255, 127, 255), "Blocked item schema update from GC\n");
}

#pragma optimize("", on)