#include "RenderTiming.h"
#include "GameVersion.h"
#include "InterpolationHook.h"
#include "MenuMotion.h"

#include <windows.h>
#include <MinHook.h>
#include <algorithm>
#include <cstring>
#include <intrin.h>

namespace {

constexpr double UpdateRate = 60.0;
constexpr double MaxRenderRate = 120.0;
constexpr double StallSeconds = 0.25;
constexpr double TimerWakeAheadSeconds = 0.0004;
constexpr double HundredNanosecondsPerSecond = 10'000'000.0;
constexpr uintptr_t CallSize = 5;
constexpr size_t LimiterFlagOperand = 6;

#if defined(GAME_VERSION_1_7_9b)
using UpdateFunction = int(__cdecl*)();
constexpr uint8_t UpdatePrologue[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x83, 0xEC};
#else
using UpdateFunction = int(__stdcall*)(int);
constexpr uint8_t UpdatePrologue[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0x81, 0xEC};
#endif

using RenderFunction = void(__cdecl*)();
enum class Limiter { Pending, Active, Unavailable };

UpdateFunction originalUpdate = nullptr;
RenderFunction originalRender = nullptr;
uintptr_t imageBase = 0;
Limiter limiter = Limiter::Pending;
double counterFrequency = 0;
double nextUpdate = 0;
double nextRender = 0;
float preparedPhase = 0;
float renderPhase = 0;

double ReadCounter() {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return static_cast<double>(counter.QuadPart);
}

double PaceRender() {
    double now = ReadCounter();
    while (now < nextRender) {
        const double sleepSeconds = (nextRender - now) / counterFrequency - TimerWakeAheadSeconds;
        if (sleepSeconds > 0.0) {
            static const HANDLE timer = CreateWaitableTimerExW(
                nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_MODIFY_STATE | SYNCHRONIZE);
            LARGE_INTEGER due;
            due.QuadPart = -1 - static_cast<LONGLONG>(sleepSeconds * HundredNanosecondsPerSecond);
            if (!timer || !SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE) ||
                WaitForSingleObject(timer, INFINITE) != WAIT_OBJECT_0) {
                SwitchToThread();
            }
        } else {
            YieldProcessor();
        }
        now = ReadCounter();
    }

    nextRender = now + counterFrequency / MaxRenderRate;
    return now;
}

bool MatchesCode() {
    auto expected = GameVersion::FrameLimiterBytes;
    const auto relocatedFlag = static_cast<uint32_t>(imageBase + GameVersion::SwapIntervalFlagRva);
    std::memcpy(expected.data() + LimiterFlagOperand, &relocatedFlag, sizeof relocatedFlag);
    const auto* check = reinterpret_cast<const uint8_t*>(imageBase + GameVersion::VSyncRefreshCheckRva);
    return check[0] == 0x76 && check[1] == GameVersion::VSyncRefreshCheckOffset &&
        std::memcmp(reinterpret_cast<const void*>(imageBase + GameVersion::FrameLimiterRva),
                    expected.data(), expected.size()) == 0;
}

bool ChangeFrameLimiter(uintptr_t manager) {

    if (!MatchesCode()) return false;

    auto jump = GameVersion::FrameLimiterBytes;
    jump.fill(0x90);
    jump[0] = 0xE9;
    constexpr auto displacement = static_cast<int32_t>(GameVersion::MainLoopRva) -
        static_cast<int32_t>(GameVersion::FrameLimiterRva + CallSize);
    std::memcpy(jump.data() + 1, &displacement, sizeof displacement);
    auto* limiterSite = reinterpret_cast<void*>(imageBase + GameVersion::FrameLimiterRva);
    auto* vsyncCheck = reinterpret_cast<uint8_t*>(imageBase + GameVersion::VSyncRefreshCheckRva);

    DWORD limiterProtection;
    DWORD vsyncProtection;
    if (!VirtualProtect(limiterSite, jump.size(), PAGE_EXECUTE_READWRITE, &limiterProtection)) return false;
    if (!VirtualProtect(vsyncCheck, 1, PAGE_EXECUTE_READWRITE, &vsyncProtection)) {
        VirtualProtect(limiterSite, jump.size(), limiterProtection, &limiterProtection);
        return false;
    }
    *vsyncCheck = 0xEB;
    std::memcpy(limiterSite, jump.data(), jump.size());
    FlushInstructionCache(GetCurrentProcess(), vsyncCheck, 1);
    FlushInstructionCache(GetCurrentProcess(), limiterSite, jump.size());
    VirtualProtect(vsyncCheck, 1, vsyncProtection, &vsyncProtection);
    VirtualProtect(limiterSite, jump.size(), limiterProtection, &limiterProtection);

    const auto options = manager + GameVersion::VideoOptionsOffset;
    const bool requested = *reinterpret_cast<const uint8_t*>(options + GameVersion::VSyncOptionOffset) != 0;
    using SetVSync = void(__thiscall*)(void*, int);
    reinterpret_cast<SetVSync>(imageBase + GameVersion::SetVSyncRva)(reinterpret_cast<void*>(options), requested);
    return true;
}

bool PrepareFrame(double now) {
    const double period = counterFrequency / UpdateRate;
    if (nextUpdate == 0 || now - nextUpdate > counterFrequency * StallSeconds) nextUpdate = now;

    const bool advance = now + 1.0 >= nextUpdate;
    if (advance) nextUpdate += period;
    preparedPhase = static_cast<float>(std::clamp((now - (nextUpdate - period)) / period, 0.0, 1.0));
    return advance;
}

