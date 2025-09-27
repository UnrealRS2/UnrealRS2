# UnrealRS2

An Unreal Engine 5.6.1 project that integrates the [client-377](https://github.com/UnrealRS2/client-377) RuneScape 2 (rev 377) library.  
It is built using **MSVC C++17** with **Rider**, but requires **Visual Studio 2022** with the Game Development and Desktop C++ workloads installed.

## Overview

UnrealRS2 acts as the presentation layer for the legacy RuneScape 2 client.  
It works in two main ways:

- **Framebuffer passthrough**:  
  The client’s login screen and UI are rendered as a 2D texture.  
  The viewport remains transparent, allowing clean overlay and integration.

- **Scene graph triangles**:  
  Instead of converting assets or reimplementing formats in Unreal, UnrealRS2 consumes the raw triangles rasterized by [client-377](https://github.com/UnrealRS2/client-377).

### Benefits

- ✅ **Authentic RS2 rendering** — CPU rasterizer provides an authentic scene, Unreal enhances it.
- ✅ **Enhanced fidelity** — Unreal can upscale resolution, apply post-processing, or modernize shaders.
- ✅ **Clean separation** — client logic stays in `client-377`, rendering is handled by Unreal.

## Requirements

- **Windows x64**
- **Visual Studio 2022** with:
    - Game Development with C++
    - Desktop Development with C++
- **JetBrains Rider** (recommended IDE)
- **Unreal Engine 5.6.1**
- [client-377](https://github.com/UnrealRS2/client-377) built and available (DLL + LIB).

## Building

With Rider (recommended):

- In Unreal, select Platforms (next to Run) -> Windows -> Cook content.

Back in Rider:
- Open the Unreal project (`UnrealRS2.uproject`).
- Select **Development | Win64** configuration.
- **Run**.

## Roadmap
- [ ] Create Unreal material for authentic RS2 lighting / texture
- [ ] Post-process pipelines

---

UnrealRS2 + [client-377](https://github.com/UnrealRS2/client-377) together provide a foundation for experiencing RuneScape 2 (rev 377) inside Unreal Engine with authentic rendering and modern graphical fidelity.
