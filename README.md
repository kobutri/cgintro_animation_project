# CG Intro Animation Project

Ein sehr kleines Echtzeit-Grafikdemoprojekt mit OpenGL, GLSL-Shadern und einer boids-ähnlichen Animationsszene.

## Struktur

- `project/` — C++ Implementierung und Assets
- `third_party/` — eingebundene Abhängigkeiten, mit `git submodule init && git submodule update` herunterladen
- `doc/` — Projektdokumentation

## Build

1. Erstelle ein Build-Verzeichnis:
   ```sh
   mkdir -p project/build
   cd project/build
   ```
2. Generiere und baue das Projekt:
   ```sh
   cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ..
   cmake --build .
   ```
