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

#include <algorithm>
#include <charconv>
#include <cmath>

namespace mocktail {
namespace runtime {

bool ParseRenderResolution(std::string_view value, RenderResolution* out) {
  if (value.empty() || value == "ask") {
    if (out != nullptr) *out = RenderResolution::kAsk;
    return true;
  }
  if (value == "native") {
    if (out != nullptr) *out = RenderResolution::kNative;
    return true;
  }
  if (value == "720p") {
    if (out != nullptr) *out = RenderResolution::k720p;
    return true;
  }
  if (value == "1080p") {
    if (out != nullptr) *out = RenderResolution::k1080p;
    return true;
  }
  return false;
}

std::string_view RenderResolutionName(RenderResolution resolution) {
  switch (resolution) {
    case RenderResolution::kAsk:
      return "ask";
    case RenderResolution::kNative:
      return "native";
    case RenderResolution::k720p:
      return "720p";
    case RenderResolution::k1080p:
      return "1080p";
  }
  return "ask";
}

bool ParseRenderScale(std::string_view value, double* out) {
  if (value.empty()) {
    if (out != nullptr) *out = kMaxRenderScale;
    return true;
  }
  double parsed = 0.0;
  const char* begin = value.data();
  const char* end = begin + value.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc() || result.ptr != end || !(parsed >= kMinRenderScale && parsed <= kMaxRenderScale)) {
    return false;
  }
  if (out != nullptr) *out = parsed;
  return true;
}

DisplaySize RenderResolutionSize(RenderResolution preset,
                                 const DisplaySize& native_display) {
  switch (preset) {
    case RenderResolution::k720p:
      return {1280, 720};
    case RenderResolution::k1080p:
      return {1920, 1080};
    case RenderResolution::kAsk:
    case RenderResolution::kNative:
      return native_display;
  }
  return native_display;
}

DisplaySize ApplyRenderScale(const DisplaySize& size, double scale) {
  if (!(scale > 0.0)) {
    scale = kMaxRenderScale;
  }
  if (scale > kMaxRenderScale) {
    scale = kMaxRenderScale;
  }
  if (scale < kMinRenderScale) {
    scale = kMinRenderScale;
  }
  return {std::max<std::int32_t>(
              320, static_cast<std::int32_t>(std::lround(size.width * scale))),
          std::max<std::int32_t>(
              180, static_cast<std::int32_t>(std::lround(size.height * scale)))};
}

}  // namespace runtime
}  // namespace mocktail
