# Corrosion

C graphics framework for GNU/Linux and Windows.

## Notes
 - Functional Vulkan backend that uses the dynamic rendering extension.
 - OpenGL ES backend. Some features (like compute shaders) are not supported on this backend.
 - Emscripten support coming soon.

## Build Instructions

### Prerequisites
 - A graphics processor that supports at least Vulkan 1.2 and the VK_KHR_dynamic_rendering extension.
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
or aren't implemented at all. Only Vulkan is supported on Windows for now.

#### Visual Studio
Open `corrosion.sln` in the `projects/vs2022` directory in Visual Studio 2022, right click
the solution in the Solution Explorer and click "Build Solution".

## Demos
<figure>
	<img src="media/demos/sand.png" alt="Compute Shader Sand Simulation">
	<figcaption>Compute Shader Sand Simulation</figcaption>
</figure>

<figure>
	<img src="media/demos/voxel.png" alt="Voxel Raytracer">
	<figcaption>Voxel Raytracer</figcaption>
</figure>

<figure>
	<img src="media/demos/3d.png" alt="3D Renderer">
	<figcaption>Simple 3D Renderer.</figcaption>
</figure>

<figure>
	<img src="media/demos/volume.png" alt="Volumetric Rendering">
	<figcaption>Volumetric Rendering</figcaption>
</figure>

## Note for GitHub users
This repository is hosted on Codeberg and mirrored on GitHub. Issues and pull requests should be
made on the [Codeberg repository](https://codeberg.org/quou/corrosion).
