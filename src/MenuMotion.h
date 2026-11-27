#pragma once

#include "GameVersion.h"
#include "VisualMotion.h"
#include <vector>

template<class T> struct MenuArray {
    T* begin;
    T* end;
    T* capacity;

    bool Contains(size_t count) const {
        const auto first = reinterpret_cast<uintptr_t>(begin);
        const auto last = reinterpret_cast<uintptr_t>(end);
        const auto limit = reinterpret_cast<uintptr_t>(capacity);
        return first && last >= first && limit >= last &&
            (last - first) % sizeof(T) == 0 && count <= (last - first) / sizeof(T);
    }
};
static_assert(sizeof(MenuArray<Vector>) == 12, "Menu vector header size mismatch");

inline uintptr_t GetPauseAnimation(uintptr_t manager, uintptr_t game) {
#if defined(GAME_VERSION_1_7_9b)
    if (manager && game && *reinterpret_cast<const int*>(manager + GameVersion::ManagerStateOffset) == 2 &&
        *reinterpret_cast<const int*>(game + GameVersion::GamePauseMenuOffset) != 0) {
        return game + GameVersion::GamePauseMenuOffset + GameVersion::PauseSpriteOffset;
    }
#endif
    return 0;
}

class NativeMenuInterpolation {
public:
    explicit NativeMenuInterpolation(uintptr_t manager, uintptr_t game = 0) {
        if (manager && (*reinterpret_cast<const int*>(manager + GameVersion::ManagerStateOffset) == GameVersion::MainMenuManagerState ||
                        GetPauseAnimation(manager, game))) {
            option = reinterpret_cast<uint8_t*>(manager + GameVersion::NativeInterpolationOffset);
            original = *option;
            *option = 1;
        }
    }
    ~NativeMenuInterpolation() { if (option) *option = original; }
    NativeMenuInterpolation(const NativeMenuInterpolation&) = delete;
    NativeMenuInterpolation& operator=(const NativeMenuInterpolation&) = delete;

private:
    uint8_t* option = nullptr;
    uint8_t original = 0;
};

struct CharacterWheelMotion {
    struct Entry {
        VisualMotion position;
        VisualMotion depth;
        Vector originalPosition;
        Vector previewPosition;
        float originalDepth = 0;
        float previewDepth = 0;
    };
    uintptr_t panel = 0;
    int page = 0;
    Vector* positions = nullptr;
    float* depths = nullptr;
    std::vector<Entry> entries;
    uint32_t tick = 0;

    void Observe(uintptr_t nativePanel, uint32_t updateTick) {
        const int count = *reinterpret_cast<const int*>(nativePanel + GameVersion::CharacterWheelCountOffset);
        const auto& nativePositions = *reinterpret_cast<const MenuArray<Vector>*>(nativePanel + GameVersion::CharacterWheelPositionsOffset);
        const auto& nativeDepths = *reinterpret_cast<const MenuArray<float>*>(nativePanel + GameVersion::CharacterWheelDepthsOffset);
        if (count <= 0 || !nativePositions.Contains(count) || !nativeDepths.Contains(count)) {
            *this = {};
            return;
        }
        const int nativePage = *reinterpret_cast<const int*>(nativePanel + GameVersion::CharacterPageOffset);
        if (panel != nativePanel || page != nativePage || positions != nativePositions.begin ||
            depths != nativeDepths.begin || entries.size() != static_cast<size_t>(count)) {
            *this = {};
            panel = nativePanel;
            page = nativePage;
            positions = nativePositions.begin;
            depths = nativeDepths.begin;
            entries.resize(count);
        }
        tick = updateTick;
        for (size_t i = 0; i < entries.size(); ++i) {
            auto& entry = entries[i];
            entry.originalPosition = positions[i];
            entry.originalDepth = depths[i];
            entry.position.Sample(tick, entry.originalPosition, 160.0f);
            entry.depth.Sample(tick, {entry.originalDepth, 0}, 2.0f);
        }
    }
};

class CharacterWheelPreview {
public:
    CharacterWheelPreview(CharacterWheelMotion& history, float phase) : history(history), panel(history.panel) {
        if (!panel || history.entries.empty()) return;
        const auto& positions = *reinterpret_cast<const MenuArray<Vector>*>(panel + GameVersion::CharacterWheelPositionsOffset);
        const auto& depths = *reinterpret_cast<const MenuArray<float>*>(panel + GameVersion::CharacterWheelDepthsOffset);
        if (positions.begin != history.positions || depths.begin != history.depths ||
            !positions.Contains(history.entries.size()) || !depths.Contains(history.entries.size()) ||
            *reinterpret_cast<const int*>(panel + GameVersion::CharacterPageOffset) != history.page ||
            *reinterpret_cast<const int*>(panel + GameVersion::CharacterWheelCountOffset) != static_cast<int>(history.entries.size())) return;
        for (size_t i = 0; i < history.entries.size(); ++i) {
            const auto& entry = history.entries[i];
            if (!SamePosition(positions.begin[i], entry.originalPosition) || depths.begin[i] != entry.originalDepth) return;
        }
        active = true;
        for (size_t i = 0; i < history.entries.size(); ++i) {
            auto& entry = history.entries[i];
            entry.previewPosition = entry.position.Sample(history.tick, entry.originalPosition, 160.0f, phase);
            entry.previewDepth = entry.depth.Sample(history.tick, {entry.originalDepth, 0}, 2.0f, phase).x;
            positions.begin[i] = entry.previewPosition;
            depths.begin[i] = entry.previewDepth;
        }
    }

