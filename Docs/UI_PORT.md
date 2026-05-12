# Native UI Port

The native client now starts with a full-screen Unreal UI shell instead of dropping directly into the proxy arena.

## Implemented Panels

- `Забег`: nickname, room code, create/join buttons, mode cards, active-room placeholder.
- `Сюжет`: campaign operation list placeholder.
- `Персонажи`: hero roster using current web portraits and loadout slots.
- `Навыки`: talent-node layout placeholder.
- `Профиль`: auth/profile/history placeholder.
- `Рейтинг`: leaderboard category placeholder.
- `Новости`: news/devlog cards using current web key art.
- `Настройки`: graphics/audio/HUD/network/assets rows.

The run HUD also exists under the menu and updates from live `state`: room, map, HP, weapon, entity counts, XP/drop counts.

The menu now listens to the native bootstrap payload from `/api/native/bootstrap` and swaps placeholders for live catalogs when the server is reachable: rooms, maps, campaigns, heroes, equipment items, leaderboard rows, and news cards.

## Source Files

- `UCWNativeMenuWidget`: main Battle Hub UI.
- `UCWNativeRunHudWidget`: in-run HUD shell.
- `FCWNativeAssetLibrary`: runtime image loading from the web project's `public/assets` folder.
- `ACWNativePlayerPawn`: creates both widgets and binds `Tab` to menu toggle.

## Assets

Runtime root is now a local mirror inside the native project:

```text
C:/Projects/crimson-wars-native/Content/CrimsonWars/RawAssets
```

Configured in:

```text
Config/DefaultGame.ini
```

The folder mirrors the web game's `public/assets` tree so bootstrap paths like `/assets/items/ruby_ring.webp` still resolve in the native menu.

Current UI uses PNG/JPG assets directly:

- `other/landing-image.jpg`
- `other/heroes-v2.jpg`
- `other/crimson wars logo.png`
- `backgrounds/start-1.png`
- `characters/*.jpg`
- `items/*.png`

The web `.webp` files are kept for parity, and PNG siblings are generated beside them for Unreal runtime loading. `FCWNativeAssetLibrary` falls back from `.webp` to `.png` automatically.

To refresh the mirror after web asset changes:

```powershell
.\Scripts\Sync-WebAssets.ps1
```

Current animation layer:

- slow landing-background drift and scale pulse;
- blood overlay pulse;
- vertical scanline sweep;
- tab pulse on the active section;
- panel reveal on menu open and tab switch;
- ember particles drifting over the menu;
- subtle logo pulse/tilt;
- content sweep over the active panel.

## Next UI Pass

- Add authenticated profile/progression calls on top of the public bootstrap payload.
- Build UMG materials for scanlines, hit flashes, blood veining, and button hover sparks.
- Convert high-value web CSS effects into Unreal materials instead of flat color overlays.
- Add controller/gamepad focus paths.
- Add localization keys rather than hard-coded Russian labels.
