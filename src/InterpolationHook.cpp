#include "InterpolationHook.h"
#include "IsaacTypes.h"
#include "Config.h"
#include "PatternScanner.h"
#include "GameVersion.h"
#include "VisualMotion.h"
#include "MenuMotion.h"
#include "RenderTiming.h"
#include <MinHook.h>
#include <cmath>
#include <cstring>
#include <intrin.h>
#include <map>

static AnimationState* s_curAnimState = nullptr;
static float s_curAnimationFrame = 0;
static uintptr_t s_moduleBase = 0;
static bool s_visualMotionSupported = false;
static bool s_useNativePositions = false;
static bool s_menuRendering = false;
static MenuMotion s_menuMotion;

struct CharacterPaperDraw {
    uintptr_t panel = 0;
    const void* clip = nullptr;
    uint32_t frame = 0;
    Vector offset;
    Vector scale;
};
static CharacterPaperDraw s_characterPaperDraw;

typedef void(__thiscall* RenderFrame_t)(AnimationLayer* thisPtr, const Vector& position, int unk, const Vector& topLeftClamp, const Vector& bottomRightClamp, ANM2* animation);
typedef void(__thiscall* AnimationStateRender_t)(AnimationState* thisPtr, const Vector* position, const Vector* topLeftClamp, const Vector* bottomRightClamp);
using MenuRender_t = void(__cdecl*)();
using CharacterWheelRender_t = void(__thiscall*)(void*, const Vector&, const Vector&, const Vector&);
using StatsRender_t = void(__thiscall*)(void*);

static RenderFrame_t oRenderFrame = nullptr;
static AnimationStateRender_t oAnimationStateRender = nullptr;
static MenuRender_t oMenuRender = nullptr;
static CharacterWheelRender_t oCharacterWheelRender = nullptr;
static StatsRender_t oStatsRender = nullptr;

using OptionsRender_t = void(__thiscall*)(void*);
static OptionsRender_t oOptionsRender = nullptr;

using ChargeBarsRender_t = void(__thiscall*)(void*, const Vector&);
static ChargeBarsRender_t oPlayerChargeBarsRender = nullptr;
#if defined(GAME_HAS_FAMILIAR_CHARGE_BARS)
static ChargeBarsRender_t oFamiliarChargeBarRender = nullptr;
#endif

using PlayerBodyRender_t = void(__thiscall*)(void*, const Vector&, bool);
using PlayerIsHoldingItem_t = bool(__thiscall*)(void*);
static PlayerBodyRender_t oPlayerBodyRender = nullptr;
static PlayerIsHoldingItem_t s_playerIsHoldingItem = nullptr;

static uintptr_t GetManager() {
    return *reinterpret_cast<const uintptr_t*>(s_moduleBase + GameVersion::ManagerRva);
}

static uint32_t GetFrameCount(uintptr_t manager) {
    return manager ? *reinterpret_cast<const uint32_t*>(manager + GameVersion::FrameCountOffset) : 0;
}

void CaptureMenuMotion() {
    if (!s_visualMotionSupported || !g_Config.Enabled) { s_menuMotion = {}; return; }
    const uintptr_t manager = GetManager();
    const uintptr_t menu = *reinterpret_cast<const uintptr_t*>(s_moduleBase + GameVersion::MenuManagerRva);
    if (!manager || !menu ||
        *reinterpret_cast<const int*>(manager + GameVersion::ManagerStateOffset) != GameVersion::MainMenuManagerState) {
        s_menuMotion = {};
        return;
    }
    s_menuMotion.Observe(menu, GetFrameCount(manager));
}

struct SpriteMotion {
    VisualMotion motion;
    VisualMotion animationMotion;
    const AnimationState* state = nullptr;
    const void* animation = nullptr;
    const void* animationData = nullptr;
    const void* callSite = nullptr;
    float animationFrame = 0;
    bool playing = false;
    uint32_t screen = 0;
    uint32_t scrollRevision = 0;
    uint32_t lastSeenFrame = 0;
    unsigned clampMask = 0;
    Vector topLeftClamp;
    Vector bottomRightClamp;
};

