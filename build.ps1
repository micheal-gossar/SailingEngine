[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release", "Performance", "RelWithDebInfo", "MinSizeRel")]
    [string]$Configuration = "Debug",

    [ValidateRange(1, 64)]
    [int]$Jobs = [Math]::Min(8, [Environment]::ProcessorCount)
)

$ErrorActionPreference = "Stop"

$nativeCMake = Join-Path $env:ProgramFiles "CMake\bin\cmake.exe"
if (-not (Test-Path -LiteralPath $nativeCMake)) {
    $nativeCMake = (Get-Command cmake -ErrorAction Stop).Source
}

$cCompiler = "C:\msys64\ucrt64\bin\gcc.exe"
$cxxCompiler = "C:\msys64\ucrt64\bin\g++.exe"
if (-not (Test-Path -LiteralPath $cCompiler)) {
    $cCompiler = (Get-Command gcc -ErrorAction Stop).Source
}
if (-not (Test-Path -LiteralPath $cxxCompiler)) {
    $cxxCompiler = (Get-Command g++ -ErrorAction Stop).Source
}

$buildDirectory = Join-Path (Join-Path $PSScriptRoot "build") $Configuration
$cacheFile = Join-Path $buildDirectory "CMakeCache.txt"

if (-not (Test-Path -LiteralPath $cacheFile)) {
    & $nativeCMake `
        -S $PSScriptRoot `
        -B $buildDirectory `
        -G Ninja `
        "-DCMAKE_BUILD_TYPE=$Configuration" `
        "-DCMAKE_C_COMPILER=$cCompiler" `
        "-DCMAKE_CXX_COMPILER=$cxxCompiler" `
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed with exit code $LASTEXITCODE"
    }
}

& $nativeCMake --build $buildDirectory --parallel $Jobs
if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE"
}
