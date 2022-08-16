# Corrosion

Framework for Vulkan.

## Notes
 - Uses the dynamic rendering extension.
 - WIP OpenGL ES backend. Currently it isn't functional at all, but most of the groundwork is there.

## Build Instructions

### Prerequisites
 - A graphics processor that supports at least Vulkan 1.2 and the following extensions:
 	- VK_KHR_dynamic_rendering
 	- VK_KHR_swapchain
 - Vulkan SDK, including shaderc and SPIRV-Cross.
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