struct RoomScroll {
    uintptr_t room = 0;
    Vector offset;
    uint32_t changedFrame = 0;
    uint32_t revision = 0;
};

static const RoomScroll& GetRoomScroll(uint32_t frame) {
    const uintptr_t game = *reinterpret_cast<const uintptr_t*>(s_moduleBase + GameVersion::GameRva);
    const uintptr_t room = game ? *reinterpret_cast<const uintptr_t*>(game + GameVersion::GameRoomOffset) : 0;
    const Vector offset = room ? *reinterpret_cast<const Vector*>(room + GameVersion::RoomRenderScrollOffset) : Vector{};
    static RoomScroll scroll;
    if (scroll.room != room || !SamePosition(scroll.offset, offset)) {
        scroll = {room, offset, frame, scroll.revision + 1};
    }
    return scroll;
}

struct PlayerPositionMotion {
    VisualMotion motion;
    uint32_t scrollRevision = 0;
    uint32_t lastSeenFrame = 0;
    uint32_t lastHoldingFrame = 0;
    bool holdingRecently = false;
    float phase = 0;
    bool hasPreview = false;
    Vector displacement;
};

struct PlayerBodyDraw {
    const Vector& anchor;
    VisualMotion& motion;
    uint32_t frame;
    float phase;
    bool interpolate;
    bool sampled = false;
    Vector displacement;

    Vector Position(const Vector& position, const Vector& fallback) {
        if (!sampled && std::isfinite(anchor.x) && std::isfinite(anchor.y)) {
            const Vector preview = motion.Sample(frame, anchor, 24.0f, phase);
            displacement = {preview.x - anchor.x, preview.y - anchor.y};
            sampled = true;
        }
        return interpolate ? Vector{position.x + displacement.x, position.y + displacement.y} : fallback;
    }
};

static std::map<const void*, PlayerPositionMotion> s_playerMotion;
static PlayerBodyDraw* s_playerBodyDraw = nullptr;

static void __fastcall Hooked_PlayerBodyRender(void* player, void*, const Vector& offset, bool drawHeldItem) {
    PlayerBodyDraw* previous = s_playerBodyDraw;
    s_playerBodyDraw = nullptr;
    const auto manager = g_Config.Enabled && drawHeldItem && player ? GetManager() : 0;
    if (!manager) {
        oPlayerBodyRender(player, offset, drawHeldItem);
        s_playerBodyDraw = previous;
        return;
    }

    const uint32_t frame = GetFrameCount(manager);
    static uint32_t cleanedFrame = 0;
    if (frame - cleanedFrame >= 120) {
        for (auto entry = s_playerMotion.begin(); entry != s_playerMotion.end();) {
            if (frame - entry->second.lastSeenFrame >= 120) entry = s_playerMotion.erase(entry);
            else ++entry;
        }
        cleanedFrame = frame;
    }
    auto& history = s_playerMotion[player];
    const RoomScroll& scroll = GetRoomScroll(frame);
    if (history.scrollRevision != scroll.revision) {
        history = {};
        history.scrollRevision = scroll.revision;
    }
    history.lastSeenFrame = frame;
    history.hasPreview = false;
    if (s_playerIsHoldingItem(player)) {
        history.lastHoldingFrame = frame;
        history.holdingRecently = true;
    }
    history.holdingRecently = history.holdingRecently && frame - history.lastHoldingFrame <= 1;
    const auto& anchor = *reinterpret_cast<const Vector*>(reinterpret_cast<uintptr_t>(player) + GameVersion::PlayerRenderPositionOffset);
    const bool interpolate = history.holdingRecently && (!scroll.room || scroll.changedFrame != frame);
    PlayerBodyDraw draw{anchor, history.motion, frame, GetRenderPhase(), interpolate};
    s_playerBodyDraw = &draw;
    oPlayerBodyRender(player, offset, drawHeldItem);
    s_playerBodyDraw = previous;
    history.hasPreview = draw.interpolate && draw.sampled;
    history.phase = draw.phase;
    history.displacement = draw.displacement;
}

