#pragma once
#include <array>
#include <cstdint>

// Build-time profiles for each supported game executable.
namespace GameVersion {
inline constexpr uintptr_t ManagerStateOffset = 0x08;

#if (defined(GAME_VERSION_1_7_9b) + defined(GAME_VERSION_1_9_7_15) + defined(GAME_VERSION_1_9_7_17)) > 1
#error Select only one game version
#elif defined(GAME_VERSION_1_7_9b)
// Repentance v1.7.9b.J835, verified against REPENTOGON's ZHL definitions.
inline constexpr uintptr_t ManagerRva = 0x7FD680;
inline constexpr uintptr_t FrameCountOffset = 0x4A264;
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
inline constexpr uintptr_t MenuScrollActiveOffset = 0x00;
inline constexpr uintptr_t MenuSelectedOffset = 0x38;
inline constexpr uintptr_t MenuViewOffset = 0x3C;
inline constexpr std::array<uint8_t, 12> MenuRenderPrologue = {
    0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x81, 0xEC, 0xE0, 0x00, 0x00, 0x00
};
inline constexpr char RenderFramePattern[] = "53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 00 00 00 00 50 53 81 EC 90 01 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 EC 56 57 50 8D 45 F4 64 A3 00 00 00 00 80 79 0C 00";
#elif defined(GAME_VERSION_1_9_7_15)
inline constexpr uintptr_t ManagerRva = 0x80C09C; // J374
inline constexpr uintptr_t FrameCountOffset = 0x4ABA8;
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
inline constexpr uintptr_t MenuScrollActiveOffset = 0x0C;
inline constexpr uintptr_t MenuSelectedOffset = 0x40;
inline constexpr uintptr_t MenuViewOffset = 0x44;
inline constexpr std::array<uint8_t, 16> MenuRenderPrologue = {
    0x53, 0x8B, 0xDC, 0x83, 0xEC, 0x08, 0x83, 0xE4, 0xF8, 0x83, 0xC4, 0x04, 0x55, 0x8B, 0x6B, 0x04
};
inline constexpr char RenderFramePattern[] = "53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B ?? 89 6C 24 ?? 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 ?? ?? ?? ?? 50 53 81 EC 98 01 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 ?? 56 57 50";
#elif defined(GAME_VERSION_1_9_7_17)
inline constexpr uintptr_t ManagerRva = 0x87169C; // J460
inline constexpr uintptr_t FrameCountOffset = 0x4ABBC;
inline constexpr uint32_t ExecutableTimestamp = 0x69E6E3A7;
inline constexpr uintptr_t UpdateRva = 0x554CD0;
inline constexpr uintptr_t UpdateCallRva = 0x5AB6F2;
inline constexpr uintptr_t RenderRva = 0x5555C0;
inline constexpr uintptr_t RenderCallRva = 0x5312F1;
inline constexpr std::array<uint8_t, 16> RenderPrologue = {
    0x53, 0x8B, 0xDC, 0x83, 0xEC, 0x08, 0x83, 0xE4, 0xF8, 0x83, 0xC4, 0x04, 0x55, 0x8B, 0x6B, 0x04
};
inline constexpr uintptr_t MainLoopRva = 0x531231;
inline constexpr uintptr_t FrameLimiterRva = 0x53134D;
inline constexpr uintptr_t SwapIntervalFlagRva = 0x8798E4;
inline constexpr std::array<uint8_t, 20> FrameLimiterBytes = {
    0x84, 0xDB, 0x75, 0x10, 0xF7, 0x05, 0, 0, 0, 0, 0, 0x02, 0, 0,
    0x0F, 0x85, 0xD0, 0xFE, 0xFF, 0xFF
};
inline constexpr uintptr_t VSyncRefreshCheckRva = 0x525D95;
inline constexpr uint8_t VSyncRefreshCheckOffset = 0x17;
inline constexpr uintptr_t SetVSyncRva = 0x525CE0;
inline constexpr uintptr_t VideoOptionsOffset = 0x2A33C;
inline constexpr uintptr_t VSyncOptionOffset = 0x67;
inline constexpr uintptr_t MenuManagerRva = 0x872A20;
inline constexpr uintptr_t MenuRenderRva = 0x58A040;
inline constexpr uintptr_t MenuScrollActiveOffset = 0x0C;
inline constexpr uintptr_t MenuSelectedOffset = 0x40;
inline constexpr uintptr_t MenuViewOffset = 0x44;
inline constexpr std::array<uint8_t, 16> MenuRenderPrologue = {
    0x53, 0x8B, 0xDC, 0x83, 0xEC, 0x08, 0x83, 0xE4, 0xF8, 0x83, 0xC4, 0x04, 0x55, 0x8B, 0x6B, 0x04
};
inline constexpr char RenderFramePattern[] = "53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B ?? 89 6C 24 ?? 8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 ?? ?? ?? ?? 50 53 81 EC E8 01 00 00 A1 ?? ?? ?? ?? 33 C5 89 45 ?? 56 57 50 8D 45 ?? 64 A3 ?? ?? ?? ?? 80 79 0C 00";
#else
#error Select a supported GAME_VERSION through CMake
#endif
}
