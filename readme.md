# Pathtracing

A CPU Monte Carlo pathtracer implemented in C++. Features:
- Spheres and triangle meshes
- KD-tree optimization for triangle meshes
- HDR output
- Textures and samplers
- Equirectangular maps for background

## Building

This project uses [Premake](https://premake.github.io/) to build project files.

## Results

### Room, 256x256, 64K samples per pixel
![Room, 256x256, 64K samples per pixel](results/room-256-i250.png)

### Mario, 128x128, 25K samples per pixel
![Mario, 128x128, 25K samples per pixel](results/mario-128-i100.png)
