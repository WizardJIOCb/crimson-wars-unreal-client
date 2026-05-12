# Crimson Wars Native Protocol Notes

The Node server is authoritative. The native client mirrors the browser client:

- Connect to `ws://host/ws`.
- Send JSON messages.
- Receive JSON messages.
- Treat server `state` as the source of truth.
- Send local intent as `input`.

## Join

Minimal create-or-join payload:

```json
{
  "type": "join",
  "roomCode": "",
  "name": "Native",
  "playerClass": "cyber"
}
```

An empty `roomCode` creates a room. A non-empty room code joins that room if it is hosted on the connected server instance.

## Input

Sent at roughly 30 Hz:

```json
{
  "type": "input",
  "seq": 1,
  "moveX": 0,
  "moveY": -1,
  "aimX": 1200,
  "aimY": 900,
  "shooting": true,
  "jump": false
}
```

Browser/world coordinate convention:

- `x`: horizontal world axis.
- `y`: vertical top-down world axis.
- Native Unreal mapping: `x -> X`, `y -> Y`, height -> `Z`.

## Welcome

The server responds with:

```json
{
  "type": "welcome",
  "id": "player-id",
  "roomCode": "ABCD",
  "sync": {
    "tickRate": 45,
    "stateSendHz": 30,
    "inputSendHz": 30
  }
}
```

## State

Realtime updates arrive as:

```json
{
  "type": "state",
  "payload": {
    "roomCode": "ABCD",
    "world": { "width": 5400, "height": 3600 },
    "players": [],
    "enemies": [],
    "bullets": []
  }
}
```

The live server uses compact arrays for some realtime collections.

### Compact enemy tuple

```text
[id, type, x, y, hp, maxHp, radius, faceLeft, mobId, behavior, color, spriteScale, name, explosionRadius]
```

### Compact bullet tuple

```text
[id, ownerId, ownerPlayerId, weaponKey, x, y, vx, vy, color, kind, radius, fromEnemy, shooterType, explosionRadius]
```

### Compact drop tuple

```text
[id, x, y, kind, weaponKey, ttlMs]
```

### Compact XP orb tuple

```text
[id, x, y, ttlMs]
```

### Compact skill orb tuple

```text
[id, ownerId, skillId, x, y, ttlMs]
```

## Public Visual Loadout

The server now exposes a stable public field on each player for native 3D equipment rendering:

```json
{
  "visualLoadout": {
    "heroId": "cyber",
    "bodyMesh": "heroes/cyber/body",
    "skin": "default",
    "slots": {
      "head": "items/tactical_helmet",
      "armor": "items/nano_mail",
      "legs": "items/grav_boots",
      "left_hand": "weapons/smg"
    }
  }
}
```

The native client can map those ids to Unreal skeletal/static mesh assets and socket attachments.