    ~CharacterWheelPreview() {
        if (!active) return;
        auto& positions = *reinterpret_cast<const MenuArray<Vector>*>(panel + GameVersion::CharacterWheelPositionsOffset);
        auto& depths = *reinterpret_cast<const MenuArray<float>*>(panel + GameVersion::CharacterWheelDepthsOffset);
        if (positions.begin != history.positions || depths.begin != history.depths ||
            !positions.Contains(history.entries.size()) || !depths.Contains(history.entries.size())) return;
        for (size_t i = 0; i < history.entries.size(); ++i) {
            const auto& entry = history.entries[i];
            if (SamePosition(positions.begin[i], entry.previewPosition)) positions.begin[i] = entry.originalPosition;
            if (depths.begin[i] == entry.previewDepth) depths.begin[i] = entry.originalDepth;
        }
    }

    CharacterWheelPreview(const CharacterWheelPreview&) = delete;
    CharacterWheelPreview& operator=(const CharacterWheelPreview&) = delete;

private:
    CharacterWheelMotion& history;
    uintptr_t panel;
    bool active = false;
};

struct MenuDrawClock {
    uintptr_t panel = 0;
    uint32_t frame = 0;
    uint32_t tick = 0;

    bool Observe(uintptr_t nativePanel, uint32_t nativeFrame) {
        if (panel != nativePanel || nativeFrame - frame > 2) {
            *this = {};
            panel = nativePanel;
        }
        if (tick && frame == nativeFrame) return false;
        frame = nativeFrame;
        ++tick;
        return true;
    }
};

struct SecretsMotion {
    MenuDrawClock clock;
    VisualMotion motion;
    Vector original;
    int selection = -1;

    void Observe(uintptr_t panel, uint32_t frame) {
        if (!clock.Observe(panel, frame)) return;
        const int currentSelection = *reinterpret_cast<const int*>(panel + GameVersion::SecretsSelectionOffset);
        if (clock.tick == 1 || selection != currentSelection) motion = {};
        selection = currentSelection;
        original = *reinterpret_cast<const Vector*>(panel + GameVersion::SecretsScrollInOffset);
        motion.Sample(clock.tick, original, 160.0f);
    }
};

class SecretsPreview {
public:
    SecretsPreview(SecretsMotion& history, float phase) : history(history) {
        auto& native = *reinterpret_cast<Vector*>(history.clock.panel + GameVersion::SecretsScrollInOffset);
        original = native;
        preview = original;
        if (SamePosition(native, history.original)) {
            preview = history.motion.Sample(history.clock.tick, history.original, 160.0f, phase);
            native = preview;
        }
    }
    ~SecretsPreview() {
        auto& native = *reinterpret_cast<Vector*>(history.clock.panel + GameVersion::SecretsScrollInOffset);
        if (SamePosition(native, preview)) native = original;
    }
    SecretsPreview(const SecretsPreview&) = delete;
    SecretsPreview& operator=(const SecretsPreview&) = delete;

private:
    SecretsMotion& history;
    Vector original;
    Vector preview;
};

struct MenuPositionMotion {
    VisualMotion motion;
    Vector original;
    Vector preview;
};

struct MenuMotion {
    uintptr_t menu = 0;
    uint32_t engineFrame = 0;
    uint32_t tick = 0;
    std::array<MenuPositionMotion, GameVersion::MenuPositionOffsets.size()> positions;

    void Observe(uintptr_t nativeMenu, uint32_t frame) {
        if (menu != nativeMenu) { *this = {}; menu = nativeMenu; }
        engineFrame = frame;
        ++tick;
        for (size_t i = 0; i < positions.size(); ++i) {
            auto& entry = positions[i];
            entry.original = *reinterpret_cast<const Vector*>(menu + GameVersion::MenuPositionOffsets[i]);
            entry.motion.Sample(tick, entry.original, 160.0f);
        }
    }
};

class MenuPositionPreview {
public:
    MenuPositionPreview(MenuMotion& history, float phase) : history(history) {
        for (size_t i = 0; i < history.positions.size(); ++i) {
            auto& entry = history.positions[i];
            auto& native = *reinterpret_cast<Vector*>(history.menu + GameVersion::MenuPositionOffsets[i]);
            entry.preview = entry.original;
            if (!SamePosition(native, entry.original)) continue;
            entry.preview = entry.motion.Sample(history.tick, entry.original, 160.0f, phase);
            native = entry.preview;
        }
    }

    ~MenuPositionPreview() {
        for (size_t i = 0; i < history.positions.size(); ++i) {
            const auto& entry = history.positions[i];
            auto& native = *reinterpret_cast<Vector*>(history.menu + GameVersion::MenuPositionOffsets[i]);
            if (SamePosition(native, entry.preview)) native = entry.original;
        }
    }

    MenuPositionPreview(const MenuPositionPreview&) = delete;
    MenuPositionPreview& operator=(const MenuPositionPreview&) = delete;

private:
    MenuMotion& history;
};
