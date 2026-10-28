# 🖥️ MyGraphicEngineOpenGL — Deferred PBR Game Engine

[![C++](https://img.shields.io/badge/C++-20-blue.svg)](https://isocpp.org/)
[![OpenGL](https://img.shields.io/badge/OpenGL-4.6-green.svg)](https://www.opengl.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Git LFS](https://img.shields.io/badge/Git_LFS-enabled-orange.svg)](https://git-lfs.com)
[![XERJ Corpus Hub contributor](https://xerj.org)](https://xerj.org)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](http://makeapullrequest.com)

<p align="center">
  <b>A custom OpenGL 4.6 game engine with Deferred PBR Rendering, Cascaded Shadow Maps, Skeletal Animation, HDR, Bloom, Volumetric Fog, Physics, Lua Scripting, and a full ImGui Level Editor — built entirely from scratch in C++.</b>
</p>

<p align="center">
  <img src="screenshots/CSM+SK.png" alt="Engine Screenshot" width="800"/>
  <br>
  <i>Deferred PBR rendering with cascaded shadows, HDR tone mapping, and skinned animation</i>
</p>

---

## 🌟 About The Project

This is my **custom game engine** — built from the ground up using **C++20** and **OpenGL 4.6**. No commercial engines, no black boxes. Just pure code, deep understanding, and hundreds of hours of debugging.

The engine features a **Deferred PBR Renderer** with a 5-attachment G-Buffer, **Cascaded Shadow Maps (CSM)** with skinned character shadows, **Skeletal Animation** with full glTF skin parsing, **HDR + Tone Mapping**, **Auto-Exposure**, **Bloom**, **Volumetric Fog**, **Cubemap Environment Reflections**, a **full Level Editor** with Undo/Redo, a **Lua Scripting** layer, **Jolt Physics** integration, and a **Play Mode** with a first-person controller.

---

## ✨ Key Features

### 🎨 Rendering Pipeline

- **Deferred Shading (5-attachment G-Buffer)** — Position, Normal, Albedo, Metallic/Roughness, Emissive
- **Physically Based Rendering (PBR)** — Cook-Torrance BRDF with GGX distribution, Smith geometry, Fresnel-Schlick
- **Cascaded Shadow Maps (CSM)** — 4 cascades, Poisson disk PCF (16-tap), cascade blending, normal-offset bias, **skinned character shadows**
- **Skeletal Animation** — Full glTF skin parsing (joints + inverse bind matrices), 128-bone uniforms, GPU skinning in both G-Buffer and CSM passes
- **Dynamic Lighting (up to 128 lights per frame)** —
  - ☀️ Directional Light
  - 💡 Point Light (distance attenuation)
  - 🔦 Spot Light (inner/outer cone falloff)
  - 🌫️ Ambient Light (uniform albedo illumination)
- **HDR + ACES Tone Mapping** — Exposure, saturation, contrast, gamma
- **Auto-Exposure** — Luminance histogram → ping-pong adaptation → PBO readback
- **Bloom** — Soft-knee threshold + mip chain
- **FXAA** — Optional fast anti-aliasing
- **Contact Shadows** — Screen-space ray-marched soft shadows
- **Volumetric Fog** — Height-based density, configurable steps, color
- **Environment Reflections** — Cubemap with roughness-based mip level
- **Skybox** — Cubemap loading with automatic face detection
- **Gradient Sky Fallback** — Procedural sky with sun + clouds (FBM noise)
- **Frustum Culling** — Sphere-vs-frustum test per entity
- **Debug View** — Position, Normal, Albedo, Metallic/Roughness, Emissive, Depth, Cascade coloring
- ⚠️ **Screen-Space Reflections (SSR)** — Work in progress

### 🦴 Animation & Models

- **Custom glTF / GLB Loader** — Full JSON + binary buffer parsing, embedded image decoding, external `.bin`, multi-root scenes
- **Skeletal Animation** — Skin parsing, joints + inverse bind matrices, JOINTS_0/WEIGHTS_0 vertex attributes
- **128-bone GPU Skinning** — `mat4[128]` uniform array applied in G-Buffer and CSM passes
- **Animation Player** — Clip selection, playback speed, loop, scrub
- **Multi-primitive meshes** — One material per primitive
- **Multi-root scene traversal** — Full node hierarchy
- **Texture Cache** — 16× anisotropic filtering

### 🛠️ Level Editor (ImGui)

- **Dark Pro Theme** — Clean, professional ImGui styling
- **ImGuizmo Gizmos** — Translate / Rotate / Scale in Local or World space (`T` / `R` / `S` hotkeys)
- **Entity Management** — Add, select, duplicate, delete, focus
- **9 Entity Types** — Static, Player, Ball, Camera, Empty, Point Light, Spot Light, Directional Light, Ambient Light
- **Entity List** — Icon-coded, color-coded, with tooltips and right-click context menu
- **Material Slots (Blender-style)** — Per-mesh material editing with live preview
- **Material Inspector** — Albedo, Normal, Metallic/Roughness, AO, Height, Emissive, SSS (with texture overrides)
- **Light Inspector** — Color, intensity, range, inner/outer cone angles
- **Script Selector** — Assign Lua scripts with per-entity parameters
- **Script Parameters** — Key/float table, editable live, serialized with the level
- **Undo / Redo** — Command pattern with `Ctrl+Z` / `Ctrl+Y`
- **Save / Load Levels** — Full material + slot serialization
- **Content Browser** — Browse and import GLTF/GLB models directly
- **Camera FOV** — Independent editor and game FOV controls
- **Performance Monitor** — FPS, frame time, entity count overlay

### ⚙️ Systems

- **Jolt Physics** — Box / Sphere / Capsule collisions, fixed 60 Hz timestep with interpolation
- **Lua Scripting (sol2)** — 40+ API functions: transforms, materials, physics, camera, input, spawn, raycast, prefabs, game state, script parameters
- **Texture Cache** — Hash map to avoid redundant loads
- **Anisotropic Filtering** — 16× on all textures

### 🎮 Play Mode

- **First-Person Controller** — WASD movement, mouse look, jump (physics-driven)
- **Physics Simulation** — Player body, dynamic entities, collision with the world
- **Play / Stop Toggle** — Restores the editor state on stop

---

## 🛠️ Tech Stack

| Component | Technology |
| :--- | :--- |
| **Language** | C++20 |
| **Graphics API** | OpenGL 4.6 Core Profile |
| **Windowing** | GLFW |
| **Math** | GLM |
| **UI** | Dear ImGui + ImGuizmo |
| **Physics** | Jolt Physics |
| **Scripting** | Lua 5.4 + sol2 |
| **Model Loading** | Custom glTF/GLB parser |
| **Image Loading** | stb_image |
| **Build System** | CMake + MSVC 2022 |
| **Large Files** | Git LFS |

---

## 🚀 Getting Started

### Prerequisites

- **Windows 10/11** (x64)
- **Visual Studio 2022** or newer with C++ development tools
- **OpenGL 4.6** compatible GPU (GTX 1060 / RX 580 or better)
- **CMake 3.15+**
- **Git LFS** installed

### Build Instructions

1. **Clone the repository**:

   ```bash
   git clone https://github.com/RodwanRamzi/MyGraphicEngineOpenGL.git
   cd MyGraphicEngineOpenGL
   ```

2. **Pull large assets** (models, textures):

   ```bash
   git lfs install
   git lfs pull
   ```

3. **Build**:

   ```bash
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```

   Or on Windows, simply run:

   ```bash
   build_engine.bat
   ```

Shaders and assets are automatically copied to `build/` after compilation.

---

## 🎮 Controls

| Key | Action |
| :--- | :--- |
| **WASD** | Move (in Play Mode) |
| **Right Mouse Button** | Look around |
| **Space** | Jump (in Play Mode) |
| **R** | Reset player & camera (in Play Mode) |
| **F5** | Toggle Play Mode |
| **Ctrl+Z** | Undo |
| **Ctrl+Y** | Redo |
| **T** | Translate gizmo |
| **R** | Rotate gizmo (in Editor Mode) |
| **S** | Scale gizmo |
| **Esc** | Exit Play Mode / Release cursor |

---

## 📁 Project Structure

```text
MyGraphicEngineOpenGL/
├── Main.cpp                       # Entry point + editor loop
├── Camera.h / .cpp                # Camera + controls
├── Model.h / .cpp                 # Custom glTF/GLB loader
├── Mesh.h / .cpp                  # Mesh + VAO/VBO/EBO + skinning
├── Shader.h / .cpp                # GLSL program wrapper
├── Texture.h / .cpp               # Texture loading
├── CSM.h / .cpp                   # Cascaded shadow maps
├── BloomPass.h / .cpp             # Bloom post-process
├── LUT.h / .cpp                   # 3D color lookup texture
├── ScriptComponent.h / .cpp       # Lua script wrapper
├── AsyncLoader.h / .cpp           # Background file loading
├── shaders/                       # GLSL sources
│   ├── gBuffer.vert / .frag
│   ├── deferred_lighting.vert / .frag
│   ├── csm.vert / .frag
│   ├── depth.vert / .frag
│   ├── ssr.vert / .frag
│   ├── ssr_composite.frag
│   ├── luminance.vert / .frag
│   ├── exposure.vert / .frag
│   ├── volumetric_fog.vert / .frag
│   ├── skybox.vert / .frag
│   ├── fxaa.vert / .frag
│   ├── fullscreen.vert
│   └── line.vert / .frag
├── models/                        # glTF / GLB assets
├── cubemap/                       # Skybox cubemaps
├── luts/                          # Color grading + SSS LUTs
├── Scripts/                       # Lua scripts
├── Levels/                        # Saved levels (.txt)
├── Libraries/                     # Vendored: Jolt, ImGui, sol2, glad, glm, stb
└── screenshots/                   # README images
```

---

## 🧪 Sample Lua Script

```lua
-- Scripts/bob.lua
-- Bobbing animation driven by editable parameters

local time = 0.0

function onStart(idx)
    if not has_param(idx, "speed")     then set_param(idx, "speed", 2.0)     end
    if not has_param(idx, "amplitude") then set_param(idx, "amplitude", 0.5) end
    if not has_param(idx, "base_y")    then set_param(idx, "base_y", 1.0)    end
end

function onUpdate(dt, idx)
    time = time + dt
    local speed     = get_param(idx, "speed")
    local amplitude = get_param(idx, "amplitude")
    local base_y    = get_param(idx, "base_y")

    local px, _, pz = get_pos(idx)
    set_pos(idx, px, base_y + math.sin(time * speed) * amplitude, pz)
end

function onDestroy(idx)
    log("Bob script stopped on entity " .. idx)
end
```

Each script can declare its own parameters, and they appear in the Script Parameters panel — fully editable live and serialized alongside the level structure!

---

## 📸 Screenshots

| View | File Path |
| :--- | :--- |
| **Hero / Main Render** | `screenshots/hero.png` |
| **Full Editor UI** | `screenshots/editor.png` |
| **Skinned Character + CSM** | `screenshots/CSM+SK+ModelLoader.png` |
| **Multi-Material / Multi-Primitive** | `screenshots/C/MultiMaterial-MultiPremitive Test.png` |
| **Bloom** | `screenshots/C/bloom Test.png` |

---

## 🗺️ Roadmap

**Recently Shipped:**

- [x] **Cascaded Shadow Maps (CSM)** — 4 cascades + skinned character shadows
- [x] **Skeletal Animation** — glTF skins, 128-bone GPU skinning
- [x] **Async Asset Loading** — background thread pool
- [x] **Auto-Exposure** — luminance histogram + PBO readback
- [x] **Material Slots** — Blender-style per-mesh editing

**In Progress:**

- [ ] ⚠️ **Screen-Space Reflections (SSR)** — ray march implemented, tuning quality/artifacts
- [ ] ⚠️ **Engine Refactor** — splitting `Main.cpp` into modules
- [ ] ⚠️ **Temporal Antialiasing (TAA)** — motion vectors + history reprojection

**Next Up:**

- [ ] **Half-Resolution SSR** — 2× faster with bilateral upsample
- [ ] **Compute-Based Bloom** — GPU-driven mip chain

**Planned:**

- [ ] **Deferred Decals**
- [ ] **Particle System** (GPU-driven)
- [ ] **LOD System** + Imposters
- [ ] **GPU Culling** — indirect draws + compute frustum test
- [ ] **Node-Based Material Editor**
- [ ] **Timeline Animation Editor**
- [ ] **Asset Pipeline** — compiled mesh/texture cache
- [ ] **Scene Streaming / Multi-scene**
- [ ] **Audio System**

---

## 📜 License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.

---

## 🤝 Acknowledgements

Special thanks to the incredible open-source community:

- **Jolt Physics** — Jorrit Rouwe
- **Dear ImGui** — Omar Cornut
- **ImGuizmo** — Cédric Guillemet
- **sol2** — ThePhD
- **stb** — Sean Barrett
- **glad** — David Herberth
- **GLM** — G-Truc
- **GLFW** — The GLFW team

---

<p align="center">
  <b>Built with ❤️ in Libya 🇱🇾 by <a href="https://github.com/RodwanRamzi">Rodwan Ramzi</a></b>
</p>
