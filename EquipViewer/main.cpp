#include "EquipViewer.h"

#pragma comment(lib, "psapi.lib")
#include <psapi.h>

EquipViewer::EquipViewer(void) : m_AshitaCore(nullptr), m_LogManager(nullptr), m_PluginId(0), m_Direct3DDevice(nullptr), m_FontConfig(), m_Sprite(nullptr), m_PositionChanged(false), m_pBackgroundIndexBuffer(nullptr), m_pBackgroundVertexBuffer(nullptr), m_pEncumberanceIndexBuffer(nullptr), m_pEncumberanceVertexBuffer(nullptr), m_AmmoFont(nullptr), m_EncumberanceFont(nullptr)
{

}

const char* EquipViewer::GetName(void) const
{
	return u8"Equip Viewer";
}

const char* EquipViewer::GetAuthor(void) const
{
	return u8"Project Tako";
}

const char* EquipViewer::GetDescription(void) const
{
	return u8"A plugin for displaying your current equipment.";
}

const char* EquipViewer::GetLink(void) const
{
	return u8"https://github.com/ProjectTako/EquipViewer/";
}

double EquipViewer::GetVersion(void) const
{
	return 4.01;
}

int32_t EquipViewer::GetPriority(void) const 
{
	return 0;
}

uint32_t EquipViewer::GetFlags(void) const 
{
	return ((uint32_t)Ashita::PluginFlags::UseCommands | (uint32_t)Ashita::PluginFlags::UsePackets | (uint32_t)Ashita::PluginFlags::UseDirect3D);
}

bool EquipViewer::Initialize(IAshitaCore* core, ILogManager* logger, const uint32_t id)
{
	this->m_AshitaCore = core;
	this->m_LogManager = logger;
	this->m_PluginId = id;

	// read signature
	MODULEINFO modInfo = { 0 };
	auto procHandle = ::GetCurrentProcess();

	if (procHandle != 0)
	{
		auto modHandle = ::GetModuleHandle("FFXiMain.dll");

		if (modHandle != 0)
		{
			if (::GetModuleInformation(procHandle, modHandle, &modInfo, sizeof(MODULEINFO)))
			{
				if (modInfo.lpBaseOfDll != 0)
				{
					// find pattern
					auto ptr = Ashita::Memory::FindPattern((uintptr_t)modInfo.lpBaseOfDll, (uintptr_t)modInfo.SizeOfImage, "8B480C85C974??8B510885D274??3B05", 0x10, 0);
					if (ptr != 0)
					{
						// get address
						this->m_pMenuInfo = *(uint32_t*)ptr;

						if (this->m_pMenuInfo == 0)
							this->m_AshitaCore->GetChatManager()->Writef(128, false, "Failed to find valid memory address for menu information. Some hiding features may not work.");
					}
				}
			}
		}
	}

	// let's load our settings
	if (this->m_AshitaCore->GetConfigurationManager()->Load("EquipViewer", "EquipViewer"))
	{
		// we have a valid settings file
		this->m_FontConfig.bgcolor.alpha = this->m_AshitaCore->GetConfigurationManager()->GetInt32("EquipViewer", "bgcolor", "alpha", 0x40);
		this->m_FontConfig.bgcolor.red = this->m_AshitaCore->GetConfigurationManager()->GetInt32("EquipViewer", "bgcolor", "red", 0x00);
		this->m_FontConfig.bgcolor.green = this->m_AshitaCore->GetConfigurationManager()->GetInt32("EquipViewer", "bgcolor", "green", 0x00);
		this->m_FontConfig.bgcolor.blue = this->m_AshitaCore->GetConfigurationManager()->GetInt32("EquipViewer", "bgcolor", "blue", 0x00);

		this->m_FontConfig.fontcolor.alpha = this->m_AshitaCore->GetConfigurationManager()->GetInt32("EquipViewer", "fontcolor", "alpha", 0xFF);
		this->m_FontConfig.fontcolor.red = this->m_AshitaCore->GetConfigurationManager()->GetInt32("EquipViewer", "fontcolor", "red", 0xFF);
		this->m_FontConfig.fontcolor.green = this->m_AshitaCore->GetConfigurationManager()->GetInt32("EquipViewer", "fontcolor", "green", 0xFF);
		this->m_FontConfig.fontcolor.blue = this->m_AshitaCore->GetConfigurationManager()->GetInt32("EquipViewer", "fontcolor", "blue", 0xFF);

		this->m_FontConfig.opacity = this->m_AshitaCore->GetConfigurationManager()->GetFloat("EquipViewer", "config", "opacity", 1.0f);
		this->m_FontConfig.position.x = this->m_AshitaCore->GetConfigurationManager()->GetFloat("EquipViewer", "position", "x", 0.0f);
		this->m_FontConfig.position.y = this->m_AshitaCore->GetConfigurationManager()->GetFloat("EquipViewer", "position", "y", 0.0f);
		this->m_FontConfig.scale = this->m_AshitaCore->GetConfigurationManager()->GetFloat("EquipViewer", "config", "scale", 1.0f);
		this->m_FontConfig.size = this->m_AshitaCore->GetConfigurationManager()->GetInt32("EquipViewer", "config", "size", 0x20);
		this->m_FontConfig.showAmmo = this->m_AshitaCore->GetConfigurationManager()->GetBool("EquipViewer", "config", "showammo", true);
		this->m_FontConfig.hideChatLog = this->m_AshitaCore->GetConfigurationManager()->GetBool("EquipViewer", "config", "hideinchatlog", false);
		this->m_FontConfig.showEncumberance = this->m_AshitaCore->GetConfigurationManager()->GetBool("EquipViewer", "config", "showencumberance", true);
	}

	// set up default data
	for (auto x = 0; x < (uint32_t)Ashita::FFXI::Enums::EquipmentSlot::Max; x++)
	{
		auto slotId = (uint32_t)g_DisplaySlotOrder[x];
		auto offsetX = (uint32_t)((x % 4) * this->m_FontConfig.size);
		auto offsetY = (uint32_t)(floor(x / 4) * this->m_FontConfig.size);

		this->m_EquipmentData.push_back(equipslotdata_t(slotId, 0, offsetX, offsetY));
	}

	return true;
}

