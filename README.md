# Ludum Dare — Unreal Engine 5 Template

A lightweight **Unreal Engine 5.8 project template for Ludum Dare**. It provides a simple starting point for building a jam game, with settings aimed at lower-end computers and smaller downloads.

The project starts with an empty level and a Player Start. Add your own gameplay, art, and lighting. You can build with Blueprints and add C++ later.

## Main features

| Feature | What it means |
| --- | --- |
| **Forward Shading** | A rendering approach suited to simpler scenes, with fewer expensive visual features. |
| **SMAA anti-aliasing** | Smooths jagged edges using the current frame, without relying on previous frames. Built into UE 5.8; no extra plugin required. |
| **Bloom** | Adds a glow around bright objects. It stays enabled even on the Low quality preset. |
| **Baked lighting support** | Lighting for static objects can be calculated in the editor instead of recalculated during gameplay. |
| **Minimal plugins** | Engine plugins are disabled by default. Only seven editor and asset-processing plugins are explicitly enabled. Add others as your game needs them. |
| **Smaller builds** | Release builds use compression and shared shader code, and leave out editor content, debug symbols, movies, and the Crash Reporter. |

Lumen, Nanite, ray tracing, Virtual Shadow Maps, virtual textures, and Substrate are disabled. Motion blur and depth of field are disabled at project level and across every quality preset. Lens flares, automatic exposure, volumetric fog, and volumetric clouds are disabled as well.

These choices reduce the baseline cost of the project. Your game's models, textures, materials, lights, and gameplay still determine its final performance and download size.

## Desktop platforms

The project is configured for:

- **Windows:** DirectX 11, Shader Model 5.
- **Linux:** Vulkan, Shader Model 5.
- **macOS:** Metal, Shader Model 5.

Each platform uses one graphics target to avoid packaging extra shader variants. Mac builds target the build machine's architecture by default, rather than including both Intel and Apple Silicon in one package. Available architectures depend on your Unreal Engine build and toolchain.

Platform configuration is included, but packaged games and rendering still need testing on each target operating system. This template does not remove Unreal Engine's hardware and driver requirements.

## Getting started

1. Install Unreal Engine **5.8**.
2. Open `LudumDare.uproject`. If Unreal cannot find UE 5.8, use **Switch Unreal Engine Version** to select your local installation.
3. Open `Content/Maps/Startup.umap` and start building your game.
4. Add geometry and lighting before pressing Play—the starter level is intentionally empty.

The initial game settings are **1280 × 720**, windowed mode, **Low** quality, and a **60 FPS limit**. Bloom and SMAA remain enabled. Saved player settings can override these defaults; the editor viewport has its own quality settings.

For simple, static scenes, bake the lighting to keep the runtime cost down. The Low preset disables dynamic shadows; increase Shadow Quality if your game needs them.

## Building your game

Use **Platforms → Package Project** in Unreal Editor, or run the included Python 3 script on the target operating system:

```sh
python3 Tools/package.py --engine /path/to/UnrealEngine
```

On Windows:

```powershell
py Tools/package.py --engine "C:\Epic Games\UE_5.8"
```

Builds are saved to `Packaged/<platform>`. Use `--output` to choose another location, or `--dry-run` to preview the build command.

The template packages only the Startup map and its referenced assets. When adding levels, update the map list in `Config/DefaultGame.ini` and the `-map` argument in `Tools/package.py`. Assets loaded only by name may need explicit inclusion through Unreal's Asset Manager.

Only English language data is included by default. Add other languages in the packaging settings if needed. The Windows prerequisite installer is excluded to save space; include it or provide the required runtime components separately when distributing your game.

## Useful files

- `Config/DefaultEngine.ini` — rendering and platform settings.
- `Config/DefaultScalability.ini` — graphics quality presets, including Bloom and SMAA.
- `Config/DefaultGameUserSettings.ini` — initial resolution and quality settings.
- `Config/DefaultGame.ini` — project details and packaging options.
- `LudumDare.uproject` — engine association and enabled plugins.

The initial template was checked with UE 5.8.1 using a headless configuration test. That check covered map loading and rendering settings, not visual quality, frame rates, or finished builds on all three platforms.

## Inspiration

This template draws on the minimal-project and optimization approaches shared by:

- [Perfectly-Optimized](https://github.com/Imloopdev/Perfectly-Optimized)
- [UE_Minimal](https://github.com/DotBow/UE_Minimal)
- [DaftSoftware StarterProject](https://github.com/daftsoftware/StarterProject)
- [MilkyEngineer's minimal project descriptor](https://gist.github.com/MilkyEngineer/a1e953f87509877adc4587cf8776c8a2)
