# Abyss Sanctum

A real-time 3D underwater cave environment built with C++ and OpenGL 3.3. Features procedurally generated cave geometry using FBM noise, dynamic multi-light bioluminescent crystal clusters, raycast-placed coral reef, animated caustics, skeletal remains, and a custom OBJ loader for a sunken treasure chest.

## Screenshots
<!-- Add screenshots here -->

## Features
- Procedurally generated cave tunnel and chamber using FBM noise displacement
- Up to 12 dynamic bioluminescent point lights with animated intensity
- Raycast-accurate coral reef placement on curved cave geometry
- Animated underwater caustic light patterns
- Procedural rock, bone, and coral surface materials in GLSL
- Custom OBJ loader with MTL texture support (treasure chest)
- Radial camera confinement matching cave tube geometry
- Floating particle system
- Exponential distance fog

## Dependencies

You will need the following libraries set up locally:

| Library | Purpose |
|--------|---------|
| [GLFW](https://www.glfw.org/) | Window creation and input |
| [GLAD](https://glad.dav1d.de/) | OpenGL function loader |
| [GLM](https://github.com/g-truc/glm) | Maths (vectors, matrices) |
| [SOIL2](https://github.com/SpartanJ/SOIL2) | Image loading for textures |
| [stb_image](https://github.com/nothings/stb) | OBJ texture loading |
| KHR | OpenGL extension headers (comes with GLAD) |

## Project Structure

    AbyssSanctum/
      ├── glad.c
      ├── AbyssSanctum.exe
      │
      ├── src/
      │   ├── main.cpp
      │   ├── Shader.cpp / Shader.h
      │   ├── Camera.cpp / Camera.h
      │   ├── Mesh.cpp / Mesh.h
      │   ├── CaveEnvironment.cpp / CaveEnvironment.h
      │   ├── CoralFormation.cpp / CoralFormation.h
      │   ├── Crystal.cpp / Crystal.h
      │   ├── CrystalCluster.cpp / CrystalCluster.h
      │   ├── RockFormation.cpp / RockFormation.h
      │   ├── Bones.cpp / Bones.h
      │   ├── ParticleSystem.cpp / ParticleSystem.h
      │   └── ObjModel.cpp / ObjModel.h
      │
      ├── shaders/
      │   ├── cave.vert
      │   ├── cave.frag
      │   ├── particle.vert
      │   └── particle.frag
      │
      ├── assets/
      │   └── models/
      │       ├── old_chest.glb.obj
      │       ├── old_chest.glb.mtl
      │       ├── Drewno_baseColor.jpg
      │       ├── Drewno_metallicRoughness.png
      │       ├── Drewno_normal.jpg
      │       ├── Metal_baseColor.jpg
      │       ├── Metal_metallicRoughness.png
      │       └── Metal_normal.jpg
      │
      ├── include/
      │   ├── stb_image.h
      │   ├── glad/
      │   ├── GLFW/
      │   ├── glm/
      │   ├── KHR/
      │   └── SOIL2/
      │
      ├── lib/
      │   ├── libglfw3.a
      │   ├── libglfw3dll.a
      │   └── libsoil2.a
      │
      └── .vscode/
          ├── c_cpp_properties.json
          └── tasks.json

## Building on Windows (MinGW)

### Prerequisites
- [MinGW-w64](https://www.mingw-w64.org/) installed at `C:/mingw64`
- VS Code with the C/C++ extension (optional)
- All library headers in `include/` and binaries in `lib/`

### Build Command

Run this from the project root in your terminal:

```bash
g++ -std=c++17 -o AbyssSanctum.exe glad.c \
  src/main.cpp \
  src/Shader.cpp \
  src/Camera.cpp \
  src/Mesh.cpp \
  src/CaveEnvironment.cpp \
  src/Crystal.cpp \
  src/CrystalCluster.cpp \
  src/RockFormation.cpp \
  src/CoralFormation.cpp \
  src/Bones.cpp \
  src/ParticleSystem.cpp \
  src/ObjModel.cpp \
  -I include -L lib \
  -lglfw3 -lSOIL2 -lopengl32 -lgdi32 -luser32 -lkernel32 \
  -Wall -g
```

### VS Code

If you are using VS Code, a `tasks.json` is included. Press `Ctrl+Shift+B` to build.

### Running

```bash
./AbyssSanctum.exe
```

Make sure the `shaders/` and `assets/` folders are in the same directory as the executable.

## Controls

| Key | Action |
|-----|--------|
| W / S | Move forward / backward |
| A / D | Strafe left / right |
| Space | Move up |
| Left Ctrl | Move down |
| Mouse | Look around |
| Scroll | Zoom in / out |
| ESC | Exit |

## Notes
- The camera is physically confined to the cave geometry and cannot clip through walls
- All geometry is generated procedurally at startup. No external 3D models are used for the cave itself
- The treasure chest (`assets/models/`) must be present for the OBJ loader to function