bool EquipViewer::HandleCommand(int32_t mode, const char* command, bool injected)
{
	UNREFERENCED_PARAMETER(mode);
	UNREFERENCED_PARAMETER(injected);

	std::vector<std::string> args;
	auto count = Ashita::Commands::GetCommandArgs(command, &args);
	if (count < 1)
		return false;

	HANDLECOMMAND("/equipviewer", "/ev")
	{
		if (count < 2)
			return false;

		if (args[1] == "position" || args[1] == "pos")
		{
			if (count < 4)
			{
				this->m_AshitaCore->GetChatManager()->Write(132, false, "[EquipViewer] Not enough arguments.");
				return false;
			}

			this->m_FontConfig.position.x = (float)std::stof(args[2]);
			this->m_FontConfig.position.y = (float)std::stof(args[3]);

			this->m_AshitaCore->GetConfigurationManager()->SetValue("EquipViewer", "position", "x", std::to_string(this->m_FontConfig.position.x).c_str());
			this->m_AshitaCore->GetConfigurationManager()->SetValue("EquipViewer", "position", "y", std::to_string(this->m_FontConfig.position.y).c_str());

			return (this->m_PositionChanged = true);
		}
		else if (args[1] == "scale")
		{
			if (count < 3)
			{
				this->m_AshitaCore->GetChatManager()->Write(132, false, "[EquipViewer] Not enough arguments.");
				return false;
			}

			this->m_FontConfig.scale = (float)fmax(0, fmin((float)std::stof(args[2]), 10.0f));
			this->m_AshitaCore->GetConfigurationManager()->SetValue("EquipViewer", "config", "scale", std::to_string(this->m_FontConfig.scale).c_str());

			// do this to trigger vertex buffer being remade
			this->m_PositionChanged = true;

			return true;
		}
		else if (args[1] == "opacity")
		{
			if (count < 3)
			{
				this->m_AshitaCore->GetChatManager()->Write(132, false, "[EquipViewer] Not enough arguments.");
				return false;
			}

			// make sure what they passed is a valid value
			this->m_FontConfig.opacity = (float)fmax(0, fmin((float)std::stof(args[2]), 1.0f));

			this->m_AshitaCore->GetConfigurationManager()->SetValue("EquipViewer", "config", "opacity", std::to_string(this->m_FontConfig.opacity).c_str());

			return true;
		}
		else if (args[1] == "bgcolor")
		{
			if (count < 5)
			{
				this->m_AshitaCore->GetChatManager()->Write(132, false, "[EquipViewer] Not enough arguments.");
				return false;
			}

			this->m_FontConfig.bgcolor.alpha = max(0, min((uint8_t)atoi(args[2].c_str()), 255));
			this->m_FontConfig.bgcolor.red = max(0, min((uint8_t)atoi(args[3].c_str()), 255));
			this->m_FontConfig.bgcolor.green = max(0, min((uint8_t)atoi(args[4].c_str()), 255));
			this->m_FontConfig.bgcolor.blue = max(0, min((uint8_t)atoi(args[5].c_str()), 255));

			this->m_AshitaCore->GetConfigurationManager()->SetValue("EquipViewer", "bgcolor", "alpha", std::to_string(this->m_FontConfig.bgcolor.alpha).c_str());
			this->m_AshitaCore->GetConfigurationManager()->SetValue("EquipViewer", "bgcolor", "red", std::to_string(this->m_FontConfig.bgcolor.red).c_str());
			this->m_AshitaCore->GetConfigurationManager()->SetValue("EquipViewer", "bgcolor", "green", std::to_string(this->m_FontConfig.bgcolor.green).c_str());
			this->m_AshitaCore->GetConfigurationManager()->SetValue("EquipViewer", "bgcolor", "blue", std::to_string(this->m_FontConfig.bgcolor.blue).c_str());

			// do this to trigger vertex buffer being remade
			this->m_PositionChanged = true;

			return true;
		}
		else if (args[1] == "showammo")
		{
			this->m_FontConfig.showAmmo = !this->m_FontConfig.showAmmo;

			this->m_AshitaCore->GetConfigurationManager()->SetValue("EquipViewer", "config", "showammo", std::to_string(this->m_FontConfig.showAmmo).c_str());

			return true;
		}
		else if (args[1] == "hideinchat")
		{
			this->m_FontConfig.hideChatLog = !this->m_FontConfig.hideChatLog;

			this->m_AshitaCore->GetConfigurationManager()->SetValue("EquipViewer", "config", "hideinchatlog", std::to_string(this->m_FontConfig.hideChatLog).c_str());

			return true;
		}
		else if (args[1] == "encumberance")
		{
			this->m_FontConfig.showEncumberance = !this->m_FontConfig.showEncumberance;

			this->m_AshitaCore->GetConfigurationManager()->SetValue("EquipViewer", "config", "showencumberance", std::to_string(this->m_FontConfig.showEncumberance).c_str());

			return true;
		}
		else
		{
			this->m_AshitaCore->GetChatManager()->Writef(128, false, "[EquipViewer] Invalid command. Valid commands:\nequipviewer pos[ition] x y\nequipviewer opacity 0-1.0\nequipviewer scale 0-1.0\nequipviewer bgcolor a r g b\nequipviewer showammo\nequipviewer hideinchat\nequipviewer encumberance");

			return true;
		}
	};

	return false;
}

