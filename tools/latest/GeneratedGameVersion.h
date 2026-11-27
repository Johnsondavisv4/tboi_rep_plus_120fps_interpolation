#pragma once
// Game version 1.9.7.20.J514
// Release version 1.9.7.20
// Executable SHA-256 c8337b957b7805aee1eac0c49cc7a8ea7ba2e195c7f84a0ab41051406df9426a

#include <array>
#include <cstdint>

#define GAME_HAS_FAMILIAR_CHARGE_BARS

namespace GameVersion {
inline constexpr uint32_t ExecutableTimestamp = 0x6B0775BF;
inline constexpr uint32_t ExecutableImageSize = 0x954000;
inline constexpr uint32_t ExecutableChecksum = 0x0;
inline constexpr int MainMenuManagerState = 1;
inline constexpr int CharacterPaperLayerId = 3;
inline constexpr uint8_t VSyncRefreshCheckOffset = 0x17;
inline constexpr uintptr_t MainLoopRva = 0x5468B1;
inline constexpr uintptr_t FrameLimiterRva = 0x5469CD;
inline constexpr uintptr_t UpdateCallRva = 0x5C2E62;
inline constexpr uintptr_t UpdateRva = 0x56AA20;
inline constexpr uintptr_t RenderCallRva = 0x546971;
inline constexpr uintptr_t RenderRva = 0x56B300;
inline constexpr uintptr_t SetVSyncRva = 0x53AF90;
inline constexpr uintptr_t VSyncRefreshCheckRva = 0x53B045;
inline constexpr uintptr_t MenuRenderRva = 0x5A1930;
inline constexpr uintptr_t OptionsRenderRva = 0x5CACF0;
inline constexpr uintptr_t PlayerChargeBarsRenderRva = 0x366C90;
inline constexpr uintptr_t PlayerBodyRenderRva = 0x393FF0;
inline constexpr uintptr_t PlayerIsHoldingItemRva = 0x3C8020;
inline constexpr uintptr_t FamiliarChargeBarRenderRva = 0x238E20;
inline constexpr uintptr_t CharacterWheelRenderRva = 0x561910;
inline constexpr uintptr_t StatsRenderRva = 0x605200;
inline constexpr uintptr_t RenderFrameRva = 0xA3F0;
inline constexpr uintptr_t AnimationStateRenderRva = 0xB2F0;
inline constexpr uintptr_t ManagerRva = 0x89B194;
inline constexpr uintptr_t GameRva = 0x89A5F8;
inline constexpr uintptr_t MenuManagerRva = 0x89B9A4;
inline constexpr uintptr_t SwapIntervalFlagRva = 0x89F954;
inline constexpr uintptr_t FrameCountOffset = 0x4AE8C;
inline constexpr uintptr_t ManagerStateOffset = 0x8;
inline constexpr uintptr_t NativeInterpolationOffset = 0x2A690;
inline constexpr uintptr_t GameRoomOffset = 0x18300;
inline constexpr uintptr_t CharacterMenuOffset = 0x1534;
inline constexpr uintptr_t StatsMenuOffset = 0x2A18;
inline constexpr uintptr_t CharacterPaperOffset = 0x37C;
inline constexpr uintptr_t SecretsSpriteOffset = 0xB48;
inline constexpr uintptr_t SecretsSelectionOffset = 0x13C;
inline constexpr uintptr_t SecretsScrollListOffset = 0x138;
inline constexpr uintptr_t SecretsScrollInOffset = 0x134;
inline constexpr uintptr_t PlayerRenderPositionOffset = 0x1A4;
inline constexpr uintptr_t MenuViewOffset = 0x44;
inline constexpr uintptr_t OptionsPositionOffset = 0x14;
inline constexpr uintptr_t VideoOptionsOffset = 0x2A60C;
inline constexpr uintptr_t RoomRenderScrollOffset = 0x1204;
inline constexpr uintptr_t VSyncOptionOffset = 0x67;
inline constexpr uintptr_t CharacterWheelCountOffset = 0xB18;
inline constexpr uintptr_t CharacterWheelPositionsOffset = 0xC38;
inline constexpr uintptr_t CharacterWheelDepthsOffset = 0xC44;
inline constexpr uintptr_t CharacterPageOffset = 0xDB4;
inline constexpr std::array<uintptr_t, 21> MenuPositionOffsets = {0x44, 0x1D4, 0x304, 0xB98, 0xDE0, 0x156C, 0x2530, 0x2694, 0x27C0, 0x2A2C, 0xCEE0, 0xD15C, 0xD290, 0xD3B4, 0xD4D0, 0xD850, 0xE650, 0xDCCC, 0xEB6C, 0xF5BC, 0xD974};
inline constexpr std::array<uint8_t, 16> RenderPrologue = {0x53, 0x8B, 0xDC, 0x83, 0xEC, 0x08, 0x83, 0xE4, 0xF8, 0x83, 0xC4, 0x04, 0x55, 0x8B, 0x6B, 0x04};
inline constexpr std::array<uint8_t, 12> MenuRenderPrologue = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x81, 0xEC, 0x38, 0x01, 0x00, 0x00};
inline constexpr std::array<uint8_t, 10> CharacterWheelPrologue = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x81, 0xEC, 0x84, 0x00};
inline constexpr std::array<uint8_t, 16> StatsRenderPrologue = {0x53, 0x8B, 0xDC, 0x83, 0xEC, 0x08, 0x83, 0xE4, 0xF8, 0x83, 0xC4, 0x04, 0x55, 0x8B, 0x6B, 0x04};
inline constexpr std::array<uint8_t, 12> OptionsRenderPrologue = {0x53, 0x8B, 0xDC, 0x83, 0xEC, 0x08, 0x83, 0xE4, 0xF8, 0x83, 0xC4, 0x04};
inline constexpr std::array<uint8_t, 8> PlayerChargeBarsRenderPrologue = {0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x38, 0x56, 0x57};
inline constexpr std::array<uint8_t, 5> PlayerBodyRenderPrologue = {0x55, 0x8B, 0xEC, 0x6A, 0xFF};
inline constexpr std::array<uint8_t, 8> PlayerIsHoldingItemPrologue = {0x8B, 0x81, 0x50, 0x04, 0x00, 0x00, 0x85, 0xC0};
inline constexpr std::array<uint8_t, 7> FamiliarChargeBarRenderPrologue = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0xA1};
inline constexpr std::array<uint8_t, 20> FrameLimiterBytes = {0x84, 0xDB, 0x75, 0x10, 0xF7, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x0F, 0x85, 0xD0, 0xFE, 0xFF, 0xFF};
inline constexpr char RenderFramePattern[] = "53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 00 00 00 00 50 53 81 EC E8 01 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 EC 56 57 50 8D 45 F4 64 A3 00 00 00 00 80 79 0C 00 8B 43 08 8B 53 18 89 85 D8 FE FF FF";
inline constexpr char AnimationStateRenderPattern[] = "55 8B EC 83 E4 F8 51 53 56 57 8B F9 8B 57 04 85 D2 74 ?? 33 F6 39 72 1C 76 ?? 33 DB 0F 1F 40 00 FF 37 8B 47 08 FF 75 10 8B 4A 18 FF 75 0C 03 CB FF 34 B0 FF 75 08 E8 ?? ?? ?? ?? 8B 57 04 46 83 C3 10 3B 72 1C 72 ?? 5F 5E 5B 8B E5 5D C2 0C 00";
}
