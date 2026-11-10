# Shrek Kart (PS Vita) — Release v1.0

First public release of the native iOS **Mach-O loader** port of *Shrek Kart* (Gameloft) for the PlayStation Vita. On real hardware the game boots through the menus into races, which run at a stable 25 fps (the game's own cap), with touch input and analog steering.

---

## 📦 Release Package Contents

1. **`shrekkart.vpk`** (v01.00): PlayStation Vita application package with the Mach-O loader, Objective-C runtime, vitaGL renderer, OpenAL audio layer and physical input handling.
2. **`rebase.bin`**: **21,977** rebase words for the 1.1.0 executable (offsets only, no game code).
3. **`binds.bin`**: **855** import slots mapping the executable's Apple framework imports to the loader (symbol names only, no game code).
4. **`controls.txt`**: default button-to-touch mapping.

---

## 🛠️ What are rebase.bin and binds.bin?

`ShrekKarting` is a non-PIE armv6 executable linked at a fixed address without relocations. The loader maps it at `0x98000000`; `rebase.bin` lists the absolute 32-bit pointers in its code and data that must be moved to the new base, and `binds.bin` lists the import slots to fill with native implementations. Both are generated from your own executable with:

```bash
python3 -I tools/ios_prep.py game_app/ShrekKarting ux0_data/shrekkart   # run by tools/stage_data.sh
```

Layout: `rebase.bin` = `'RBS1'`, count, count × vmaddr. `binds.bin` = `'BND1'`, count, {u32 vmaddr, u32 addend, u8 lazy, char name[]}. They only match the **1.1.0** executable.

---

## 🚀 Player Installation Guide

1. **PS Vita requirements**:
   - Custom Firmware (**Enso 3.60 / 3.65** recommended).
   - `kubridge.skprx` under `*KERNEL` in `ur0:tai/config.txt`.
   - `libshacccg.suprx` in `ur0:data/` (obtainable via **ShaRKBR33D**).

2. **Game assets**:
   - Extract `Payload/ShrekKarting.app/` from your decrypted iOS **1.1.0** `.ipa` (FairPlay `cryptid 0`).
   - Copy its contents to `ux0:data/shrekkart/game_app/`, and the three release files next to it.
   - Layout:
     ```text
     ux0:data/shrekkart/
     ├── game_app/      (contents of ShrekKarting.app: ShrekKarting, *.bar, *.adp, Info.plist, ...)
     ├── rebase.bin     (from this release)
     ├── binds.bin      (from this release)
     ├── controls.txt   (from this release)
     ├── Documents/     (created on first launch, save games)
     ├── Library/       (created on first launch, settings)
     └── logs/          (created on first launch, one log per run)
     ```

3. **Install the VPK**: transfer and install `shrekkart.vpk` with VitaShell.

---

## 🎮 Controls

| PS Vita Input | Action |
| :--- | :--- |
| **Front Touchscreen** | iPhone touch screen (menus, HUD) |
| **Left Stick** | Steering (sent as phone tilt) |
| **Cross (✖)** | Taps bottom-right (430, 270) |
| **Circle (●)** | Taps bottom-left (50, 270) |
| **Square (◼)** | Taps top-right (430, 50) |
| **Start** | Taps center (240, 160) |
| **Select + Start (hold 2 s)** | Cycle picture rotation |

Touch points can be edited in `ux0:data/shrekkart/controls.txt` (`BUTTON X Y`, 480×320 landscape coordinates).

---

## 📋 Changelog (v1.0)

- **Loader**: armv6 Mach-O runtime with a new shared ObjC runtime → **21,977** rebase words, **855** import slots, 0 unimplemented, 7 global constructors.
- **Paths**: the game builds paths from `getcwd` (`ux0:/data/.../ShrekKarting.app/`), which bypassed the bundle mapping (black screen) → `.app/` is mapped to `game_app/` first.
- **Rebase**: `sizeof(inflate_state)` (0x2530) in `inflateInit2_` was rebased as a pointer (`operator new(2550146352)`) → restored in `patch.c`.
- **Rebase**: zlib `short` tables in `__TEXT,__const` were rebased (endless `Z_DATA_ERROR` in `LZMAFile::readInt`) → `numeric_table` rule in `ios_prep.py`.
- **Rebase**: Musepack Huffman/quantization tables in `__TEXT,__const` were rebased (music played as noise) → only pointers to cstrings are rebased there.
- **CPU**: VFPv2 short vectors (ARMv6) in 9 `CMatrix` routines raised Undefined Instruction on the Cortex-A9 → rewritten to NEON at load time.
- **Graphics**: CPU emulation of `GL_OES_matrix_palette` took 47–108 ms per frame (8–17 fps) → skins only the used vertex range and converts attributes once per draw: 3–5 ms, 25 fps.
- **Graphics**: the Gameloft logo screen was black → the game's clear color is restored after each present.
- **Audio**: choppy sound → openal-soft mixer thread on its own core (priority 64).

---

## ⚠️ Known Issues

- **Music**: the Musepack table fix is in this release's `rebase.bin` but has not been checked on console yet.
- **Button mapping**: the default `controls.txt` touch points are not calibrated for the in-game buttons yet; use the touchscreen.
- **Short stalls in menus** while `.bar` resources load.
- **Saves**: persistence across restarts has not been verified yet.
- **Online features disabled**: Game Center, WLAN multiplayer and the iPod music picker are bypassed.
- **Trophies**: not implemented.

Installation and build instructions: see [README.md](README.md).