struct SpriteDraw {
    Vector position;
    float animationFrame;
};

static SpriteDraw PrepareDraw(AnimationState* state, const void* callSite, const Vector& position,
                              const Vector* topLeftClamp, const Vector* bottomRightClamp) {
    SpriteDraw draw{position, state ? state->_animFrame : 0.0f};
    if (!s_visualMotionSupported || !g_Config.Enabled || !state || !state->_animData) {
        return draw;
    }
    const uintptr_t manager = GetManager();
    if (!manager) return draw;

    const uint32_t frame = GetFrameCount(manager);
    const RoomScroll& scroll = GetRoomScroll(frame);
    static std::map<std::pair<uintptr_t, uintptr_t>, SpriteMotion> sprites;
    static uint32_t cleanedFrame = 0;
    if (frame - cleanedFrame >= 120) {
        for (auto entry = sprites.begin(); entry != sprites.end();) {
            if (frame - entry->second.lastSeenFrame >= 120) entry = sprites.erase(entry);
            else ++entry;
        }
        cleanedFrame = frame;
    }
    const auto identity = reinterpret_cast<uintptr_t>(state);
    const auto call = reinterpret_cast<uintptr_t>(callSite);
    auto& sprite = sprites[{identity, call}];
    const uint32_t screen = *reinterpret_cast<const uint32_t*>(manager + GameVersion::ManagerStateOffset);
    const unsigned clampMask = (topLeftClamp ? 1u : 0u) | (bottomRightClamp ? 2u : 0u);
    const Vector topLeft = topLeftClamp ? *topLeftClamp : Vector{};
    const Vector bottomRight = bottomRightClamp ? *bottomRightClamp : Vector{};

    const bool animationChanged = sprite.animation != state->_animation ||
        sprite.animationData != state->_animData || sprite.playing != state->_isPlaying;
    if (sprite.state != state || animationChanged || sprite.callSite != callSite ||
        sprite.screen != screen || sprite.clampMask != clampMask ||
        !SamePosition(sprite.topLeftClamp, topLeft) || !SamePosition(sprite.bottomRightClamp, bottomRight)) {
        sprite = {};
        sprite.state = state;
        sprite.animation = state->_animation;
        sprite.animationData = state->_animData;
        sprite.callSite = callSite;
        sprite.screen = screen;
        sprite.clampMask = clampMask;
        sprite.topLeftClamp = topLeft;
        sprite.bottomRightClamp = bottomRight;
    } else if (state->_animFrame < sprite.animationFrame) {
        sprite.animationMotion = {};
    }
    sprite.animationFrame = state->_animFrame;
    sprite.playing = state->_isPlaying;
    sprite.lastSeenFrame = frame;
    draw.animationFrame = sprite.animationMotion.Sample(frame, {state->_animFrame, 0}, 8.0f, GetRenderPhase()).x;

    if (s_useNativePositions || sprite.scrollRevision != scroll.revision ||
        (scroll.room && scroll.changedFrame == frame)) {
        sprite.motion = {};
        sprite.scrollRevision = scroll.revision;
    } else {
        draw.position = sprite.motion.Sample(frame, position, 24.0f, GetRenderPhase());
    }
    return draw;
}

