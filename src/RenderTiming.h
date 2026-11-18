#pragma once

#include <cstddef>
#include <cstdint>

void InitializeRenderTiming(uintptr_t moduleBase, std::size_t imageSize);
float GetRenderPhase();
