#ifndef MOCKTAIL_GRAPHICS_RENDER_SCALE_POLICY_H_
#define MOCKTAIL_GRAPHICS_RENDER_SCALE_POLICY_H_

#include <cstdint>
#include <string_view>

#include <vulkan/vulkan.h>

namespace mocktail {
namespace graphics {

// Parses a render-scale value with the same bounds as the window.render_scale
// YAML parser (0.25..1.0). Returns 1.0 for empty, malformed, or out-of-range
// input.
double ParseRenderScaleValue(std::string_view raw);

// Host-side framebuffer scale from window.render_scale (exported through the
// environment as MOCKTAIL_RENDER_SCALE), cached at first use.
double CachedRenderScale();

// True when CachedRenderScale() is in (0, 1): the host clamps the Vulkan
// swapchain extent, so the engine DRS flag must not apply on top of it.
bool HostFramebufferScaleActive();

// Computes the swapchain image extent for `requested` under `scale`, clamped
// to the surface capabilities [min_extent, max_extent] and never larger than
// `requested` (host scaling only shrinks). A scale <= 0 or >= 1 returns
// `requested` unchanged.
VkExtent2D ScaleSwapchainExtent(VkExtent2D requested, double scale,
                                VkExtent2D min_extent, VkExtent2D max_extent);

// Scales a 2D rect (scissor offset/extent or a rendering area) in place by
// `scale`. MangoHud-style: the game keeps working in full-window
// coordinates, the layer shrinks the GPU-side rect to the clamped
// framebuffer, and presentation upscales the result to the window.
// A scale <= 0 or >= 1 leaves the rect unchanged.
void ScaleRect2D(VkRect2D* rect, double scale);

// Scales `count` viewports in place (x, y, width, height; depth untouched).
// No-op for scale <= 0 or >= 1 or a null array.
void ScaleViewports(VkViewport* viewports, std::uint32_t count, double scale);

// Scales `count` scissors in place. No-op for scale <= 0 or >= 1 or null.
void ScaleScissors(VkRect2D* scissors, std::uint32_t count, double scale);

} // namespace graphics
} // namespace mocktail

#endif // MOCKTAIL_GRAPHICS_RENDER_SCALE_POLICY_H_