static void __cdecl Hooked_MenuRender() {
    if (s_menuRendering) {
        oMenuRender();
        return;
    }
    const uintptr_t menu = *reinterpret_cast<const uintptr_t*>(s_moduleBase + GameVersion::MenuManagerRva);
    const uintptr_t manager = GetManager();
    if (!g_Config.Enabled || !menu || !manager) {
        s_menuMotion = {};
        oMenuRender();
        return;
    }
    s_menuRendering = true;
    s_characterPaperDraw = {};

    const bool hasSamples = s_menuMotion.menu == menu && s_menuMotion.engineFrame == GetFrameCount(manager);
    const bool previous = s_useNativePositions;
    s_useNativePositions = previous || hasSamples;
    if (hasSamples) {
        const float phase = GetRenderPhase();
        MenuPositionPreview positions(s_menuMotion, phase);
        oMenuRender();
    } else {
        oMenuRender();
    }
    s_useNativePositions = previous;
    s_menuRendering = false;
}

static void __fastcall Hooked_OptionsRender(void* thisPtr, void*) {
    const uintptr_t manager = GetManager();
    if (!g_Config.Enabled || !manager || !thisPtr || s_useNativePositions) {
        oOptionsRender(thisPtr);
        return;
    }

    static void* panel = nullptr;
    static VisualMotion motion;
    if (panel != thisPtr) {
        panel = thisPtr;
        motion = {};
    }

    auto& position = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(thisPtr) + GameVersion::OptionsPositionOffset);
    const Vector original = position;
    const Vector preview = motion.Sample(GetFrameCount(manager), original, 160.0f, GetRenderPhase());
    position = preview;
    s_useNativePositions = true;
    oOptionsRender(thisPtr);
    s_useNativePositions = false;
    if (SamePosition(position, preview)) position = original;
}

template<ChargeBarsRender_t& render>
static void __fastcall Hooked_ChargeBarsRender(void* thisPtr, void*, const Vector& position) {
    const bool previous = s_useNativePositions;
    s_useNativePositions = true;
    render(thisPtr, position);
    s_useNativePositions = previous;
}

static void __fastcall Hooked_PlayerChargeBarsRender(void* player, void* edx, const Vector& position) {
    const auto entry = s_playerMotion.find(player);
    Vector shown = position;
    if (g_Config.Enabled && entry != s_playerMotion.end()) {
        const auto& history = entry->second;
        const auto manager = GetManager();
        if (manager && history.hasPreview && history.lastSeenFrame == GetFrameCount(manager) &&
            history.phase == GetRenderPhase()) {
            shown.x += history.displacement.x;
            shown.y += history.displacement.y;
        }
    }
    Hooked_ChargeBarsRender<oPlayerChargeBarsRender>(player, edx, shown);
}

static void __fastcall Hooked_CharacterWheelRender(void* thisPtr, void*, const Vector& position,
                                                 const Vector& offset, const Vector& scale) {
    static MenuDrawClock clock;
    static CharacterWheelMotion motion;
    static bool rendering = false;
    const uintptr_t manager = s_visualMotionSupported ? GetManager() : 0;
    if (rendering) {
        oCharacterWheelRender(thisPtr, position, offset, scale);
        return;
    }
    if (!g_Config.Enabled || !manager || !thisPtr) {
        clock = {};
        motion = {};
        oCharacterWheelRender(thisPtr, position, offset, scale);
        return;
    }
    const uint32_t frame = GetFrameCount(manager);
    if (clock.Observe(reinterpret_cast<uintptr_t>(thisPtr), frame)) {
        if (clock.tick == 1) motion = {};
        motion.Observe(reinterpret_cast<uintptr_t>(thisPtr), clock.tick);
    }
    const bool previous = s_useNativePositions;
    s_useNativePositions = true;
    rendering = true;
    {
        CharacterWheelPreview preview(motion, GetRenderPhase());
        const auto panel = reinterpret_cast<uintptr_t>(thisPtr);
        const auto* paper = reinterpret_cast<const ANM2*>(panel + GameVersion::CharacterPaperOffset);

        const bool followsPaper = s_menuRendering &&
            s_characterPaperDraw.panel == panel && s_characterPaperDraw.frame == frame &&
            s_characterPaperDraw.clip == paper->_animState._animData;
        const Vector shownOffset = followsPaper ? s_characterPaperDraw.offset : offset;
        const Vector shownScale = followsPaper ? s_characterPaperDraw.scale : scale;
        oCharacterWheelRender(thisPtr, position, shownOffset, shownScale);
    }
    rendering = false;
    s_useNativePositions = previous;
}

