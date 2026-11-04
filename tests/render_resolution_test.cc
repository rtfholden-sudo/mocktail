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