bool UpdateIsDue(const void* caller) {
    if (reinterpret_cast<uintptr_t>(caller) != imageBase + GameVersion::UpdateCallRva + CallSize ||
        limiter == Limiter::Unavailable) return true;

    if (limiter == Limiter::Pending) {
        const auto manager = *reinterpret_cast<const uintptr_t*>(imageBase + GameVersion::ManagerRva);
        if (!manager) return true;
        limiter = ChangeFrameLimiter(manager) ? Limiter::Active : Limiter::Unavailable;
        if (limiter == Limiter::Unavailable) return true;
    }

    return PrepareFrame(PaceRender());
}

#if defined(GAME_VERSION_1_7_9b)
int __cdecl HookedUpdate() {
    const void* caller = _ReturnAddress();
    if (!UpdateIsDue(caller)) return 0;
    const int result = originalUpdate();
    if (limiter == Limiter::Active &&
        reinterpret_cast<uintptr_t>(caller) == imageBase + GameVersion::UpdateCallRva + CallSize) CaptureMenuMotion();
    return result;
}
#else
int __stdcall HookedUpdate(int advance) {
    const void* caller = _ReturnAddress();
    if (!UpdateIsDue(caller)) return 0;
    const int result = originalUpdate(advance);
    if (limiter == Limiter::Active &&
        reinterpret_cast<uintptr_t>(caller) == imageBase + GameVersion::UpdateCallRva + CallSize) CaptureMenuMotion();
    return result;
}
#endif

void __cdecl HookedRender() {
    if (limiter != Limiter::Active ||
        reinterpret_cast<uintptr_t>(_ReturnAddress()) != imageBase + GameVersion::RenderCallRva + CallSize) {
        originalRender();
        return;
    }

    renderPhase = preparedPhase;
    const auto manager = *reinterpret_cast<const uintptr_t*>(imageBase + GameVersion::ManagerRva);

#if defined(GAME_VERSION_1_7_9b)
    const auto game = *reinterpret_cast<const uintptr_t*>(imageBase + GameVersion::GameRva);
    NativeMenuInterpolation nativeInterpolation(manager, game);
#else
    NativeMenuInterpolation nativeInterpolation(manager);
#endif
    originalRender();
    renderPhase = 0;
}

bool Calls(uintptr_t callerRva, uintptr_t targetRva) {
    const auto* call = reinterpret_cast<const uint8_t*>(imageBase + callerRva);
    int32_t displacement;
    std::memcpy(&displacement, call + 1, sizeof displacement);
    return call[0] == 0xE8 && callerRva + CallSize + displacement == targetRva;
}

}

void InitializeRenderTiming(uintptr_t moduleBase, std::size_t imageSize) {
    if (imageSize < GameVersion::FrameLimiterRva + GameVersion::FrameLimiterBytes.size() ||
        imageSize < GameVersion::VSyncRefreshCheckRva + 2 ||
        imageSize < GameVersion::UpdateCallRva + CallSize ||
        imageSize < GameVersion::RenderCallRva + CallSize ||
        imageSize < GameVersion::UpdateRva + sizeof UpdatePrologue ||
        imageSize < GameVersion::RenderRva + GameVersion::RenderPrologue.size() ||
        imageSize < GameVersion::ManagerRva + sizeof(uintptr_t)) {
        return;
    }

    imageBase = moduleBase;
    auto* update = reinterpret_cast<void*>(imageBase + GameVersion::UpdateRva);
    auto* render = reinterpret_cast<void*>(imageBase + GameVersion::RenderRva);
    const auto& renderPrologue = GameVersion::RenderPrologue;
    const bool matchesLimiter = MatchesCode();
    const bool matchesUpdateCall = Calls(GameVersion::UpdateCallRva, GameVersion::UpdateRva);
    const bool matchesRenderCall = Calls(GameVersion::RenderCallRva, GameVersion::RenderRva);
    const bool matchesUpdate = std::memcmp(update, UpdatePrologue, sizeof UpdatePrologue) == 0;
    const bool matchesRender = std::memcmp(render, renderPrologue.data(), renderPrologue.size()) == 0;
    if (!matchesLimiter || !matchesUpdateCall || !matchesRenderCall || !matchesUpdate || !matchesRender) return;

    LARGE_INTEGER frequency;
    if (!QueryPerformanceFrequency(&frequency)) return;
    counterFrequency = static_cast<double>(frequency.QuadPart);
    const auto renderStatus = MH_CreateHook(render, reinterpret_cast<void*>(&HookedRender),
                                            reinterpret_cast<void**>(&originalRender));
    if (renderStatus != MH_OK) return;
    const auto updateStatus = MH_CreateHook(update, reinterpret_cast<void*>(&HookedUpdate),
                                            reinterpret_cast<void**>(&originalUpdate));
    if (updateStatus != MH_OK) {
        MH_RemoveHook(render);
        return;
    }

    HMODULE self;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                            reinterpret_cast<LPCWSTR>(&HookedRender), &self) ||
        MH_EnableHook(render) != MH_OK || MH_EnableHook(update) != MH_OK) {
        MH_RemoveHook(update);
        MH_RemoveHook(render);
    }
}

float GetRenderPhase() {
    return renderPhase;
}
