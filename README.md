# URRL — Unreal x RuneLite (WIP)

URRL is a work-in-progress Unreal Engine 5.7.4 frontend for RuneLite (only gpu plugin modified)  
see: https://github.com/UnrealRS2/runelite  
It communicates through a shared RAM block to sync data in real time.  
see: bundled under ./rl-gpushared-shim

<img width="500" height="500" alt="image" src="https://github.com/user-attachments/assets/b29c2cfc-5d27-429c-86e7-f397a52039de" />
<img width="500" height="500" alt="image" src="https://github.com/user-attachments/assets/f6098fcd-5534-4991-a4fb-e51cee131d81" />

## Status
UI / Overlays forwarded to unreal  
(Captures 4K/120FPS with reasonable hardware.)  
Scene Graph forwarded to unreal  
Textures / Animations  

There are some minor bugs but this is still WIP, and not ready for end users.  

## Goal
A modern Unreal-based frontend for RuneLite with real-time scene graph / camera / input sync

## Building
Setup your windows environment for Unreal 5.7.4
Build the Game configuration using Visual Studio 2022  
Cook the content using Unreal

## Running
You must run via PIE (Play in Editor - Buggy) or via packaged build, it will not run correctly inside Rider  
Using this would require a runelite plugin such as the one bundled in https://github.com/UnrealRS2/rl-gpushared-shim  
You must also have the rl-gpushared-shim .dll on your PATH (or drop it in jvm/bin dir if you are lazy)

