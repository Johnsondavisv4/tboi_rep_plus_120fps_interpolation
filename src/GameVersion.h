#pragma once
#include <array>
#include <cstdint>

#if defined(GAME_VERSION_GENERATED)
#include "GeneratedGameVersion.h"
#else
namespace GameVersion {
inline constexpr uintptr_t ManagerStateOffset = 0x08;

inline constexpr int MainMenuManagerState = 1;
inline constexpr uintptr_t CharacterPaperOffset = 0x37C;
inline constexpr int CharacterPaperLayerId = 3;
inline constexpr uintptr_t RoomRenderScrollOffset = 0x1204;
inline constexpr uintptr_t SecretsScrollInOffset = 0x134;
inline constexpr uintptr_t SecretsSelectionOffset = 0x13C;
inline constexpr char AnimationStateRenderPattern[] = "55 8B EC 83 E4 F8 51 53 56 57 8B F9 8B 57 04 85 D2 74 34 33 F6 39 72 1C";
inline constexpr std::array<uint8_t, 5> PlayerBodyRenderPrologue = {0x55, 0x8B, 0xEC, 0x6A, 0xFF};

#if defined(GAME_VERSION_1_7_9b) && defined(GAME_VERSION_1_9_7_15)
#error Select only one game version
#elif defined(GAME_VERSION_1_7_9b)

inline constexpr uintptr_t ManagerRva = 0x7FD680;
inline constexpr uintptr_t FrameCountOffset = 0x4A264;
inline constexpr uintptr_t GameRva = 0x7FD65C;
inline constexpr uintptr_t GameRoomOffset = 0x18190;
inline constexpr uint32_t ExecutableTimestamp = 0x63914F0A;
inline constexpr uintptr_t UpdateRva = 0x4B0010;
inline constexpr uintptr_t UpdateCallRva = 0x48BDF5;
inline constexpr uintptr_t RenderRva = 0x4B0600;
inline constexpr uintptr_t RenderCallRva = 0x48BDFA;
inline constexpr std::array<uint8_t, 12> RenderPrologue = {
    0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x81, 0xEC, 0x80, 0, 0, 0
};
inline constexpr uintptr_t MainLoopRva = 0x48BD40;
inline constexpr uintptr_t FrameLimiterRva = 0x48BE2E;
inline constexpr uintptr_t SwapIntervalFlagRva = 0x7C7B0D;
inline constexpr std::array<uint8_t, 16> FrameLimiterBytes = {
    0x84, 0xDB, 0x75, 0x0C, 0x38, 0x1D, 0, 0, 0, 0, 0x0F, 0x85, 0x02, 0xFF, 0xFF, 0xFF
};
inline constexpr uintptr_t VSyncRefreshCheckRva = 0x481312;
inline constexpr uint8_t VSyncRefreshCheckOffset = 0x15;
inline constexpr uintptr_t SetVSyncRva = 0x481280;
inline constexpr uintptr_t VideoOptionsOffset = 0x29DF8;
inline constexpr uintptr_t VSyncOptionOffset = 0x55;
inline constexpr uintptr_t MenuManagerRva = 0x7FD688;
inline constexpr uintptr_t MenuRenderRva = 0x4E19D0;
inline constexpr uintptr_t OptionsRenderRva = 0x513E20;
inline constexpr uintptr_t OptionsPositionOffset = 0x10;
inline constexpr uintptr_t PlayerChargeBarsRenderRva = 0x32ABA0;
inline constexpr uintptr_t PlayerBodyRenderRva = 0x352A50;
inline constexpr uintptr_t PlayerIsHoldingItemRva = 0x383490;
inline constexpr uintptr_t PlayerRenderPositionOffset = 0x19C;
inline constexpr std::array<uint8_t, 8> PlayerIsHoldingItemPrologue = {
    0x8B, 0x81, 0xA0, 0x03, 0x00, 0x00, 0x85, 0xC0
};
inline constexpr uintptr_t MenuScrollActiveOffset = 0x00;
inline constexpr uintptr_t MenuSelectedOffset = 0x38;
inline constexpr uintptr_t MenuViewOffset = 0x3C;
inline constexpr uintptr_t CharacterMenuOffset = 0x101C00;
inline constexpr uintptr_t CharacterWheelCountOffset = 0xB10;
inline constexpr uintptr_t CharacterWheelPositionsOffset = 0xC30;
inline constexpr uintptr_t CharacterWheelDepthsOffset = 0xC3C;
inline constexpr uintptr_t CharacterPageOffset = 0xDAC;
inline constexpr uintptr_t NativeInterpolationOffset = 0x29E73;
inline constexpr uintptr_t StatsMenuOffset = 0x1030E0;
inline constexpr uintptr_t SecretsSpriteOffset = 0xB38;
inline constexpr uintptr_t GamePauseMenuOffset = 0x10203C;
inline constexpr uintptr_t PauseSpriteOffset = 0x2C;
inline constexpr uintptr_t CharacterWheelRenderRva = 0x4A7080;
inline constexpr uintptr_t StatsRenderRva = 0x53F6A0;
inline constexpr std::array<uint8_t, 9> CharacterWheelPrologue = {0x55, 0x8B, 0xEC, 0x64, 0xA1, 0x2C, 0, 0, 0};
inline constexpr std::array<uint8_t, 6> StatsRenderPrologue = {0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x5C};
inline constexpr std::array<uint8_t, 7> OptionsRenderPrologue = {0x55, 0x8B, 0xEC, 0xF3, 0x0F, 0x10, 0x15};
inline constexpr std::array<uint8_t, 9> PlayerChargeBarsRenderPrologue = {
    0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x40, 0x53, 0x56, 0x57
};

inline constexpr std::array<uintptr_t, 17> MenuPositionOffsets = {
    MenuViewOffset, 0x514, 0x634 + 0x10, 0x80FAC + 8, 0x8130C + 0xC,
    CharacterMenuOffset + 0x38, 0x102BEC + 0xC, 0x102D50 + 0xC,
    0x102E80 + 8, StatsMenuOffset + 0x14, 0x10D4A4 + 0x10,
    0x10D710 + 8, 0x10D840 + 0xC, 0x10D968 + 8, 0x10DA8C, 0x10DE04 + 8,
    0x10DF28 + 8
};
inline constexpr std::array<uint8_t, 12> MenuRenderPrologue = {
    0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x81, 0xEC, 0xE0, 0x00, 0x00, 0x00
};
inline constexpr char RenderFramePattern[] = "53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 00 00 00 00 50 53 81 EC 90 01 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 EC 56 57 50 8D 45 F4 64 A3 00 00 00 00 80 79 0C 00";
#elif defined(GAME_VERSION_1_9_7_15)
inline constexpr uintptr_t ManagerRva = 0x80C09C;
inline constexpr uintptr_t FrameCountOffset = 0x4ABA8;
inline constexpr uintptr_t GameRva = 0x80C074;
inline constexpr uintptr_t GameRoomOffset = 0x18300;
inline constexpr uint32_t ExecutableTimestamp = 0x68D45723;
inline constexpr uintptr_t UpdateRva = 0x54CAD0;
inline constexpr uintptr_t UpdateCallRva = 0x5295D8;
inline constexpr uintptr_t RenderRva = 0x54D340;
inline constexpr uintptr_t RenderCallRva = 0x5295DD;
inline constexpr std::array<uint8_t, 9> RenderPrologue = {
    0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x83, 0xEC, 0x68
};
inline constexpr uintptr_t MainLoopRva = 0x529514;
inline constexpr uintptr_t FrameLimiterRva = 0x529611;
inline constexpr uintptr_t SwapIntervalFlagRva = 0x7D5795;
inline constexpr std::array<uint8_t, 16> FrameLimiterBytes = {
    0x84, 0xDB, 0x75, 0x0C, 0x38, 0x1D, 0, 0, 0, 0, 0x0F, 0x85, 0xF3, 0xFE, 0xFF, 0xFF
};
inline constexpr uintptr_t VSyncRefreshCheckRva = 0x51E1BE;
inline constexpr uint8_t VSyncRefreshCheckOffset = 0x15;
inline constexpr uintptr_t SetVSyncRva = 0x51E150;
inline constexpr uintptr_t VideoOptionsOffset = 0x2A334;
inline constexpr uintptr_t VSyncOptionOffset = 0x67;
inline constexpr uintptr_t MenuManagerRva = 0x80D018;
inline constexpr uintptr_t MenuRenderRva = 0x580E20;
inline constexpr uintptr_t OptionsRenderRva = 0x5A99A0;
inline constexpr uintptr_t OptionsPositionOffset = 0x14;
inline constexpr uintptr_t PlayerChargeBarsRenderRva = 0x360060;
inline constexpr uintptr_t PlayerBodyRenderRva = 0x38BBC0;
inline constexpr uintptr_t PlayerIsHoldingItemRva = 0x3BF4A0;
inline constexpr uintptr_t PlayerRenderPositionOffset = 0x1A4;
inline constexpr std::array<uint8_t, 8> PlayerIsHoldingItemPrologue = {
    0x8B, 0x81, 0x50, 0x04, 0x00, 0x00, 0x85, 0xC0
};
inline constexpr uintptr_t MenuScrollActiveOffset = 0x0C;
inline constexpr uintptr_t MenuSelectedOffset = 0x40;
inline constexpr uintptr_t MenuViewOffset = 0x44;
inline constexpr uintptr_t CharacterMenuOffset = 0x1530;
inline constexpr uintptr_t CharacterWheelCountOffset = 0xB18;
inline constexpr uintptr_t CharacterWheelPositionsOffset = 0xC38;
inline constexpr uintptr_t CharacterWheelDepthsOffset = 0xC44;
inline constexpr uintptr_t CharacterPageOffset = 0xDB4;
inline constexpr uintptr_t NativeInterpolationOffset = 0x2A3B8;
inline constexpr uintptr_t StatsMenuOffset = 0x2A14;
inline constexpr uintptr_t SecretsSpriteOffset = 0xB48;
inline constexpr uintptr_t CharacterWheelRenderRva = 0x544120;
inline constexpr uintptr_t StatsRenderRva = 0x5E2B00;
inline constexpr std::array<uint8_t, 12> CharacterWheelPrologue = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x64, 0xA1, 0x2C, 0, 0, 0};
inline constexpr std::array<uint8_t, 5> StatsRenderPrologue = {0x55, 0x8B, 0xEC, 0x6A, 0xFF};
inline constexpr std::array<uint8_t, 5> OptionsRenderPrologue = {0x55, 0x8B, 0xEC, 0x6A, 0xFF};
inline constexpr std::array<uint8_t, 12> PlayerChargeBarsRenderPrologue = {
    0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x83, 0xEC, 0x3C, 0x53, 0x56, 0x57
};
inline constexpr std::array<uintptr_t, 21> MenuPositionOffsets = {
    MenuViewOffset, 0x1D4, 0x2F8 + 0x10, 0xB90 + 8, 0xDD0 + 0xC,
    CharacterMenuOffset + 0x38, 0x2518 + 0x14, 0x2684 + 0xC,
    0x27B4 + 8, StatsMenuOffset + 0x14, 0xCEC8 + 0x14,
    0xD150 + 8, 0xD280 + 0xC, 0xD3A8 + 8, 0xD4CC, 0xD844 + 8,
    0xE690, 0xDA98 + 0x234, 0xEB98 + 0x14, 0xEE20 + 0x7DC,

    0xD968 + 8
};
inline constexpr std::array<uint8_t, 16> MenuRenderPrologue = {
    0x53, 0x8B, 0xDC, 0x83, 0xEC, 0x08, 0x83, 0xE4, 0xF8, 0x83, 0xC4, 0x04, 0x55, 0x8B, 0x6B, 0x04
};
inline constexpr char RenderFramePattern[] = "53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B ?? 89 6C 24 ?? 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 ?? ?? ?? ?? 50 53 81 EC 98 01 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 ?? 56 57 50";
#else
#error Select a supported GAME_VERSION through CMake
#endif
}
#endif
