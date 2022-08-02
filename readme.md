# Corrosion

Framework for Vulkan.

## Build Instructions

### Prerequisites
 - A graphics processor that supports at least Vulkan 1.0.
 - Vulkan SDK, including `glslc` working and added to path.
 - Vulkan validation layers (optional but highly recommended).
 - On GNU/Linux:
	 - X11 (Wayland might be supported in the future as well, but don't count on it).

### GNU/Linux
From the `projects/gmake` directory, run:

```
make config=debug run_sbox
```

### Windows
The Windows platform layer is still work in progress, so some things don't work properly
or aren't implemented at all.

#### Visual Studio
Open `corrosion.sln` in the `projects/vs2022` directory in Visual Studio 2022, right click
the solution in the Solution Explorer and click "Build Solution".

## Note for GitHub users
This repository is hosted on Codeberg and mirrored on GitHub. Issues and pull requests should be
made on the [Codeberg repository](https://codeberg.org/quou/corrosion).
