

<p align="center">
    <img src="https://i.imgur.com/FUdbWMA.png" width="256"/>
</p>
<h1 align="center">Shadow Recompiled</h1>

> [!CAUTION]
> This recompilation is still under active development, expect bugs and crashes.

Shadow Recompiled is an unofficial PC port of the original Xbox version of Shadow The Hedgehog (2005) created through the process of static recompilation.
The port supports Windows and Linux.

**This project does not include any game assets. You must provide the files from your own legally acquired copy of the game to install or build Shadow Recompiled.**

[XboxRecomp](https://github.com/FoxWorkshop/XboxRecomp) is the main recompiler used for converting the game's original X86 code into compatible C++. 
The development of that recompiler was directly inspired by [XenonRecomp](https://github.com/hedge-dev/XenonRecomp), which was used to create [Sonic Unleashed Recompiled](https://github.com/hedge-dev/UnleashedRecomp).

## Table of Contents

- [Known Issues](#known-issues)
- [FAQ](#faq)
- [Building](#building)
- [Credits](#credits)

## Known Issues

Before reporting any issues, check if they are listed [here](https://github.com/FoxWorkshop/ShadowRecomp/issues).

## FAQ

> [!CAUTION]
> Do not download builds of Shadow Recompiled from anywhere but our [Releases](https://github.com/FoxWorkshop/ShadowRecomp/releases) page.
>
> **We will never distribute builds on other websites, via Discord servers or via third-party update tools.**

### Can I install the game using a GameCube / PlayStation 2 version?

**You cannot use the files from the GameCube or PlayStation 2 version of the game.** Supporting these files would require an entirely new recompilation to handle all the differences.

### What other platforms will be supported?

This project does not plan to support any more platforms other than Windows and Linux at the moment. Any contributors who wish to support more platforms should do so through a fork.

## Building

[Check out the building instructions here](/docs/BUILDING.md).

## Credits

### Shadow Recompiled
- [Sora](https://github.com/Sora-yx): Creator and Lead Developer of the recompilation, as well as the developer of technologies created for it such as [XboxRecomp](https://github.com/FoxWorkshop/XboxRecomp) and [XShaderRecomp](https://github.com/FoxWorkshop/XShaderRecomp).

- [dreamsyntax](https://github.com/dreamsyntax): Second Developer, added linux and keyboard compatibility, provided multiplayer input and fixed various bugs. Also helped with Shadow knowledge and author of [Shadow Reloaded](https://github.com/ShadowTheHedgehogHacking/ShadowReloaded).

- [DonutStopGaming](https://github.com/DonutStopGaming): Help with understanding RenderWare and finding Shadow functions.

- [Sewer56](https://github.com/Sewer56): Tips to handle x86 recompilation including the idea of using [Zydis](https://github.com/zyantific/zydis) for XboxRecomp.

### Special Thanks

- CXBX-Reloaded Development Team: This project would not have been possible without the [CXBX emulator](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded-legacy). Nearly every aspect of the development was guided by referencing CXBX's research on the original Xbox, especially the graphics part.

- [extract-xiso](https://github.com/XboxDev/extract-xiso): extract-xiso devs (in@fishtank.com and other extract-xiso contributors) for core extraction in the installer.

- Unleashed Recompiled Development Team: Created much of the ground work that made all of this possible and sped up development time considerably.

- [Skyth](https://github.com/blueskythlikesclouds): Provided help to understand how Recompilation works, also the author of the original graphics pipeline code which was (mostly) reused here.

- [Darío](https://github.com/DarioSamo): Creator of the graphics hardware abstraction layer [plume](https://github.com/renderbag/plume), used by the project's graphics backend.

- MattKC: Useful explanation of Xbox's Push Buffers system on [his blog with Reburn3.](https://forum.mattkc.com/viewtopic.php?t=154)

- [ocornut](https://github.com/ocornut): Creator of Dear ImGui, which is used as the backbone of the custom menus.
