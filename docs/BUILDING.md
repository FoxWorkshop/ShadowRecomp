# Building

## 1. Clone the Repository

Clone **ShadowRecomp** with submodules using [Git](https://git-scm.com/).
```
git clone --recurse-submodules https://github.com/FoxWorkShop/ShadowRecomp.git
```

### Windows
If you skipped the `--recurse-submodules` argument during cloning, you can run `update_submodules.bat` to ensure the submodules are pulled.

## 2. Add the Required Game Files

Copy the file `default.xbe` from the game and place it inside `./ShadowRecompLib/private/`.

This file is located in the game's root directory.

> [!TIP]
> It is recommended that you install the game using [an existing Shadow Recompiled release](https://github.com/FoxWorkshop/ShadowRecomp/releases) to acquire this file, otherwise you'll need to rely on third-party tools to extract it.

## 3. Install Dependencies

### Windows
You will need to install [Visual Studio 2026](https://visualstudio.microsoft.com/downloads/).

In the installer, you must select the following **Workloads** and **Individual components** for installation:
- Desktop development with C++
- C++ Clang Compiler for Windows
- C++ CMake tools for Windows

### Linux
The following command will install the required dependencies on a distro that uses `apt` (such as Debian-based distros).
```bash
sudo apt install autoconf automake libtool pkg-config curl cmake ninja-build clang clang-tools libgtk-3-dev
```
The following command will install the required dependencies on a distro that uses `pacman` (such as Arch-based distros).
```bash
sudo pacman -S base-devel ninja lld clang gtk3 zip ibus autoconf-archive
```
You can also find the equivalent packages for your preferred distro.

> [!NOTE]
> This list may not be comprehensive for your particular distro and you may be required to install additional packages, should an error occur during configuration.


## 4. Build the Project

### Windows
1. Open the repository directory in Visual Studio and wait for CMake generation to complete. If you don't plan to debug, switch to the `Release` configuration.

> [!TIP]
> If you need a Release-performant build and want to iterate on development without debugging, **it is highly recommended** that you use the `RelWithDebInfo` configuration for faster compile times.

2. Under **Solution Explorer**, right-click and choose **Switch to CMake Targets View**.
3. Right-click the **ShadowRecomp** project and choose **Set as Startup Item**, then choose **Add Debug Configuration**.
4. Add a `currentDir` property to the first element under `configurations` in the generated JSON and set its value to the path to your game directory (where root is the directory containing `character`, `sound`, `enemy`, etc).
5. Start **ShadowRecomp**. The initial compilation may take a while to complete due to code and shader recompilation.

### Linux
1. Configure the project using CMake by navigating to the repository and running the following command.
```bash
cmake . --preset linux-release
```

> [!NOTE]
> The available presets are `linux-debug`, `linux-relwithdebinfo` and `linux-release`.

2. Build the project using the selected configuration.
```bash
cmake --build ./out/build/linux-release --target ShadowRecomp
```

3. Navigate to the directory that was specified as the output in the previous step and run the game.
```bash
./ShadowRecomp
```

## Building with Nix

A dev shell is provided (`flake.nix`). Requirements:
- Nix with flakes enabled
- `ShadowRecompLib/private/default.xbe` present (same as the other build methods)

```bash
# Enter the dev shell (or use direnv: it loads automatically via .envrc)
nix develop

# Build / run (available: debug, relwithdebinfo, release)
build-release
run-release

# Other commands
sr-help
```

All dependencies (SDL2, freetype, GTK, Vulkan loader, ...) come from nixpkgs.
vcpkg is replaced by a stub.

The prebuilt DXC shader compiler shipped in `tools/XShaderRecomp/thirdparty/dxc-bin`
is custom-invoked with extra added library paths.

Outside of that, all else works relatively normal.

### VSCode Intellisense

Note: If you're using VSCode as your IDE:
- Install direnv & direnv extension.
- Install clangd extension
- Run `configure-release`.
  - This will generate a `compile_commands.json` file in the root of the project.
- Ctrl+Shift+P → `clangd: Restart language server`.