bool EquipViewer::HandleIncomingPacket(uint16_t id, uint32_t size, const uint8_t* data, uint8_t* modified, uint32_t sizeChunk, const uint8_t* dataChunk, bool injected, bool blocked)
{
	UNREFERENCED_PARAMETER(size);
	UNREFERENCED_PARAMETER(data);
	UNREFERENCED_PARAMETER(modified);
	UNREFERENCED_PARAMETER(sizeChunk);
	UNREFERENCED_PARAMETER(dataChunk);
	UNREFERENCED_PARAMETER(injected);

	if (std::set<uint16_t>{ 0x001D, 0x001F, 0x0037, 0x0050, 0x0051, 0x0116, 0x0117 }.count(id) > 0U && !blocked) // these are all equipment or inventory related packets
	{
		this->UpdateEquipmentTextures();
	}
	else if (id == 0x01B && !blocked) // 0x01B = Job Info which contains the Encumberance flags
	{
		for (auto x = 0; x < (uint32_t)Ashita::FFXI::Enums::EquipmentSlot::Max; x++)
		{
			// get where this slot is in our equipment data
			auto iter = std::find_if(this->m_EquipmentData.begin(), this->m_EquipmentData.end(), [x](equipslotdata_t &slot) { return slot.slotId == x; });
			if (iter != this->m_EquipmentData.end())
			{
				iter->encumbered = Ashita::BinaryData::UnpackBitsBE((uint8_t*)data, 0x60, x, 1);
			}
		}
	}

	return false;
}

bool EquipViewer::HandleOutgoingPacket(uint16_t id, uint32_t size, const uint8_t* data, uint8_t* modified, uint32_t sizeChunk, const uint8_t* dataChunk, bool injected, bool blocked)
{
	UNREFERENCED_PARAMETER(size);
	UNREFERENCED_PARAMETER(data);
	UNREFERENCED_PARAMETER(modified);
	UNREFERENCED_PARAMETER(sizeChunk);
	UNREFERENCED_PARAMETER(dataChunk);
	UNREFERENCED_PARAMETER(injected);

	if (std::set<uint16_t>{ 0x001A, 0x0050, 0x0051, 0x0052, 0x0053, 0x0061, 0x0102, 0x0111 }.count(id) > 0U && !blocked) // these are all equipment or inventory related packets
	{
		this->UpdateEquipmentTextures();
	}

	return false;
}