static void __fastcall Hooked_StatsRender(void* thisPtr, void*) {
    static SecretsMotion motion;
    static bool rendering = false;
    const uintptr_t manager = s_visualMotionSupported ? GetManager() : 0;
    if (rendering) {
        oStatsRender(thisPtr);
        return;
    }
    if (!g_Config.Enabled || !manager || !thisPtr) {
        motion = {};
        oStatsRender(thisPtr);
        return;
    }
    const uint32_t frame = GetFrameCount(manager);
    const auto panel = reinterpret_cast<uintptr_t>(thisPtr);
    motion.Observe(panel, frame);
    const bool previous = s_useNativePositions;
    s_useNativePositions = true;
    rendering = true;
    {
        SecretsPreview preview(motion, GetRenderPhase());
        oStatsRender(thisPtr);
    }
    rendering = false;
    s_useNativePositions = previous;
}

static float ShortestAngleDelta(float from, float to) {
    float diff = fmodf(to - from, 360.0f);
    if (diff > 180.0f) {
        diff -= 360.0f;
    } else if (diff < -180.0f) {
        diff += 360.0f;
    }
    return diff;
}

void __fastcall Hooked_AnimationStateRender(AnimationState* thisPtr, void* _edx, const Vector* position, const Vector* topLeftClamp, const Vector* bottomRightClamp) {
    AnimationState* prev = s_curAnimState;
    const float previousFrame = s_curAnimationFrame;
    s_curAnimState = thisPtr;
    SpriteDraw draw = position ? PrepareDraw(thisPtr, _ReturnAddress(), *position, topLeftClamp, bottomRightClamp)
                               : SpriteDraw{{}, thisPtr ? thisPtr->_animFrame : 0.0f};
    if (position && s_playerBodyDraw) {
        draw.position = s_playerBodyDraw->Position(*position, draw.position);
    }
    s_curAnimationFrame = draw.animationFrame;
    oAnimationStateRender(thisPtr, position && !SamePosition(draw.position, *position) ? &draw.position : position,
                          topLeftClamp, bottomRightClamp);
    s_curAnimState = prev;
    s_curAnimationFrame = previousFrame;
}

static void ForwardRenderFrame(AnimationLayer* layer, const Vector& position, int key,
                               const Vector& topLeftClamp, const Vector& bottomRightClamp,
                               ANM2* animation, AnimationState* state) {
    if (g_Config.Enabled && s_visualMotionSupported && s_menuRendering && animation &&
        state == &animation->_animState && layer && layer->_visible && layer->_animFrames &&
        layer->_layerID == GameVersion::CharacterPaperLayerId && key >= 0 && key < layer->_numFrames) {
        const uintptr_t menu = *reinterpret_cast<const uintptr_t*>(s_moduleBase + GameVersion::MenuManagerRva);
        const uintptr_t panel = menu ? menu + GameVersion::CharacterMenuOffset : 0;
        if (panel && reinterpret_cast<uintptr_t>(animation) == panel + GameVersion::CharacterPaperOffset) {
            const auto& drawn = layer->_animFrames[key];
            if (drawn.visible) {

                s_characterPaperDraw = {panel, state->_animData, GetFrameCount(GetManager()),
                    {drawn.pos.x - drawn.pivot.x * drawn.scale.x,
                     drawn.pos.y - drawn.pivot.y * drawn.scale.y}, drawn.scale};
            }
        }
    }
    oRenderFrame(layer, position, key, topLeftClamp, bottomRightClamp, animation);
}

