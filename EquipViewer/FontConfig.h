#ifndef __EQUIPVIEWER_PROJECTTAKO_FONTCONFIG_H__
#define __EQUIPVIEWER_PROJECTTAKO_FONTCONFIG_H__

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <stdint.h>

struct position_t
{
public:
	float x;
	float y;

	position_t() : x(0.0f), y(0.0f) { }
};

struct color_t
{
public:
	uint8_t alpha;
	uint8_t red;
	uint8_t green;
	uint8_t blue;

	color_t() : alpha(255), red(255), green(255), blue(255) { }
	color_t(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue) : alpha(alpha), red(red), green(green), blue(blue) { }
};

struct fontconfig_t
{
public:
	position_t position;
	color_t fontcolor;
	color_t bgcolor;
	uint32_t size;
	float scale;
	float opacity;
	bool showAmmo;
	bool hideChatLog;
	bool showEncumberance;

	fontconfig_t() : position(), fontcolor(), bgcolor(64, 0, 0, 0), size(32), scale(1.0f), opacity(1.0f), showAmmo(false), hideChatLog(false), showEncumberance(true) { }
};

#endif