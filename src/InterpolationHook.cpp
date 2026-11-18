#include "InterpolationHook.h"
#include "IsaacTypes.h"
#include "Config.h"
#include "PatternScanner.h"
#include "GameVersion.h"
#include "VisualMotion.h"
#include "RenderTiming.h"
#include <MinHook.h>
#include <cmath>
#include <cstring>
#include <intrin.h>

static AnimationState* s_curAnimState = nullptr;
static uintptr_t s_moduleBase = 0;
static bool s_visualMotionSupported = false;
static bool s_menuViewAdjusted = false;

typedef void(__thiscall* RenderFrame_t)(AnimationLayer* thisPtr, const Vector& position, int unk, const Vector& topLeftClamp, const Vector& bottomRightClamp, ANM2* animation);
typedef void(__thiscall* AnimationStateRender_t)(AnimationState* thisPtr, const Vector* position, const Vector* topLeftClamp, const Vector* bottomRightClamp);
using MenuRender_t = void(__cdecl*)();

static RenderFrame_t oRenderFrame = nullptr;
static AnimationStateRender_t oAnimationStateRender = nullptr;
static MenuRender_t oMenuRender = nullptr;

static uintptr_t GetManager() {
    return *reinterpret_cast<const uintptr_t*>(s_moduleBase + GameVersion::ManagerRva);
}

static uint32_t GetFrameCount(uintptr_t manager) {
    return manager ? *reinterpret_cast<const uint32_t*>(manager + GameVersion::FrameCountOffset) : 0;
}

struct SpriteMotion {
    VisualMotion motion;
    const AnimationState* state = nullptr;
    const void* animation = nullptr;
    const void* callSite = nullptr;
    uint32_t screen = 0;
    unsigned clampMask = 0;
    Vector topLeftClamp;
    Vector bottomRightClamp;
};

static Vector DrawPosition(AnimationState* state, const void* callSite, const Vector& position,
                           const Vector* topLeftClamp, const Vector* bottomRightClamp) {
    if (!s_visualMotionSupported || !g_Config.Enabled || s_menuViewAdjusted || !state || !state->_animData) {
        return position;
    }
    const uintptr_t manager = GetManager();
    if (!manager) return position;

    static std::array<SpriteMotion, 2048> sprites;
    const auto identity = reinterpret_cast<uintptr_t>(state);
    const auto call = reinterpret_cast<uintptr_t>(callSite);
    auto& sprite = sprites[((identity >> 4) ^ (call >> 3)) % sprites.size()];
    const uint32_t screen = *reinterpret_cast<const uint32_t*>(manager + GameVersion::ManagerStateOffset);
    const unsigned clampMask = (topLeftClamp ? 1u : 0u) | (bottomRightClamp ? 2u : 0u);
    const Vector topLeft = topLeftClamp ? *topLeftClamp : Vector{};
    const Vector bottomRight = bottomRightClamp ? *bottomRightClamp : Vector{};
    if (sprite.state != state || sprite.animation != state->_animation || sprite.callSite != callSite ||
        sprite.screen != screen || sprite.clampMask != clampMask ||
        !SamePosition(sprite.topLeftClamp, topLeft) || !SamePosition(sprite.bottomRightClamp, bottomRight)) {
        sprite = {VisualMotion{}, state, state->_animation, callSite, screen, clampMask, topLeft, bottomRight};
    }
    return sprite.motion.Sample(GetFrameCount(manager), position, 24.0f, GetRenderPhase());
}

