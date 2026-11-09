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

// True when the viewport's covered pixel range extends beyond `framebuffer`
// (handles flipped viewports with negative height/width). Viewports that exceed the clamped
// framebuffer come from full-window coordinates and must be scaled onto
// it; viewports already inside it were sized from the framebuffer itself
// and must be left alone (scaling them again would shrink the scene into
// a corner and leave black areas).
bool ViewportExceedsFramebuffer(const VkViewport& viewport,
                                VkExtent2D framebuffer);
// Same test for a scissor / render-area rect.
bool RectExceedsFramebuffer(const VkRect2D& rect, VkExtent2D framebuffer);

// Scales only the viewports that exceed `framebuffer`, by (scale_x,
// scale_y) per axis; the rest are left unchanged.
void ScaleViewportsToFramebuffer(VkViewport* viewports, std::uint32_t count,
                                 double scale_x, double scale_y,
                                 VkExtent2D framebuffer);
// Scales only the rect that exceeds `framebuffer` (render area).
void ScaleRect2DToFramebuffer(VkRect2D* rect, double scale_x, double scale_y,
                              VkExtent2D framebuffer);
// Scales only the scissors that exceed `framebuffer`.
void ScaleScissorsToFramebuffer(VkRect2D* scissors, std::uint32_t count,
                                double scale_x, double scale_y,
                                VkExtent2D framebuffer);

// True when the rect lies entirely inside `framebuffer` (per axis, int64).
bool RectFitsWithin(const VkRect2D& rect, VkExtent2D framebuffer);
// True when the viewport's covered pixel range lies inside `framebuffer`
// (handles flipped viewports with negative height).
bool ViewportFitsWithin(const VkViewport& viewport, VkExtent2D framebuffer);

// Unconditional per-axis scaling of a rect / of a viewport array.
void ScaleRect2DBy(VkRect2D* rect, double scale_x, double scale_y);
void ScaleViewportsBy(VkViewport* viewports, std::uint32_t count,
                      double scale_x, double scale_y);

// Mapping for passes that render into the swapchain itself: a rect recorded in
// the advertised (scaled) framebuffer's coordinates is stretched onto the real
// swapchain extent; a rect exceeding the swapchain is scaled down by
// swapchain/window; anything in between is left unchanged.
void MapRectToSwapchain(VkRect2D* rect, VkExtent2D advertised,
                        VkExtent2D swapchain, VkExtent2D window);
void MapViewportToSwapchain(VkViewport* viewport, VkExtent2D advertised,
                            VkExtent2D swapchain, VkExtent2D window);

// Same mapping for vkCmdBlitImage regions (two corner offsets) and for
// vkCmdCopyImage/vkCmdResolveImage regions (offset + extent): the rect is
// mapped onto the swapchain and written back through the out-params.
void MapOffsets3DToSwapchain(VkOffset3D* corner_a, VkOffset3D* corner_b,
                             VkExtent2D advertised, VkExtent2D swapchain,
                             VkExtent2D window);
void MapOffsetExtentToSwapchain(VkOffset3D* offset, VkExtent3D* extent,
                                VkExtent2D advertised, VkExtent2D swapchain,
                                VkExtent2D window);

} // namespace graphics
} // namespace mocktail

#endif // MOCKTAIL_GRAPHICS_RENDER_SCALE_POLICY_H_
