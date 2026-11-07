#!/usr/bin/env bash
# Copyright 2026 Mocktail Project Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Regression guard: the Roblox base/API/cookie endpoints and the client
# settings CDN URL must never be read from environment overrides. If they
# were, a wrapper script or tampered environment could redirect the
# authenticated session (including the .ROBLOSECURITY cookie) to an
# arbitrary host. First-party sources must pin them to roblox.com values.

set -euo pipefail

ROOT="$(cd -P -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
LEGACY_RUNTIME="${ROOT}/src/legacy/legacy_runtime.cc"

# Environment variables that must NOT be consumed as endpoint overrides in
# first-party sources (tests may quote them as blocked names).
FORBIDDEN_OVERRIDES=(
  "MOCKTAIL_BASE_URL"
  "MOCKTAIL_API_URL"
  "MOCKTAIL_COOKIE_BASE_URL"
  "MOCKTAIL_COOKIE_DOMAIN"
  "MOCKTAIL_CLIENT_SETTINGS_URL"
)

status=0

while IFS= read -r -d '' file; do
  for name in "${FORBIDDEN_OVERRIDES[@]}"; do
    if grep -qE "GetEnv(String|Int)?\(\"${name}\"" "${file}"; then
      echo "FAIL: endpoint override '${name}' is read from the environment in ${file#"${ROOT}/"}" >&2
      status=1
    fi
  done
done < <(find "${ROOT}/src" "${ROOT}/include" -type f \
           \( -name '*.cc' -o -name '*.h' -o -name '*.c' \) -print0)

# The pinning helper must exist and every pinned override must be reported
# through it, so an override attempt is always logged, never honored.
if ! grep -qF "void IgnoreRobloxEndpointOverride(" "${LEGACY_RUNTIME}"; then
  echo "FAIL: IgnoreRobloxEndpointOverride is missing in ${LEGACY_RUNTIME#"${ROOT}/"}" >&2
  status=1
fi
for name in "${FORBIDDEN_OVERRIDES[@]}"; do
  if ! grep -qF "\"${name}\"" "${LEGACY_RUNTIME}"; then
    echo "FAIL: ${name} is not pinned via IgnoreRobloxEndpointOverride in ${LEGACY_RUNTIME#"${ROOT}/"}" >&2
    status=1
  fi
done

if (( status != 0 )); then
  echo "roblox_endpoint_pinning_test: FAILED" >&2
  exit 1
fi
echo "roblox_endpoint_pinning_test passed"
