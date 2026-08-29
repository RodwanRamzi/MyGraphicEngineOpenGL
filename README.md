# 🖥️ MyGraphicEngineOpenGL – Deferred PBR Renderer

[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![OpenGL](https://img.shields.io/badge/OpenGL-4.6-green.svg)](https://www.opengl.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![GitHub LFS](https://img.shields.io/badge/Git_LFS-enabled-orange.svg)](https://git-lfs.com)

A **custom OpenGL 4.6 game engine** built from scratch featuring **Deferred Shading**, **Physically Based Rendering (PBR)**, and a fully functional **Level Editor**. No commercial engines—just pure C++, OpenGL, and thousands of hours of dedication.

<p align="center">
  <img src="screenshots/screenshot1.png" alt="Engine Screenshot" width="800"/>
</p>

---

## ✨ Key Features

### 🎨 Rendering Pipeline
- **Deferred Shading (G-Buffer)**: Position, Normal, Albedo, and Metallic/Roughness textures for efficient multi-light rendering.
- **Physically Based Rendering (PBR)**: Cook-Torrance BRDF with GGX distribution, Smith geometry, and Fresnel-Schlick.
- **Dynamic Lighting**: Directional (with Shadow Mapping), Point, and Spot Lights—all in a single lighting pass.
- **HDR & Tone Mapping**: ACES-style tone mapping with exposure, saturation, and gamma correction.

### 🛠️ Level Editor (ImGui)
- **Entity Management**: Add, select, move, rotate, and scale entities (Static, Player, Ball).
- **Save/Load**: Serialize and deserialize entire levels to/from JSON.
- **Undo/Redo**: Full command pattern implementation.
- **Mouse Picking**: Ray-casting for precise object selection.

### 📦 Asset Pipeline
- **Custom GLTF/GLB Loader**: Parses binary buffers, accessors, node hierarchies, textures, and tangents/bitangents for TBN matrices—all from scratch.
- **Game Mode**: First-person player controller with physics (gravity, bouncing, collision).

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

### Build Instructions
1. **Clone the repository**:
   ```bash
   git clone https://github.com/RodwanRamzi/MyGraphicEngineOpenGL.git