static bool IsSecretsPanel(const ANM2* animation) {
    if (!s_menuRendering || !s_visualMotionSupported || !animation) return false;
    const uintptr_t menu = *reinterpret_cast<const uintptr_t*>(s_moduleBase + GameVersion::MenuManagerRva);
    return menu && reinterpret_cast<uintptr_t>(animation) ==
        menu + GameVersion::StatsMenuOffset + GameVersion::SecretsSpriteOffset;
}

static bool CanInterpolate(const AnimationFrame& from, const AnimationFrame& to, bool isSecretsPanel) {
    return (from.interpolated || isSecretsPanel) && from.visible && to.visible &&
        from.width == to.width && from.height == to.height &&
        (SamePosition(from.pivot, to.pivot) || SamePosition(from.crop, to.crop));
}

static void RenderInterpolatedFrame(AnimationLayer* thisPtr, const Vector& position, int unk,
                                    const Vector& topLeftClamp, const Vector& bottomRightClamp, ANM2* animation,
                                    AnimationState* state, float renderFrame) {
    if (!g_Config.Enabled || !thisPtr || !thisPtr->_animFrames || !state || !state->_isPlaying || unk < 0 || unk >= thisPtr->_numFrames) {
        ForwardRenderFrame(thisPtr, position, unk, topLeftClamp, bottomRightClamp, animation, state);
        return;
    }

    const bool isSecretsPanel = IsSecretsPanel(animation);
    auto* frames = thisPtr->_animFrames;
    const auto& nativeFrame = frames[unk];
    if (state->_animFrame >= nativeFrame.startFrame &&
        state->_animFrame < nativeFrame.startFrame + nativeFrame.duration) {
        while (unk > 0 && renderFrame < frames[unk].startFrame) {
            const auto& previous = frames[unk - 1];
            const auto& current = frames[unk];
            if (!CanInterpolate(previous, current, isSecretsPanel) || !SamePosition(previous.crop, current.crop)) {
                renderFrame = static_cast<float>(current.startFrame);
                break;
            }
            --unk;
        }
    } else {
        renderFrame = state->_animFrame;
    }

    if (unk + 1 >= thisPtr->_numFrames || !CanInterpolate(frames[unk], frames[unk + 1], isSecretsPanel)) {
        ForwardRenderFrame(thisPtr, position, unk, topLeftClamp, bottomRightClamp, animation, state);
        return;
    }

    auto* ourframe = &frames[unk];
    const auto* nextframe = &frames[unk + 1];
    float duration = (ourframe->duration > 0) ? (float)ourframe->duration : 1.0f;
    float lerpperc = (renderFrame - static_cast<float>(ourframe->startFrame)) / duration;
    if (lerpperc > 1.0f) {
        lerpperc = 1.0f;
    } else if (lerpperc < 0.0f) {
        lerpperc = 0.0f;
    }
    float lerpbegin = (1.0f - lerpperc);

    Vector oldscale = ourframe->scale;
    Vector oldpos = ourframe->pos;
    const Vector oldPivot = ourframe->pivot;
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

    const auto blendScale = [&](float from, float to, float nativeScale) {
        const bool mirrored = (from < 0.0f && to > 0.0f) || (from > 0.0f && to < 0.0f);
        if (ourframe->duration <= 1 && mirrored) {
            return std::copysign(std::abs(from) * lerpbegin + std::abs(to) * lerpperc, nativeScale);
        }
        return from * lerpbegin + to * lerpperc;
    };
    ourframe->scale.x = blendScale(oldscale.x, nextframe->scale.x, nativeFrame.scale.x);
    ourframe->scale.y = blendScale(oldscale.y, nextframe->scale.y, nativeFrame.scale.y);
    ourframe->pos.x = ourframe->pos.x * lerpbegin + nextframe->pos.x * lerpperc;
    ourframe->pos.y = ourframe->pos.y * lerpbegin + nextframe->pos.y * lerpperc;
    ourframe->pivot.x = oldPivot.x + (nextframe->pivot.x - oldPivot.x) * lerpperc;
    ourframe->pivot.y = oldPivot.y + (nextframe->pivot.y - oldPivot.y) * lerpperc;
    const float angleDelta = ourframe->duration <= 2 ? ShortestAngleDelta(oldrot, nextframe->rotation)
                                                    : nextframe->rotation - oldrot;
    ourframe->rotation = oldrot + angleDelta * lerpperc;

    ForwardRenderFrame(thisPtr, position, unk, topLeftClamp, bottomRightClamp, animation, state);

    for (int i = 0; i < 3; i++) {
        ourframe->color._offset[i] = oldc._offset[i];
    }
    for (int i = 0; i < 4; i++) {
        ourframe->color._tint[i] = oldc._tint[i];
    }

    ourframe->scale = oldscale;
    ourframe->pos = oldpos;
    ourframe->pivot = oldPivot;
    ourframe->rotation = oldrot;
}

