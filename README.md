<p align="center">
    <img src="https://github.com/FoxWorkshop/ShadowRecompResources/blob/master/images/logo/logo.png?raw=true" width="1024"/>
</p>

---

Shadow Recompiled is an unofficial PC port of the original Xbox version of Shadow the Hedgehog (2005) created through the process of static recompilation.
The port supports Windows and Linux.

**This project does not include any game assets. You must provide the files from your own legally acquired copy of the game to install or build Shadow Recompiled.**

[XboxRecomp2] (previously [XboxRecomp](https://github.com/FoxWorkshop/XboxRecomp)) is the main recompiler used for converting the game's original x86 code into compatible C++. 
The development of that recompiler was directly inspired by [XenonRecomp](https://github.com/hedge-dev/XenonRecomp), which was used to create [Sonic Unleashed Recompiled](https://github.com/hedge-dev/UnleashedRecomp).

## Table of Contents

- [Minimum System Requirements](#minimum-system-requirements)
- [Features](#features)
- [Known Issues](#known-issues)
- [Controls](#controls)
- [FAQ](#faq)
- [Building](#building)
- [Credits](#credits)


## Minimum System Requirements

- CPU with support for the AVX instruction set:
    - Intel: Sandy Bridge (Intel Core 2nd Generation)
    - AMD: Bulldozer (AMD FX series)
- GPU with support for Direct3D 12.0 (Shader Model 6) or Vulkan 1.2:
    - NVIDIA: GeForce GT 630 (Kepler)
    - AMD: Radeon HD 7750 (2012, not the RX 7000)
    - Intel: HD Graphics 510 (Skylake)
- Memory:
    - 8 GB minimum
- Operating System:
    - Windows 10 (version 1909)
    - A modern Linux distro (or one with flatpak support)
- Storage:
    - 2.4 GiB required


## Features

### Easy to Use Installer

A built-in installer will guide you through the game installation process, performing many integrity checks to ensure the process goes as smoothly as possible.

### Options Menu

A new options menu for PC has been added, replacing the game's original settings. 
Get access to many quality of life features and graphics options directly in-game with navigation that is fully compatible with a controller.

### Widescreen and Ultrawide Support

Aspect ratios for 16:9 and 16:10, as well as ultrawide displays (such as 21:9 or even wider) are supported with options to adjust the alignment of the UI.

### High Frame Rate Support

Support for high frame rate has been added as an available option. Many glitches that normally occur at higher frame rates have been fixed and are included as part of the recompilation.
Cutscenes have also an option to play at unlocked frame rate.

### Enhancements

Extra care and attention was taken to make the experience the best as possible:
- Bloom has been overhauled to fixes many issues such as not covering the whole screen, bottom artifacts, and extra dark band / box blending on the screen.
- Sound effect limits are expanded to fix sound cutting off in areas with many sound effects.
- Edge artifacts that are in every version of the game has been completely fixed.
- Shadow's shoes have been patched to restore the use of 4 Jet Effects instead of 3.
- Cutscenes can always be skipped even if you never watched them before.
- Player 2 can now properly control the Partner on the screen like on the others versions of the game.
- Animated textures which normally have a chance to not work in every version of the game have been fixed.
- Space Gadget's gravity system has been updated to fix wrong rotation issues, fixing the infamous inverted Spin Dash, slide, and landing flip bugs.
- Many audio issues exclusive to the Xbox port have been fixed (distance, loop etc.).
- Various minor graphics glitches like transparency issues and the Xbox exclusive 'white flash' on object load-in have been fixed.

other fixes:
- Drop shadows vanishing during laser lock-on (e.g. 4SHOT, 8SHOT, Satellite Laser).
- AirSaucer Effect Speed Multiplying by Number of Saucers
-  Rails applying velocity on detach incorrectly
-  Fixed Black Hawk softlock at end of some paths (e.g. Cryptic Castle)
-  Black Tank invincible while waiting for the player
-  Ball Switches ignoring melee hits

### Achievements

Achievements have been implemented to the game, they can be checked from the pause menu; a full new menu has been added.

You will be rewarded with achievements as you progress through the game. 

> [!NOTE]
Achievements cannot be used with platforms such as Steam or RetroAchievements.

### Mod Support

The game includes a mod loader compatible with [SA Mod Manager](https://github.com/X-Hax/SA-Mod-Manager).

The mod format uses the same standard as the one used by Sonic Adventure mods. 

> [!NOTE]
Code modding is currently not possible and is planned for a future update. As a workaround for the time being, some codes have been directly embedded into the game and can be accessed through SA Mod Manager's Codes tab.


## Known Issues

Before reporting any issues, check if they are listed [here](https://github.com/FoxWorkshop/ShadowRecomp/issues).

## Controls

Default keyboard controls:

- Loosely based on Dolphin Emulator's default keyboard layout
    - Adapted to the Xbox controller
- Remap them in the in-game options, or in `config.toml`

![Keymap](docs/assets/controls-xbox.webp)

| Hotkey    | Action                          |
| --------- | ------------------------------- |
| Alt+Enter | Toggle fullscreen/windowed mode |
| Ctrl+1    | Keyboard controls port 1        |
| Ctrl+2    | Keyboard controls port 2        |
| Ctrl+3    | Keyboard controls port 3        |
| Ctrl+4    | Keyboard controls port 4        |
| F1        | Toggle profiler                 |
| F8        | Toggle HUD                      |

## FAQ

### Do you have a website?

Shadow Recompiled does not have an official website.

**Please link here when directing anyone to the project.**

> [!CAUTION]
> Do not download builds of Shadow Recompiled from anywhere but our [Releases](https://github.com/FoxWorkshop/ShadowRecomp/releases/latest) page.
>
> **We will never distribute builds on other websites, via Discord servers or via third-party update tools.**

### Can I install the game using a GameCube / PlayStation 2 version?

**You cannot use the files from the GameCube or PlayStation 2 version of the game.** Supporting these files would require an entirely new recompilation to handle all the differences.

### Where are save data & configs?

- Windows: `%APPDATA%\ShadowRecomp\`
- Linux: `~/.config/ShadowRecomp/`

Inside, saves are in `save/` and settings in `config.toml`.

You can also create `portable.txt` beside the `.exe` to keep everything in
your game folder.

### How do I update?

Copy the files from a new [release][all releases] over your existing install.

- Saves and settings are kept
- Reinstall not needed afterwards. Just boot the game

### How do I run the installer again?

Launch the game with `--install`.
In a console/terminal window.

You'll only need this if you changed the game files; and
want to revert to a stock setup.

### Why does some of the UI have visible lines or gaps?

This is a vanilla game issue.
Higher resolutions unfortunately make this issue easier to see.

- Please don't open an issue for it.
- To fix it, use HD GUI mod(s).

### Why are some sound effects missing

The Xbox version of the game has several missing sound effects.
See [details here][issue 44] if interested.

You will need to use a SFX Restoration mod to fix these defects.

### What other platforms will be supported?

We only have Windows + Linux machines.  
We'd happily accept community contribution for macOS.  

But folks who want to support platforms that require more significant modifications 
(e.g. touchscreen on mobile); should make a fork.

## Known Issues

Before reporting any issues, check if they are listed [here][issues].

## Building

[Check out the building instructions here][BUILDING.md].

## Credits

### Shadow Recompiled

- [Sora](https://github.com/Sora-yx): Creator and Lead Developer of the recompilation, as well as the developer of technologies created for it such as [XboxRecomp](https://github.com/FoxWorkshop/XboxRecomp) (before the XboxRecomp2 switch) and [XShaderRecomp](https://github.com/FoxWorkshop/XShaderRecomp). Provided French localization for the custom menus. Final design decisions and UI for installer, achievements, and other custom menus.

- [dreamsyntax](https://github.com/dreamsyntax): Second Developer, source for Shadow knowledge, ensuring port accuracy, and identifying flaws with the Xbox port of the game. Provided English localization for the custom menus.

- [Sewer56](https://github.com/Sewer56): Third Developer, who joined relatively late, added NixOS support for Linux, Discord RPC, implemented better widescreen, and significantly improved performance with shaders and [XboxRecomp2].

- [TheHatedGravity](https://github.com/TheHatedGravity): First installer design, UI polish, spotting visual flaws

- [DonutStopGaming](https://github.com/DonutStopGaming): Help with understanding RenderWare and finding Shadow functions.

- [Jesus_PK](https://github.com/Jesus-PK): Provided Spanish localization for the custom menus, also helped testing and finding bugs.

- [Deep-Sea Berry](https://github.com/Deep-SeaBerry): Artist behind the project logo, along with custom options icons. Provided German localization for the custom menus and helped testing and finding bugs.

- [Emasplatter](https://twitter.com/emasplatter): Provided Italian localization for the custom menus.

- [Windii](https://www.youtube.com/@WindiiGitlord/videos): Provided Japanese localization for the custom menus.

- [Kondrashov's Lair](https://www.youtube.com/channel/UCUU-r-7kVY3ig-GY7KTFvlw): Produced the installer music (original prod. by Sins Of A Divine Mother).

### Tester Thanks
- [Rawlins]
- [DarkShadow17]
- [The Spectral Star]
- [Pixel The Retro]
- [ultra0000]
- Many others who helped briefly test

### Special Thanks

- CXBX-Reloaded Development Team: This project would not have been possible without the [CXBX emulator](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded-legacy). Nearly every aspect of the development was guided by referencing CXBX's research on the original Xbox, especially the graphics part.

- [extract-xiso](https://github.com/XboxDev/extract-xiso): extract-xiso devs (in@fishtank.com and other extract-xiso contributors) for core extraction in the installer.

- Unleashed Recompiled Development Team: Created much of the ground work that made all of this possible and sped up development time considerably.

- [Marathon Recompiled](https://github.com/sonicnext-dev/MarathonRecomp): Used as a reference for various features and help with debugging.

- [Skyth](https://github.com/blueskythlikesclouds): Provided help to understand how Recompilation works, also the author of the original graphics pipeline code which was (mostly) reused here.

- [Darío](https://github.com/DarioSamo): Creator of the graphics hardware abstraction layer [plume](https://github.com/renderbag/plume), used by the project's graphics backend.

- [MattKC](https://github.com/itsmattkc): Useful explanation of Xbox's Push Buffers system on [his blog with Reburn3.](https://forum.mattkc.com/viewtopic.php?t=154)

- [ocornut](https://github.com/ocornut): Creator of Dear ImGui, which is used as the backbone of the custom menus.


[issues]: https://github.com/FoxWorkshop/ShadowRecomp/issues
[all releases]: https://github.com/FoxWorkshop/ShadowRecomp/releases
[issue 44]: https://github.com/FoxWorkshop/ShadowRecomp/issues/44
[BUILDING.md]: /docs/BUILDING.md
[XboxRecomp2]: https://github.com/Sewer56/XboxRecomp2
[ultra0000]: https://github.com/ultra0000
[Rawlins]: https://www.youtube.com/channel/UCjbUqRzVay-6HjoJjODre5w
[DarkShadow17]: https://www.youtube.com/@DarkShadowtH
[The Spectral Star]: https://www.youtube.com/@the-spectral-star5086
[Marathon Recompiled]: https://github.com/sonicnext-dev/MarathonRecomp
[Pixel The Retro]: https://x.com/PixelTheRetro
[Unleashed Recompiled]: https://github.com/hedge-dev/UnleashedRecomp
