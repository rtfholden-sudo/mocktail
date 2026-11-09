// Copyright 2026 Mocktail Project Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mocktail/graphics/render_scale_policy.h"

#include <gtest/gtest.h>

#include <cstdlib>

namespace mocktail {
namespace graphics {
namespace {

TEST(RenderScalePolicyTest, ScalesRequestedExtentDown) {
  const VkExtent2D scaled = ScaleSwapchainExtent(
      {1280, 720}, 0.5, {1, 1}, {UINT32_MAX, UINT32_MAX});
  EXPECT_EQ(scaled.width, 640u);
  EXPECT_EQ(scaled.height, 360u);
}

TEST(RenderScalePolicyTest, ClampsToSurfaceCapabilities) {
  // minImageExtent larger than the scaled target wins.
  const VkExtent2D clamped_min =
      ScaleSwapchainExtent({1280, 720}, 0.25, {400, 300}, {4096, 4096});
  EXPECT_EQ(clamped_min.width, 400u);
  EXPECT_EQ(clamped_min.height, 300u);
  // maxImageExtent smaller than the requested extent caps the target.
  const VkExtent2D clamped_max =
      ScaleSwapchainExtent({1280, 720}, 0.5, {1, 1}, {1000, 1000});
  EXPECT_EQ(clamped_max.width, 640u);
  EXPECT_EQ(clamped_max.height, 360u);
  const VkExtent2D capped = ScaleSwapchainExtent({1280, 720}, 0.9, {1, 1},
                                                 {1000, 500});
  EXPECT_EQ(capped.width, 1000u);
  EXPECT_EQ(capped.height, 500u);
}

TEST(RenderScalePolicyTest, NeverGrowsBeyondRequested) {
  const VkExtent2D grown =
      ScaleSwapchainExtent({320, 180}, 0.9, {1, 1}, {UINT32_MAX, UINT32_MAX});
  EXPECT_EQ(grown.width, 288u);
  EXPECT_EQ(grown.height, 162u);
}

TEST(RenderScalePolicyTest, IdentityForFullOrInvalidScale) {
  const VkExtent2D requested{1280, 720};
  EXPECT_EQ(ScaleSwapchainExtent(requested, 1.0, {1, 1}, {UINT32_MAX, UINT32_MAX}).width, 1280u);
  EXPECT_EQ(ScaleSwapchainExtent(requested, 0.0, {1, 1}, {UINT32_MAX, UINT32_MAX}).width, 1280u);
  EXPECT_EQ(ScaleSwapchainExtent(requested, -1.0, {1, 1}, {UINT32_MAX, UINT32_MAX}).height, 720u);
  EXPECT_EQ(ScaleSwapchainExtent(requested, 2.0, {1, 1}, {UINT32_MAX, UINT32_MAX}).height, 720u);
}

TEST(RenderScalePolicyTest, RoundsToNearestPixel) {
  const VkExtent2D scaled = ScaleSwapchainExtent(
      {1281, 721}, 0.5, {1, 1}, {UINT32_MAX, UINT32_MAX});
  EXPECT_EQ(scaled.width, 641u);  // lround(640.5)
  EXPECT_EQ(scaled.height, 361u); // lround(360.5)
}

TEST(RenderScalePolicyTest, ParsesEnvironmentValues) {
  EXPECT_DOUBLE_EQ(ParseRenderScaleValue(""), 1.0);
  EXPECT_DOUBLE_EQ(ParseRenderScaleValue("0.5"), 0.5);
  EXPECT_DOUBLE_EQ(ParseRenderScaleValue("0.25"), 0.25);
  EXPECT_DOUBLE_EQ(ParseRenderScaleValue("1.0"), 1.0);
  EXPECT_DOUBLE_EQ(ParseRenderScaleValue("1"), 1.0);
  // Out of range or malformed: fall back to 1.0 (no host scaling).
  EXPECT_DOUBLE_EQ(ParseRenderScaleValue("0.1"), 1.0);
  EXPECT_DOUBLE_EQ(ParseRenderScaleValue("2.0"), 1.0);
  EXPECT_DOUBLE_EQ(ParseRenderScaleValue("abc"), 1.0);
  EXPECT_DOUBLE_EQ(ParseRenderScaleValue("0.5x"), 1.0);
  EXPECT_DOUBLE_EQ(ParseRenderScaleValue(" 0.5"), 1.0);
}

TEST(RenderScalePolicyTest, CachedScaleDefaultsToOne) {
  // The test environment does not set MOCKTAIL_RENDER_SCALE.
  unsetenv("MOCKTAIL_RENDER_SCALE");
  EXPECT_DOUBLE_EQ(CachedRenderScale(), 1.0);
  EXPECT_FALSE(HostFramebufferScaleActive());
}

TEST(RenderScalePolicyTest, ScalesViewportsAndScissorsInPlace) {
  VkViewport viewports[2] = {{10.0f, 20.0f, 1280.0f, 720.0f, 0.0f, 1.0f},
                             {0.0f, 0.0f, 640.0f, 360.0f, 0.25f, 0.75f}};
  ScaleViewports(viewports, 2, 0.5);
  EXPECT_FLOAT_EQ(viewports[0].x, 5.0f);
  EXPECT_FLOAT_EQ(viewports[0].y, 10.0f);
  EXPECT_FLOAT_EQ(viewports[0].width, 640.0f);
  EXPECT_FLOAT_EQ(viewports[0].height, 360.0f);
  EXPECT_FLOAT_EQ(viewports[0].minDepth, 0.0f);
  EXPECT_FLOAT_EQ(viewports[0].maxDepth, 1.0f);
  EXPECT_FLOAT_EQ(viewports[1].width, 320.0f);

  VkRect2D scissors[1] = {{{8, 16}, {1280u, 720u}}};
  ScaleScissors(scissors, 1, 0.25);
  EXPECT_EQ(scissors[0].offset.x, 2);
  EXPECT_EQ(scissors[0].offset.y, 4);
  EXPECT_EQ(scissors[0].extent.width, 320u);
  EXPECT_EQ(scissors[0].extent.height, 180u);
}

TEST(RenderScalePolicyTest, ViewportScalingIsNoOpAtFullScale) {
  VkViewport viewport = {10.0f, 20.0f, 1280.0f, 720.0f, 0.0f, 1.0f};
  ScaleViewports(&viewport, 1, 1.0);
  EXPECT_FLOAT_EQ(viewport.width, 1280.0f);
  ScaleViewports(&viewport, 1, 0.0);
  EXPECT_FLOAT_EQ(viewport.width, 1280.0f);
  ScaleViewports(nullptr, 1, 0.5);
  VkRect2D rect = {{4, 8}, {100u, 50u}};
  ScaleRect2D(&rect, 1.0);
  EXPECT_EQ(rect.extent.width, 100u);
  ScaleRect2D(nullptr, 0.5);
}

TEST(RenderScalePolicyTest, ScalesOnlyViewportsExceedingTheFramebuffer) {
  const VkExtent2D fb{320, 180};
  VkViewport viewports[3] = {
      {0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f},   // full window -> scale
      {0.0f, 0.0f, 320.0f, 180.0f, 0.0f, 1.0f},    // already framebuffer-sized
      {0.0f, 720.0f, 1280.0f, -720.0f, 0.0f, 1.0f}};  // flipped full window
  ScaleViewportsToFramebuffer(viewports, 3, 0.25, 0.25, fb);
  EXPECT_FLOAT_EQ(viewports[0].width, 320.0f);
  EXPECT_FLOAT_EQ(viewports[0].height, 180.0f);
  EXPECT_FLOAT_EQ(viewports[1].width, 320.0f);   // untouched
  EXPECT_FLOAT_EQ(viewports[1].height, 180.0f);
  EXPECT_FLOAT_EQ(viewports[2].y, 180.0f);
  EXPECT_FLOAT_EQ(viewports[2].width, 320.0f);
  EXPECT_FLOAT_EQ(viewports[2].height, -180.0f);
  EXPECT_TRUE(ViewportExceedsFramebuffer({0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f}, fb));
  EXPECT_FALSE(ViewportExceedsFramebuffer({0.0f, 0.0f, 320.0f, 180.0f, 0.0f, 1.0f}, fb));
}

TEST(RenderScalePolicyTest, ScalesOnlyScissorsExceedingTheFramebuffer) {
  const VkExtent2D fb{320, 180};
  VkRect2D scissors[2] = {{{0, 0}, {1280u, 720u}}, {{8, 16}, {300u, 160u}}};
  ScaleScissorsToFramebuffer(scissors, 2, 0.25, 0.25, fb);
  EXPECT_EQ(scissors[0].extent.width, 320u);
  EXPECT_EQ(scissors[0].extent.height, 180u);
  EXPECT_EQ(scissors[1].offset.x, 8);    // inside the framebuffer: untouched
  EXPECT_EQ(scissors[1].extent.width, 300u);
  VkRect2D area{{0, 0}, {1280u, 720u}};
  ScaleRect2DToFramebuffer(&area, 0.25, 0.25, fb);
  EXPECT_EQ(area.extent.width, 320u);
  VkRect2D inside{{0, 0}, {320u, 180u}};
  ScaleRect2DToFramebuffer(&inside, 0.25, 0.25, fb);
  EXPECT_EQ(inside.extent.width, 320u);
  EXPECT_TRUE(RectExceedsFramebuffer({{0, 0}, {321u, 180u}}, fb));
  EXPECT_FALSE(RectExceedsFramebuffer({{0, 0}, {320u, 180u}}, fb));
}

TEST(RenderScalePolicyTest, FitsWithinTests) {
  const VkExtent2D fb{1242, 631};
  EXPECT_TRUE(RectFitsWithin({{0, 0}, {1242u, 631u}}, fb));
  EXPECT_TRUE(RectFitsWithin({{8, 16}, {300u, 160u}}, fb));
  EXPECT_FALSE(RectFitsWithin({{0, 0}, {1243u, 631u}}, fb));
  EXPECT_FALSE(RectFitsWithin({{-1, 0}, {100u, 100u}}, fb));
  EXPECT_TRUE(ViewportFitsWithin({0.0f, 631.0f, 1242.0f, -631.0f, 0.0f, 1.0f}, fb));
  EXPECT_TRUE(ViewportFitsWithin({0.0f, 0.0f, 1242.0f, 631.0f, 0.0f, 1.0f}, fb));
  EXPECT_FALSE(ViewportFitsWithin({0.0f, 0.0f, 1243.0f, 631.0f, 0.0f, 1.0f}, fb));
}

TEST(RenderScalePolicyTest, ScalesByPerAxisUnconditionally) {
  VkRect2D rect{{8, 16}, {300u, 160u}};
  ScaleRect2DBy(&rect, 2.0, 0.5);
  EXPECT_EQ(rect.offset.x, 16);
  EXPECT_EQ(rect.offset.y, 8);
  EXPECT_EQ(rect.extent.width, 600u);
  EXPECT_EQ(rect.extent.height, 80u);
  VkViewport vps[2] = {{1.0f, 2.0f, 10.0f, 20.0f, 0.0f, 1.0f},
                      {0.0f, 5.0f, -10.0f, 20.0f, 0.0f, 1.0f}};
  ScaleViewportsBy(vps, 2, 0.25, 4.0);
  EXPECT_FLOAT_EQ(vps[0].x, 0.25f);
  EXPECT_FLOAT_EQ(vps[0].y, 8.0f);
  EXPECT_FLOAT_EQ(vps[0].width, 2.5f);
  EXPECT_FLOAT_EQ(vps[0].height, 80.0f);
  EXPECT_FLOAT_EQ(vps[1].width, -2.5f);
}

TEST(RenderScalePolicyTest, MapsPresentPassRectsOntoTheSwapchain) {
  // User's geometry: window 1280x650, render_scale 0.97 -> advertised
  // framebuffer 1242x631, swapchain kept at the window 1280x650.
  const VkExtent2D advertised{1242, 631};
  const VkExtent2D swapchain{1280, 650};
  const VkExtent2D window{1280, 650};
  VkRect2D fb_sized{{0, 0}, {1242u, 631u}};
  MapRectToSwapchain(&fb_sized, advertised, swapchain, window);
  EXPECT_EQ(fb_sized.offset.x, 0);
  EXPECT_EQ(fb_sized.extent.width, 1280u);
  EXPECT_EQ(fb_sized.extent.height, 650u);
  VkRect2D window_sized{{0, 0}, {1280u, 650u}};
  MapRectToSwapchain(&window_sized, advertised, swapchain, window);
  EXPECT_EQ(window_sized.extent.width, 1280u);  // already fills: untouched
  VkRect2D huge{{0, 0}, {2560u, 1300u}};
  MapRectToSwapchain(&huge, advertised, swapchain, window);
  // exceeds the swapchain, but swapchain == window so the factor is 1
  EXPECT_EQ(huge.extent.width, 2560u);
  EXPECT_EQ(huge.extent.height, 1300u);
  VkViewport vp{0.0f, 0.0f, 1242.0f, 631.0f, 0.0f, 1.0f};
  MapViewportToSwapchain(&vp, advertised, swapchain, window);
  EXPECT_FLOAT_EQ(vp.width, 1280.0f);
  EXPECT_FLOAT_EQ(vp.height, 650.0f);
  VkViewport flipped{0.0f, 631.0f, 1242.0f, -631.0f, 0.0f, 1.0f};
  MapViewportToSwapchain(&flipped, advertised, swapchain, window);
  EXPECT_FLOAT_EQ(flipped.y, 650.0f);
  EXPECT_FLOAT_EQ(flipped.height, -650.0f);
  VkViewport window_vp{0.0f, 0.0f, 1280.0f, 650.0f, 0.0f, 1.0f};
  MapViewportToSwapchain(&window_vp, advertised, swapchain, window);
  EXPECT_FLOAT_EQ(window_vp.width, 1280.0f);  // untouched
}

TEST(RenderScalePolicyTest, PresentPassFillsWindowAtLowScale) {
  // Follow-up run geometry: window 1366x768, render_scale 0.7 -> advertised
  // 956x537, swapchain kept at the window 1366x768.
  const VkExtent2D window{1366, 768};
  const VkExtent2D advertised =
      ScaleSwapchainExtent(window, 0.7, {1, 1}, {UINT32_MAX, UINT32_MAX});
  EXPECT_EQ(advertised.width, 956u);
  EXPECT_EQ(advertised.height, 538u);
  const VkExtent2D swapchain = window;
  VkViewport vp{0.0f, 0.0f, 956.0f, 538.0f, 0.0f, 1.0f};
  MapViewportToSwapchain(&vp, advertised, swapchain, window);
  EXPECT_FLOAT_EQ(vp.width, 1366.0f);
  EXPECT_FLOAT_EQ(vp.height, 768.0f);
  VkViewport window_vp{0.0f, 0.0f, 1366.0f, 768.0f, 0.0f, 1.0f};
  MapViewportToSwapchain(&window_vp, advertised, swapchain, window);
  EXPECT_FLOAT_EQ(window_vp.width, 1366.0f);  // already full: untouched
  EXPECT_FLOAT_EQ(window_vp.height, 768.0f);
  VkRect2D ra{{0, 0}, {956u, 538u}};
  MapRectToSwapchain(&ra, advertised, swapchain, window);
  EXPECT_EQ(ra.extent.width, 1366u);
  EXPECT_EQ(ra.extent.height, 768u);
  VkViewport off{0.0f, 0.0f, 1366.0f, 768.0f, 0.0f, 1.0f};
  ScaleViewportsToFramebuffer(&off, 1, 956.0 / 1366.0, 538.0 / 768.0,
                              advertised);
  EXPECT_FLOAT_EQ(off.width, 956.0f);
  EXPECT_FLOAT_EQ(off.height, 538.0f);
  VkViewport off_fb{0.0f, 0.0f, 956.0f, 538.0f, 0.0f, 1.0f};
  ScaleViewportsToFramebuffer(&off_fb, 1, 956.0 / 1366.0, 538.0 / 768.0,
                              advertised);
  EXPECT_FLOAT_EQ(off_fb.width, 956.0f);  // fits: untouched
  EXPECT_FLOAT_EQ(off_fb.height, 538.0f);
}

TEST(RenderScalePolicyTest, MapsBlitAndCopyRegionsOntoTheSwapchain) {
  // Latest run geometry: window 1280x650, render_scale 0.5 -> advertised
  // 640x325, swapchain kept at the window 1280x650.
  const VkExtent2D window{1280, 650};
  const VkExtent2D advertised =
      ScaleSwapchainExtent(window, 0.5, {1, 1}, {UINT32_MAX, UINT32_MAX});
  EXPECT_EQ(advertised.width, 640u);
  EXPECT_EQ(advertised.height, 325u);
  const VkExtent2D swapchain = window;
  // vkCmdBlitImage dst region (two corners) into the swapchain
  VkOffset3D a{0, 0, 0}, b{640, 325, 1};
  MapOffsets3DToSwapchain(&a, &b, advertised, swapchain, window);
  EXPECT_EQ(a.x, 0);
  EXPECT_EQ(b.x, 1280);
  EXPECT_EQ(b.y, 650);
  EXPECT_EQ(b.z, 1);
  // vkCmdCopyImage/vkCmdResolveImage dst region (offset + extent)
  VkOffset3D off{0, 0, 0};
  VkExtent3D ext{640, 325, 1};
  MapOffsetExtentToSwapchain(&off, &ext, advertised, swapchain, window);
  EXPECT_EQ(ext.width, 1280u);
  EXPECT_EQ(ext.height, 650u);
  EXPECT_EQ(ext.depth, 1u);
  // present-pass renderArea stretches exactly onto the swapchain
  VkRect2D ra{{0, 0}, {640u, 325u}};
  MapRectToSwapchain(&ra, advertised, swapchain, window);
  EXPECT_EQ(ra.extent.width, 1280u);
  EXPECT_EQ(ra.extent.height, 650u);
}

TEST(RenderScalePolicyTest, AdvertisedFrameExtentIsTheGuestCoordinateSpace) {
  // Guest space (pointer coordinates, IME rectangles) is the frame the engine
  // draws into, not the window: 1280x650 at 0.7 -> 896x455.
  const VkExtent2D frame = AdvertisedFrameExtent({1280, 650}, 0.7);
  EXPECT_EQ(frame.width, 896u);
  EXPECT_EQ(frame.height, 455u);
  // A window point maps into guest space with the same factor the layer uses
  // to stretch the frame back onto the window, so the engine cursor lands
  // under the real cursor. Reported position: window (238, 307).
  const double scale_x = static_cast<double>(frame.width) / 1280.0;
  const double scale_y = static_cast<double>(frame.height) / 650.0;
  const double guest_x = 238.0 * scale_x;
  const double guest_y = 307.0 * scale_y;
  EXPECT_DOUBLE_EQ(guest_x, 166.6);
  EXPECT_DOUBLE_EQ(guest_y, 214.9);
  EXPECT_DOUBLE_EQ(guest_x / scale_x, 238.0);
  EXPECT_DOUBLE_EQ(guest_y / scale_y, 307.0);
  // Scale 1 is a passthrough, and other scales use the same llround rule the
  // surface clamp uses.
  const VkExtent2D native = AdvertisedFrameExtent({1280, 650}, 1.0);
  EXPECT_EQ(native.width, 1280u);
  EXPECT_EQ(native.height, 650u);
  const VkExtent2D half = AdvertisedFrameExtent({1280, 650}, 0.5);
  EXPECT_EQ(half.width, 640u);
  EXPECT_EQ(half.height, 325u);
  const VkExtent2D odd = AdvertisedFrameExtent({1366, 768}, 0.7);
  EXPECT_EQ(odd.width, 956u);
  EXPECT_EQ(odd.height, 538u);
}

}  // namespace
}  // namespace graphics
}  // namespace mocktail
