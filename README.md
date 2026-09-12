# 🖥️ MyGraphicEngineOpenGL — Deferred PBR Game Engine

[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![OpenGL](https://img.shields.io/badge/OpenGL-4.6-green.svg)](https://www.opengl.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Git LFS](https://img.shields.io/badge/Git_LFS-enabled-orange.svg)](https://git-lfs.com)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](http://makeapullrequest.com)

<p align="center">
  <b>A custom OpenGL 4.6 game engine with Deferred PBR Rendering, SSAO, HDR, Bloom, Volumetric Fog, Physics, Lua Scripting, and a full ImGui Level Editor — built entirely from scratch in C++17.</b>
</p>

<p align="center">
  <img src="screenshots/hero.png" alt="Engine Screenshot" width="800"/>
  <br>
  <i>Deferred PBR rendering with SSAO, HDR tone mapping, and environment reflections</i>
</p>

---

## 🌟 About The Project

This is my **custom game engine** — built from the ground up using **C++17** and **OpenGL 4.6**. No commercial engines, no black boxes. Just pure code, deep understanding, and hundreds of hours of debugging.

The engine features a **Deferred PBR Renderer** with a 5-attachment G-Buffer, **SSAO**, **HDR + Tone Mapping**, **Bloom**, **PCF Shadow Mapping**, **Volumetric Fog**, **Cubemap Environment Reflections**, a **full Level Editor** with Undo/Redo, a **Lua Scripting** layer, **Jolt Physics** integration, and a **Play Mode** with a first-person controller.

> *"Built during power cuts, with no formal CS degree — just persistence."*

---

## ✨ Key Features

### 🎨 Rendering Pipeline

- **Deferred Shading (5-attachment G-Buffer)** — Position, Normal, Albedo, Metallic/Roughness, Emissive
- **Physically Based Rendering (PBR)** — Cook-Torrance BRDF with GGX distribution, Smith geometry, Fresnel-Schlick
- **Dynamic Lighting (up to 128 lights per frame)** —
  - ☀️ Directional Light
  - 💡 Point Light (distance attenuation)
  - 🔦 Spot Light (inner/outer cone falloff)
  - 🌫️ Ambient Light (uniform albedo illumination)
- **Shadow Mapping** — 2048² depth map with PCF (3×3 kernel), slope-scaled bias, normal offset
- **Screen-Space Ambient Occlusion (SSAO)** — 32-sample kernel + 4×4 noise + blur pass
- **HDR + Tone Mapping** — Reinhard operator with exposure, saturation, contrast, gamma
- **Bloom** — Threshold-based post-processing with intensity control
- **FXAA** — Optional fast anti-aliasing
- **Contact Shadows** — Screen-space ray-marched soft shadows
- **Volumetric Fog** — Height-based density, configurable steps, color
- **Environment Reflections** — Cubemap with roughness-based mip level (IBL-ready)
- **Skybox** — Cubemap loading with automatic face detection
- **Gradient Sky Fallback** — Procedural sky with sun + clouds (FBM noise)
- **Frustum Culling** — Sphere-vs-frustum test per entity
- **Debug View** — 7 modes: Position, Normal, Albedo, Metallic/Roughness, Emissive, SSAO, Depth

### 🛠️ Level Editor (ImGui)

- **Dark Pro Theme** — Clean, professional ImGui styling
- **ImGuizmo Gizmos** — Translate / Rotate / Scale in Local or World space (`T` / `R` / `S` hotkeys)
- **Entity Management** — Add, select, duplicate, delete, focus
- **9 Entity Types** — Static, Player, Ball, Camera, Empty, Point Light, Spot Light, Directional Light, Ambient Light
- **Entity List** — Icon-coded, color-coded, with tooltips and right-click context menu
- **Material Inspector** — Albedo, Normal, Metallic/Roughness, AO, Height, Emissive (with texture overrides)
- **Light Inspector** — Color, intensity, range, inner/outer cone angles
- **Script Selector** — Assign Lua scripts with per-entity parameters
- **Script Parameters** — Key/float table, editable live, serialized with the level
- **Undo / Redo** — Command pattern with `Ctrl+Z` / `Ctrl+Y`
- **Save / Load Levels** — Pipe-delimited text format
- **Content Browser** — Browse and import GLTF/GLB models directly
- **Camera FOV** — Independent editor and game FOV controls
- **Performance Monitor** — FPS, frame time, entity count overlay

### ⚙️ Systems

- **Jolt Physics** — Box / Sphere / Capsule collisions, fixed 60 Hz timestep with interpolation
- **Lua Scripting (sol2)** — 40+ API functions: transforms, materials, physics, camera, input, spawn, raycast, prefabs, game state, script parameters
- **Custom GLTF/GLB Loader** — Full JSON + binary buffer parsing, node hierarchy, tangent/bitangent auto-generation
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
| **Language** | C++17 |
| **Graphics API** | OpenGL 4.6 Core Profile |
| **Windowing** | GLFW |
| **Math** | GLM |
| **UI** | Dear ImGui + ImGuizmo |
| **Physics** | Jolt Physics |
| **Scripting** | Lua 5.4 + sol2 |
| **Model Loading** | Custom GLTF/GLB parser |
| **Image Loading** | stb_image |
| **Build System** | Visual Studio 2022 (v143) |
| **Large Files** | Git LFS |

---

## 🚀 Getting Started

### Prerequisites

- **Windows 10/11** (x64)
- **Visual Studio 2022** or newer with C++ development tools
- **OpenGL 4.6** compatible GPU (GTX 1060 / RX 580 or better)
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

3. **Open the solution**:
   - Launch `MyGraphicEngineOpenGL.sln` (or your updated configuration file) in Visual Studio 2022.

4. **Configure and build**:
   - Set configuration to **Debug x64** (or **Release x64**).
   - Build with `Ctrl+Shift+B`.
   - Run with `F5`.

### Controls

| Key | Action |
| :--- | :--- |
| **WASD** | Move (in Play Mode) |
| **Right Mouse Button** | Look around (in Play Mode) |
| **Space** | Jump (in Play Mode) |
| **R** | Reset player & camera (in Play Mode) |
| **F5** | Toggle Play Mode |
| **Ctrl+Z** | Undo |
| **Ctrl+Y** | Redo |
| **T / R / S** | Translate / Rotate / Scale gizmo |
| **Esc** | Exit Play Mode |
| **G** | Toggle Cursor Lock When Click Right Mouse Button |

---

## 📁 Project Structure

```text
MyGraphicEngineOpenGL/
├── Main.cpp                       # Entry point + editor loop
├── Camera.h / .cpp                # Camera + controls
├── Model.h / .cpp                 # Custom GLTF/GLB loader
├── Mesh.h / .cpp                  # Mesh + VAO/VBO/EBO
├── Shader.h / .cpp                # GLSL program wrapper
├── Texture.h / .cpp               # Texture loading
├── VAO.h / VBO.h / EBO.h          # OpenGL buffer wrappers
├── ScriptComponent.h / .cpp       # Lua script wrapper
├── shaders/                       # GLSL shaders
│   ├── gBuffer.vert / .frag
│   ├── deferred_lighting.vert / .frag
│   ├── depth.vert / .frag
│   ├── ssao.vert / .frag
│   ├── volumetric_fog.vert / .frag
│   ├── skybox.vert / .frag
│   ├── fxaa.vert / .frag
│   └── line.vert / .frag
├── Models/                        # GLTF / GLB assets
├── cubemap/                       # Skybox cubemaps
├── Scripts/                       # Lua scripts
├── Levels/                        # Saved levels (.txt)
├── Libraries/                     # Vendored: Jolt, ImGui, sol2, glad, glm, stb
└── docs/
    └── screenshots/               # README images
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

## 📸 Screenshots

| View | File Path |
| :--- | :--- |
| **Hero / Main Render** | `screenshots/hero.png` |
| **Full Editor UI** | `screenshots/editor.png` |
| **G-Buffer Debug Views** | `screenshots/gbuffer.png` |
| **SSAO On/Off** | `screenshots/SSAO_ON.png` `screenshots/SSAO_OFF.png` |

---

## 🗺️ Roadmap

- [ ] **Cascaded Shadow Maps (CSM)** for large scenes
- [ ] **Screen-Space Reflections (SSR)**
- [ ] **Deferred Decals**
- [ ] **Skeletal Animation**
- [ ] **Particle System** (GPU-driven)
- [ ] **LOD System** + Imposters
- [ ] **Async Asset Loading**
- [ ] **Multi-scene** / scene streaming

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
=======
>>>>>>> Stashed changes