bool EquipViewer::Direct3DInitialize(IDirect3DDevice8* device)
{
	this->m_Direct3DDevice = device;
	
	//create sprite
	auto result = D3DXCreateSprite(this->m_Direct3DDevice, &this->m_Sprite);
	if (result != S_OK)
	{
		char buffer[255];
		auto error = D3DXGetErrorStringA(result, buffer, 255);
		this->m_AshitaCore->GetChatManager()->Writef(128, false, "[EquipViewer] Failed to create sprite!");
		this->m_LogManager->Logf((uint32_t)Ashita::LogLevel::Error, "EquipViewer", "D3DXCreateSprite failure! HRESULT: %d. Error: %s", error, buffer);

		return false;
	}

	// create font for ammo
	auto hFont = CreateFontA(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, "Arial");
	if (hFont)
	{
		result = D3DXCreateFont(this->m_Direct3DDevice, hFont, &this->m_AmmoFont);
		if (result != S_OK)
		{
			char buffer[255];
			auto error = D3DXGetErrorStringA(result, buffer, 255);
			this->m_AshitaCore->GetChatManager()->Writef(128, false, "[EquipViewer] Failed to create font! Some features may no work correctly.");
			this->m_LogManager->Logf((uint32_t)Ashita::LogLevel::Error, "EquipViewer", "D3DXCreateFont failure for m_AmmoFont! HRESULT: %d. Error: %s", error, buffer);
		}
	}
	else
	{
		this->m_AshitaCore->GetChatManager()->Writef(128, false, "[EquipViewer] Failed to create font! Some features may no work correctly.");
	}

	// create font for encumberance
	hFont = CreateFontA(18, 0, 0, 0, 1200, 0, 0, 0, DEFAULT_CHARSET, OUT_STROKE_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, "Comic Sans MS");
	if (hFont)
	{
		result = D3DXCreateFont(this->m_Direct3DDevice, hFont, &this->m_EncumberanceFont);
		if (result != S_OK)
		{
			char buffer[255];
			auto error = D3DXGetErrorStringA(result, buffer, 255);
			this->m_AshitaCore->GetChatManager()->Writef(128, false, "[EquipViewer] Failed to create font! Some features may no work correctly.");
			this->m_LogManager->Logf((uint32_t)Ashita::LogLevel::Error, "EquipViewer", "D3DXCreateFont failure for m_EncumberanceFont! HRESULT: %d. Error: %s", error, buffer);
		}
	}
	else
	{
		this->m_AshitaCore->GetChatManager()->Writef(128, false, "[EquipViewer] Failed to create font! Some features may no work correctly.");
	}

	// doesn't hurt
	if (this->m_Sprite != nullptr)
	{
		// update the textures on load
		this->UpdateEquipmentTextures();
	}

	return true;
}

void EquipViewer::Direct3DBeginScene(bool isRenderingBackBuffer)
{
	UNREFERENCED_PARAMETER(isRenderingBackBuffer);
}

void EquipViewer::Direct3DEndScene(bool isRenderingBackBuffer)
{
	UNREFERENCED_PARAMETER(isRenderingBackBuffer);
}

