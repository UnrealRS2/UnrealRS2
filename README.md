# URRL — Unreal RuneLite Frontend (WIP)

URRL is a work-in-progress Unreal Engine 5.6.1 frontend for RuneLite.  
It communicates through a shared memory region to sync data in real time.

## Shared Memory Sync

### RuneLite → Unreal
- Camera position & rotation
- Framebuffer (game frame, UI, overlays)

### Unreal → RuneLite
- Resolution updates
- Mouse move / press / release

## Status
UI / Overlays are forwarded to unreal  
(Captures 4K frames at 120FPS with reasonable hardware.)  
  
**No scene graph forwarding to unreal**
  
**Partial input forwarding to runelite**

## Goal
A modern Unreal-based frontend for RuneLite with real-time camera and framebuffer sync.

## Building
Setup your windows environment for Unreal 5.6.1  
Build the Game configuration using Visual Studio 2022  
Cook the content using Unreal

## Running
You must run via PIE (Play in Editor - Buggy) or via packaged build, it will not run correctly inside Rider  
Using this would require a plugin capable of interfacing with https://github.com/UnrealRS2/rl-gpushared-shim