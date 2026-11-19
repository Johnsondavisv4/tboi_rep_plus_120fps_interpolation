#pragma once
#include <cstdint>
#include <cmath>

#pragma pack(push, 1)

struct Vector {
    float x;
    float y;

    Vector() : x(0.0f), y(0.0f) {}
    Vector(float _x, float _y) : x(_x), y(_y) {}
};

struct ColorMod {
    float _tint[4];
    float _colorize[4];
    float _offset[3];
};

struct AnimationFrame {
    Vector crop;
    float width;
    float height;
    Vector pos;
    Vector scale;
    Vector pivot;
    int duration;
    bool visible;
    uint8_t _pad0[3];
    ColorMod color;
    float rotation;
    bool interpolated;
    uint8_t _pad1[3];
    int startFrame;
    int endFrame;
};
static_assert(sizeof(AnimationFrame) == 0x6C, "AnimationFrame size mismatch");

struct AnimationLayer {
    int _layerID;
    AnimationFrame* _animFrames;
    int _numFrames;
    bool _visible;
    uint8_t _pad0[3];
};
static_assert(sizeof(AnimationLayer) == 0x10, "AnimationLayer size mismatch");

struct AnimationState {
    void* _animation;
    void* _animData;
    int* _layerFrames;
    int* _nullLayerFrames;
    float _animFrame;
    bool _isPlaying;
    uint8_t _pad0[3];
    int _currentlyTriggeredEvents;
    int _previouslyTriggeredEvents;
};
static_assert(sizeof(AnimationState) == 0x20, "AnimationState size mismatch");

struct AnimationData {
    uint8_t _pad0[0x18];
    AnimationLayer* _animLayers;
    int _layerCount;
};

struct ANM2 {
    uint8_t _pad0[0x30];
    AnimationState _animState;
    AnimationState _overlayAnimState;
    uint8_t _pad1[0x104 - 0x70];
    float _playbackSpeed;
};
static_assert(sizeof(ANM2) == 0x108, "ANM2 prefix size mismatch");

#pragma pack(pop)
