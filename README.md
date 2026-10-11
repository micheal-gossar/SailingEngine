# Vulkan device test

A minimal C++20 project that opens a GLFW window, creates a Vulkan instance
and surface, selects a graphics/presentation-capable physical device, and
opens a Vulkan logical device. It does not create a swapchain or render.

GLFW 3.4 and GLM 1.0.1 are checked out physically under `third_party/`.
Source files placed under `src/` and headers placed under `header/` are
discovered automatically when CMake configures the project.

## Requirements

- Vulkan SDK
- CMake 3.20 or newer
- Ninja
- A C++20 compiler
- A Vulkan-capable GPU and driver

## Build

From PowerShell:

```powershell
.\build.ps1
```

The script selects native Windows CMake and the GCC/G++ executables available
on `PATH`, then configures Ninja and builds the project.

## VS Code

The workspace recommends the C/C++ and CMake Tools extensions. Run
`.\build.ps1` once to generate `build/compile_commands.json`; IntelliSense
uses that compilation database for the same include paths, definitions, and
compiler flags as the real build.

- **Build:** press `Ctrl+Shift+B`.
- **Run:** select **Tasks: Run Test Task**.
- **Debug:** open the Run and Debug view, select **Debug Vulkan Device Test**,
  and press `F5`.

## Run

```powershell
.\build\vulkan_device_test.exe
```

The selected GPU is printed to the terminal. Press Escape or close the window
to exit.

For a non-interactive initialization check that exits immediately:

```powershell
.\build\vulkan_device_test.exe --smoke-test
```