static void __cdecl Hooked_MenuRender() {
    static VisualMotion menuMotion;
    static uintptr_t lastMenu = 0;
    static int lastSelection = 0;

    if (s_menuViewAdjusted) {
        oMenuRender();
        return;
    }
    const uintptr_t menu = *reinterpret_cast<const uintptr_t*>(s_moduleBase + GameVersion::MenuManagerRva);
    const uintptr_t manager = GetManager();
    if (!g_Config.Enabled || !menu || !manager ||
        !*reinterpret_cast<const uint8_t*>(menu + GameVersion::MenuScrollActiveOffset)) {
        menuMotion = {};
        oMenuRender();
        return;
    }

    const int selection = *reinterpret_cast<const int*>(menu + GameVersion::MenuSelectedOffset);
    if (menu != lastMenu || selection != lastSelection) {
        menuMotion = {};
        lastMenu = menu;
        lastSelection = selection;
    }

    auto& view = *reinterpret_cast<Vector*>(menu + GameVersion::MenuViewOffset);
    const Vector originalView = view;
    const Vector preview = menuMotion.Sample(GetFrameCount(manager), originalView, 160.0f, GetRenderPhase());
    if (SamePosition(preview, originalView)) {
        oMenuRender();
        return;
    }

    // Shift the whole menu pass, including text, then restore its real position.
    s_menuViewAdjusted = true;
    view = preview;
    oMenuRender();
    if (SamePosition(view, preview)) view = originalView;
    s_menuViewAdjusted = false;
}

// Accurate shortest-path angular difference in degrees
inline float _interpol_short_angle_dis(float from, float to) {
    float diff = fmodf(to - from, 360.0f);
    if (diff > 180.0f) {
        diff -= 360.0f;
    } else if (diff < -180.0f) {
        diff += 360.0f;
    }
    return diff;
}

inline float _interpol_angle_lerp(float from, float to, float perc) {
    return from + _interpol_short_angle_dis(from, to) * perc;
}

void __fastcall Hooked_AnimationStateRender(AnimationState* thisPtr, void* _edx, const Vector* position, const Vector* topLeftClamp, const Vector* bottomRightClamp) {
    AnimationState* prev = s_curAnimState;
    s_curAnimState = thisPtr;
    const Vector preview = position ? DrawPosition(thisPtr, _ReturnAddress(), *position, topLeftClamp, bottomRightClamp) : Vector{};
    oAnimationStateRender(thisPtr, position && !SamePosition(preview, *position) ? &preview : position,
                          topLeftClamp, bottomRightClamp);
    s_curAnimState = prev;
}

void __fastcall Hooked_RenderFrame(AnimationLayer* thisPtr, void* _edx, const Vector& position, int unk, const Vector& topLeftClamp, const Vector& bottomRightClamp, ANM2* animation) {
    if (!g_Config.Enabled || !thisPtr || !thisPtr->_animFrames || !s_curAnimState || !s_curAnimState->_isPlaying || unk < 0 || unk >= thisPtr->_numFrames) {
        oRenderFrame(thisPtr, position, unk, topLeftClamp, bottomRightClamp, animation);
        return;
    }

    AnimationFrame* ourframe = &thisPtr->_animFrames[unk];
    if (!ourframe->interpolated) {
        oRenderFrame(thisPtr, position, unk, topLeftClamp, bottomRightClamp, animation);
        return;
    }

    if (unk + 1 >= thisPtr->_numFrames) {
        oRenderFrame(thisPtr, position, unk, topLeftClamp, bottomRightClamp, animation);
        return;
    }

    AnimationFrame* nextframe = &thisPtr->_animFrames[unk + 1];

    // Two original engine phases make up an animation tick. Additional draws
    // sample between those phases without advancing the animation state.
    const float nativeHalfStep = GetFrameCount(GetManager()) % 2 == 0 ? 0.5f : 0.0f;
    float lerpappend = nativeHalfStep + GetRenderPhase() * 0.5f;
    if (animation) {
        lerpappend *= animation->_playbackSpeed;
    }

    float duration = (ourframe->duration > 0) ? (float)ourframe->duration : 1.0f;
    float lerpperc = (lerpappend + s_curAnimState->_animFrame - (float)ourframe->startFrame) / duration;
    if (lerpperc > 1.0f) {
        lerpperc = 1.0f;
    } else if (lerpperc < 0.0f) {
        lerpperc = 0.0f;
    }
    float lerpbegin = (1.0f - lerpperc);

    Vector oldscale = ourframe->scale;
    Vector oldpos = ourframe->pos;
    float oldrot = ourframe->rotation;

    ColorMod oldc;
    for (int i = 0; i < 3; i++) {
        oldc._offset[i] = ourframe->color._offset[i];
        ourframe->color._offset[i] = ourframe->color._offset[i] * lerpbegin + nextframe->color._offset[i] * lerpperc;
    }
    for (int i = 0; i < 4; i++) {
        oldc._tint[i] = ourframe->color._tint[i];
        ourframe->color._tint[i] = ourframe->color._tint[i] * lerpbegin + nextframe->color._tint[i] * lerpperc;
    }

    ourframe->scale.x = ourframe->scale.x * lerpbegin + nextframe->scale.x * lerpperc;
    ourframe->scale.y = ourframe->scale.y * lerpbegin + nextframe->scale.y * lerpperc;
    ourframe->pos.x = ourframe->pos.x * lerpbegin + nextframe->pos.x * lerpperc;
    ourframe->pos.y = ourframe->pos.y * lerpbegin + nextframe->pos.y * lerpperc;
    ourframe->rotation = _interpol_angle_lerp(ourframe->rotation, nextframe->rotation, lerpperc);

    oRenderFrame(thisPtr, position, unk, topLeftClamp, bottomRightClamp, animation);

    for (int i = 0; i < 3; i++) {
        ourframe->color._offset[i] = oldc._offset[i];
    }
    for (int i = 0; i < 4; i++) {
        ourframe->color._tint[i] = oldc._tint[i];
    }

    ourframe->scale = oldscale;
    ourframe->pos = oldpos;
    ourframe->rotation = oldrot;
}

