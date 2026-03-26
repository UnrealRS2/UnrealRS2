# URRL — Unreal x RuneLite (WIP)

URRL is a work-in-progress Unreal Engine 5.7.4 frontend for RuneLite (only gpu plugin modified)  
see: https://github.com/UnrealRS2/runelite  
It communicates through a shared RAM block to sync data in real time.  
see: bundled under ./rl-gpushared-shim

<img width="500" height="500" alt="image" src="https://github.com/user-attachments/assets/b29c2cfc-5d27-429c-86e7-f397a52039de" />
<img width="2048" height="1044" alt="image" src="https://github.com/user-attachments/assets/84463d0c-5b81-4a3c-a0cf-00d8b1741ebc" />



## Status  
Playable!  
There are some bugs but this is WIP, and not ready for end users.  

## Goal  
A modern Unreal-based frontend for RuneLite with real-time scene graph / camera / input sync  

## Building  
Ensure URRL AND Unreal Editor are closed!  
USE UNREAL BUILD TOOL, NOT AN IDE  
```
(Powershell)
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" URRLEditor Win64 Development -Project="***\UnrealRS2\URRL.uproject"
```

## Running  
You must build (or use from a release) rl-gpushared-shim.dll from this project (Clion) and place DLL on PATH  
You must build (or use from a release): https://github.com/UnrealRS2/runelite (modified GPU plugin / GPU Shared plugin)  

Once the rl-gpushared-shim DLL is on PATH, start our RuneLite  
(Disable custom window chrome, restart, then maximize runelite)  
Once at the login screen, open URRL.exe, then maximize URRL  
  
You are ready to play from Unreal!  
  
WARNING:  
Do NOT close URRL while logged in, it will thread lock RuneLite  
Do NOT disable GPU Shared plugin while logged in, it will crash RuneLite  
Do NOT do any risky shit, this is a PREVIEW  
