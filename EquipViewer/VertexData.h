#ifndef __EQUIPVIEWER_PROJECTTAKO_VERTEXDATA_H__
#define __EQUIPVIEWER_PROJECTTAKO_VERTEXDATA_H__

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <stdint.h>

struct vertexdata_t
{
public:
	float x;
	float y;
	float z;
	float rhw;
	uint32_t color;

	vertexdata_t() : x(0.0f), y(0.0f), z(0.0f), rhw(0.0f), color(0) { }
	vertexdata_t(float x, float y, float z, float rhw, uint32_t color) : x(x), y(y), z(z), rhw(rhw), color(color) { }
};

#endif