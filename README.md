# Mocktail

[![CI](https://github.com/komaruworld/mocktail/actions/workflows/ci.yml/badge.svg)](https://github.com/komaruworld/mocktail/actions/workflows/ci.yml)
[![Stars](https://img.shields.io/github/stars/komaruworld/mocktail?style=flat&logo=github)](https://github.com/komaruworld/mocktail/stargazers)
[![Downloads](https://img.shields.io/github/downloads/komaruworld/mocktail/total?logo=github)](https://github.com/komaruworld/mocktail/releases/latest)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![Discord](https://img.shields.io/discord/1543955607869063269?label=discord)](https://discord.gg/rhgpfcmFSD)

Mocktail runs the Android `x86_64` Roblox client on Linux and has experimental
`aarch64` builds for the Android `arm64-v8a` client. It also runs in a Linux
userspace hosted by FreeBSD's Linuxulator. Mocktail provides the Android ABI
and JNI pieces the client expects, then connects them to SDL3 and Vulkan or
OpenGL on the Linux side.

Mocktail is an independent community project. It is not affiliated with Roblox
Corporation or VinegarHQ and does not distribute the Roblox client.

## How it works

```
Roblox APK -> signature and ABI checks -> Bionic + JNI -> SDL3 + Vulkan/OpenGL
```

The APK is checked before any native code is loaded. It is downloaded on first
launch and is not bundled with Mocktail. The last working copy is kept in case
an update fails.

## Install with Flatpak

Install the stable release from Flathub:

```bash
flatpak install flathub space.bigrat.mocktail
flatpak run space.bigrat.mocktail
```

Nightly builds are produced automatically from the latest `main` branch:

```bash
flatpak install --user https://mocktail.bigrat.space/mocktail.flatpakref
flatpak run space.bigrat.mocktail
```

## Install from the AUR

Arch Linux users can install either the pinned source release (`mocktail`) or
the current development version (`mocktail-git`) with an AUR helper:

```bash
paru -S mocktail-git
# or
yay -S mocktail-git
```

Use `mocktail` instead of `mocktail-git` to build the pinned release from
source, or install the stable prebuilt package with `paru -S mocktail-bin` or
`yay -S mocktail-bin`.

## Install with DNF

Fedora 44 users can install either the stable `mocktail` package or the
`mocktail-nightly` package built from `main`:

```bash
sudo curl -fsSL https://mocktail.bigrat.space/rpm/mocktail.repo \
  -o /etc/yum.repos.d/mocktail.repo
sudo dnf install mocktail
# or
sudo dnf install mocktail-nightly
```

## Install on Bazzite (immutable Fedora)

Bazzite's base is read-only (rpm-ostree), so prefer the self-contained Flatpak —
it bundles its own libraries and already talks to Bazzite's GameMode daemon:

```bash
flatpak install flathub space.bigrat.mocktail
flatpak run space.bigrat.mocktail
```

Ready-made `ujust`/`just` recipes ship in
[`packaging/bazzite/mocktail.just`](packaging/bazzite/mocktail.just). The RPM
repo also matches Bazzite 44 and can be layered with `rpm-ostree install
mocktail` (last resort, needs a reboot), and AppImage/native builds can run
inside a distrobox container. See the full
[Bazzite guide](packaging/bazzite/README.md) for these options plus GameMode,
MangoHud, and Gamescope tuning. Mocktail detects Bazzite at startup and logs
it (`bazzite=yes immutable=yes`) in every session log and support bundle.

## Install with APT

Ubuntu 26.04 users can install either the stable `mocktail` package or the
`mocktail-nightly` package built from `main`:

```bash
sudo install -d -m 0755 /etc/apt/keyrings
sudo curl -fsSL https://mocktail.bigrat.space/mocktail-packages.gpg \
  -o /etc/apt/keyrings/mocktail.gpg
echo "deb [arch=amd64 signed-by=/etc/apt/keyrings/mocktail.gpg] https://mocktail.bigrat.space/apt mocktail main" | \
  sudo tee /etc/apt/sources.list.d/mocktail.list >/dev/null
sudo apt update
sudo apt install mocktail
# or
sudo apt install mocktail-nightly
```

## Direct downloads

Direct AppImage, DEB, and RPM downloads are available from the
[Website](https://mocktail.bigrat.space/) and
[GitHub Releases](https://github.com/komaruworld/mocktail/releases).

<details>
<summary>Screenshots</summary>

![Roblox home in Mocktail](assets/screenshots/flatpak-home.png)

![Roblox gameplay in Mocktail](assets/screenshots/flatpak-gameplay-tower.png)

![Roblox experience in Mocktail](assets/screenshots/flatpak-gameplay-lobby.png)

</details>

## FFlag overrides

Put a JSON object in `$XDG_CONFIG_HOME/mocktail/fflags.json` (usually
`~/.config/mocktail/fflags.json`; inside the Flatpak
`~/.var/app/space.bigrat.mocktail/config/mocktail/fflags.json`). Mocktail
applies it on the next launch. The main YAML config lives next to it as
`config.yaml`.

PC profiles use the legacy Charts page by default to avoid the blank screen
caused by `Color3` errors in the newer SDUI page. An explicit
`FFlagLuaAppChartsAppPage` override takes precedence over this default.

```json
{
  "FFlagExample": "True",
  "DFIntExample": "120"
}
```

Note: since Roblox's FastFlag allowlist (2025-09-29), the client silently
ignores any flag in `fflags.json` that is not on the current allowlist, so
this file only works for the handful of allowlisted flags. Mocktail's own
YAML-driven policies (in `config.yaml`) are injected through the host
channel instead.

## Game network domain allowlist

Set `network.allowed_domains` in `config.yaml` to a comma-separated list
(e.g. `allowed_domains: "roblox.com,google.com,cloudflare.com,discord.com"`).
While playing, the engine may then only resolve/connect to those domains
and their subdomains; every other host is refused at DNS time
(`EAI_NONAME`) and logged as `[dns] blocked host=... reason=domain-allowlist`
in the session log. **Raw IPs are covered too:** a hard-coded IPv4 literal
is reverse-resolved (PTR) via 8.8.8.8 / 8.8.4.4 / 1.1.1.1 and allowed only
when a PTR name passes the allowlist *and* forward A records point back to
that IP (so the owner of an IP's reverse zone cannot spoof `roblox.com`);
direct `connect()` calls to public IPs are gated the same way and logged as
`[net] blocked ip=... reason=domain-allowlist`. IPs that an allowlisted
hostname resolved to are trusted directly (recorded in the same cache,
overriding any earlier cached PTR denial for that IP), so CDN edge IPs
whose PTR name is not under an allowed domain are not blocked; the PTR
gate applies only to IPs no allowlisted hostname ever produced.
Loopback, RFC1918, and link-local ranges never leave the machine and always
pass. Raw-IP decisions
are cached on disk at `$XDG_CACHE_HOME/mocktail/domain_allowlist_cache.json`
(mode 0600) so later launches skip the PTR/A round trips; the cache is
stamped with the active Roblox payload id and is discarded/overwritten
whenever that changes (the updater also deletes the file on every payload
promotion), i.e. after each Roblox update. Empty (default) disables the
restriction. Mocktail's own updater and client-settings downloads are
unaffected.

## Universal disk logging

Set `logs: true` at the top level of `config.yaml` to write **every** log
type to disk under `$XDG_STATE_HOME/mocktail/logs` (inside the Flatpak
`~/.var/app/space.bigrat.mocktail/state/mocktail/logs/`): the session log
tees all process stdout/stderr (Mocktail, SDL, engine JNI/Android logs)
to `logs/sessions/<timestamp>.log` plus a `logs/latest.log` symlink, and
all trace channels are enabled (engine, JNI, window, GL/EGL, FPS, DNS,
asset, post-client-settings), so FPS and flag-application lines land in
the same file. Default is `logs: false`.

## Hardware tuning

Mocktail detects the host CPU and GPU at startup and records them in
`latest.log` (`cpu="..." threads=12 cores=10/12 hybrid=yes p_cores=2 e_cores=8
... gpu_integrated=yes`) and in support bundles, then adapts:

- **Hybrid Intel CPUs (Alder Lake and newer, e.g. 12th Gen i5-1235U with
  2 P-cores + 8 E-cores = 10 cores / 12 threads).** P/E cores are read from
  `/sys/devices/cpu_core/cpus` and `/sys/devices/cpu_atom/cpus`, falling back
  to per-CPU max-frequency clustering (4.40 GHz P vs 3.30 GHz E). With the
  default `performance.physics_worker_mode: throughput`, worker pools are
  sized from the **performance cores only** (4 threads on a 2P+8E chip)
  instead of all 10 physical cores, keeping the main/render threads on fast
  P-cores and leaving package power for them. Set `performance.worker_cores:
  all` in `config.yaml` to restore every-physical-core sizing, or
  `performance` to force P-cores. The chosen basis is logged as
  `[performance] worker_cores=auto hybrid=yes p_threads=4 sizing_cores=4`.
  Steady-state worker pools always size from that basis (P-core threads on a
  2P+8E chip), but the scheduler's hard ceiling (`FIntTaskSchedulerAutoThreadLimit`)
  and the mesh-cluster queue follow the **logical** thread count (12 on the
  i5-1235U), so a sudden burst — camera whip, teleport, new area streaming in —
  can spill onto the E-cores instead of dropping frames. Logged as
  `burst limit=12` in the `[runtime] physics worker mode=throughput` line.
- **Integrated GPUs (Intel Iris Xe / UHD via i915 or xe, virtual GPUs).** They
  have no dedicated VRAM and share system RAM. If you set
  `performance.memory_limit_mb`, Mocktail warns when the cap leaves under
  4 GiB of headroom for the GPU. The texture budget
  (`FIntRenderForceVideoMemorySize`) is derived from total host RAM, and the
  default `graphics.backend: direct-vulkan` uses Mesa's ANV driver on Iris Xe;
  keep `opengl` as the fallback for hosts without a working Vulkan driver.
- **Frame rate on fixed-refresh panels.** `graphics.frame_rate_limit: display`
  caps to the detected refresh rate; `unlimited` with `graphics.vsync: off`
  removes the cap (see `make run-unlimited`).
- **Render resolution vs. window size (YAML only, no popup).** In
  `config.yaml` (`$XDG_CONFIG_HOME/mocktail/config.yaml`, inside the Flatpak
  `~/.var/app/space.bigrat.mocktail/config/mocktail/config.yaml`):
  `window.render_resolution` picks `native` (default, host display), `720p`, or
  `1080p` — note this only changes the resolution the game is *told* about
  (reported display metrics); the real framebuffer always matches the SDL
  window size. `window.render_scale` (0.25–1.0, default 1.0) is the actual
  performance lever: it sets the engine's dynamic-resolution target
  (`FIntAutomaticDRSHundredthPercent`), so the 3D scene renders at that
  fraction of the window surface (e.g. `render_scale: 0.5` on a 1280x650
  window renders at 640x325) and is upscaled to the window at present — a real
  FPS win on an Iris Xe iGPU without shrinking the window.
  `window.fxaa: true` turns on Roblox's FXAA post-process to soften the
  upscale. On the `direct-vulkan` backend Mocktail applies a host-side
  framebuffer scale that needs no engine flag at all, so it works even on
  Roblox builds that ignore locally-set flags (the 2025-09-29 allowlist,
  which kills the DRS flag injected through the host client-settings
  channel): the surface `currentExtent` from
  `vkGetPhysicalDeviceSurfaceCapabilitiesKHR` is reported at `render_scale`
  of the real window, so the game sizes its offscreen render targets to that
  smaller framebuffer, while the swapchain `imageExtent` in
  `vkCreateSwapchainKHR` is kept at the real window size so presentation
  always fills the window — no black bars or top-left pinning, even on WSI
  implementations that do not upscale a smaller swapchain. Recorded rects are
  mapped accordingly in the `vkCmdSetViewport`/`vkCmdSetScissor`/
  `vkCmdSetViewportWithCount`/`vkCmdSetScissorWithCount`/
  `vkCmdBeginRendering`/`vkCmdBeginRenderPass` hooks: passes rendering into
  offscreen targets get full-window viewports/scissors/render areas scaled
  down onto the advertised framebuffer (framebuffer-sized rects are left
  untouched), while everything that lands in the swapchain — dynamic-rendering
  passes (detected via their attachment image views), legacy render passes
  (framebuffers over swapchain images are kept swapchain-sized), and
  blit/copy/resolve regions targeting a swapchain image — is stretched onto
  the window-sized swapchain, including viewports/scissors recorded before
  the pass began. A swapchain attachment is recognised both by the
  registered swapchain images and by its extent matching the swapchain, so a
  composite still gets stretched if the game's swapchain images were not
  tracked. MangoHud-style. The window itself is not resized,
  so input keeps mapping correctly because the game still sees the real
  window size. The engine DRS flag is skipped on that
  backend so the scale is not applied twice. On non-Vulkan
  backends only the flag path exists; if the build ignores it, lower
  `window.width`/`window.height` instead — that directly shrinks the real
  framebuffer, which is the guaranteed lever on an iGPU. Use `logs: true` to
  check in `latest.log` whether the DRS flag was applied. With `logs: true`
  the Vulkan adapter also prints rate-limited `[vulkan] dbg` lines
  (`swapchain#`, `swapchain image view`, `beginRendering`, `setViewport`,
  `blit`) that show which composite path is firing when a render scale looks
  cropped.

## Privacy: telemetry is disabled

Mocktail ships with all Roblox telemetry and crash reporting turned off:

- The JNI service-URL fields `telemetryUrl` / `telemetryEndpoint`
  (experience-signals ingest), `ecsv2Url` / `ecsUrl` / `eventStreamUrl`
  (client event stream), and `timeSpentUrl` are set to empty strings, so the
  client has no tracking endpoint to report to.
- All crash/backtrace upload flags (Backtrace, MetricKit, crash-reason
  telemetry) are forced off via the crash-report policy
  (`src/runtime/crash_report_policy.cc`), and the startup log confirms:
  `[runtime] Roblox crash-report uploads disabled by policy`.
- Support bundles are built locally (`scripts/collect_support_bundle.sh`) and
  never uploaded anywhere; secrets and tokens are redacted before archiving.
- `tests/no_telemetry_endpoints_test.sh` is a regression guard that fails CI
  if any tracking endpoint reappears in first-party sources.

## FAQ

To see the most Frequently Asked Question go check [FAQ.md](FAQ.md)


## Building

Linux `x86_64` is supported. Linux `aarch64` builds are available, with Roblox
runtime validation still experimental. FreeBSD 15.1 Linuxulator support is
experimental and has been tested with an `x86_64` Fedora 44 userspace. On
FreeBSD, Mocktail runs inside Linuxulator; it is not a native FreeBSD binary.
Building requires CMake 3.20+, Git, pkg-config, LLD, binutils, a C++17
compiler, SDL 3.4+, SDL3_ttf,
Vulkan, EGL, libplacebo, fontconfig, libcurl, OpenSSL, libelf, libyaml, minizip,
Capstone 5, utf8proc, nlohmann/json, GTK4, libadwaita 1.6+, and WebKitGTK 6.0.
Optional: libsecret.

See the [FreeBSD Guide](packaging/freebsd/README.md) for setup and launch instructions.

<details>
<summary>Ubuntu 26.04+</summary>

```bash
sudo apt update
sudo apt install build-essential cmake git ninja-build pkg-config lld \
  libsdl3-dev libsdl3-ttf-dev libcurl4-openssl-dev libssl-dev \
  nlohmann-json3-dev libyaml-dev libelf-dev libminizip-dev \
  libcapstone-dev libgtk-4-dev libadwaita-1-dev libwebkitgtk-6.0-dev libsecret-1-dev \
  libutf8proc-dev libfontconfig1-dev libegl-dev libvulkan-dev \
  libplacebo-dev libpng-dev zlib1g-dev
```
</details>

<details>
<summary>Arch Linux</summary>

```bash
sudo pacman -S --needed base-devel cmake git ninja pkgconf lld sdl3 sdl3_ttf \
  curl openssl nlohmann-json libyaml libelf minizip capstone gtk4 \
  libadwaita libsecret webkitgtk-6.0 libutf8proc fontconfig libglvnd \
  libplacebo vulkan-headers vulkan-icd-loader zlib
```
</details>

<details>
<summary>Fedora 44+</summary>

```bash
sudo dnf install gcc-c++ cmake git ninja-build pkgconf-pkg-config lld \
  SDL3-devel SDL3_ttf-devel libcurl-devel openssl-devel \
  nlohmann-json-devel libyaml-devel elfutils-libelf-devel minizip-ng-compat-devel \
  capstone-devel gtk4-devel libadwaita-devel webkitgtk6.0-devel libsecret-devel \
  utf8proc-devel fontconfig-devel libglvnd-devel vulkan-headers \
  vulkan-loader-devel libplacebo-devel zlib-ng-compat-devel
```
</details>

```bash
git clone --recurse-submodules https://github.com/komaruworld/mocktail.git
cd mocktail
make build
./build/mocktail
```

## Known limitations

Mocktail does not currently work with `hardened_malloc`. Using
`hardened_malloc` may cause Mocktail to fail to start or crash during runtime.
Run Mocktail without `hardened_malloc` enabled.

## License

[Apache License 2.0](LICENSE). Third-party components keep their own licenses.

## Support

You can support the project by giving it a star or with cryptocurrency:

- USDT (TON): `UQCi6Yzcc9cOctoij6n_r1K90-OdVxAT0D_xo2UzGKkQaJDY`
- USDT (TRC20): `TNPMG9Vig2xiuo2r1QqnXRChPH7Vu28Jmx`