void EquipViewer::Direct3DPresent(const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
{
	// if we don't have a sprite for some reason, something is fucked but don't try and do any drawing work
	if (this->m_Sprite == nullptr)
		return;

	// if Ashita is hiding objects..
	if (!this->m_AshitaCore->GetFontManager()->GetVisible())
		return;

	// check for event status
	if (this->m_AshitaCore->GetMemoryManager()->GetEntity()->GetStatusServer(this->m_AshitaCore->GetMemoryManager()->GetParty()->GetMemberTargetIndex(0)) == 0x04)
		return;

	// check to see if we're not rendering when full chat log
	if (this->m_FontConfig.hideChatLog)
	{
		// read to see if the menu is open
		auto menuPointer = *(uint32_t*)this->m_pMenuInfo;
		// if it's 0 it means no menu is open and we can continue as normal
		if (menuPointer != 0)
		{
			// read to make sure we have a valid menu
			auto menuInfo = *(uint32_t*)(menuPointer + 0x04);
			if (menuInfo != 0)
			{
				// menu name is located 0x46 past this location, assume it's 16? 18? characters long
				auto size = 16;
				char* buffer = new char[size + 1];
				memcpy(buffer, (void*)(menuInfo + 0x46), size);
				buffer[size] = '\0';

				if (strstr(buffer, "fulllog") != 0)
				{
					delete[] buffer;
					return;
				}

				delete[] buffer;
			}
		}
	}

	// try and setup indicies and verticies for the background
	if (this->SetupBackground())
	{
		this->m_Direct3DDevice->SetIndices(this->m_pBackgroundIndexBuffer, 0);
		this->m_Direct3DDevice->SetStreamSource(0, this->m_pBackgroundVertexBuffer, sizeof(vertexdata_t));

		// set render states to allow alpha to back semi transparent backgrounds
		this->m_Direct3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
		this->m_Direct3DDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		this->m_Direct3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		this->m_Direct3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		// record states so we can reset after our draws
		DWORD currentAlphaState, currentColorState;
		this->m_Direct3DDevice->GetTextureStageState(0, D3DTSS_COLOROP, &currentAlphaState);
		this->m_Direct3DDevice->GetTextureStageState(0, D3DTSS_COLOROP, &currentColorState);

		this->m_Direct3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		this->m_Direct3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);

		// draw background rect
		// two triangles to make a square
		auto result = this->m_Direct3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 4, 0, 2);
		if (result != S_OK)
		{
			this->m_AshitaCore->GetChatManager()->Writef(128, false, "[EquipViewer] Failed to DrawIndexedPrimitive!");
		}

		// reset states
		this->m_Direct3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, currentAlphaState);
		this->m_Direct3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, currentColorState);
	}

	// begin sprite
	this->m_Sprite->Begin();

	auto color = D3DCOLOR_ARGB((uint32_t)(this->m_FontConfig.opacity * 255), 255, 255, 255);

	for (auto it = this->m_EquipmentData.begin(); it != this->m_EquipmentData.end(); ++it)
	{
		if (it->texturedata.itemTexture != nullptr)
		{
			// calculate rect
			auto rect = RECT();
			rect.left = 0;
			rect.top = 0;
			rect.right = this->m_FontConfig.size;
			rect.bottom = this->m_FontConfig.size;

			// draw texture to sprite
			this->m_Sprite->Draw(it->texturedata.itemTexture, &rect, &D3DXVECTOR2(this->m_FontConfig.scale, this->m_FontConfig.scale), NULL, 0.0f, &D3DXVECTOR2(this->m_FontConfig.position.x + (it->texturedata.xoffset * this->m_FontConfig.scale), this->m_FontConfig.position.y + (it->texturedata.yoffset * this->m_FontConfig.scale)), color);
		}

		// handle emcumberance even if we don't have anything equipped there
		if (this->m_FontConfig.showEncumberance && it->encumbered && this->m_EncumberanceFont != nullptr)
		{
			// begin font work
			this->m_EncumberanceFont->Begin();

			// draw a red X near bottom right
			// don't bother with position as we'll use DT_CALCRECT
			auto rect = RECT();

			char buffer[2];
			sprintf_s(buffer, "X");

			this->m_EncumberanceFont->DrawText(buffer, -1, &rect, DT_CALCRECT, 0xFFFFFFFF);

			// adjust 
			rect.left = (this->m_FontConfig.position.x + it->texturedata.xoffset + (this->m_FontConfig.size * this->m_FontConfig.scale)) - (rect.right - rect.left);
			rect.top = (this->m_FontConfig.position.y + it->texturedata.yoffset + (this->m_FontConfig.size * this->m_FontConfig.scale)) - rect.bottom;

			// rect should be right now, call draw normally
			this->m_EncumberanceFont->DrawText(buffer, -1, &rect, DT_LEFT, D3DCOLOR_ARGB(255, 255, 0, 0));

			// end font work
			this->m_EncumberanceFont->End();
		}
	}

	// end sprite work
	this->m_Sprite->End();

	// draw ammo count if we're doing that
	if (this->m_FontConfig.showAmmo && this->m_AmmoFont != nullptr)
	{
		auto ammoCount = this->GetAmmoCount();
		if (ammoCount > 0)
		{
			// begin font work
			this->m_AmmoFont->Begin();

			// don't bother with position as we'll use DT_CALCRECT
			auto rect = RECT();

			char buffer[4];
			sprintf_s(buffer, "%d", ammoCount);

			// call DrawText with DT_CALCRECT to get the size of the RECT we need
			this->m_AmmoFont->DrawText(buffer, -1, &rect, DT_CALCRECT, 0xFFFFFFFF);

			// once we have the rect size, we can use the size to offset the RECT so it's overlaying the ammo slot
			rect.left = this->m_FontConfig.position.x + (this->m_FontConfig.size * 4 * this->m_FontConfig.scale) - (rect.right - rect.left);

			// rect should be right now, call draw normal
			this->m_AmmoFont->DrawText(buffer, -1, &rect, DT_LEFT, 0xFFFFFFFF);

			// end font work
			this->m_AmmoFont->End();
		}
	}
}