static AnimationState* FindMenuLayerState(ANM2* animation, const AnimationLayer* layer) {
    if (!animation || !layer) return nullptr;
    for (AnimationState* state : {&animation->_animState, &animation->_overlayAnimState}) {
        if (state->_animation != animation || !state->_animData) continue;
        const auto* data = static_cast<const AnimationData*>(state->_animData);
        const auto first = reinterpret_cast<uintptr_t>(data->_animLayers);
        const auto address = reinterpret_cast<uintptr_t>(layer);

        if (first && address >= first && data->_layerCount > 0 &&
            (address - first) % sizeof(AnimationLayer) == 0 &&
            (address - first) / sizeof(AnimationLayer) < static_cast<size_t>(data->_layerCount)) return state;
    }
    return nullptr;
}

void __fastcall Hooked_RenderFrame(AnimationLayer* thisPtr, void* _edx, const Vector& position, int unk,
                                   const Vector& topLeftClamp, const Vector& bottomRightClamp, ANM2* animation) {
    AnimationState* state = s_curAnimState;
    float renderFrame = s_curAnimationFrame;

    if (!state && g_Config.Enabled && s_visualMotionSupported &&
        (s_menuRendering || (animation && reinterpret_cast<uintptr_t>(animation) == GetPauseAnimation(
            GetManager(), *reinterpret_cast<const uintptr_t*>(s_moduleBase + GameVersion::GameRva))))) {
        state = FindMenuLayerState(animation, thisPtr);
        if (state) renderFrame = PrepareDraw(state, _ReturnAddress(), position, &topLeftClamp, &bottomRightClamp).animationFrame;
    }
    const Vector shown = !s_curAnimState && s_playerBodyDraw ? s_playerBodyDraw->Position(position, position) : position;
    RenderInterpolatedFrame(thisPtr, shown, unk, topLeftClamp, bottomRightClamp, animation, state, renderFrame);
}

