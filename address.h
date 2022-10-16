#pragma once

struct AddressBase;
typedef std::vector<AddressBase*> AddressList;
const int maxAddresses = 2;

struct AddressBase
{
	virtual bool Find() = 0;
};

template <class addrtype>
struct AddressInfo : public AddressBase
{
	const char* name;
	const char* sig;
	size_t		len;
	const char* mask;
	addrtype	addr[maxAddresses];
	ModuleName	mod[maxAddresses];

	AddressInfo(const char* _name, const char* _sig, size_t _len, const char* _mask,
		ModuleName _mod1, ModuleName _mod2, AddressList& list)
	{
		name = _name;
		sig = _sig;
		len = _len;
		mask = _mask;
		memset(addr, 0, sizeof(addr));
		mod[0] = _mod1;
		mod[1] = _mod2;
		list.push_back(this);
	}

	virtual bool Find()
	{
		int foundAddr = 0;
		for (int m = 0; m < maxAddresses; m++)
		{
			ModuleName curmod = mod[m];
			if (curmod == MOD_INVALID)
				continue;

			ModuleInfo& info = modules[curmod];

			intptr_t ptr = 0;
			intptr_t end = info.base + info.size - len;

			for (intptr_t i = info.base; i < end; i++)
			{
				bool found = true;
				for (size_t j = 0; j < len; j++)
				{
					if (mask)
						found &= (mask[j] == '?') || (sig[j] == *(char*)(i + j));
					else
						found &= (sig[j] == *(char*)(i + j));

				}

				if (found)
				{
					ptr = i;
					foundAddr++;
					Log(Color(0, 255, 255, 255), "Signature %s found at 0x%X in %s.%s\n",
						name, ptr, info.name, "dll");
					break;
				}
			}

			addr[m] = (addrtype)ptr;
		}

		if (foundAddr == 0)
			Log(Color(255, 0, 0, 255), "Failed to find signature for %s\n", name);

		return foundAddr != 0;
	}

	constexpr addrtype& operator[](ModuleName m)
	{
		for (int k = 0; k < maxAddresses; k++)
			if (mod[k] == m)
				return addr[k];
		UNREACHABLE;
	}
};

std::vector<AddressBase*> addresses;

#define CHECK_SIG(name, sig, mask) static_assert(sizeof(#sig) == sizeof(#mask), "Mismatch in signature/mask length for " name)

#define ADDR(var, name, mod, sig, mask) \
AddressInfo<var> address_##var = {name, #sig, sizeof(#sig) - 1, #mask, mod, MOD_INVALID, addresses}; \
CHECK_SIG(name, sig, mask);

#define ADDR_GAME(var, name, sig, mask) \
AddressInfo<var> address_##var = {name, #sig, sizeof(#sig) - 1, #mask, MOD_CLIENT, MOD_SERVER, addresses}; \
CHECK_SIG(name, sig, mask);

#define DETOUR_LOAD(addrtype) \
for (int k = 0; k < maxAddresses; k++) \
	if (address_##addrtype.addr[k]) DetourAttach(&(LPVOID&)address_##addrtype.addr[k], &hook_##addrtype);

#define DETOUR_LOAD_GAME(addrtype) \
if (address_##addrtype[MOD_CLIENT]) DetourAttach(&(LPVOID&)address_##addrtype[MOD_CLIENT], &hook_client_##addrtype); \
if (address_##addrtype[MOD_SERVER]) DetourAttach(&(LPVOID&)address_##addrtype[MOD_SERVER], &hook_server_##addrtype);

#define DETOUR_UNLOAD(addrtype) \
for (int k = 0; k < maxAddresses; k++) \
if (address_##addrtype.addr[k]) DetourDetach(&(LPVOID&)address_##addrtype.addr[k], &hook_##addrtype);

#define DETOUR_UNLOAD_GAME(addrtype) \
if (address_##addrtype[MOD_SERVER]) DetourDetach(&(LPVOID&)address_##addrtype[MOD_CLIENT], &hook_client_##addrtype); \
if (address_##addrtype[MOD_SERVER]) DetourDetach(&(LPVOID&)address_##addrtype[MOD_SERVER], &hook_server_##addrtype);