bool EquipViewer::Direct3DSetRenderState(D3DRENDERSTATETYPE State, DWORD* Value)
{
	UNREFERENCED_PARAMETER(State);
	UNREFERENCED_PARAMETER(Value);

	return false;
}

bool EquipViewer::Direct3DDrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
	UNREFERENCED_PARAMETER(PrimitiveType);
	UNREFERENCED_PARAMETER(StartVertex);
	UNREFERENCED_PARAMETER(PrimitiveCount);

	return false;
}

bool EquipViewer::Direct3DDrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT minIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
	UNREFERENCED_PARAMETER(PrimitiveType);
	UNREFERENCED_PARAMETER(minIndex);
	UNREFERENCED_PARAMETER(NumVertices);
	UNREFERENCED_PARAMETER(startIndex);
	UNREFERENCED_PARAMETER(primCount);

	return false;
}

bool EquipViewer::Direct3DDrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
	UNREFERENCED_PARAMETER(PrimitiveType);
	UNREFERENCED_PARAMETER(PrimitiveCount);
	UNREFERENCED_PARAMETER(pVertexStreamZeroData);
	UNREFERENCED_PARAMETER(VertexStreamZeroStride);

	return false;
}

bool EquipViewer::Direct3DDrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
	UNREFERENCED_PARAMETER(PrimitiveType);
	UNREFERENCED_PARAMETER(MinVertexIndex);
	UNREFERENCED_PARAMETER(NumVertexIndices);
	UNREFERENCED_PARAMETER(PrimitiveCount);
	UNREFERENCED_PARAMETER(pIndexData);
	UNREFERENCED_PARAMETER(IndexDataFormat);
	UNREFERENCED_PARAMETER(pVertexStreamZeroData);
	UNREFERENCED_PARAMETER(VertexStreamZeroStride);

	return false;
}

void EquipViewer::Release(void)
{
	// release textures
	for (auto it = this->m_EquipmentData.begin(); it != this->m_EquipmentData.end(); ++it)
	{
		if (it->texturedata.itemTexture != nullptr)
		{
			it->texturedata.itemTexture->Release();
			it->texturedata.itemTexture = nullptr;
		}
	}

	// release sprite
	if (this->m_Sprite != nullptr)
	{
		this->m_Sprite->Release();
		this->m_Sprite = nullptr;
	}

	// font
	if (this->m_AmmoFont != nullptr)
	{
		this->m_AmmoFont->Release();
		this->m_AmmoFont = nullptr;
	}

	if (this->m_EncumberanceFont != nullptr)
	{
		this->m_EncumberanceFont->Release();
		this->m_EncumberanceFont = nullptr;
	}

	// directx buffers
	if (this->m_pBackgroundIndexBuffer != nullptr)
	{
		this->m_pBackgroundIndexBuffer->Release();
	}

	if (this->m_pBackgroundVertexBuffer != nullptr)
	{
		this->m_pBackgroundVertexBuffer->Release();
	}

	if (this->m_pEncumberanceIndexBuffer != nullptr)
	{
		this->m_pEncumberanceIndexBuffer->Release();
	}

	if (this->m_pEncumberanceVertexBuffer != nullptr)
	{
		this->m_pEncumberanceVertexBuffer->Release();
	}

	// save config
	this->m_AshitaCore->GetConfigurationManager()->Save("EquipViewer", "EquipViewer");
}

