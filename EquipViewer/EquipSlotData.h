#ifndef __EQUIPVIEWER_PROJECTTAKO_EQUIPSLOTDATA_H__
#define __EQUIPVIEWER_PROJECTTAKO_EQUIPSLOTDATA_H__

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <Ashita.h>

#include <stdint.h>
#include <d3d8/includes/d3d8.h>
#include <d3d8/includes/d3dx8.h>

#pragma comment(lib, "d3d8.lib")
#pragma comment(lib, "d3dx8.lib")

const Ashita::FFXI::Enums::EquipmentSlot g_DisplaySlotOrder[(uint32_t)Ashita::FFXI::Enums::EquipmentSlot::Max] =
{
	Ashita::FFXI::Enums::EquipmentSlot::Main,
	Ashita::FFXI::Enums::EquipmentSlot::Sub,
	Ashita::FFXI::Enums::EquipmentSlot::Range,
	Ashita::FFXI::Enums::EquipmentSlot::Ammo,
	Ashita::FFXI::Enums::EquipmentSlot::Head,
	Ashita::FFXI::Enums::EquipmentSlot::Neck,
	Ashita::FFXI::Enums::EquipmentSlot::Ear1,
	Ashita::FFXI::Enums::EquipmentSlot::Ear2,
	Ashita::FFXI::Enums::EquipmentSlot::Body,
	Ashita::FFXI::Enums::EquipmentSlot::Hands,
	Ashita::FFXI::Enums::EquipmentSlot::Ring1,
	Ashita::FFXI::Enums::EquipmentSlot::Ring2,
	Ashita::FFXI::Enums::EquipmentSlot::Back,
	Ashita::FFXI::Enums::EquipmentSlot::Waist,
	Ashita::FFXI::Enums::EquipmentSlot::Legs,
	Ashita::FFXI::Enums::EquipmentSlot::Feet
};

struct equipslottexture_t
{
public:
	IDirect3DTexture8* bgTexture;
	IDirect3DTexture8* itemTexture;
	uint32_t xoffset;
	uint32_t yoffset;

	equipslottexture_t() : bgTexture(nullptr), itemTexture(nullptr), xoffset(0), yoffset(0) { }

	equipslottexture_t(uint32_t xoffset, uint32_t yoffset) : bgTexture(nullptr), itemTexture(nullptr), xoffset(xoffset), yoffset(yoffset) { }
};

struct equipslotdata_t
{
public:
	uint32_t slotId;
	uint16_t itemId;
	bool encumbered;
	equipslottexture_t texturedata;

	equipslotdata_t() : slotId(0), itemId(0), encumbered(false), texturedata() { }

	equipslotdata_t(uint32_t slotId, uint16_t itemId, uint32_t xoffset, uint32_t yoffset) : slotId(slotId), itemId(itemId), encumbered(false), texturedata(xoffset, yoffset) { }
};

#endif