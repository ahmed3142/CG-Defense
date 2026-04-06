# CG Defense

A feature-rich **tower defense game** built in C++ using the [Raylib 5.0](https://www.raylib.com/) graphics library. Place towers, manage your economy, and survive 10 waves of increasingly difficult enemies to defend your castle.


---

## Features

| Category | Details |
|---|---|
| **Towers** | Basic, Sniper, Cannon — each with 3 upgrade levels |
| **Targeting modes** | Closest · Strongest · First · Last — cycle with **T** |
| **Enemies** | 5 types with distinct stats, colour tints, and health bars |
| **Pathfinding** | Flow-field BFS — enemies reroute dynamically when walls are placed |
| **Ballistic aim** | Towers lead-predict enemy movement using a quadratic solver |
| **Economy** | Buy, upgrade, sell (60 % refund) towers; income grows each round |
| **Difficulty** | Easy / Medium / Hard — affects starting credits, HP, spawn rate, rewards |
| **Level editor** | Build and save custom maps stored in `all_levels.json` |
| **Wave preview** | See the exact enemy composition of the next wave before starting |
| **Speed toggle** | Switch between 1× and 2× game speed mid-round |
| **Castle health bar** | Visual HP bar that transitions green → yellow → red |
| **Round stats** | After each round shows enemies killed and credits earned |
| **High scores** | Best round reached per level saved to `highscores.txt` |
| **Placement preview** | Ghost of the selected tower follows your cursor when placing |
| **Audio** | Background music + tower-shoot and cannon-explosion sound effects |
| **Volume control** | In-game music and sound sliders accessible from Pause and Settings |

---

## Gameplay

### Objective
Survive all **10 rounds** without your castle's HP reaching zero.  
Enemies follow the shortest path to the castle tile via a flow-field algorithm.  
You earn credits for every enemy killed — spend them on towers between rounds.

### Economy
- Starting credits scale with your chosen difficulty
- Each round grants **income** that grows round-over-round
- Selling a tower refunds **60 %** of the total amount spent on it (including upgrades)

### Win & Lose Conditions
- **Win** — all 10 rounds cleared with castle HP > 0
- **Lose** — castle HP reaches 0 (enemies that breach the castle deal damage equal to their remaining health)

---

## Towers

| Tower | Cost | Range | Damage | Fire Rate | Special |
|-------|------|-------|--------|-----------|---------|
| **Basic** | $200 | 4 tiles | 2 | 1.0 s | Targets the closest enemy by default |
| **Sniper** | $400 | 7 tiles | 3 | 2.0 s | Defaults to *Strongest* targeting; high projectile speed |
| **Cannon** | $1 000 | 3 tiles | 2 + AoE | 1.5 s | Explodes on impact, splashing all enemies within 1 tile |

### Upgrade Costs

| Tower | → Level 2 | → Level 3 |
|-------|-----------|-----------|
| Basic | $100 | $150 |
| Sniper | $1 000 | $500 |
| Cannon | $1 000 | $5 000 |

### Targeting Modes
Press **T** while a tower is selected to cycle through:

| Mode | Behaviour |
|------|-----------|
| **Closest** | Nearest enemy to the tower |
| **Strongest** | Enemy with the highest remaining HP |
| **First** | Enemy closest to the castle (arrives soonest) |
| **Last** | Enemy furthest from the castle (just spawned) |

---

## Enemies

| Enemy | Speed | HP | Reward | Colour |
|-------|-------|----|--------|--------|
| **Basic** | 1.0× | 5 | $10 | White |
| **Fast** | 2.0× | 3 | $6 | Yellow |
| **Tank** | 0.5× | 70 | $45 | Red |
| **Physics** | 1.0× | 1 | $20 | Cyan |
| **Final Boss** | 0.2× | 10 000 | $500 | Purple |

> Health bars appear above units once they take damage, cycling green → yellow → red.

---

## Difficulty Settings

| Setting | Easy | Medium | Hard |
|---------|------|--------|------|
| Starting credits | $6 200 | $5 000 | $4 200 |
| Castle HP | 520 | 400 | 300 |
| Income (round 1) | $2 400 | $2 000 | $1 600 |
| Reward multiplier | ×1.15 | ×1.00 | ×0.85 |
| Spawn speed | Slower | Normal | Faster |
| Enemy HP scaling | Reduced | Normal | Increased |

---

## Controls

| Input | Action |
|-------|--------|
| **Left Click** | Place tower / Select tower |
| **Double Left Click** | Upgrade selected tower |
| **S** | Arm sell confirmation for selected tower |
| **Right Click** *(after S)* | Confirm sell |
| **T** | Cycle targeting mode of selected tower |
| **SPACE** | Start next round |
| **P** | Pause / Resume |
| **M** | Toggle flow-field direction overlay |
| **R + Right Click** | Remove tower at cursor |
| **U + Right Click** | Upgrade tower at cursor |
| **ESC** | Quit |

---

## Getting Started

### Prerequisites

- **Windows 10 / 11**
- [w64devkit](https://github.com/skeeto/w64devkit) or another MinGW-w64 g++ toolchain at `C:/raylib/w64devkit/bin`
- [Raylib 5.0](https://github.com/raysan5/raylib/releases/tag/5.0.0) static library placed two directories above the project root (standard Raylib starter-template layout)

### Build

```bash
# Release build (from the project root directory)
mingw32-make

# Debug build
mingw32-make BUILD_MODE=DEBUG

# Clean
mingw32-make clean
```

### Run

The executable **must be launched from the project root** so that relative asset paths resolve correctly:

```bash
./game.exe
```

> Textures are loaded from `src/assets/Images/`  
> Audio files are loaded from `src/assets/Audios/`  
> Level data is read from `all_levels.json` (project root)  
> High scores are saved to `highscores.txt` (project root, auto-created)

---

## Project Structure

```
projectDemo-ProjectDefense/
├── Makefile
├── all_levels.json          # All playable level layouts
├── highscores.txt           # Auto-generated: best round reached per level
├── preview.jpg              # Screenshot used in this README
└── src/
    ├── main.cpp             # Entry point — window init, Game construction
    ├── game.h / game.cpp    # Game loop, state machine, HUD, UI drawing
    ├── level.h / level.cpp  # Tile grid, flow-field pathfinding (BFS)
    ├── tower.h / tower.cpp  # Tower logic, targeting modes, ballistic aim
    ├── unit.h  / unit.cpp   # Enemy movement, collision avoidance, health bars
    ├── projectile.h / .cpp  # Projectile physics, AoE explosion animation
    ├── timer.h / timer.cpp  # Reusable countdown / count-up timer
    ├── textureloader.h/.cpp # Cached texture loading (unordered_map)
    ├── levelEditor.h / .cpp # In-game map editor with JSON save
    ├── leveldata.h          # LevelData POD struct
    ├── leveldataio.h        # JSON serialisation via nlohmann/json
    ├── json.hpp             # nlohmann/json single-header library
    └── assets/
        ├── Images/          # PNG / BMP textures and sprite animations
        └── Audios/          # MP3 / WAV sound files
```

---

## Tech Stack

| Component | Technology |
|-----------|-----------|
| Language | C++14 |
| Graphics & windowing | [Raylib 5.0](https://www.raylib.com/) |
| Math helpers | raymath (bundled with Raylib) |
| JSON serialisation | [nlohmann/json](https://github.com/nlohmann/json) |
| Build system | GNU Make (`mingw32-make` on Windows) |
| Compiler | g++ via w64devkit / MinGW-w64 |

---

## Authors

Developed as a university project for **CSE 1202 — Structured Programming Lab**.

---

## License

This project is released for educational purposes.  
Raylib is licensed under the [zlib license](https://github.com/raysan5/raylib/blob/master/LICENSE).  
nlohmann/json is licensed under the [MIT license](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT).
