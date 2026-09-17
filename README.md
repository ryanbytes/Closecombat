# Closecombat Android

Clean-room, top-down tactical engine for Android inspired by the classic Close Combat series.

## Scope

- 2D top-down presentation only.
- One engine with per-title rules/data adapters.
- Android-native touch controls.
- Original commercial game assets are not bundled.
- Importers may read data from legally owned installations where formats are supported.

## Current milestone

Milestone 0 builds a playable technical prototype with:

- top-down battlefield rendering;
- selectable infantry teams;
- tap-to-move orders;
- deterministic simulation tick;
- A* obstacle routing;
- morale, suppression, health and side state;
- pan and pinch zoom;
- JNI boundary between Android UI and the native C++ simulation.

## Build

Requirements:

- JDK 17
- Android SDK 35
- Android NDK 27.2.12479018
- CMake 3.22.1
- Gradle 8.11.1 or Android Studio with a compatible Gradle installation

From the repository root:

```bash
gradle :app:assembleDebug
```

The debug APK is written under `app/build/outputs/apk/debug/`.

## Architecture

- `app/src/main/java/` — Android activity, touch input, camera and renderer.
- `app/src/main/cpp/` — deterministic simulation, units, pathfinding and JNI bridge.
- `docs/` — format research and compatibility notes.

## Legal

This repository contains no Close Combat copyrighted artwork, maps, audio, executable code, or other proprietary assets.
