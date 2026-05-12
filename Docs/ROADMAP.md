# Native Client Roadmap

## Phase 1: Protocol Parity

- Open WebSocket connection to the Node server.
- Join/create a room.
- Parse `welcome`, `state`, `system`, and `joinError`.
- Send `input` at the server input rate.
- Render simple 3D proxies for players, enemies, bullets, pickups, and XP.

Current scaffold covers this phase at code level. It still needs a local Unreal compile pass.

## Phase 2: Real 3D View

- Add a top-down/angled combat camera with zoom settings.
- Add ground plane material and map bounds.
- Convert map objects to simple 3D blockers.
- Add interpolation buffers so movement feels smooth at 30 Hz state updates.
- Add basic VFX for shooting, hits, pickups, shields, portals, and boss warnings.

## Phase 3: Characters And Monsters

- Use one base humanoid skeleton for heroes.
- Use separate monster skeletons for infected archetypes and bosses.
- Add animation blueprints for idle, locomotion, attack, hit react, death transition.
- Map `playerClass`, `mobId`, `behavior`, and `weaponKey` to animation sets.

## Phase 4: Equipment

- Use the server `visualLoadout` field now included in player state.
- Define socket names: `head`, `chest`, `back`, `left_hand`, `right_hand`, `hips`, `legs`, `feet`.
- Attach armor/weapons as mesh components.
- Later optimize modular characters via merged skeletal meshes for crowded scenes.

## Phase 5: Ragdoll

- Keep ragdoll local and visual first.
- Trigger ragdoll when an enemy disappears, dies, explodes, or receives a fatal hit event.
- Apply impulse from bullet velocity or player-to-enemy vector.
- Limit active ragdolls by distance, screen visibility, and count.
- Freeze or dissolve old ragdolls into static corpse meshes/decals.

## Phase 6: Boss Presentation

- Boss-specific rigs and silhouettes.
- Bigger physical assets and tuned constraints.
- Special hit reactions per boss.
- Death sequences that blend animation into ragdoll/destruction.

## Phase 7: Full Client Parity

- Native login/profile.
- Character selection and hero talents.
- Inventory and equipment UI.
- Room list, leaderboard, news, records, replay viewing.
- Settings, localization, audio, controller support.
