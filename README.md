<p align="center">
    <img src="https://github.com/FoxWorkshop/ShadowRecompResources/blob/master/images/logo/logo.png?raw=true" width="1024"/>
</p>

---

Shadow Recompiled is an unofficial PC port of the original Xbox version of Shadow the Hedgehog (2005) created through the process of static recompilation.
The port supports Windows and Linux.

**This project does not include any game assets. You must provide the files from your own legally acquired copy of the game to install or build Shadow Recompiled.**

[XboxRecomp](https://github.com/FoxWorkshop/XboxRecomp) is the main recompiler used for converting the game's original X86 code into compatible C++. 
The development of that recompiler was directly inspired by [XenonRecomp](https://github.com/hedge-dev/XenonRecomp), which was used to create [Sonic Unleashed Recompiled](https://github.com/hedge-dev/UnleashedRecomp).

## Table of Contents

- [Minimum System Requirements](#minimum-system-requirements)
- [Features](#features)
- [Known Issues](#known-issues)
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

### Achievements

Achievements have been implemented to the game, they can be checked from the pause menu; a full new menu has been added.

You will be rewarded with achievements as you progress through the game. 

> [!NOTE]
Achievements cannot be used with platforms such as Steam or RetroAchievements.

## Known Issues

Before reporting any issues, check if they are listed [here](https://github.com/FoxWorkshop/ShadowRecomp/issues).

## FAQ

> [!CAUTION]
> Do not download builds of Shadow Recompiled from anywhere but our [Releases](https://github.com/FoxWorkshop/ShadowRecomp/releases) page.
>
> **We will never distribute builds on other websites, via Discord servers or via third-party update tools.**

### Can I install the game using a GameCube / PlayStation 2 version?

**You cannot use the files from the GameCube or PlayStation 2 version of the game.** Supporting these files would require an entirely new recompilation to handle all the differences.

### Why are there line breaks on some of the UI?

Shadow was designed for 480p, and unfortuntely defects in the art are exasperated at higher resolutions. This is not a bug, please do not open an issue about this. We use the original files the game ships with. You will need to use a HD GUI mod to fix these defects.

### Why are some sound effects missing

The Xbox version of the game has several missing sound effects. See [details here](https://github.com/FoxWorkshop/ShadowRecomp/issues/44) if interested. You will need to use a SFX Restoration mod to fix these defects.

### What other platforms will be supported?

This project does not plan to support any more platforms other than Windows and Linux at the moment. Any contributors who wish to support more platforms should do so through a fork.

## Building

[Check out the building instructions here](/docs/BUILDING.md).

## Credits

### Shadow Recompiled
- [Sora](https://github.com/Sora-yx): Creator and Lead Developer of the recompilation, as well as the developer of technologies created for it such as [XboxRecomp](https://github.com/FoxWorkshop/XboxRecomp) and [XShaderRecomp](https://github.com/FoxWorkshop/XShaderRecomp).

- [dreamsyntax](https://github.com/dreamsyntax): Second Developer, source for Shadow knowledge, ensuring port accuracy, and identifying flaws with the Xbox port of the game.

- [Sewer56](https://github.com/Sewer56): Third Developer, who joined relatively late, implemented better widescreen, significantly improved performance with shaders and added NixOS support for Linux.

- [TheHatedGravity](https://github.com/TheHatedGravity): Installer design, UI polish + eagle eye for finding flaws in our visuals

- [DonutStopGaming](https://github.com/DonutStopGaming): Help with understanding RenderWare and finding Shadow functions.

- [Deep-Sea Berry](https://github.com/Deep-SeaBerry): Artist behind the project logo.


TODO: Add translation credits too


### Tester Thanks
- Rawlins
- DarkShadow17
- The Spectral Star
- [Jesus_PK](https://github.com/Jesus-PK)
- Pixel
- [ultra0000](https://github.com/ultra0000)
- [Deep-Sea Berry](https://github.com/Deep-SeaBerry)
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
