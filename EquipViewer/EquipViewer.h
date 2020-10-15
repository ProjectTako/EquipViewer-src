#ifndef __EQUIPVIEWER_PROJECTTAKO_H__
#define __EQUIPVIEWER_PROJECTTAKO_H__

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "FontConfig.h"
#include "EquipSlotData.h"
#include "VertexData.h"

#include <Ashita.h>
#include <Windows.h>
#include <set>
#include <vector>
#include <string>
#include <stdio.h>
#include <tchar.h>

#include <d3d8/includes/d3d8.h>
#include <d3d8/includes/d3dx8.h>

#pragma comment(lib, "d3d8.lib")
#pragma comment(lib, "d3dx8.lib")

class EquipViewer : IPlugin 
{
protected:
	IAshitaCore* m_AshitaCore;          // The main AshitaCore object pointer.
	ILogManager* m_LogManager;          // The main LogManager object pointer.
	IDirect3DDevice8* m_Direct3DDevice; // The main Direct3D device object pointer.
	uint32_t m_PluginId;                // The plugins id. (It's base address.)

protected: // d3dx stuff
	LPD3DXSPRITE m_Sprite;
	LPDIRECT3DINDEXBUFFER8 m_pBackgroundIndexBuffer;
	LPDIRECT3DVERTEXBUFFER8 m_pBackgroundVertexBuffer;
	LPDIRECT3DINDEXBUFFER8 m_pEncumberanceIndexBuffer;
	LPDIRECT3DVERTEXBUFFER8 m_pEncumberanceVertexBuffer;
	LPD3DXFONT m_AmmoFont;
	LPD3DXFONT m_EncumberanceFont;

protected: // internal stuff
	fontconfig_t m_FontConfig;
	std::vector<equipslotdata_t> m_EquipmentData;
	bool m_PositionChanged;
	uintptr_t m_pMenuInfo;

public:
	EquipViewer(void);
	~EquipViewer(void);

public:
	const char* GetName(void) const override;
	const char* GetAuthor(void) const override;
	const char* GetDescription(void) const override;
	const char* GetLink(void) const override;
	double GetVersion(void) const override;
	int32_t GetPriority(void) const override;
	uint32_t GetFlags(void) const override;

public:
	// Methods
	bool Initialize(IAshitaCore* core, ILogManager* logger, const uint32_t id) override;
	void Release(void) override;

	// ChatManager
	bool HandleCommand(int32_t mode, const char* command, bool injected) override;

	//PacketManager
	bool HandleIncomingPacket(uint16_t id, uint32_t size, const uint8_t* data, uint8_t* modified, uint32_t sizeChunk, const uint8_t* dataChunk, bool injected, bool blocked) override;
	bool HandleOutgoingPacket(uint16_t id, uint32_t size, const uint8_t* data, uint8_t* modified, uint32_t sizeChunk, const uint8_t* dataChunk, bool injected, bool blocked) override;

public:
	bool Direct3DInitialize(IDirect3DDevice8* device) override;
	void Direct3DBeginScene(bool isRenderingBackBuffer) override;
	void Direct3DEndScene(bool isRenderingBackBuffer) override;
	void Direct3DPresent(const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) override;
	bool Direct3DSetRenderState(D3DRENDERSTATETYPE State, DWORD* Value) override;
	bool Direct3DDrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) override;
	bool Direct3DDrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT minIndex, UINT NumVertices, UINT startIndex, UINT primCount) override;
	bool Direct3DDrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) override;
	bool Direct3DDrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) override;

private:
	bool SetupBackground();
	bool SetupEncumberance(equipslotdata_t equipSlot);
	IItem* GetEquippedItem(uint32_t slotId);
	uint32_t GetAmmoCount(void);
	IDirect3DTexture8* LoadItemTexture(IItem* item);
	void UpdateEquipmentTextures(void);
};

__declspec(dllexport) IPlugin* __stdcall expCreatePlugin(const char* arg)
{
	UNREFERENCED_PARAMETER(arg);

	return (IPlugin*)(new EquipViewer());
}

__declspec(dllexport) double __stdcall expGetInterfaceVersion(void)
{
	return ASHITA_INTERFACE_VERSION;
}

#endif