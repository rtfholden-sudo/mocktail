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

}  // namespace runtime
}  // namespace mocktail