bool EquipViewer::SetupBackground()
{
	if (this->m_pBackgroundIndexBuffer == nullptr)
	{
		// we're drawing two triangles
		uint16_t indicies[] =
		{
			0, 1, 3,
			0, 3, 2
		};

		this->m_Direct3DDevice->CreateIndexBuffer(6 * sizeof(uint16_t), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &this->m_pBackgroundIndexBuffer);

		void* pIndicies;
		if (this->m_pBackgroundIndexBuffer->Lock(0, 0, (byte * *)& pIndicies, D3DLOCK_DISCARD) < 0)
			return false;

		memcpy(pIndicies, indicies, sizeof(indicies));
		this->m_pBackgroundIndexBuffer->Unlock();
		pIndicies = nullptr;
	}

	if (this->m_pBackgroundVertexBuffer == nullptr || this->m_PositionChanged)
	{
		// release old buffer
		if (this->m_pBackgroundVertexBuffer != nullptr)
			this->m_pBackgroundVertexBuffer->Release();
		
		// set this up here since all verticies will use
		auto color = D3DCOLOR_ARGB(this->m_FontConfig.bgcolor.alpha, this->m_FontConfig.bgcolor.red, this->m_FontConfig.bgcolor.green, this->m_FontConfig.bgcolor.blue);

		// vertex buffer
		struct vertexdata_t v[] =
		{
			{ this->m_FontConfig.position.x, this->m_FontConfig.position.y, 0.0f, 1.0f, color },
			{ this->m_FontConfig.position.x + (this->m_FontConfig.size * 4 * this->m_FontConfig.scale), this->m_FontConfig.position.y, 0.0f, 1.0f, color },
			{ this->m_FontConfig.position.x, this->m_FontConfig.position.y + (this->m_FontConfig.size * 4 * this->m_FontConfig.scale), 0.0f, 1.0f, color },
			{ this->m_FontConfig.position.x + (this->m_FontConfig.size * 4 * this->m_FontConfig.scale), this->m_FontConfig.position.y + (this->m_FontConfig.size * 4 * this->m_FontConfig.scale), 0.0f, 1.0f, color },
		};

		this->m_Direct3DDevice->CreateVertexBuffer(4 * sizeof(vertexdata_t), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_MANAGED, &this->m_pBackgroundVertexBuffer);

		// copy our vertex data to memory where the buffer is
		void* pVerticies;
		this->m_pBackgroundVertexBuffer->Lock(0, 0, (byte * *)& pVerticies, 0);
		memcpy(pVerticies, v, sizeof(v));
		this->m_pBackgroundVertexBuffer->Unlock();
		pVerticies = nullptr;
	}

	return this->m_pBackgroundIndexBuffer != nullptr && this->m_pBackgroundVertexBuffer != nullptr;
}

bool EquipViewer::SetupEncumberance(equipslotdata_t equipSlot)
{
	// unsed.. but keeping it in here incase I deceide to go back to this style 
	if (this->m_pEncumberanceIndexBuffer == nullptr)
	{
		uint16_t indicies[] =
		{
			0, 1, 2,
			0, 2, 3
		};

		this->m_Direct3DDevice->CreateIndexBuffer(6 * sizeof(uint16_t), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &this->m_pEncumberanceIndexBuffer);

		void* pIndicies;
		if (this->m_pEncumberanceIndexBuffer->Lock(0, 0, (byte * *)& pIndicies, D3DLOCK_DISCARD) < 0)
			return false;

		memcpy(pIndicies, indicies, sizeof(indicies));
		this->m_pEncumberanceIndexBuffer->Unlock();
		pIndicies = nullptr;
	}

	// we have to do the below no matter what since the position is dynamic by slot

	// release old buffer
	if (this->m_pEncumberanceVertexBuffer != nullptr)
		this->m_pEncumberanceVertexBuffer->Release();

	// red for encumberance
	auto color = D3DCOLOR_ARGB(64, 255, 0, 0);

	// vertex buffer
	struct vertexdata_t v[] =
	{
		// top left of equip place
		{ this->m_FontConfig.position.x + equipSlot.texturedata.xoffset, this->m_FontConfig.position.y + equipSlot.texturedata.yoffset, 0.0f, 1.0f, color },
		// top right
		{ this->m_FontConfig.position.x + equipSlot.texturedata.xoffset + (this->m_FontConfig.size * this->m_FontConfig.scale), this->m_FontConfig.position.y + equipSlot.texturedata.yoffset, 0.0f, 1.0f, color },
		// bottom right
		{ this->m_FontConfig.position.x + equipSlot.texturedata.xoffset + (this->m_FontConfig.size * this->m_FontConfig.scale), this->m_FontConfig.position.y + equipSlot.texturedata.yoffset + (this->m_FontConfig.size * this->m_FontConfig.scale), 0.0f, 1.0f, color },
		// bottom left
		{ this->m_FontConfig.position.x + equipSlot.texturedata.xoffset, this->m_FontConfig.position.y + equipSlot.texturedata.yoffset + (this->m_FontConfig.size * this->m_FontConfig.scale), 0.0f, 1.0f, color },
	};

	this->m_Direct3DDevice->CreateVertexBuffer(4 * sizeof(vertexdata_t), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_MANAGED, &this->m_pEncumberanceVertexBuffer);

	// copy our vertex data to memory where the buffer is
	void* pVerticies;
	if (this->m_pEncumberanceVertexBuffer->Lock(0, 0, (byte**)&pVerticies, D3DLOCK_DISCARD) < 0)
		return false;

	memcpy(pVerticies, v, sizeof(v));
	this->m_pEncumberanceVertexBuffer->Unlock();
	pVerticies = nullptr;

	return this->m_pEncumberanceIndexBuffer != nullptr && this->m_pEncumberanceVertexBuffer != nullptr;
}

