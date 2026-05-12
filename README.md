# Crimson Wars Native

Unreal C++ native client for the existing `crimson-wars` Node/WebSocket server.

This project is intentionally a sibling of the browser game, not a replacement for it:

- Server authority stays in `C:\Projects\crimson-wars`.
- Native rendering, animation, ragdolls, models, and equipment visuals live here.
- The first milestone is protocol parity: join a room, receive `state`, send `input`, and render a 3D proxy scene.

## Requirements

- Unreal Engine 5.x with C++ toolchain installed.
- Unreal Engine 5.7 currently lives at `C:\UE_5.7` on this machine.
- Visual Studio 2022 17.8+ Build Tools with MSVC v143 14.38+.
- .NET Framework Developer Pack 4.8+ for UnrealEd `SwarmInterface`.
- The Crimson Wars server running locally.

Unreal 5.7 will not build this C++ project with Visual Studio 2019 Build Tools. Install the VS 2022 toolchain and .NET Framework Developer Pack before running the build script:

```powershell
winget install --id Microsoft.VisualStudio.2022.BuildTools --source winget --accept-package-agreements --accept-source-agreements --override "--wait --quiet --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --add Microsoft.VisualStudio.Component.Windows11SDK.26100 --includeRecommended"
winget install --id Microsoft.DotNet.Framework.DeveloperPack_4 --source winget --accept-package-agreements --accept-source-agreements
```

Preflight:

```powershell
C:\Projects\crimson-wars-native\Scripts\Check-UnrealPrereqs.ps1
```

## Run

1. Start the web server:

   ```powershell
   cd C:\Projects\crimson-wars
   npm start
   ```

2. Open:

   ```text
   C:\Projects\crimson-wars-native\CrimsonWarsNative.uproject
   ```

3. Let Unreal rebuild C++ modules when prompted.

4. Press Play. By default the game instance connects to:

   ```text
   ws://127.0.0.1:8080/ws
   ```

The first build renders simple 3D proxy geometry for players, enemies, bullets, drops, and XP orbs. The next layers replace those proxies with skeletal meshes, animation blueprints, sockets, and ragdoll assets.

The native Battle Hub UI is already scaffolded in C++ UMG. It mirrors the browser menu structure first, then the combat renderer can grow underneath it.
On startup the game instance also fetches `http://127.0.0.1:8080/api/native/bootstrap` for menu catalogs, rooms, leaderboard preview, news preview, and native asset hints.

Command-line build after prerequisites are installed:

```powershell
C:\Projects\crimson-wars-native\Scripts\Build-Unreal.ps1
```

## Controls

- `WASD`: movement input sent to server.
- Mouse: aim point on the ground plane.
- Left mouse: shooting input.
- Space: jump/dodge input.
- Enter: join/create room again.
- Escape: leave current room.
- Tab: show/hide the native Battle Hub menu.

## UI

See `Docs/UI_PORT.md` for the current menu port. The native menu loads current web PNG/JPG artwork from `C:\Projects\crimson-wars\public\assets`, so the first Unreal pass already has the same Crimson Wars flavor without waiting for imported `.uasset` packages.

## Milestones

1. Native protocol parity.
2. Stable 3D top-down arena camera and proxy visualization.
3. Skeletal hero/enemy meshes with animation states.
4. Public visual loadout in the server state.
5. Modular equipment attached to sockets.
6. Local visual ragdolls for monster deaths.
7. Boss-specific rigs, death reactions, gore, and LOD budgets.
