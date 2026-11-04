#ifndef MOCKTAIL_RUNTIME_RENDER_RESOLUTION_H_
#define MOCKTAIL_RUNTIME_RENDER_RESOLUTION_H_

#include <string_view>

#include "runtime/display_size.h"

namespace mocktail {
namespace runtime {

// Render resolution reported to the Roblox client (displayResolution /
// displayPhysical*Pixels). The SDL window keeps its configured size; the
// compositor scales the rendered frames to the window.
enum class RenderResolution {
  kAsk,     // interactive picker at startup (windowed mode only)
  kNative,  // host display physical resolution
  k720p,    // 1280x720
  k1080p,   // 1920x1080
};

bool ParseRenderResolution(std::string_view value, RenderResolution* out);
std::string_view RenderResolutionName(RenderResolution resolution);

// Pixel size a preset renders at. kAsk resolves to the native display size;
// the interactive choice is made in the window layer.
DisplaySize RenderResolutionSize(RenderResolution preset,
                                 const DisplaySize& native_display);

}  // namespace runtime
}  // namespace mocktail

#endif  // MOCKTAIL_RUNTIME_RENDER_RESOLUTION_H_
