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

typedef void (*LogFunc)(const Color& clr, const char* msg, ...);
LogFunc Log = nullptr;