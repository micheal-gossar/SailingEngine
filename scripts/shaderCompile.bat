@echo off
setlocal

rem Compiles the Slang shaders into a single SPIR-V module that contains every
rem entry point. Entry points keep their source names so the renderer can select
rem them by name when creating pipeline stages.
rem
rem Usage: shaderCompile.bat [source_file] [output_file]

set "REPO_ROOT=%~dp0.."

set "SHADER_SOURCE=%~1"
if "%SHADER_SOURCE%"=="" set "SHADER_SOURCE=%REPO_ROOT%\assets\shaders\shader.slang"

set "OUTPUT_FILE=%~2"
if "%OUTPUT_FILE%"=="" set "OUTPUT_FILE=%REPO_ROOT%\build\shaders\slang.spv"

for %%I in ("%OUTPUT_FILE%") do set "OUTPUT_DIR=%%~dpI"

rem An externally supplied SLANGC wins, so the build system stays authoritative.
if defined SLANGC if not exist "%SLANGC%" set "SLANGC="
if not defined SLANGC if defined SLANG_ROOT if exist "%SLANG_ROOT%\bin\slangc.exe" set "SLANGC=%SLANG_ROOT%\bin\slangc.exe"
if not defined SLANGC if defined VULKAN_SDK if exist "%VULKAN_SDK%\Bin\slangc.exe" set "SLANGC=%VULKAN_SDK%\Bin\slangc.exe"
if not defined SLANGC for %%I in (slangc.exe) do if not "%%~$PATH:I"=="" set "SLANGC=%%~$PATH:I"

if not defined SLANGC (
    echo [shaderCompile] ERROR: slangc.exe not found.
    echo [shaderCompile] Set SLANG_ROOT or VULKAN_SDK, or add slangc.exe to PATH.
    exit /b 1
)

if not exist "%SHADER_SOURCE%" (
    echo [shaderCompile] ERROR: shader source not found: "%SHADER_SOURCE%"
    exit /b 1
)

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
if not "%errorlevel%"=="0" (
    echo [shaderCompile] ERROR: could not create output directory: "%OUTPUT_DIR%"
    exit /b 1
)

echo [shaderCompile] Compiler: "%SLANGC%"
echo [shaderCompile] Source:   "%SHADER_SOURCE%"

"%SLANGC%" "%SHADER_SOURCE%" ^
    -target spirv ^
    -profile spirv_1_4 ^
    -emit-spirv-directly ^
    -fvk-use-entrypoint-name ^
    -entry vertMain ^
    -entry fragMain ^
    -o "%OUTPUT_FILE%"

rem slangc reports failure with a negative exit code, which "if errorlevel 1"
rem would not catch, so compare against zero explicitly.
set "SLANGC_EXIT=%errorlevel%"
if not "%SLANGC_EXIT%"=="0" (
    echo [shaderCompile] ERROR: shader compilation failed with exit code %SLANGC_EXIT%.
    exit /b 1
)

if not exist "%OUTPUT_FILE%" (
    echo [shaderCompile] ERROR: shader compilation produced no output file.
    exit /b 1
)

echo [shaderCompile] Wrote "%OUTPUT_FILE%"
exit /b 0