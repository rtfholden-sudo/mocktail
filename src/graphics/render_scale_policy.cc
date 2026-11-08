#include "mocktail/graphics/render_scale_policy.h"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace mocktail {
namespace graphics {

namespace {

// Same bounds as the window.render_scale YAML parser (kMinRenderScale ..
// 1.0 in runtime/render_resolution.h).
constexpr double kMinScale = 0.25;
constexpr double kMaxScale = 1.0;

std::uint32_t ScaleAxis(std::uint32_t requested, std::uint32_t min_extent,
                        std::uint32_t max_extent, double scale) {
  std::uint64_t lo = min_extent == 0 ? 1 : min_extent;
  std::uint64_t hi = max_extent;  // UINT32_MAX means "no limit".
  if (hi > requested) {
    hi = requested;  // Host scaling only shrinks, never grows.
  }
  if (hi < lo) {
    hi = lo;
  }
  std::uint64_t target = static_cast<std::uint64_t>(
      std::llround(static_cast<double>(requested) * scale));
  if (target < lo) {
    target = lo;
  }
  if (target > hi) {
    target = hi;
  }
  return static_cast<std::uint32_t>(target);
}

} // namespace

double ParseRenderScaleValue(std::string_view raw) {
  if (raw.empty()) {
    return kMaxScale;
  }
  double value = 0.0;
  const char* begin = raw.data();
  const char* end = begin + raw.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return kMaxScale;
  }
  if (!(value >= kMinScale && value <= kMaxScale)) {
    return kMaxScale;
  }
  return value;
}

double CachedRenderScale() {
  static const double scale = [] {
    const char* raw = std::getenv("MOCKTAIL_RENDER_SCALE");
    if (raw == nullptr) {
      return kMaxScale;
    }
    return ParseRenderScaleValue(std::string_view(raw, std::strlen(raw)));
  }();
  return scale;
}

bool HostFramebufferScaleActive() {
  const double scale = CachedRenderScale();
  return scale > 0.0 && scale < 1.0;
}

VkExtent2D ScaleSwapchainExtent(VkExtent2D requested, double scale,
                                VkExtent2D min_extent, VkExtent2D max_extent) {
  if (!(scale > 0.0) || scale >= 1.0) {
    return requested;
  }
  return {ScaleAxis(requested.width, min_extent.width, max_extent.width, scale),
          ScaleAxis(requested.height, min_extent.height, max_extent.height,
                    scale)};
}

void ScaleRect2D(VkRect2D* rect, double scale) {
  if (rect == nullptr || !(scale > 0.0) || scale >= 1.0) {
    return;
  }
  rect->offset.x = static_cast<std::int32_t>(rect->offset.x * scale);
  rect->offset.y = static_cast<std::int32_t>(rect->offset.y * scale);
  rect->extent.width =
      static_cast<std::uint32_t>(rect->extent.width * scale);
  rect->extent.height =
      static_cast<std::uint32_t>(rect->extent.height * scale);
}

void ScaleViewports(VkViewport* viewports, std::uint32_t count,
                    double scale) {
  if (viewports == nullptr || !(scale > 0.0) || scale >= 1.0) {
    return;
  }
  for (std::uint32_t i = 0; i < count; ++i) {
    viewports[i].x *= static_cast<float>(scale);
    viewports[i].y *= static_cast<float>(scale);
    viewports[i].width *= static_cast<float>(scale);
    viewports[i].height *= static_cast<float>(scale);
  }
}

void ScaleScissors(VkRect2D* scissors, std::uint32_t count, double scale) {
  if (scissors == nullptr) {
    return;
  }
  for (std::uint32_t i = 0; i < count; ++i) {
    ScaleRect2D(&scissors[i], scale);
  }
}

bool ViewportExceedsFramebuffer(const VkViewport& viewport,
                                VkExtent2D framebuffer) {
  if (framebuffer.width == 0 || framebuffer.height == 0) {
    return false;
  }
  const double right =
      static_cast<double>(viewport.x) + std::fabs(viewport.width);
  const double bottom =
      static_cast<double>(viewport.y) + std::fabs(viewport.height);
  return viewport.x < 0.0f || viewport.y < 0.0f ||
         right > static_cast<double>(framebuffer.width) ||
         bottom > static_cast<double>(framebuffer.height);
}

bool RectExceedsFramebuffer(const VkRect2D& rect, VkExtent2D framebuffer) {
  if (framebuffer.width == 0 || framebuffer.height == 0) {
    return false;
  }
  return rect.offset.x < 0 || rect.offset.y < 0 ||
         static_cast<std::int64_t>(rect.offset.x) + rect.extent.width >
             framebuffer.width ||
         static_cast<std::int64_t>(rect.offset.y) + rect.extent.height >
             framebuffer.height;
}

void ScaleViewportsToFramebuffer(VkViewport* viewports, std::uint32_t count,
                                 double scale_x, double scale_y,
                                 VkExtent2D framebuffer) {
  if (viewports == nullptr) {
    return;
  }
  for (std::uint32_t i = 0; i < count; ++i) {
    if (!ViewportExceedsFramebuffer(viewports[i], framebuffer)) {
      continue;
    }
    viewports[i].x *= static_cast<float>(scale_x);
    viewports[i].y *= static_cast<float>(scale_y);
    viewports[i].width *= static_cast<float>(scale_x);
    viewports[i].height *= static_cast<float>(scale_y);
  }
}

void ScaleRect2DToFramebuffer(VkRect2D* rect, double scale_x, double scale_y,
                              VkExtent2D framebuffer) {
  if (rect == nullptr || !RectExceedsFramebuffer(*rect, framebuffer)) {
    return;
  }
  rect->offset.x = static_cast<std::int32_t>(rect->offset.x * scale_x);
  rect->offset.y = static_cast<std::int32_t>(rect->offset.y * scale_y);
  rect->extent.width =
      static_cast<std::uint32_t>(rect->extent.width * scale_x);
  rect->extent.height =
      static_cast<std::uint32_t>(rect->extent.height * scale_y);
}

void ScaleScissorsToFramebuffer(VkRect2D* scissors, std::uint32_t count,
                                double scale_x, double scale_y,
                                VkExtent2D framebuffer) {
  if (scissors == nullptr) {
    return;
  }
  for (std::uint32_t i = 0; i < count; ++i) {
    ScaleRect2DToFramebuffer(&scissors[i], scale_x, scale_y, framebuffer);
  }
}

} // namespace graphics
} // namespace mocktail