bool InitializeHooks() {
    s_moduleBase = (uintptr_t)GetModuleHandleA(nullptr);
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(s_moduleBase);
    const auto* pe = reinterpret_cast<const IMAGE_NT_HEADERS32*>(s_moduleBase + dos->e_lfanew);
#if defined(GAME_VERSION_GENERATED)
    if (pe->Signature != IMAGE_NT_SIGNATURE ||
        pe->FileHeader.TimeDateStamp != GameVersion::ExecutableTimestamp ||
        pe->OptionalHeader.SizeOfImage != GameVersion::ExecutableImageSize ||
        pe->OptionalHeader.CheckSum != GameVersion::ExecutableChecksum) {
        return false;
    }
#endif
    if (MH_Initialize() != MH_OK) {
        return false;
    }

    uintptr_t pRenderFrame = PatternScanner::FindPattern(GameVersion::RenderFramePattern);
    if (pRenderFrame) {
        MH_CreateHook((LPVOID)pRenderFrame, (LPVOID)&Hooked_RenderFrame, (LPVOID*)&oRenderFrame);
    }

    uintptr_t pAnimStateRender = PatternScanner::FindPattern(GameVersion::AnimationStateRenderPattern);
    if (pAnimStateRender) {
        MH_CreateHook((LPVOID)pAnimStateRender, (LPVOID)&Hooked_AnimationStateRender, (LPVOID*)&oAnimationStateRender);
    }

    s_visualMotionSupported = pe->Signature == IMAGE_NT_SIGNATURE &&
        pe->FileHeader.TimeDateStamp == GameVersion::ExecutableTimestamp &&
        pe->OptionalHeader.SizeOfImage >= GameVersion::GameRva + sizeof(uintptr_t) &&
        pe->OptionalHeader.SizeOfImage > GameVersion::MenuRenderRva + GameVersion::MenuRenderPrologue.size() &&
        pe->OptionalHeader.SizeOfImage > GameVersion::MenuManagerRva + sizeof(uintptr_t);
    if (s_visualMotionSupported) {
        auto* menuRender = reinterpret_cast<void*>(s_moduleBase + GameVersion::MenuRenderRva);
        const auto& prologue = GameVersion::MenuRenderPrologue;
        if (std::memcmp(menuRender, prologue.data(), prologue.size()) == 0) {
            MH_CreateHook(menuRender, reinterpret_cast<void*>(&Hooked_MenuRender), reinterpret_cast<void**>(&oMenuRender));
        }
        const auto matchesDrawCode = [&](uintptr_t rva, const auto& expected) {
            return pe->OptionalHeader.SizeOfImage >= rva + expected.size() &&
                std::memcmp(reinterpret_cast<const void*>(s_moduleBase + rva), expected.data(), expected.size()) == 0;
        };
        const auto installDrawHook = [&](uintptr_t rva, const auto& expected, void* detour, void** original) {
            if (matchesDrawCode(rva, expected)) MH_CreateHook(reinterpret_cast<void*>(s_moduleBase + rva), detour, original);
        };
        installDrawHook(GameVersion::CharacterWheelRenderRva, GameVersion::CharacterWheelPrologue,
            reinterpret_cast<void*>(&Hooked_CharacterWheelRender), reinterpret_cast<void**>(&oCharacterWheelRender));
        installDrawHook(GameVersion::StatsRenderRva, GameVersion::StatsRenderPrologue,
            reinterpret_cast<void*>(&Hooked_StatsRender), reinterpret_cast<void**>(&oStatsRender));
        installDrawHook(GameVersion::OptionsRenderRva, GameVersion::OptionsRenderPrologue,
            reinterpret_cast<void*>(&Hooked_OptionsRender), reinterpret_cast<void**>(&oOptionsRender));
        installDrawHook(GameVersion::PlayerChargeBarsRenderRva, GameVersion::PlayerChargeBarsRenderPrologue,
            reinterpret_cast<void*>(&Hooked_PlayerChargeBarsRender),
            reinterpret_cast<void**>(&oPlayerChargeBarsRender));
        if (matchesDrawCode(GameVersion::PlayerIsHoldingItemRva, GameVersion::PlayerIsHoldingItemPrologue)) {
            s_playerIsHoldingItem = reinterpret_cast<PlayerIsHoldingItem_t>(s_moduleBase + GameVersion::PlayerIsHoldingItemRva);
            installDrawHook(GameVersion::PlayerBodyRenderRva, GameVersion::PlayerBodyRenderPrologue,
                reinterpret_cast<void*>(&Hooked_PlayerBodyRender), reinterpret_cast<void**>(&oPlayerBodyRender));
        }
#if defined(GAME_HAS_FAMILIAR_CHARGE_BARS)
        installDrawHook(GameVersion::FamiliarChargeBarRenderRva, GameVersion::FamiliarChargeBarRenderPrologue,
            reinterpret_cast<void*>(&Hooked_ChargeBarsRender<oFamiliarChargeBarRender>),
            reinterpret_cast<void**>(&oFamiliarChargeBarRender));
#endif
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
