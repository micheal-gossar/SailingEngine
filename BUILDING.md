# Building SailingEngine

## Requirements

- CMake 3.25 or newer
- Ninja
- A C++20 compiler
- Vulkan SDK
- Slang (`slangc`), available through `SLANG_ROOT`, `VULKAN_SDK`, or `PATH`

The repository includes GLFW, GLM, Dear ImGui, and ImPlot under `third_party`.
Dear ImGui tracks its `docking` branch.

## Presets

Configure and build directly with CMake:

```powershell
cmake --preset debug
cmake --build --preset debug
```

Available configure and build presets are `debug` and `release`.
Build outputs are written to `build/<preset>/bin`. Compiled shaders are deployed
to `assets/shaders` below that executable directory.

## PowerShell build driver

On Windows, the build driver locates a native CMake installation, selects the
matching preset, configures the project, and builds it:

```powershell
.\scripts\build.ps1 -Configuration Debug
```

Useful options:

```powershell
# Start from a clean build directory.
.\scripts\build.ps1 -Configuration Debug -Clean

# Configure without building.
.\scripts\build.ps1 -Configuration Release -ConfigureOnly

# Build one target with strict first-party warnings.
.\scripts\build.ps1 -Target oblong_sailing_game -WarningsAsErrors

# Enable AddressSanitizer and UndefinedBehaviorSanitizer where supported.
.\scripts\build.ps1 -Configuration Debug -Sanitizers

# Show complete compiler and linker commands.
.\scripts\build.ps1 -VerboseBuild
```

The `release` preset enables interprocedural optimization.

## Runtime logging

The application writes timestamped logs to `logFile` below the executable's
working directory. Terminal output uses severity colors, while log files remain
plain text. Debug messages are emitted only by Debug builds.

Subsystems create hierarchical category streams from the application logger:

```cpp
const LogStream rendering = logger.CreateStream("Rendering");
const LogStream swapChain = rendering.Child("Swap Chain");

swapChain.Debug("Creating swap chain");
swapChain.Info("VSync enabled");
swapChain.Warning("Falling back to FIFO");
swapChain.Error("Presentation failed");
```

Each category appears in an ASCII tree column, including its complete parent
path. Creating an existing stream path is safe and returns another handle to
the same category.

## Shader compilation

CMake tracks `assets/shaders/shader.slang` and rebuilds `slang.spv` when the
source or shader script changes. Windows builds invoke:

```powershell
.\scripts\shaderCompile.bat [source_file] [output_file]
```

The shader compiler produces one SPIR-V module containing the `vertMain` and
`fragMain` entry points. A missing compiler, compilation failure, or missing
output file fails the build.
