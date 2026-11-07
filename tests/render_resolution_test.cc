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

#include "runtime/render_resolution.h"

#include <gtest/gtest.h>

namespace mocktail {
namespace runtime {
namespace {

TEST(RenderResolutionTest, ParsesPresets) {
  RenderResolution preset = RenderResolution::kNative;
  EXPECT_TRUE(ParseRenderResolution("ask", &preset));
  EXPECT_EQ(preset, RenderResolution::kAsk);
  EXPECT_TRUE(ParseRenderResolution("native", &preset));
  EXPECT_EQ(preset, RenderResolution::kNative);
  EXPECT_TRUE(ParseRenderResolution("720p", &preset));
  EXPECT_EQ(preset, RenderResolution::k720p);
  EXPECT_TRUE(ParseRenderResolution("1080p", &preset));
  EXPECT_EQ(preset, RenderResolution::k1080p);
  // Empty means the default (ask).
  EXPECT_TRUE(ParseRenderResolution("", &preset));
  EXPECT_EQ(preset, RenderResolution::kAsk);
  // Null output is allowed for validity-only checks.
  EXPECT_TRUE(ParseRenderResolution("720p", nullptr));
  EXPECT_FALSE(ParseRenderResolution("4k", &preset));
  EXPECT_FALSE(ParseRenderResolution("720", &preset));
  EXPECT_FALSE(ParseRenderResolution("1080P", &preset));
  EXPECT_FALSE(ParseRenderResolution("p", nullptr));
}

TEST(RenderResolutionTest, NamesRoundTrip) {
  EXPECT_EQ(RenderResolutionName(RenderResolution::kAsk), "ask");
  EXPECT_EQ(RenderResolutionName(RenderResolution::kNative), "native");
  EXPECT_EQ(RenderResolutionName(RenderResolution::k720p), "720p");
  EXPECT_EQ(RenderResolutionName(RenderResolution::k1080p), "1080p");
  for (const char* name : {"ask", "native", "720p", "1080p"}) {
    RenderResolution preset = RenderResolution::kAsk;
    ASSERT_TRUE(ParseRenderResolution(name, &preset));
    EXPECT_EQ(RenderResolutionName(preset), std::string_view(name));
  }
}

TEST(RenderResolutionTest, ParsesRenderScale) {
  double scale = 0.0;
  EXPECT_TRUE(ParseRenderScale("1.0", &scale));
  EXPECT_DOUBLE_EQ(scale, 1.0);
  EXPECT_TRUE(ParseRenderScale("0.5", &scale));
  EXPECT_DOUBLE_EQ(scale, 0.5);
  EXPECT_TRUE(ParseRenderScale("0.25", &scale));
  EXPECT_DOUBLE_EQ(scale, 0.25);
  // Empty means no extra scaling.
  EXPECT_TRUE(ParseRenderScale("", &scale));
  EXPECT_DOUBLE_EQ(scale, 1.0);
  EXPECT_TRUE(ParseRenderScale("0.5", nullptr));
  EXPECT_FALSE(ParseRenderScale("0.1", &scale));
  EXPECT_FALSE(ParseRenderScale("1.5", &scale));
  EXPECT_FALSE(ParseRenderScale("half", &scale));
  EXPECT_FALSE(ParseRenderScale("0.5x", &scale));
}

TEST(RenderResolutionTest, ApplyRenderScaleClampsAndKeepsMinimum) {
  const DisplaySize size{1280, 720};
  const DisplaySize half = ApplyRenderScale(size, 0.5);
  EXPECT_EQ(half.width, 640);
  EXPECT_EQ(half.height, 360);
  const DisplaySize full = ApplyRenderScale(size, 1.0);
  EXPECT_EQ(full.width, 1280);
  EXPECT_EQ(full.height, 720);
  // Out-of-range scales are clamped; tiny results keep a 320x180 floor.
  const DisplaySize clamped = ApplyRenderScale(size, 5.0);
  EXPECT_EQ(clamped.width, 1280);
  const DisplaySize floored = ApplyRenderScale({640, 360}, 0.25);
  EXPECT_EQ(floored.width, 320);
  EXPECT_GE(floored.height, 180);
  const DisplaySize tiny = ApplyRenderScale({400, 200}, 0.1);
  EXPECT_EQ(tiny.width, 320);
  EXPECT_EQ(tiny.height, 180);
}

TEST(RenderResolutionTest, PresetSizesKeepWindowIndependent) {
  const DisplaySize native{2560, 1440};
  // Fixed presets ignore the native display size; ask/native keep it.
  EXPECT_EQ(RenderResolutionSize(RenderResolution::k720p, native).width, 1280);
  EXPECT_EQ(RenderResolutionSize(RenderResolution::k720p, native).height, 720);
  EXPECT_EQ(RenderResolutionSize(RenderResolution::k1080p, native).width, 1920);
  EXPECT_EQ(RenderResolutionSize(RenderResolution::k1080p, native).height, 1080);
  const DisplaySize native_result =
      RenderResolutionSize(RenderResolution::kNative, native);
  EXPECT_EQ(native_result.width, 2560);
  EXPECT_EQ(native_result.height, 1440);
  const DisplaySize ask_result =
      RenderResolutionSize(RenderResolution::kAsk, native);
  EXPECT_EQ(ask_result.width, 2560);
  EXPECT_EQ(ask_result.height, 1440);
  // A small native display still maps presets to their fixed sizes.
  const DisplaySize small_native{1366, 768};
  EXPECT_EQ(RenderResolutionSize(RenderResolution::k1080p, small_native).width,
            1920);
}

}  // namespace
}  // namespace runtime
}  // namespace mocktail
