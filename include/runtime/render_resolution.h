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
  kAsk,     // legacy alias of kNative (no startup dialog is shown anymore)
  kNative,  // host display physical resolution
  k720p,    // 1280x720
  k1080p,   // 1920x1080
};

bool ParseRenderResolution(std::string_view value, RenderResolution* out);
std::string_view RenderResolutionName(RenderResolution resolution);

// Additional render-resolution scale applied on top of the preset, e.g. 0.5
// renders at half the chosen resolution and the window upscales the frames.
inline constexpr double kMinRenderScale = 0.25;
inline constexpr double kMaxRenderScale = 1.0;

bool ParseRenderScale(std::string_view value, double* out);

// Pixel size a preset renders at. kAsk resolves to the native display size.
DisplaySize RenderResolutionSize(RenderResolution preset,
                                 const DisplaySize& native_display);

// Pixel size after applying the render scale (clamped to at least 320x180).
DisplaySize ApplyRenderScale(const DisplaySize& size, double scale);

}  // namespace runtime
}  // namespace mocktail

#endif  // MOCKTAIL_RUNTIME_RENDER_RESOLUTION_H_