bool InitializeHooks() {
    if (MH_Initialize() != MH_OK) {
        return false;
    }

    s_moduleBase = (uintptr_t)GetModuleHandleA(nullptr);

    // Pattern for AnimationLayer::RenderFrame in the selected game version
    uintptr_t pRenderFrame = PatternScanner::FindPattern(GameVersion::RenderFramePattern);
    if (pRenderFrame) {
        MH_CreateHook((LPVOID)pRenderFrame, (LPVOID)&Hooked_RenderFrame, (LPVOID*)&oRenderFrame);
    }

    // Shared pattern for AnimationState::Render in the supported versions.
    uintptr_t pAnimStateRender = PatternScanner::FindPattern("55 8B EC 83 E4 F8 51 53 56 57 8B F9 8B 57 04 85 D2 74 34 33 F6 39 72 1C");
    if (pAnimStateRender) {
        MH_CreateHook((LPVOID)pAnimStateRender, (LPVOID)&Hooked_AnimationStateRender, (LPVOID*)&oAnimationStateRender);
    }

    // The menu and sprite motion hooks read fields specific to the target PE.
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(s_moduleBase);
    const auto* pe = reinterpret_cast<const IMAGE_NT_HEADERS32*>(s_moduleBase + dos->e_lfanew);
    s_visualMotionSupported = pe->Signature == IMAGE_NT_SIGNATURE &&
        pe->FileHeader.TimeDateStamp == GameVersion::ExecutableTimestamp &&
        pe->OptionalHeader.SizeOfImage > GameVersion::MenuRenderRva + GameVersion::MenuRenderPrologue.size() &&
        pe->OptionalHeader.SizeOfImage > GameVersion::MenuManagerRva + sizeof(uintptr_t);
    if (s_visualMotionSupported) {
        auto* menuRender = reinterpret_cast<void*>(s_moduleBase + GameVersion::MenuRenderRva);
        const auto& prologue = GameVersion::MenuRenderPrologue;
        if (std::memcmp(menuRender, prologue.data(), prologue.size()) == 0) {
            MH_CreateHook(menuRender, reinterpret_cast<void*>(&Hooked_MenuRender), reinterpret_cast<void**>(&oMenuRender));
        }
    }

    MH_EnableHook(MH_ALL_HOOKS);
    if (s_visualMotionSupported && g_Config.Enabled) {
        InitializeRenderTiming(s_moduleBase, pe->OptionalHeader.SizeOfImage);
    }
    return true;
}

void UninitializeHooks() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}
