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

}  // namespace
}  // namespace graphics
}  // namespace mocktail
