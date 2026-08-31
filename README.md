# 🖥️ MyGraphicEngineOpenGL – Deferred PBR Game Engine

[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![OpenGL](https://img.shields.io/badge/OpenGL-4.6-green.svg)](https://www.opengl.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![GitHub LFS](https://img.shields.io/badge/Git_LFS-enabled-orange.svg)](https://git-lfs.com)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](http://makeapullrequest.com)

<p align="center">
  <b>A custom OpenGL 4.6 game engine with Deferred PBR Rendering, SSAO, Bloom, and a full ImGui Level Editor – built entirely from scratch in C++17.</b>
</p>

<p align="center">
  <img src="screenshots/screenshot5.png" alt="Engine Screenshot" width="800"/>
  <br>
  <i>Engine running with PBR, SSAO, and Bloom</i>
</p>

---

## 🌟 About The Project

This is my **custom game engine** – built from the ground up using **C++17** and **OpenGL 4.6**. No commercial engines, no black boxes. Just pure code, deep understanding, and hundreds of hours of debugging.

The engine features a **Deferred PBR Renderer**, **SSAO**, **Bloom**, **Dynamic Shadows**, a **Level Editor** with Undo/Redo, and a **Game Mode** with physics and player controls.

> *"Built during power cuts, with no formal CS degree – just persistence."*

---

## ✨ Key Features

### 🎨 Rendering Pipeline
- **Deferred Shading (G-Buffer)**: Position, Normal, Albedo, Metallic/Roughness textures for efficient multi-light rendering.
- **Physically Based Rendering (PBR)**: Cook-Torrance BRDF with GGX distribution, Smith geometry, and Fresnel-Schlick.
- **SSAO (Screen-Space Ambient Occlusion)**: Realistic ambient occlusion with adjustable radius, bias, and power.
- **Bloom**: Full post-processing bloom with threshold and intensity controls.
- **Shadow Mapping**: Directional shadow maps with Percentage Closer Filtering (PCF).
- **HDR & Tone Mapping**: ACES-style tone mapping with exposure, saturation, contrast, and gamma controls.
- **Dynamic Lighting**: Directional, Point, and Spot lights in a single pass.

### 🛠️ Level Editor (ImGui)
- **Entity Management**: Add, select, move, rotate, and scale entities (Static, Player, Ball).
- **Save/Load**: Serialize and deserialize entire levels to/from text format.
- **Undo/Redo**: Full command pattern implementation (Ctrl+Z / Ctrl+Y).
- **Mouse Picking**: Ray-casting for precise object selection.
- **Content Browser**: Browse and import GLTF/GLB models directly from the editor.

### 📦 Asset Pipeline
- **Custom GLTF/GLB Loader**: Parses binary buffers, accessors, node hierarchies, textures, and tangents/bitangents for TBN matrices.
- **Game Mode**: First-person player controller with ball physics (gravity, bouncing, collision).

### 🎮 Game Mode
- **Player Controller**: Move with WASD, look with mouse.
- **Ball Physics**: Gravity, bouncing, and collision with the ground.
- **Entity Types**: Static, Player, and Ball with custom behaviors.

---

## 🛠️ Tech Stack

| Component | Technology |
| :--- | :--- |
| **Language** | C++17 |
| **Graphics API** | OpenGL 4.6 |
| **Windowing** | GLFW |
| **Math** | GLM |
| **UI** | ImGui (Dear ImGui) |
| **Textures** | stb_image |
| **Build System** | Visual Studio 2022 / CMake |

---

## 🚀 Getting Started

### Prerequisites
- Windows 10/11
- Visual Studio 2022 (or newer) with C++ development tools
- OpenGL 4.6 compatible GPU (GTX 1060 / RX 580 or better)
- Git LFS (for large assets)

### Build Instructions

1. **Clone the repository**:
   ```bash
   git clone https://github.com/RodwanRamzi/MyGraphicEngineOpenGL.git
   cd MyGraphicEngineOpenGL
