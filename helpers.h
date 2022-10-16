#pragma once

struct Color
{
	Color(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a)
	{
		r = _r;
		g = _g;
		b = _b;
		a = _a;
	}

	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};

#define UNREACHABLE __assume(0)

#define PLUGIN_NAME "[Custom Items Game] "

typedef void (*ConColorMsg)(const Color& clr, const char* msg, ...);
ConColorMsg LogFunc = nullptr;

#define Log(clr, msg, ...) \
LogFunc(clr, PLUGIN_NAME); \
LogFunc(clr, msg, __VA_ARGS__);