IItem* EquipViewer::GetEquippedItem(uint32_t slotId)
{
	auto equippedItem = this->m_AshitaCore->GetMemoryManager()->GetInventory()->GetEquippedItem(slotId);
	if (equippedItem == nullptr || equippedItem->Index == 0)
		return nullptr;

	auto inventoryItem = this->m_AshitaCore->GetMemoryManager()->GetInventory()->GetContainerItem((equippedItem->Index & 0xFF00) / 256, (equippedItem->Index % 256));
	if (inventoryItem == nullptr || inventoryItem->Id < 1 || inventoryItem->Id == 65535)
		return nullptr;

	return this->m_AshitaCore->GetResourceManager()->GetItemById(inventoryItem->Id);
}

uint32_t EquipViewer::GetAmmoCount()
{
	auto equippedItem = this->m_AshitaCore->GetMemoryManager()->GetInventory()->GetEquippedItem((uint32_t)Ashita::FFXI::Enums::EquipmentSlot::Ammo);
	if (equippedItem == nullptr || equippedItem->Index == 0)
		return 0;

	auto inventoryItem = this->m_AshitaCore->GetMemoryManager()->GetInventory()->GetContainerItem((equippedItem->Index & 0xFF00) / 256, (equippedItem->Index % 256));
	if (inventoryItem == nullptr || inventoryItem->Id < 1 || inventoryItem->Id == 65535)
		return 0;

	auto resItem = this->m_AshitaCore->GetResourceManager()->GetItemById(inventoryItem->Id);
	if (resItem == nullptr)
		return 0;

	if (resItem->StackSize == 1)
		return 0;

	return inventoryItem->Count;
}

IDirect3DTexture8* EquipViewer::LoadItemTexture(IItem* item)
{
	if (item == nullptr || item->ImageSize == 0 || item->Bitmap == nullptr)
		return nullptr;

	auto ptr = malloc(item->ImageSize + 0x0E);
	if (ptr == nullptr)
	{
		this->m_AshitaCore->GetChatManager()->Writef(128, false, "[EquipViewer] Could not allocate memory for item image.");
		return nullptr;
	}

	auto ptrOffset = static_cast<uint8_t*>(ptr) + 0x02;
	if (ptrOffset == nullptr)
	{
		this->m_AshitaCore->GetChatManager()->Writef(128, false, "[EquipViewer] Could not allocate memory for item image.");
		return nullptr;
	}

	auto size = item->ImageSize + 0x0E;

	// copy image to allocated memory
	memcpy(ptr, (void*)item->Bitmap, size);
	memcpy(ptrOffset, &size, sizeof(uint32_t));

	// creeate texture
	D3DXIMAGE_INFO pImgInfo;
	LPDIRECT3DTEXTURE8 pTexture;
	auto result = D3DXCreateTextureFromFileInMemoryEx(this->m_Direct3DDevice, (LPCVOID)ptr, item->ImageSize + 0x0E, 0xFFFFFFFF, 0xFFFFFFFF, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, &pImgInfo, NULL, &pTexture);

	if (result != S_OK)
	{
		char buffer[255];
		auto error = D3DXGetErrorStringA(result, buffer, 255);

		this->m_AshitaCore->GetChatManager()->Writef(128, false, "[EquipViewer] Error creating item texture.");
		this->m_LogManager->Logf((uint32_t)Ashita::LogLevel::Error, "EquipViewer", "Error on D3DXCreateTextureFromFileInMemoryEx! HRESULT: %d. Error: %s.", error, buffer);
	}

	free(ptr);

	return pTexture;
}

void EquipViewer::UpdateEquipmentTextures(void)
{
	for (auto it = this->m_EquipmentData.begin(); it != this->m_EquipmentData.end(); ++it)
	{
		auto item = this->GetEquippedItem(it->slotId);

		// if item is nullptr, do some clean up
		if (item == nullptr)
		{
			if (it->texturedata.itemTexture != nullptr)
			{
				it->texturedata.itemTexture->Release();
				it->texturedata.itemTexture = nullptr;
			}

			it->itemId = 0;
		}
		else
		{
			if (it->texturedata.itemTexture == nullptr || it->itemId != item->Id)
			{
				if (it->texturedata.itemTexture != nullptr)
				{
					it->texturedata.itemTexture->Release();
					it->texturedata.itemTexture = nullptr;
				}

				auto texture = this->LoadItemTexture(item);

				if (texture != nullptr)
				{
					it->itemId = item->Id;
					it->texturedata.itemTexture = texture;
				}
			}
		}

	}
}

EquipViewer::~EquipViewer(void)
{

}